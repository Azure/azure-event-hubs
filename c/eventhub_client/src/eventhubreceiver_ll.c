// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <inttypes.h>
#include <limits.h>
#include <string.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/connection_string_parser.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/xlogging.h"

#include "azure_uamqp_c/connection.h"
#include "azure_uamqp_c/link.h"
#include "azure_uamqp_c/message.h"
#include "azure_uamqp_c/messaging.h"
#include "azure_uamqp_c/message_receiver.h"
#include "azure_uamqp_c/saslclientio.h"
#include "azure_uamqp_c/sasl_plain.h"
#include "azure_uamqp_c/session.h"

#include "version.h"
#include "eventhubreceiver_ll.h"

/* Event Hub Receiver States */
#define EVENTHUBRECEIVER_STATE_VALUES       \
        RECEIVER_STATE_INACTIVE,            \
        RECEIVER_STATE_INACTIVE_PENDING,    \
        RECEIVER_STATE_ACTIVE

DEFINE_ENUM(EVENTHUBRECEIVER_STATE, EVENTHUBRECEIVER_STATE_VALUES);

/* Event Hub Receiver AMQP States */
#define EVENTHUBRECEIVER_AMQP_STATE_VALUES  \
        RECEIVER_AMQP_UNINITIALIZED,        \
        RECEIVER_AMQP_INITIALIZED

DEFINE_ENUM(EVENTHUBRECEIVER_AMQP_STATE, EVENTHUBRECEIVER_AMQP_STATE_VALUES);

/* Default TLS port */
#define TLS_PORT                    5671

/* length for sb:// */
static const char SB_STRING[] = "sb://";
#define SB_STRING_LENGTH        ((sizeof(SB_STRING) / sizeof(SB_STRING[0])) - 1)

/* AMPQ payload max size */
#define AMQP_MAX_MESSAGE_SIZE       (256*1024)

typedef struct EVENTHUBCONNECTIONPARAMS_TAG
{
    STRING_HANDLE hostName;
    STRING_HANDLE eventHubPath;
    STRING_HANDLE eventHubSharedAccessKeyName;
    STRING_HANDLE eventHubSharedAccessKey;
    STRING_HANDLE targetAddress;
    STRING_HANDLE consumerGroup;
    STRING_HANDLE partitionId;
} EVENTHUBCONNECTIONPARAMS_STRUCT;

typedef struct EVENTHUBAMQPSTACK_STRUCT_TAG
{
    CONNECTION_HANDLE       connection;
    SESSION_HANDLE          session;
    LINK_HANDLE             link;
    XIO_HANDLE              saslIO;
    XIO_HANDLE              tlsIO;
    SASL_MECHANISM_HANDLE   saslMechanismHandle;
    MESSAGE_RECEIVER_HANDLE messageReceiver;
} EVENTHUBAMQPSTACK_STRUCT;

typedef struct EVENTHUBRECEIVER_LL_CALLBACK_TAG
{
    EVENTHUBRECEIVER_ASYNC_CALLBACK onEventReceiveCallback;
    void* onEventReceiveUserContext;
    volatile int wasMessageReceived;
    EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK onEventReceiveErrorCallback;
    void* onEventReceiveErrorUserContext;
    EVENTHUBRECEIVER_ASYNC_END_CALLBACK onEventReceiveEndCallback;
    void* onEventReceiveEndUserContext;
} EVENTHUBRECEIVER_LL_CALLBACK_STRUCT;

typedef struct EVENTHUBRECEIVER_LL_STRUCT_TAG
{
    EVENTHUBCONNECTIONPARAMS_STRUCT     connectionParams;
    EVENTHUBAMQPSTACK_STRUCT            receiverAMQPConnection;
    EVENTHUBRECEIVER_AMQP_STATE         receiverAMQPConnectionState;
    EVENTHUBRECEIVER_STATE              state;
    EVENTHUBRECEIVER_LL_CALLBACK_STRUCT callback;
    STRING_HANDLE                       receiverQueryFilter;
    uint64_t            startTimestamp;
    tickcounter_ms_t    lastActivityTimestamp;
    unsigned int        waitTimeoutInMs;
    TICK_COUNTER_HANDLE tickCounter;
    bool                connectionTracing;
} EVENTHUBRECEIVER_LL_STRUCT;

// Forward Declarations
static void EventHubConnectionParams_DeInitialize(EVENTHUBCONNECTIONPARAMS_STRUCT* eventHubConnectionParams);
static int EventHubConnectionParams_Initialize
(
    EVENTHUBCONNECTIONPARAMS_STRUCT* eventHubConnectionParams,
    const char* connectionString,
    const char* eventHubPath,
    const char* consumerGroup,
    const char* partitionId
);

static const char* EventHubConnectionParams_GetHostName(EVENTHUBCONNECTIONPARAMS_STRUCT* eventHubConnectionParams)
{
    return STRING_c_str(eventHubConnectionParams->hostName);
}

static const char* EventHubConnectionParams_GetSharedAccessKeyName(EVENTHUBCONNECTIONPARAMS_STRUCT* eventHubConnectionParams)
{
    return STRING_c_str(eventHubConnectionParams->eventHubSharedAccessKeyName);
}

static const char* EventHubConnectionParams_GetSharedAccessKey(EVENTHUBCONNECTIONPARAMS_STRUCT* eventHubConnectionParams)
{
    return STRING_c_str(eventHubConnectionParams->eventHubSharedAccessKey);
}

static const char* EventHubConnectionParams_GetTargetAddress(EVENTHUBCONNECTIONPARAMS_STRUCT* eventHubConnectionParams)
{
    return STRING_c_str(eventHubConnectionParams->targetAddress);
}

static int EventHubConnectionParams_CreateHostName
(
    EVENTHUBCONNECTIONPARAMS_STRUCT* eventHubConnectionParams,
    MAP_HANDLE connectionStringValuesMap
)
{
    int result;
    const char* endpoint;
    if ((endpoint = Map_GetValueFromKey(connectionStringValuesMap, "Endpoint")) == NULL)
    {
        LogError("Couldn't find endpoint in connection string.\r\n");
        result = __LINE__;
    }
    else
    {
        size_t hostnameLength = strlen(endpoint);
        if ((hostnameLength > SB_STRING_LENGTH) && (endpoint[hostnameLength - 1] == '/'))
        {
            hostnameLength--;
        }
        if ((hostnameLength <= SB_STRING_LENGTH) ||
            (strncmp(endpoint, SB_STRING, SB_STRING_LENGTH) != 0))
        {
            LogError("Invalid Host Name String\r\n");
            result = __LINE__;
        }
        else if ((eventHubConnectionParams->hostName = STRING_construct_n(endpoint + SB_STRING_LENGTH, hostnameLength - SB_STRING_LENGTH)) == NULL)
        {
            LogError("Couldn't create host name string\r\n");
            result = __LINE__;
        }
        else
        {
            result = 0;
        }
    }

    return result;
}

static int EventHubConnectionParams_CommonInit
(
    EVENTHUBCONNECTIONPARAMS_STRUCT* eventHubConnectionParams,
    const char* connectionString,
    const char* eventHubPath
)
{
    int result;
    MAP_HANDLE connectionStringValuesMap = NULL;
    STRING_HANDLE connectionStringHandle = NULL;

    //**SRS_EVENTHUBRECEIVER_LL_29_103: \[**`EventHubReceiver_LL_Create` shall expect a service bus connection string in one of the following formats:
    //Endpoint=sb://[namespace].servicebus.windows.net/;SharedAccessKeyName=[keyname];SharedAccessKey=[keyvalue] 
    //Endpoint=sb://[namespace].servicebus.windows.net;SharedAccessKeyName=[keyname];SharedAccessKey=[keyvalue]
    //**\]**
    if ((connectionStringHandle = STRING_construct(connectionString)) == NULL)
    {
        LogError("Error creating connection string handle.\r\n");
        result = __LINE__;
    }
    //**SRS_EVENTHUBRECEIVER_LL_29_104: \[**The connection string shall be parsed to a map of strings by using connection_string_parser_parse.**\]**
    else if ((connectionStringValuesMap = connectionstringparser_parse(connectionStringHandle)) == NULL)
    {
        //**SRS_EVENTHUBRECEIVER_LL_29_105: \[**If connection_string_parser_parse fails then `EventHubReceiver_LL_Create` shall fail and return NULL.**\]**
        LogError("Error parsing connection string.\r\n");
        result = __LINE__;
    }
    else
    {
        if ((eventHubConnectionParams->eventHubPath = STRING_construct(eventHubPath)) == NULL)
        {
            LogError("Failure allocating eventHubPath string.");
            result = __LINE__;
        }
        else
        {
            const char* value;
            //**SRS_EVENTHUBRECEIVER_LL_29_106: \[**The endpoint shall be looked up in the resulting map and used to construct the host name to be used for connecting by removing the sb://.**\]**
            //**SRS_EVENTHUBRECEIVER_LL_29_108: \[**Initialize receiver host address using the supplied connection string parameters host name, domain suffix, event hub name. **\]**
            if (EventHubConnectionParams_CreateHostName(eventHubConnectionParams, connectionStringValuesMap) != 0)
            {
                LogError("Couldn't create host name from connection string.\r\n");
                result = __LINE__;
            }
            //**SRS_EVENTHUBRECEIVER_LL_29_107: \[**The key name and key shall be looked up in the resulting map and they should be stored as is for later use in connecting.**\]**
            else if (((value = Map_GetValueFromKey(connectionStringValuesMap, "SharedAccessKeyName")) == NULL) ||
                 (strlen(value) == 0))
            {
                LogError("Couldn't find SharedAccessKeyName in connection string\r\n");
                result = __LINE__;
            }
            else if ((eventHubConnectionParams->eventHubSharedAccessKeyName = STRING_construct(value)) == NULL)
            {
                LogError("Couldn't create SharedAccessKeyName string\r\n");
                result = __LINE__;
            }
            else if (((value = Map_GetValueFromKey(connectionStringValuesMap, "SharedAccessKey")) == NULL) ||
                (strlen(value) == 0))
            {
                LogError("Couldn't find SharedAccessKey in connection string\r\n");
                result = __LINE__;
            }
            else if ((eventHubConnectionParams->eventHubSharedAccessKey = STRING_construct(value)) == NULL)
            {
                LogError("Couldn't create SharedAccessKey string\r\n");
                result = __LINE__;
            }
            else
            {
                result = 0;
            }
        }
    }

    if (connectionStringValuesMap != NULL)
    {
        Map_Destroy(connectionStringValuesMap);
    }
    if (connectionStringHandle != NULL)
    {
        STRING_delete(connectionStringHandle);
    }

    return result;
}

static int EventHubConnectionParams_InitReceiverPartitionURI
(
    EVENTHUBCONNECTIONPARAMS_STRUCT* eventHubConnectionParams
)
{
    int result;

    // Format - {eventHubName}/ConsumerGroups/{consumerGroup}/Partitions/{partitionID}
    if (((eventHubConnectionParams->targetAddress = STRING_clone(eventHubConnectionParams->eventHubPath)) == NULL) ||
        (STRING_concat(eventHubConnectionParams->targetAddress, "/ConsumerGroups/") != 0) ||
        (STRING_concat_with_STRING(eventHubConnectionParams->targetAddress, eventHubConnectionParams->consumerGroup) != 0) ||
        (STRING_concat(eventHubConnectionParams->targetAddress, "/Partitions/") != 0) ||
        (STRING_concat_with_STRING(eventHubConnectionParams->targetAddress, eventHubConnectionParams->partitionId) != 0))
    {
        LogError("Couldn't assemble target URI\r\n");
        result = __LINE__;
    }
    else
    {
        result = 0;
    }

    return result;
}

static int EventHubConnectionParams_ReceiverInit
(
    EVENTHUBCONNECTIONPARAMS_STRUCT* eventHubConnectionParams,
    const char* consumerGroup,
    const char* partitionId
)
{
    int result;
    //**SRS_EVENTHUBRECEIVER_LL_29_109: \[**Initialize receiver partition target address using consumer group and partition id data with format {eventHubName}/ConsumerGroups/{consumerGroup}/Partitions/{partitionID}.**\]**
    if ((eventHubConnectionParams->consumerGroup = STRING_construct(consumerGroup)) == NULL)
    {
        LogError("Failure allocating consumerGroup string.");
        result = __LINE__;
    }
    else if ((eventHubConnectionParams->partitionId = STRING_construct(partitionId)) == NULL)
    {
        LogError("Failure allocating partitionId string.");
        result = __LINE__;
    }
    else if ((result = EventHubConnectionParams_InitReceiverPartitionURI(eventHubConnectionParams)) != 0)
    {
        LogError("Couldn't create receiver target partition URI.\r\n");
        result = __LINE__;
    }
    else
    {
        result = 0;
    }

    return result;
}

static int EventHubConnectionParams_Initialize
(
    EVENTHUBCONNECTIONPARAMS_STRUCT* eventHubConnectionParams,
    const char* connectionString,
    const char* eventHubPath,
    const char* consumerGroup,
    const char* partitionId
)
{
    int result;
    
    //**SRS_EVENTHUBRECEIVER_LL_29_111: \[**`EventHubReceiver_LL_Create` shall return NULL if any parameter connectionString, eventHubPath, consumerGroup and partitionId is NULL.**\]**
    if ((connectionString == NULL) || (eventHubPath == NULL) || (consumerGroup == NULL) || (partitionId == NULL))
    {
        LogError("Invalid arguments. connectionString=%p, eventHubPath=%p consumerGroup=%p partitionId=%p\r\n",
            connectionString, eventHubPath, consumerGroup, partitionId);
        result = __LINE__;
    }
    else
    {
        int errorCode;
        eventHubConnectionParams->hostName = NULL;
        eventHubConnectionParams->eventHubPath = NULL;
        eventHubConnectionParams->eventHubSharedAccessKeyName = NULL;
        eventHubConnectionParams->eventHubSharedAccessKey = NULL;
        eventHubConnectionParams->targetAddress = NULL;
        eventHubConnectionParams->consumerGroup = NULL;
        eventHubConnectionParams->partitionId = NULL;
        if ((errorCode = EventHubConnectionParams_CommonInit(eventHubConnectionParams, connectionString, eventHubPath)) != 0)
        {
            LogError("Could Not Initialize Common Connection Parameters. Line:%d\r\n", errorCode);
            EventHubConnectionParams_DeInitialize(eventHubConnectionParams);
            result = __LINE__;
        }
        else if ((errorCode = EventHubConnectionParams_ReceiverInit(eventHubConnectionParams, consumerGroup, partitionId)) != 0)
        {
            LogError("Could Not Initialize Receiver Connection Parameters. Line:%d\r\n", errorCode);
            EventHubConnectionParams_DeInitialize(eventHubConnectionParams);
            result = __LINE__;
        }
        else
        {
            result = 0;
        }
    }

    return result;
}

static void EventHubConnectionParams_DeInitialize(EVENTHUBCONNECTIONPARAMS_STRUCT* eventHubConnectionParams)
{
    if (eventHubConnectionParams->hostName != NULL)
    {
        STRING_delete(eventHubConnectionParams->hostName);
        eventHubConnectionParams->hostName = NULL;
    }
    if (eventHubConnectionParams->eventHubPath != NULL)
    {
        STRING_delete(eventHubConnectionParams->eventHubPath);
        eventHubConnectionParams->eventHubPath = NULL;
    }
    if (eventHubConnectionParams->eventHubSharedAccessKeyName != NULL)
    {
        STRING_delete(eventHubConnectionParams->eventHubSharedAccessKeyName);
        eventHubConnectionParams->eventHubSharedAccessKeyName = NULL;
    }
    if (eventHubConnectionParams->eventHubSharedAccessKey != NULL)
    {
        STRING_delete(eventHubConnectionParams->eventHubSharedAccessKey);
        eventHubConnectionParams->eventHubSharedAccessKey = NULL;
    }
    if (eventHubConnectionParams->targetAddress != NULL)
    {
        STRING_delete(eventHubConnectionParams->targetAddress);
        eventHubConnectionParams->targetAddress = NULL;
    }
    if (eventHubConnectionParams->consumerGroup != NULL)
    {
        STRING_delete(eventHubConnectionParams->consumerGroup);
        eventHubConnectionParams->consumerGroup = NULL;
    }
    if (eventHubConnectionParams->partitionId != NULL)
    {
        STRING_delete(eventHubConnectionParams->partitionId);
        eventHubConnectionParams->partitionId = NULL;
    }
}

void EventHubAMQP_DeInitializeStack(EVENTHUBAMQPSTACK_STRUCT* eventHubCommStack)
{
    //**SRS_EVENTHUBRECEIVER_LL_29_920: \[**All pending message data not reported to the calling client shall be freed by calling messagereceiver_close and messagereceiver_destroy.**\]**
    if (eventHubCommStack->messageReceiver != NULL)
    {
        if (messagereceiver_close(eventHubCommStack->messageReceiver) != 0)
        {
            LogError("Failed closing the AMQP message receiver.");
        }
        messagereceiver_destroy(eventHubCommStack->messageReceiver);
        eventHubCommStack->messageReceiver = NULL;
    }
    //**SRS_EVENTHUBRECEIVER_LL_29_921: \[**The link shall be freed by calling link_destroy.**\]** 
    if (eventHubCommStack->link != NULL)
    {
        link_destroy(eventHubCommStack->link);
        eventHubCommStack->link = NULL;
    }
    //**SRS_EVENTHUBRECEIVER_LL_29_922: \[**The session shall be freed by calling session_destroy.**\]** 
    if (eventHubCommStack->session != NULL)
    {
        session_destroy(eventHubCommStack->session);
        eventHubCommStack->session = NULL;
    }
    //**SRS_EVENTHUBRECEIVER_LL_29_923: \[**The connection shall be freed by calling connection_destroy.**\]** 
    if (eventHubCommStack->connection != NULL)
    {
        connection_destroy(eventHubCommStack->connection);
        eventHubCommStack->connection = NULL;
    }
    //**SRS_EVENTHUBRECEIVER_LL_29_924: \[**The SASL client IO shall be freed by calling xio_destroy.**\]** 
    if (eventHubCommStack->saslIO != NULL)
    {
        xio_destroy(eventHubCommStack->saslIO);
        eventHubCommStack->saslIO = NULL;
    }
    //**SRS_EVENTHUBRECEIVER_LL_29_925: \[**The TLS IO shall be freed by calling xio_destroy.**\]** 
    if (eventHubCommStack->tlsIO != NULL)
    {
        xio_destroy(eventHubCommStack->tlsIO);
        eventHubCommStack->tlsIO = NULL;
    }
    //**SRS_EVENTHUBRECEIVER_LL_29_926: \[**The SASL plain mechanism shall be freed by calling saslmechanism_destroy.**\]** 
    if (eventHubCommStack->saslMechanismHandle != NULL)
    {
        saslmechanism_destroy(eventHubCommStack->saslMechanismHandle);
        eventHubCommStack->saslMechanismHandle = NULL;
    }
}

static int EventHubAMQP_InitializeStackCommon
(
    EVENTHUBAMQPSTACK_STRUCT* eventHubCommStack,
    EVENTHUBCONNECTIONPARAMS_STRUCT* connectionParams,
    bool connectionTracing,
    const char* connectionName
)
{
    int result;

    //**SRS_EVENTHUBRECEIVER_LL_29_502: \[**The interface passed to saslmechanism_create shall be obtained by calling saslplain_get_interface.**\]**
    const SASL_MECHANISM_INTERFACE_DESCRIPTION* saslMechanismInterface = saslplain_get_interface();
    if (saslMechanismInterface == NULL)
    {
        //**SRS_EVENTHUBRECEIVER_LL_29_505: \[**If saslplain_get_interface fails then a log message will be logged and the function returns immediately.**\]**
        LogError("Cannot obtain SASL PLAIN interface.\r\n");
        result = __LINE__;
    }
    else
    {
        const char* authcid;
        const char* passwd;
        const char* hostName;
        TLSIO_CONFIG tlsIOConfig;
        const IO_INTERFACE_DESCRIPTION* saslClientIOInterface;
        const IO_INTERFACE_DESCRIPTION* tlsioInterface;

        //**SRS_EVENTHUBRECEIVER_LL_29_503: \[**The authcid shall be set to the key name parsed from the connection string.**\]**
        //**SRS_EVENTHUBRECEIVER_LL_29_504: \[**The passwd members shall be set to the key value from the connection string.**\]**    
        //**SRS_EVENTHUBRECEIVER_LL_29_506: \[**The creation parameters for the SASL plain mechanism shall be in the form of a SASL_PLAIN_CONFIG structure.**\]**
        authcid = EventHubConnectionParams_GetSharedAccessKeyName(connectionParams);
        passwd = EventHubConnectionParams_GetSharedAccessKey(connectionParams);
        SASL_PLAIN_CONFIG saslPlainConfig = { authcid, passwd, NULL };

        //**SRS_EVENTHUBRECEIVER_LL_29_501: \[**A SASL plain mechanism shall be created by calling saslmechanism_create.**\]**
        eventHubCommStack->saslMechanismHandle = saslmechanism_create(saslMechanismInterface, &saslPlainConfig);
        if (eventHubCommStack->saslMechanismHandle == NULL)
        {
            LogError("saslmechanism_create failed.\r\n");
            result = __LINE__;
        }
        else
        {            
            //**SRS_EVENTHUBRECEIVER_LL_29_520: \[**A TLS IO shall be created by calling xio_create using TLS port 5671 and host name obtained from the connection string**\]**
            hostName = EventHubConnectionParams_GetHostName(connectionParams);
            tlsIOConfig.hostname = hostName;
            tlsIOConfig.port = TLS_PORT;

            //**SRS_EVENTHUBRECEIVER_LL_29_521: \[**The interface passed to xio_create shall be obtained by calling platform_get_default_tlsio.**\]**
            if ((tlsioInterface = platform_get_default_tlsio()) == NULL)
            {
                LogError("Could not obtain default TLS IO.\r\n");
                result = __LINE__;
            }
            else if ((eventHubCommStack->tlsIO = xio_create(tlsioInterface, &tlsIOConfig)) == NULL)
            {
                //**SRS_EVENTHUBRECEIVER_LL_29_522: \[**If xio_create fails then a log message will be logged and the function returns immediately.**\]** 
                LogError("TLS IO creation failed.\r\n");
                result = __LINE__;
            }
            else if ((saslClientIOInterface = saslclientio_get_interface_description()) == NULL)
            {
                LogError("SASL Client Info IO creation failed.\r\n");
                result = __LINE__;
            }
            else
            {
                //**SRS_EVENTHUBRECEIVER_LL_29_523: \[**A SASL IO shall be created by calling xio_create using TLS IO interface created previously and the SASL plain mechanism created earlier. The SASL interface shall be obtained using `saslclientio_get_interface_description`**\]**
                SASLCLIENTIO_CONFIG saslIOConfig = { eventHubCommStack->tlsIO, eventHubCommStack->saslMechanismHandle };
                if ((eventHubCommStack->saslIO = xio_create(saslClientIOInterface, &saslIOConfig)) == NULL)
                {
                    //**SRS_EVENTHUBRECEIVER_LL_29_524: \[**If xio_create fails then a log message will be logged and the function returns immediately..**\]** 
                    LogError("SASL client IO creation failed.\r\n");
                    result = __LINE__;
                }
                //**SRS_EVENTHUBRECEIVER_LL_29_530: \[**An AMQP connection shall be created by calling connection_create and passing as arguments the SASL client IO handle created previously, hostname, connection name and NULL for the new session handler end point and context.**\]** 
                else if ((eventHubCommStack->connection = connection_create(eventHubCommStack->saslIO, hostName, connectionName, NULL, NULL)) == NULL)
                {
                    //**SRS_EVENTHUBRECEIVER_LL_29_531: \[**If connection_create fails then a log message will be logged and the function returns immediately.**\]** 
                    LogError("connection_create failed.\r\n");
                    result = __LINE__;
                }
                else
                {
                    //**SRS_EVENTHUBRECEIVER_LL_29_532: \[**Connection tracing shall be called with the current value of the tracing flag**\]**
                    connection_set_trace(eventHubCommStack->connection, connectionTracing);
                    result = 0;
                }
            }
        }
    }
    if (result != 0)
    {
        EventHubAMQP_DeInitializeStack(eventHubCommStack);
    }
    return result;
}

static int EventHubAMQP_InitializeStackReceiver
(
    EVENTHUBAMQPSTACK_STRUCT* eventHubCommStack,
    EVENTHUBCONNECTIONPARAMS_STRUCT* connectionParams,
    ON_MESSAGE_RECEIVER_STATE_CHANGED onStateChangeCB,
    void* onStateChangeCBContext,
    ON_MESSAGE_RECEIVED onMsgReceivedCB,
    void* onMsgReceivedCBContext,
    STRING_HANDLE filter
)
{
    static const char filterName[] = "apache.org:selector-filter:string";
    int result;

    //**SRS_EVENTHUBRECEIVER_LL_29_540: \[**An AMQP session shall be created by calling session_create and passing as arguments the connection handle, and NULL for the new link handler and context.**\]**
    if ((eventHubCommStack->session = session_create(eventHubCommStack->connection, NULL, NULL)) == NULL)
    {
        //**SRS_EVENTHUBRECEIVER_LL_29_541: \[**If session_create fails then a log message will be logged and the function returns immediately.**\]**
        LogError("session_create failed.\r\n");
        result = __LINE__;
    }
    //**SRS_EVENTHUBRECEIVER_LL_29_542: \[**Configure the session incoming window by calling session_set_incoming_window and set value to INTMAX.**\]**
    else if (session_set_incoming_window(eventHubCommStack->session, INT_MAX) != 0)
    {
        LogError("session_set_outgoing_window failed.\r\n");
        result = __LINE__;
    }
    else
    {
        AMQP_VALUE source = NULL;
        AMQP_VALUE target = NULL;
        filter_set filterSet = NULL;
        AMQP_VALUE filterKey = NULL;
        AMQP_VALUE descriptor = NULL;
        AMQP_VALUE filterValue = NULL;
        AMQP_VALUE describedFilterValue = NULL;
        SOURCE_HANDLE sourceHandle = NULL;
        AMQP_VALUE addressValue = NULL;


        //**SRS_EVENTHUBRECEIVER_LL_29_543: \[**A filter_set shall be created and initialized using key "apache.org:selector-filter:string" and value as the query filter created previously.**\]**
        if ((filterSet = amqpvalue_create_map()) == NULL)
        {
            //**SRS_EVENTHUBRECEIVER_LL_29_544: \[**If creation of the filter_set fails, then a log message will be logged and the function returns immediately.**\]**
            LogError("Failed creating filter map with filter\r\n");
            result = __LINE__;
        }
        else if ((filterKey = amqpvalue_create_symbol(filterName)) == NULL)
        {
            //**SRS_EVENTHUBRECEIVER_LL_29_544: \[**If creation of the filter_set fails, then a log message will be logged and the function returns immediately.**\]**
            LogError("Failed creating filter key for filter %s", filterName);
            result = __LINE__;
        }
        else if ((descriptor = amqpvalue_create_symbol(filterName)) == NULL)
        {
            //**SRS_EVENTHUBRECEIVER_LL_29_544: \[**If creation of the filter_set fails, then a log message will be logged and the function returns immediately.**\]**
            LogError("Failed creating filter descriptor for filter %s", filterName);
            result = __LINE__;
        }
        else if ((filterValue = amqpvalue_create_string(STRING_c_str(filter))) == NULL)
        {
            //**SRS_EVENTHUBRECEIVER_LL_29_544: \[**If creation of the filter_set fails, then a log message will be logged and the function returns immediately.**\]**
            LogError("Failed creating filterValue for query filter %s", STRING_c_str(filter));
            result = __LINE__;
        }
        else if ((describedFilterValue = amqpvalue_create_described(descriptor, filterValue)) == NULL)
        {
            //**SRS_EVENTHUBRECEIVER_LL_29_544: \[**If creation of the filter_set fails, then a log message will be logged and the function returns immediately.**\]**
            LogError("Failed creating describedFilterValue with %s filter.", filterName);
            result = __LINE__;
        }
        else if (amqpvalue_set_map_value(filterSet, filterKey, describedFilterValue) != 0)
        {
            //**SRS_EVENTHUBRECEIVER_LL_29_544: \[**If creation of the filter_set fails, then a log message will be logged and the function returns immediately.**\]**
            LogError("Failed amqpvalue_set_map_value.");
            result = __LINE__;
        }
        else
        {
            const char* receiveAddress = EventHubConnectionParams_GetTargetAddress(connectionParams);
            
            //**SRS_EVENTHUBRECEIVER_LL_29_547: \[**The message receiver 'source' shall be created using source_create.**\]**
            if ((sourceHandle = source_create()) == NULL)
            {
                //**SRS_EVENTHUBRECEIVER_LL_29_545: \[**If a failure is observed during source creation and initialization, then a log message will be logged and the function returns immediately.**\]**
                LogError("Failed source_create.");
                result = __LINE__;
            }
            else if ((addressValue = amqpvalue_create_string(receiveAddress)) == NULL)
            {
                //**SRS_EVENTHUBRECEIVER_LL_29_545: \[**If a failure is observed during source creation and initialization, then a log message will be logged and the function returns immediately.**\]**
                LogError("Failed to create AMQP string for receive address.");
                result = __LINE__;
            }
            //**SRS_EVENTHUBRECEIVER_LL_29_553: \[**The message receiver link 'source' address shall be initialized using the partition target address created earlier.**\]**
            else if (source_set_address(sourceHandle, addressValue) != 0)
            {
                //**SRS_EVENTHUBRECEIVER_LL_29_545: \[**If a failure is observed during source creation and initialization, then a log message will be logged and the function returns immediately.**\]**
                LogError("Failed source_set_address.");
                result = __LINE__;
            }
            //**SRS_EVENTHUBRECEIVER_LL_29_552: \[**The message receiver link 'source' filter shall be initialized by calling source_set_filter and using the filter_set created earlier.**\]**
            else if (source_set_filter(sourceHandle, filterSet) != 0)
            {
                //**SRS_EVENTHUBRECEIVER_LL_29_545: \[**If a failure is observed during source creation and initialization, then a log message will be logged and the function returns immediately.**\]**
                LogError("Failed source_set_filter.");
                result = __LINE__;
            }
            //**SRS_EVENTHUBRECEIVER_LL_29_551: \[**The message receiver link 'source' shall be created using API amqpvalue_create_source**\]**
            else if ((source = amqpvalue_create_source(sourceHandle)) == NULL)
            {
                //**SRS_EVENTHUBRECEIVER_LL_29_545: \[**If a failure is observed during source creation and initialization, then a log message will be logged and the function returns immediately.**\]**
                LogError("Failed to create source.");
                result = __LINE__;
            }
            //**SRS_EVENTHUBRECEIVER_LL_29_554: \[**The message receiver link target shall be created using messaging_create_target with address obtained from the partition target address created earlier.**\]**
            else if ((target = messaging_create_target(receiveAddress)) == NULL)
            {
                //**SRS_EVENTHUBRECEIVER_LL_29_546: \[**If messaging_create_target fails, then a log message will be logged and the function returns immediately.**\]**
                LogError("Failed creating target for link.");
                result = __LINE__;
            }
            //**SRS_EVENTHUBRECEIVER_LL_29_550: \[**An AMQP link for the to be created message receiver shall be created by calling link_create with role as role_receiver and name as "receiver-link"**\]**
            else if ((eventHubCommStack->link = link_create(eventHubCommStack->session, "receiver-link", role_receiver, source, target)) == NULL)
            {
                //**SRS_EVENTHUBRECEIVER_LL_29_555: \[**If link_create fails then a log message will be logged and the function returns immediately.**\]**
                LogError("Failed creating link.");
                result = __LINE__;
            }
            //**SRS_EVENTHUBRECEIVER_LL_29_556: \[**Configure the link settle mode by calling link_set_rcv_settle_mode and set value to receiver_settle_mode_first.**\]**
            else if (link_set_rcv_settle_mode(eventHubCommStack->link, receiver_settle_mode_first) != 0)
            {
                //**SRS_EVENTHUBRECEIVER_LL_29_557: \[**If link_set_rcv_settle_mode fails then a log message will be logged and the function returns immediately.**\]**
                LogError("Failed setting link receive settle mode.");
                result = __LINE__;
            }
            //**SRS_EVENTHUBRECEIVER_LL_29_558: \[**The message size shall be set to 256K by calling link_set_max_message_size.**\]**
            else if (link_set_max_message_size(eventHubCommStack->link, AMQP_MAX_MESSAGE_SIZE) != 0)
            {
                //**SRS_EVENTHUBRECEIVER_LL_29_559: \[**If link_set_max_message_size fails then a log message will be logged and the function returns immediately.**\]**
                LogError("link_set_max_message_size failed.\r\n");
                result = __LINE__;
            }
            //**SRS_EVENTHUBRECEIVER_LL_29_560: \[**A message receiver shall be created by calling messagereceiver_create and passing as arguments the link handle, a state changed callback and context.**\]**
            else if ((eventHubCommStack->messageReceiver = messagereceiver_create(eventHubCommStack->link, onStateChangeCB, onStateChangeCBContext)) == NULL)
            {
                //**SRS_EVENTHUBRECEIVER_LL_29_561: \[**If messagereceiver_create fails then a log message will be logged and the function returns immediately.**\]**
                LogError("messagesender_create failed.\r\n");
                result = __LINE__;
            }
            //**SRS_EVENTHUBRECEIVER_LL_29_562: \[**The created message receiver shall be transitioned to OPEN by calling messagereceiver_open.**\]** 
            else if (messagereceiver_open(eventHubCommStack->messageReceiver, onMsgReceivedCB, onMsgReceivedCBContext) != 0)
            {
                //**SRS_EVENTHUBRECEIVER_LL_29_563: \[**If messagereceiver_open fails then a log message will be logged and the function returns immediately.**\]**
                LogError("Error opening message receiver.\r\n");
                result = __LINE__;
            }
            else
            {
                result = 0;
            }
        }
        if (sourceHandle != NULL) source_destroy(sourceHandle);
        if (addressValue != NULL) amqpvalue_destroy(addressValue);
        if (filterKey != NULL) amqpvalue_destroy(filterKey);
        if (target != NULL) amqpvalue_destroy(target);
        if (source != NULL) amqpvalue_destroy(source);
        if (describedFilterValue != NULL) amqpvalue_destroy(describedFilterValue);
        if (filterSet != NULL) amqpvalue_destroy(filterSet);
        
        if (result != 0)
        {
            EventHubAMQP_DeInitializeStack(eventHubCommStack);
        }
    }

    return result;
}

void static EventHubConnectionAMQP_Reset(EVENTHUBAMQPSTACK_STRUCT* receiverAMQPConnection)
{
    receiverAMQPConnection->connection = NULL;
    receiverAMQPConnection->link = NULL;
    receiverAMQPConnection->messageReceiver = NULL;
    receiverAMQPConnection->saslIO = NULL;
    receiverAMQPConnection->saslMechanismHandle = NULL;
    receiverAMQPConnection->session = NULL;
    receiverAMQPConnection->tlsIO = NULL;
}

static int EventHubConnectionAMQP_ReceiverInitialize
(
    EVENTHUBRECEIVER_LL_STRUCT* eventHubReceiverLL,
    ON_MESSAGE_RECEIVER_STATE_CHANGED onStateChangeCB,
    void* onStateChangeCBContext,
    ON_MESSAGE_RECEIVED onMsgReceivedCB,
    void* onMsgReceivedCBContext
)
{
    static const char receiverConnectionName[] = "eh_receiver_connection";
    int result, errorCode;

    EventHubConnectionAMQP_Reset(&eventHubReceiverLL->receiverAMQPConnection);
    if ((errorCode = EventHubAMQP_InitializeStackCommon(&eventHubReceiverLL->receiverAMQPConnection, 
                                                        &eventHubReceiverLL->connectionParams,
                                                        eventHubReceiverLL->connectionTracing,
                                                        receiverConnectionName)) != 0)
    {
        LogError("Could Not Initialize Common AMQP Connection Data. Line:%d\r\n", errorCode);
        result = __LINE__;
    }
    else if ((errorCode = EventHubAMQP_InitializeStackReceiver(&eventHubReceiverLL->receiverAMQPConnection,
                                                               &eventHubReceiverLL->connectionParams,
                                                               onStateChangeCB, onStateChangeCBContext,
                                                               onMsgReceivedCB, onMsgReceivedCBContext, 
                                                               eventHubReceiverLL->receiverQueryFilter)) != 0)
    {
        LogError("Could Not Initialize Receiver AMQP Connection Data. Line:%d\r\n", errorCode);
        result = __LINE__;
    }
    else
    {
        eventHubReceiverLL->receiverAMQPConnectionState = RECEIVER_AMQP_INITIALIZED;
        result = 0;
    }
    return result;
}

static void EventHubConnectionAMQP_ReceiverDeInitialize(EVENTHUBRECEIVER_LL_STRUCT* eventHubReceiverLL)
{
    EventHubAMQP_DeInitializeStack(&eventHubReceiverLL->receiverAMQPConnection);
    eventHubReceiverLL->receiverAMQPConnectionState = RECEIVER_AMQP_UNINITIALIZED;
}

static void EventHubConnectionAMQP_ConnectionDoWork(EVENTHUBRECEIVER_LL_STRUCT* eventHubReceiverLL)
{
    if (eventHubReceiverLL->receiverAMQPConnectionState == RECEIVER_AMQP_INITIALIZED)
    {
        connection_dowork(eventHubReceiverLL->receiverAMQPConnection.connection);
    }
}

static void EventHubConnectionAMQP_SetConnectionTracing(EVENTHUBRECEIVER_LL_STRUCT* eventHubReceiverLL, bool onOff)
{
    if (eventHubReceiverLL->receiverAMQPConnectionState == RECEIVER_AMQP_INITIALIZED)
    {
        connection_set_trace(eventHubReceiverLL->receiverAMQPConnection.connection, onOff);
    }
}

static STRING_HANDLE EventHubConnectionAMQP_CreateStartTimeFilter(uint64_t startTimestampInSec)
{
    STRING_HANDLE handle = STRING_construct_sprintf("amqp.annotation.x-opt-enqueuedtimeutc > %" PRIu64, startTimestampInSec);
    if (handle == NULL)
    {
        LogError("Failure allocating filter string.");
    }
    return handle;
}

static void EventHubConnectionAMQP_DestroyFilter(STRING_HANDLE filter)
{
    //**SRS_EVENTHUBRECEIVER_LL_29_927: \[**The filter string shall be freed by STRING_delete.**\]**
    STRING_delete(filter);
}

EVENTHUBRECEIVER_LL_HANDLE EventHubReceiver_LL_Create
(
    const char* connectionString,
    const char* eventHubPath,
    const char* consumerGroup,
    const char* partitionId
)
{
    EVENTHUBRECEIVER_LL_STRUCT* eventHubReceiverLL;

    //**SRS_EVENTHUBRECEIVER_LL_29_101: \[**`EventHubReceiver_LL_Create` shall obtain the version string by a call to EVENTHUBRECEIVER_GetVersionString.**\]**
    //**SRS_EVENTHUBRECEIVER_LL_29_102: \[**`EventHubReceiver_LL_Create` shall print the version string to standard output.**\]**
    LogInfo("Event Hubs Client SDK for C, version %s", EventHubClient_GetVersionString());

    //**SRS_EVENTHUBRECEIVER_LL_29_114: \[**`EventHubReceiver_LL_Create` shall allocate a new event hub receiver LL instance, initialize it with the user supplied data.**\]**
    eventHubReceiverLL = (EVENTHUBRECEIVER_LL_STRUCT*) malloc(sizeof(EVENTHUBRECEIVER_LL_STRUCT));
    if (eventHubReceiverLL == NULL)
    {
        //**SRS_EVENTHUBRECEIVER_LL_29_115: \[**For all other errors, `EventHubReceiver_LL_Create` shall return NULL.**\]**
        LogError("Could not allocate memory for EVENTHUBRECEIVER_LL_STRUCT\r\n");
    }
    //**SRS_EVENTHUBRECEIVER_LL_29_117: \[**`EventHubReceiver_LL_Create` shall create a tickcounter.**\]**
    else if ((eventHubReceiverLL->tickCounter = tickcounter_create()) == NULL)
    {
        //**SRS_EVENTHUBRECEIVER_LL_29_115: \[**For all other errors, `EventHubReceiver_LL_Create` shall return NULL.**\]**
        LogError("Could not create tick counter\r\n");
        free(eventHubReceiverLL);
        eventHubReceiverLL = NULL;
    }
    else
    {
        int errorCode;
        if ((errorCode = EventHubConnectionParams_Initialize(&eventHubReceiverLL->connectionParams, connectionString, eventHubPath, consumerGroup, partitionId)) != 0)
        {
            //**SRS_EVENTHUBRECEIVER_LL_29_115: \[**For all other errors, `EventHubReceiver_LL_Create` shall return NULL.**\]**
            LogError("Could Not Initialize Connection Parameters. Code:%d\r\n", errorCode);
            free(eventHubReceiverLL);
            eventHubReceiverLL = NULL;
        }
        else
        {
            //**SRS_EVENTHUBRECEIVER_LL_29_112: \[**`EventHubReceiver_LL_Create` shall return a non-NULL handle value upon success.**\]**
            //**SRS_EVENTHUBRECEIVER_LL_29_116: \[**Initialize connection tracing to false by default.**\]**
            eventHubReceiverLL->connectionTracing = false;
            eventHubReceiverLL->receiverQueryFilter = NULL;
            eventHubReceiverLL->callback.onEventReceiveCallback = NULL;
            eventHubReceiverLL->callback.onEventReceiveUserContext = NULL;
            eventHubReceiverLL->callback.wasMessageReceived = 0;
            eventHubReceiverLL->callback.onEventReceiveErrorCallback = NULL;
            eventHubReceiverLL->callback.onEventReceiveErrorUserContext = NULL;
            eventHubReceiverLL->startTimestamp = 0;
            eventHubReceiverLL->waitTimeoutInMs = 0;
            eventHubReceiverLL->lastActivityTimestamp = 0;
            EventHubConnectionAMQP_Reset(&eventHubReceiverLL->receiverAMQPConnection);
            eventHubReceiverLL->state = RECEIVER_STATE_INACTIVE;
            eventHubReceiverLL->receiverAMQPConnectionState = RECEIVER_AMQP_UNINITIALIZED;
        }
    }

    return (EVENTHUBRECEIVER_LL_HANDLE)eventHubReceiverLL;
}

void EventHubReceiver_LL_Destroy(EVENTHUBRECEIVER_LL_HANDLE eventHubReceiverLLHandle)
{
    EVENTHUBRECEIVER_LL_STRUCT* eventHubReceiverLL = (EVENTHUBRECEIVER_LL_STRUCT*)eventHubReceiverLLHandle;
    if (eventHubReceiverLL != NULL)
    {
        //**SRS_EVENTHUBRECEIVER_LL_29_201: \[**`EventHubReceiver_LL_Destroy` shall tear down connection with the event hub.**\]**
        //**SRS_EVENTHUBRECEIVER_LL_29_202: \[**`EventHubReceiver_LL_Destroy` shall terminate the usage of the EVENTHUBRECEIVER_LL_STRUCT and cleanup all associated resources.**\]**
        EventHubConnectionAMQP_ReceiverDeInitialize(eventHubReceiverLL);
        EventHubConnectionAMQP_DestroyFilter(eventHubReceiverLL->receiverQueryFilter);
        EventHubConnectionParams_DeInitialize(&eventHubReceiverLL->connectionParams);
        tickcounter_destroy(eventHubReceiverLL->tickCounter);
        free(eventHubReceiverLL);
    }
}

EVENTHUBRECEIVER_RESULT EventHubReceiver_LL_ReceiveEndAsync
(
    EVENTHUBRECEIVER_LL_HANDLE eventHubReceiverLLHandle,
    EVENTHUBRECEIVER_ASYNC_END_CALLBACK onEventReceiveEndCallback,
    void* onEventReceiveEndUserContext
)
{
    EVENTHUBRECEIVER_LL_STRUCT* eventHubReceiverLL = (EVENTHUBRECEIVER_LL_STRUCT*)eventHubReceiverLLHandle;
    EVENTHUBRECEIVER_RESULT result;

    //**SRS_EVENTHUBRECEIVER_LL_29_900: \[**`EventHubReceiver_LL_ReceiveEndAsync` shall validate arguments, in case they are invalid, error code EVENTHUBRECEIVER_INVALID_ARG will be returned.**\]**
    if (eventHubReceiverLL == NULL)
    {
        //**SRS_EVENTHUBRECEIVER_LL_29_901: \[**`EventHubReceiver_LL_ReceiveEndAsync` shall check if a receiver connection is currently active. If no receiver is active, EVENTHUBRECEIVER_NOT_ALLOWED shall be returned and a message will be logged.**\]**
        LogError("Invalid arguments\r\n");
        result = EVENTHUBRECEIVER_INVALID_ARG;
    }
    else if (eventHubReceiverLL->state != RECEIVER_STATE_ACTIVE)
    {
        //**SRS_EVENTHUBRECEIVER_LL_29_901: \[**`EventHubReceiver_LL_ReceiveEndAsync` shall check if a receiver connection is currently active. If no receiver is active, EVENTHUBRECEIVER_NOT_ALLOWED shall be returned and a message will be logged.**\]**
        LogError("Operation not permitted as there is no active Receiver instance.\r\n");
        result = EVENTHUBRECEIVER_NOT_ALLOWED;
    }
    else
    {
        //**SRS_EVENTHUBRECEIVER_LL_29_902: \[**`EventHubReceiver_LL_ReceiveEndAsync` save off the user callback and context and defer the UAMQP stack tear down to `EventHubReceiver_LL_DoWork`.**\]**
        eventHubReceiverLL->state = RECEIVER_STATE_INACTIVE_PENDING;
        eventHubReceiverLL->callback.onEventReceiveEndCallback = onEventReceiveEndCallback;
        eventHubReceiverLL->callback.onEventReceiveEndUserContext = onEventReceiveEndUserContext;
        //**SRS_EVENTHUBRECEIVER_LL_29_903: \[**Upon Success, `EventHubReceiver_LL_ReceiveEndAsync` shall return EVENTHUBRECEIVER_OK.**\]**
        result = EVENTHUBRECEIVER_OK;
    }

    return result;
}

static EVENTHUBRECEIVER_RESULT EHR_LL_ReceiveAsyncCommon
(
    EVENTHUBRECEIVER_LL_HANDLE eventHubReceiverLLHandle,
    EVENTHUBRECEIVER_ASYNC_CALLBACK onEventReceiveCallback,
    void *onEventReceiveUserContext,
    EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK onEventReceiveErrorCallback,
    void *onEventReceiveErrorUserContext,
    uint64_t startTimestampInSec,
    unsigned int waitTimeoutInMs
)
{
    EVENTHUBRECEIVER_RESULT result;
    EVENTHUBRECEIVER_LL_STRUCT* eventHubReceiverLL = (EVENTHUBRECEIVER_LL_STRUCT*)eventHubReceiverLLHandle;
    int errorCode;
    
    //**SRS_EVENTHUBRECEIVER_LL_29_301: \[**`EventHubReceiver_LL_ReceiveFromStartTimestamp*Async` shall fail and return EVENTHUBRECEIVER_INVALID_ARG if parameter eventHubReceiverHandle, onEventReceiveErrorCallback, onEventReceiveErrorCallback are NULL.**\]**
    if ((eventHubReceiverLL == NULL) || (onEventReceiveCallback == NULL) || (onEventReceiveErrorCallback == NULL))
    {
        LogError("Invalid arguments\r\n");
        result = EVENTHUBRECEIVER_INVALID_ARG;
    }
    else if (eventHubReceiverLL->state != RECEIVER_STATE_INACTIVE)
    {
        //**SRS_EVENTHUBRECEIVER_LL_29_307: \[**`EventHubReceiver_LL_ReceiveFromStartTimestamp*Async` shall return an error code of EVENTHUBRECEIVER_NOT_ALLOWED if a user called EventHubReceiver_LL_Receive* more than once on the same handle.**\]**
        LogError("Operation not permitted as there is an exiting Receiver instance.\r\n");
        result = EVENTHUBRECEIVER_NOT_ALLOWED;
    }
    else
    {
        startTimestampInSec *= 1000;
        //**SRS_EVENTHUBRECEIVER_LL_29_306: \[**Initialize timeout value (zero if no timeout) and a current timestamp of now.**\]**
        if (waitTimeoutInMs && ((errorCode = tickcounter_get_current_ms(eventHubReceiverLL->tickCounter, &eventHubReceiverLL->lastActivityTimestamp)) != 0))
        {
            //**SRS_EVENTHUBRECEIVER_LL_29_303: \[**If cloning and/or adding the information fails for any reason, `EventHubReceiver_LL_ReceiveFromStartTimestamp*Async` shall fail and return EVENTHUBRECEIVER_ERROR.**\]**
            LogError("Could not get Tick Counter. Code:%d\r\n", errorCode);
            result = EVENTHUBRECEIVER_ERROR;
        }
        //**SRS_EVENTHUBRECEIVER_LL_29_304: \[**Create a filter string using format "apache.org:selector-filter:string" and "amqp.annotation.x-opt-enqueuedtimeutc > startTimestampInSec" using STRING_sprintf**\]**
        else if ((eventHubReceiverLL->receiverQueryFilter = EventHubConnectionAMQP_CreateStartTimeFilter(startTimestampInSec)) == NULL)
        {
            //**SRS_EVENTHUBRECEIVER_LL_29_305: \[**If filter string create fails then a log message will be logged and an error code of EVENTHUBRECEIVER_ERROR shall be returned.**\]**
            LogError("Operation not permitted as there is an exiting Receiver instance.\r\n");
            result = EVENTHUBRECEIVER_ERROR;
        }
        else
        {
            //**SRS_EVENTHUBRECEIVER_LL_29_302: \[**`EventHubReceiver_LL_ReceiveFromStartTimestamp*Async` shall record the callbacks and contexts in the EVENTHUBRECEIVER_LL_STRUCT.**\]** 
            eventHubReceiverLL->state = RECEIVER_STATE_ACTIVE;
            eventHubReceiverLL->callback.onEventReceiveCallback = onEventReceiveCallback;
            eventHubReceiverLL->callback.onEventReceiveUserContext = onEventReceiveUserContext;
            eventHubReceiverLL->callback.wasMessageReceived = 0;
            eventHubReceiverLL->callback.onEventReceiveErrorCallback = onEventReceiveErrorCallback;
            eventHubReceiverLL->callback.onEventReceiveErrorUserContext = onEventReceiveErrorUserContext;
            //**SRS_EVENTHUBRECEIVER_LL_29_306: \[**Initialize timeout value (zero if no timeout) and a current timestamp of now.**\]**
            eventHubReceiverLL->startTimestamp = startTimestampInSec;
            eventHubReceiverLL->waitTimeoutInMs = waitTimeoutInMs;
            eventHubReceiverLL->callback.onEventReceiveEndCallback = NULL;
            eventHubReceiverLL->callback.onEventReceiveEndUserContext = NULL;
            //**SRS_EVENTHUBRECEIVER_LL_29_308: \[**Otherwise `EventHubReceiver_LL_ReceiveFromStartTimestamp*Async` shall succeed and return EVENTHUBRECEIVER_OK.**\]**
            result = EVENTHUBRECEIVER_OK;
        }
    }

    return result;
}

EVENTHUBRECEIVER_RESULT EventHubReceiver_LL_ReceiveFromStartTimestampAsync
(
    EVENTHUBRECEIVER_LL_HANDLE eventHubReceiverLLHandle,
    EVENTHUBRECEIVER_ASYNC_CALLBACK onEventReceiveCallback,
    void *onEventReceiveUserContext,
    EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK onEventReceiveErrorCallback,
    void *onEventReceiveErrorUserContext,
    uint64_t startTimestampInSec
)
{
    return EHR_LL_ReceiveAsyncCommon(eventHubReceiverLLHandle,
                                        onEventReceiveCallback, onEventReceiveUserContext,
                                        onEventReceiveErrorCallback, onEventReceiveErrorUserContext,
                                        startTimestampInSec, 0);
}

EVENTHUBRECEIVER_RESULT EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync
(
    EVENTHUBRECEIVER_LL_HANDLE eventHubReceiverLLHandle,
    EVENTHUBRECEIVER_ASYNC_CALLBACK onEventReceiveCallback,
    void *onEventReceiveUserContext,
    EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK onEventReceiveErrorCallback,
    void *onEventReceiveErrorUserContext,
    uint64_t startTimestampInSec,
    unsigned int waitTimeoutInMs
)
{
    return EHR_LL_ReceiveAsyncCommon(eventHubReceiverLLHandle,
                                        onEventReceiveCallback, onEventReceiveUserContext,
                                        onEventReceiveErrorCallback, onEventReceiveErrorUserContext,
                                        startTimestampInSec, waitTimeoutInMs);
}

EVENTHUBRECEIVER_RESULT EventHubReceiver_LL_SetConnectionTracing
(
    EVENTHUBRECEIVER_LL_HANDLE eventHubReceiverLLHandle,
    bool traceEnabled
)
{
    EVENTHUBRECEIVER_LL_STRUCT* eventHubReceiverLL = (EVENTHUBRECEIVER_LL_STRUCT*)eventHubReceiverLLHandle;
    EVENTHUBRECEIVER_RESULT result;

    //**SRS_EVENTHUBRECEIVER_LL_29_800: \[**`EventHubReceiver_LL_SetConnectionTracing` shall fail and return EVENTHUBRECEIVER_INVALID_ARG if parameter eventHubReceiverLLHandle.**\]**
    if (eventHubReceiverLL == NULL)
    {
        LogError("Invalid arguments\r\n");
        result = EVENTHUBRECEIVER_INVALID_ARG;
    }
    else
    {
        //**SRS_EVENTHUBRECEIVER_LL_29_801: \[**`EventHubReceiver_LL_SetConnectionTracing` shall save the value of tracingOnOff in eventHubReceiverLLHandle**\]**
        eventHubReceiverLL->connectionTracing = traceEnabled;
        //**SRS_EVENTHUBRECEIVER_LL_29_802: \[**If an active connection has been setup, `EventHubReceiver_LL_SetConnectionTracing` shall be called with the value of connection_set_trace tracingOnOff**\]**
        EventHubConnectionAMQP_SetConnectionTracing(eventHubReceiverLL, traceEnabled);
        //**SRS_EVENTHUBRECEIVER_LL_29_803: \[**`Upon success, EventHubReceiver_LL_SetConnectionTracing` shall return EVENTHUBRECEIVER_OK**\]**
        result = EVENTHUBRECEIVER_OK;
    }

    return result;
}

static void EHR_LL_OnStateChanged(const void* context, MESSAGE_RECEIVER_STATE newState, MESSAGE_RECEIVER_STATE previousState)
{
    //**SRS_EVENTHUBRECEIVER_LL_29_600: \[**When `EHR_LL_OnStateChanged` is invoked, obtain the EventHubReceiverLL handle from the context and update the message receiver state with the new state received in the callback.**\]**
    EVENTHUBRECEIVER_LL_STRUCT* eventHubReceiverLL = (EVENTHUBRECEIVER_LL_STRUCT*)context;
    if ((newState != previousState) && (newState == MESSAGE_RECEIVER_STATE_ERROR))
    {
        //**SRS_EVENTHUBRECEIVER_LL_29_601: \[**If the new state is MESSAGE_RECEIVER_STATE_ERROR, and previous state is not MESSAGE_RECEIVER_STATE_ERROR, `EHR_LL_OnStateChanged` shall invoke the user supplied error callback along with error callback context`**\]**
        LogError("Message Receiver Error Observed.\r\n");
        if (eventHubReceiverLL->callback.onEventReceiveErrorCallback != NULL)
        {
            eventHubReceiverLL->callback.onEventReceiveErrorCallback(EVENTHUBRECEIVER_CONNECTION_RUNTIME_ERROR, eventHubReceiverLL->callback.onEventReceiveErrorUserContext);
        }
    }
}

static int ProcessApplicationProperties(MESSAGE_HANDLE message, EVENTDATA_HANDLE eventDataHandle)
{
    int result, errorCode;
    AMQP_VALUE handle = NULL;
    MAP_HANDLE eventDataMap;

    if ((eventDataMap = EventData_Properties(eventDataHandle)) == NULL)
    {
        LogError("EventData_Properties Failed %d.\r\n");
        result = __LINE__;
    }
    else if ((errorCode = message_get_application_properties(message, &handle)) != 0)
    {
        LogError("message_get_properties Failed. Code:%d.\r\n", errorCode);
        result = __LINE__;
    }
    else if (handle != NULL)
    {
        AMQP_VALUE descriptor, descriptorValue, uamqpMap;
        uint32_t idx;
        uint32_t propertyCount = 0;

        if ((descriptor = amqpvalue_get_inplace_descriptor(handle)) == NULL)
        {
            LogError("Unexpected Inplace Properties Received.\r\n");
            result = __LINE__;
        }
        else if ((descriptorValue = amqpvalue_get_inplace_described_value(handle)) == NULL)
        {
            LogError("Unexpected Inplace Described Properties Type.\r\n");
            result = __LINE__;
        } 
        else if ((errorCode = amqpvalue_get_map(descriptorValue, &uamqpMap)) != 0)
        {
            LogError("Could not get Map type. Code:%d.\r\n", errorCode);
            result = __LINE__;
        }
        else if ((errorCode = amqpvalue_get_map_pair_count(uamqpMap, &propertyCount)) != 0)
        {
            LogError("Could not Map KVP count. Code:%d.\r\n", errorCode);
            result = __LINE__;
        }
        else
        {
            result = 0;
            for (idx = 0; ((idx < propertyCount) && (!result)); idx++)
            {
                AMQP_VALUE amqpKey = NULL;
                AMQP_VALUE amqpValue = NULL;

                if ((errorCode = amqpvalue_get_map_key_value_pair(uamqpMap, idx, &amqpKey, &amqpValue)) != 0)
                {
                    LogError("Error Getting KVP. Code:%d.\r\n", errorCode);
                    result = __LINE__;
                }
                else
                {
                    const char *key = NULL, *value = NULL;
                    if ((amqpvalue_get_string(amqpKey, &key) != 0) || (amqpvalue_get_string(amqpValue, &value) != 0))
                    {
                        LogError("Error Getting AMQP KVP Strings.\r\n");
                        result = __LINE__;
                    }
                    else if ((errorCode = Map_Add(eventDataMap, key, value)) != MAP_OK)
                    {
                        LogError("Error Adding AMQP KVP to Event Data Map. Code:%d\r\n", errorCode);
                        result = __LINE__;
                    }
                }
                if (amqpKey != NULL)
                {
                    amqpvalue_destroy(amqpKey);
                }
                if (amqpValue != NULL)
                {
                    amqpvalue_destroy(amqpValue);
                }
            }
        }
        application_properties_destroy(handle);
    }
    else
    {
        result = 0;
    }

    return result;
}

static int ProcessMessageAnnotations(MESSAGE_HANDLE message, EVENTDATA_HANDLE eventDataHandle)
{
    static const char* KEY_NAME_ENQUEUED_TIME = "x-opt-enqueued-time";
    annotations handle = NULL;
    int errorCode, result;
    
    if ((errorCode = message_get_message_annotations(message, &handle)) != 0)
    {
        LogError("message_get_message_annotations Failed. Code:%d.\r\n", errorCode);
        result = __LINE__;
    }
    else if (handle != NULL)
    {
        AMQP_VALUE uamqpMap;
        uint32_t idx;
        uint32_t propertyCount = 0;
        if ((errorCode = amqpvalue_get_map(handle, &uamqpMap)) != 0)
        {
            LogError("Could not retrieve map from message annotations. Code%d\r\n", errorCode);
            result = __LINE__;
        }
        else if ((errorCode = amqpvalue_get_map_pair_count(uamqpMap, &propertyCount)) != 0)
        {
            LogError("Could not Map KVP count. Code:%d.\r\n", errorCode);
            result = __LINE__;
        }
        else
        {
            result = 0;
            for (idx = 0; ((idx < propertyCount) && (!result)); idx++)
            {
                AMQP_VALUE amqpKey = NULL;
                AMQP_VALUE amqpValue = NULL;

                if ((errorCode = amqpvalue_get_map_key_value_pair(uamqpMap, idx, &amqpKey, &amqpValue)) != 0)
                {
                    LogError("Error Getting KVP. Code:%d.\r\n", errorCode);
                    result = __LINE__;
                }
                else
                {
                    const char *key = NULL;
                    uint64_t timestampUTC = 0;
                    if (amqpvalue_get_symbol(amqpKey, &key) != 0)
                    {
                        LogError("Error Getting AMQP key Strings.\r\n");
                        result = __LINE__;
                    }
                    else
                    {
                        if (strcmp(key, KEY_NAME_ENQUEUED_TIME) == 0)
                        {
                            if ((errorCode = amqpvalue_get_timestamp(amqpValue, (int64_t*)(&timestampUTC))) != 0)
                            {
                                LogError("Could not get UTC timestamp. Code:%u\r\n", errorCode);
                                result = __LINE__;
                            }
                            else if ((errorCode = EventData_SetEnqueuedTimestampUTCInMs(eventDataHandle, timestampUTC)) != EVENTDATA_OK)
                            {
                                LogError("Could not set UTC timestamp in event data. Code:%u\r\n", errorCode);
                                result = __LINE__;
                            }
                        }
                    }
                }
                if (amqpKey != NULL)
                {
                    amqpvalue_destroy(amqpKey);
                }
                if (amqpValue != NULL)
                {
                    amqpvalue_destroy(amqpValue);
                }
            }
        }
        annotations_destroy(handle);
    }
    else
    {
        result = 0;
    }

    return result;
}

static AMQP_VALUE EHR_LL_OnMessageReceived(const void* context, MESSAGE_HANDLE message)
{
    EVENTHUBRECEIVER_LL_STRUCT* eventHubReceiverLL = (EVENTHUBRECEIVER_LL_STRUCT*)context;
    BINARY_DATA binaryData;
    EVENTDATA_HANDLE  eventDataHandle;
    AMQP_VALUE result;
    int errorCode;

    //**SRS_EVENTHUBRECEIVER_LL_29_700: \[**When `EHR_LL_OnMessageReceived` is invoked, message_get_body_amqp_data shall be called to obtain the data into a BINARY_DATA buffer.**\]**
    if ((errorCode = message_get_body_amqp_data(message, 0, &binaryData)) != 0)
    {
        //**SRS_EVENTHUBRECEIVER_LL_29_701: \[**If any errors are seen `EHR_LL_OnMessageReceived` shall reject the incoming message by calling messaging_delivery_rejected() and return.**\]**
        LogError("message_get_body_amqp_data Failed. Code:%d.\r\n", errorCode);
        result = messaging_delivery_rejected("Rejected due to failure reading AMQP message", "Failed reading message body");
    }
    //**SRS_EVENTHUBRECEIVER_LL_29_702: \[**`EHR_LL_OnMessageReceived` shall create a EVENT_DATA handle using EventData_CreateWithNewMemory and pass in the buffer data pointer and size as arguments.**\]**
    else if ((eventDataHandle = EventData_CreateWithNewMemory(binaryData.bytes, binaryData.length)) == NULL)
    {
        //**SRS_EVENTHUBRECEIVER_LL_29_701: \[**If any errors are seen `EHR_LL_OnMessageReceived` shall reject the incoming message by calling messaging_delivery_rejected() and return.**\]**
        LogError("EventData_CreateWithNewMemory Failed.\r\n");
        result = messaging_delivery_rejected("Rejected due to failure reading AMQP message", "Failed to allocate memory for received event data");
    }
    //**SRS_EVENTHUBRECEIVER_LL_29_704: \[**`EHR_LL_OnMessageReceived` shall obtain event data specific properties using message_get_message_annotations() and populate the EVENT_DATA handle with these properties.**\]**
    else if ((errorCode = ProcessMessageAnnotations(message, eventDataHandle)) != 0)
    {
        //**SRS_EVENTHUBRECEIVER_LL_29_701: \[**If any errors are seen `EHR_LL_OnMessageReceived` shall reject the incoming message by calling messaging_delivery_rejected() and return.**\]**
        LogError("ProcessMessageAnnotations Failed. Code:%d.\r\n", errorCode);
        EventData_Destroy(eventDataHandle);
        result = messaging_delivery_rejected("Rejected due to failure reading AMQP message", "Failed to process message properties");
    }
    //**SRS_EVENTHUBRECEIVER_LL_29_703: \[**`EHR_LL_OnMessageReceived` shall obtain the application properties using message_get_application_properties() and populate the EVENT_DATA handle map with these key value pairs.**\]**
    else if ((errorCode = ProcessApplicationProperties(message, eventDataHandle)) != 0)
    {
        //**SRS_EVENTHUBRECEIVER_LL_29_701: \[**If any errors are seen `EHR_LL_OnMessageReceived` shall reject the incoming message by calling messaging_delivery_rejected() and return.**\]**
        LogError("ProcessApplicationProperties Failed. Code:%d.\r\n", errorCode);
        EventData_Destroy(eventDataHandle);
        result = messaging_delivery_rejected("Rejected due to failure reading AMQP message", "Failed to process application properties");
    }
    else
    {
        //**SRS_EVENTHUBRECEIVER_LL_29_706: \[**After `EHR_LL_OnMessageReceived` invokes the user callback, messaging_delivery_accepted shall be called.**\]**
        eventHubReceiverLL->callback.wasMessageReceived = 1;
        eventHubReceiverLL->callback.onEventReceiveCallback(EVENTHUBRECEIVER_OK, eventDataHandle, eventHubReceiverLL->callback.onEventReceiveUserContext);
        EventData_Destroy(eventDataHandle);
        result = messaging_delivery_accepted();
    }

    return result;
}

void EventHubReceiver_LL_DoWork(EVENTHUBRECEIVER_LL_HANDLE eventHubReceiverLLHandle)
{
    EVENTHUBRECEIVER_LL_STRUCT* eventHubReceiverLL = (EVENTHUBRECEIVER_LL_STRUCT*)eventHubReceiverLLHandle;
    
    //**SRS_EVENTHUBRECEIVER_LL_29_450: \[**`EventHubReceiver_LL_DoWork` shall return immediately if the supplied handle is NULL**\]**
    if (eventHubReceiverLL == NULL)
    {
        LogError("Invalid arguments\r\n");
    }
    else if (eventHubReceiverLL->state == RECEIVER_STATE_INACTIVE_PENDING)
    {
        //**SRS_EVENTHUBRECEIVER_LL_29_919:\[**`EventHubReceiver_LL_DoWork` shall do the work to tear down the AMQP stack when a user had called `EventHubReceiver_LL_ReceiveEndAsync`.**\]**
        EventHubConnectionAMQP_ReceiverDeInitialize(eventHubReceiverLL);
        EventHubConnectionAMQP_DestroyFilter(eventHubReceiverLL->receiverQueryFilter);
        eventHubReceiverLL->receiverQueryFilter = NULL;
        eventHubReceiverLL->callback.onEventReceiveCallback = NULL;
        eventHubReceiverLL->callback.onEventReceiveUserContext = NULL;
        eventHubReceiverLL->callback.wasMessageReceived = 0;
        eventHubReceiverLL->callback.onEventReceiveErrorCallback = NULL;
        eventHubReceiverLL->callback.onEventReceiveErrorUserContext = NULL;
        eventHubReceiverLL->startTimestamp = 0;
        eventHubReceiverLL->waitTimeoutInMs = 0;
        eventHubReceiverLL->lastActivityTimestamp = 0;
        eventHubReceiverLL->state = RECEIVER_STATE_INACTIVE;
        //**SRS_EVENTHUBRECEIVER_LL_29_928: \[**Upon Success, `EventHubReceiver_LL_DoWork` shall invoke the onEventReceiveEndCallback along with onEventReceiveEndUserContext with result code EVENTHUBRECEIVER_OK.**\]**
        //**SRS_EVENTHUBRECEIVER_LL_29_929: \[**Upon failure, `EventHubReceiver_LL_DoWork` shall invoke the onEventReceiveEndCallback along with onEventReceiveEndUserContext with result code EVENTHUBRECEIVER_ERROR.**\]**
        if (eventHubReceiverLL->callback.onEventReceiveEndCallback)
        {
            eventHubReceiverLL->callback.onEventReceiveEndCallback(EVENTHUBRECEIVER_OK, eventHubReceiverLL->callback.onEventReceiveEndUserContext);
            eventHubReceiverLL->callback.onEventReceiveEndCallback = NULL;
            eventHubReceiverLL->callback.onEventReceiveEndUserContext = NULL;
        }
    }
    else if (eventHubReceiverLL->state == RECEIVER_STATE_ACTIVE)
    {
        //**SRS_EVENTHUBRECEIVER_LL_29_451: \[**`EventHubReceiver_LL_DoWork` shall initialize and bring up the uAMQP stack if it has not already brought up**\]** 
        //**SRS_EVENTHUBRECEIVER_LL_29_452: \[**`EventHubReceiver_LL_DoWork` shall create a message receiver if not already created**\]** 
        //**SRS_EVENTHUBRECEIVER_LL_29_570: \[**`EventHubReceiver_LL_DoWork` shall call messagereceiver_create and pass link created previously along with `EHR_LL_OnStateChanged` as the ON_MESSAGE_RECEIVER_STATE_CHANGED parameter and the EVENTHUBRECEIVER_LL_HANDLE as the callback context**\]**
        //**SRS_EVENTHUBRECEIVER_LL_29_571: \[**Once the message receiver is created, `EventHubReceiver_LL_DoWork` shall call messagereceiver_open and pass in `EHR_LL_OnMessageReceived` as the ON_MESSAGE_RECEIVED parameter and the EVENTHUBRECEIVER_LL_HANDLE as the callback context**\]**
        int errorCode;
        if ((eventHubReceiverLL->receiverAMQPConnectionState == RECEIVER_AMQP_UNINITIALIZED) &&
            ((errorCode = EventHubConnectionAMQP_ReceiverInitialize(eventHubReceiverLL,
                                                                    EHR_LL_OnStateChanged, (void*)eventHubReceiverLL,
                                                                    EHR_LL_OnMessageReceived, (void*)eventHubReceiverLL)) != 0))
        {
            LogError("Error initializing uAMPQ stack. Code:%d\r\n", errorCode);
        }
        else
        {
            tickcounter_ms_t nowTimestamp, timespan;

            //**SRS_EVENTHUBRECEIVER_LL_29_453: \[**`EventHubReceiver_LL_DoWork` shall invoke connection_dowork **\]** 
            EventHubConnectionAMQP_ConnectionDoWork(eventHubReceiverLL);

            //**SRS_EVENTHUBRECEIVER_LL_29_454: \[**`EventHubReceiver_LL_DoWork` shall manage timeouts as long as the user specified timeout value is non zero **\]**
            if (eventHubReceiverLL->waitTimeoutInMs != 0)
            {
                if ((errorCode = tickcounter_get_current_ms(eventHubReceiverLL->tickCounter, &nowTimestamp)) != 0)
                {
                    //**SRS_EVENTHUBRECEIVER_LL_29_753: \[**If at any point there is an error for a specific eventHubReceiverLLHandle, the user callback should be invoked along with the error code.**\]**
                    LogError("Could not get Tick Counter. Code:%d\r\n", errorCode);
                }
                else
                {
                    if (eventHubReceiverLL->callback.wasMessageReceived == 0)
                    {
                        //**SRS_EVENTHUBRECEIVER_LL_29_751: \[**If a message was not received, check if the time now minus the last activity time is greater than or equal to the user specified timeout. If greater, the user registered callback is invoked along with the user supplied context with status code EVENTHUBRECEIVER_TIMEOUT. Last activity time shall be updated to the current time i.e. now.**\]**
                        timespan = nowTimestamp - eventHubReceiverLL->lastActivityTimestamp;
                        if (timespan >= (uint64_t)(eventHubReceiverLL->waitTimeoutInMs))
                        {
                            if (eventHubReceiverLL->callback.onEventReceiveCallback != NULL)
                            {
                                eventHubReceiverLL->callback.onEventReceiveCallback(EVENTHUBRECEIVER_TIMEOUT, NULL, eventHubReceiverLL->callback.onEventReceiveUserContext);
                            }
                            eventHubReceiverLL->lastActivityTimestamp = nowTimestamp;
                        }
                    }
                    else
                    {
                        //**SRS_EVENTHUBRECEIVER_LL_29_750: \[**`EventHubReceiver_LL_DoWork` shall check if a message was received, if so, reset the last activity time to the current time i.e. now**\]**
                        eventHubReceiverLL->lastActivityTimestamp = nowTimestamp;
                    }
                }
            }
            eventHubReceiverLL->callback.wasMessageReceived = 0;
        }
    }
}