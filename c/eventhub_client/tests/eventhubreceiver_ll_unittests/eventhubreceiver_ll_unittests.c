// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <time.h>
#include <stddef.h>

static void* TestHook_malloc(size_t size)
{
    return malloc(size);
}

static void TestHook_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umocktypes_charptr.h"
#include "umock_c_negative_tests.h"

#define ENABLE_MOCKS

#include "azure_c_shared_utility/connection_string_parser.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/platform.h"

#include "azure_uamqp_c/connection.h"
#include "azure_uamqp_c/session.h"
#include "azure_uamqp_c/link.h"
#include "azure_uamqp_c/message.h"
#include "azure_uamqp_c/messaging.h"
#include "azure_uamqp_c/message_receiver.h"
#include "azure_uamqp_c/saslclientio.h"
#include "azure_uamqp_c/sasl_plain.h"
#include "eventdata.h"
#include "version.h"
#undef  ENABLE_MOCKS

// interface under test
#include "eventhubreceiver_ll.h"

//#################################################################################################
// EventHubReceiver LL Test Defines and Data types
//#################################################################################################
#define TEST_EVENTHUB_RECEIVER_LL_VALID                 (EVENTHUBRECEIVER_LL_HANDLE)0x43
#define TEST_EVENTDATA_HANDLE_VALID                     (EVENTDATA_HANDLE)0x45

#define TEST_MAP_HANDLE_VALID                           (MAP_HANDLE)0x46
#define TEST_TICK_COUNTER_HANDLE_VALID                  (TICK_COUNTER_HANDLE)0x47
// ensure that this is always >= 1000
#define TEST_EVENTHUB_RECEIVER_TIMEOUT_MS               1000
#define TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP            10000

#define TEST_CONNECTION_STRING_HANDLE_VALID             (STRING_HANDLE)0x100
#define TEST_EVENTHUBPATH_STRING_HANDLE_VALID           (STRING_HANDLE)0x101
#define TEST_CONSUMERGROUP_STRING_HANDLE_VALID          (STRING_HANDLE)0x102
#define TEST_PARTITIONID_STRING_HANDLE_VALID            (STRING_HANDLE)0x103
#define TEST_SHAREDACCESSKEYNAME_STRING_HANDLE_VALID    (STRING_HANDLE)0x104
#define TEST_SHAREDACCESSKEY_STRING_HANDLE_VALID        (STRING_HANDLE)0x105
#define TEST_HOSTNAME_STRING_HANDLE_VALID               (STRING_HANDLE)0x106
#define TEST_TARGETADDRESS_STRING_HANDLE_VALID          (STRING_HANDLE)0x107
#define TEST_CONNECTION_ENDPOINT_HANDLE_VALID           (STRING_HANDLE)0x108
#define TEST_FILTER_QUERY_STRING_HANDLE_VALID           (STRING_HANDLE)0x109

#define TEST_SASL_PLAIN_INTERFACE_HANDLE                (SASL_MECHANISM_INTERFACE_DESCRIPTION*)0x200
#define TEST_SASL_MECHANISM_HANDLE_VALID                (SASL_MECHANISM_HANDLE)0x201
#define TEST_TLS_IO_INTERFACE_DESCRPTION_HANDLE_VALID   (const IO_INTERFACE_DESCRIPTION*)0x202
#define TEST_SASL_CLIENT_IO_HANDLE_VALID                (const IO_INTERFACE_DESCRIPTION*)0x203
#define TEST_TLS_XIO_VALID_HANDLE                       (XIO_HANDLE)0x204
#define TEST_SASL_XIO_VALID_HANDLE                      (XIO_HANDLE)0x205
#define TEST_CONNECTION_HANDLE_VALID                    (CONNECTION_HANDLE)0x206
#define TEST_SESSION_HANDLE_VALID                       (SESSION_HANDLE)0x207
#define TEST_AMQP_VALUE_MAP_HANDLE_VALID                (AMQP_VALUE)0x208
#define TEST_AMQP_VALUE_SYMBOL_HANDLE_VALID             (AMQP_VALUE)0x209
#define TEST_AMQP_VALUE_FILTER_HANDLE_VALID             (AMQP_VALUE)0x210
#define TEST_AMQP_VALUE_DESCRIBED_HANDLE_VALID          (AMQP_VALUE)0x211
#define TEST_SOURCE_HANDLE_VALID                        (SOURCE_HANDLE)0x212
#define TEST_AMQP_SOURCE_HANDLE_VALID                   (AMQP_VALUE)0x213
#define TEST_MESSAGING_TARGET_VALID                     (AMQP_VALUE)0x214
#define TEST_LINK_HANDLE_VALID                          (LINK_HANDLE)0X215
#define TEST_MESSAGE_RECEIVER_HANDLE_VALID              (MESSAGE_RECEIVER_HANDLE)0x216
#define TEST_MESSAGE_HANDLE_VALID                       (MESSAGE_HANDLE)0x217
#define TEST_AMQP_DUMMY_KVP_KEY_VALUE                   (AMQP_VALUE)0x218
#define TEST_AMQP_DUMMY_KVP_VAL_VALUE                   (AMQP_VALUE)0x219


#define TEST_MESSAGE_APP_PROPS_HANDLE_VALID 			(AMQP_VALUE)  0x300
#define TEST_EVENT_DATA_PROPS_MAP_HANDLE_VALID      	(MAP_HANDLE)  0x301
#define TEST_AMQP_INPLACE_DESCRIPTOR 					(AMQP_VALUE)  0x302
#define TEST_AMQP_INPLACE_DESCRIBED_DESCRIPTOR 			(AMQP_VALUE)  0x303
#define TEST_AMQP_MESSAGE_APP_PROPS_MAP_HANDLE_VALID  	(AMQP_VALUE)  0x304
#define TEST_AMQP_MESSAGE_ANNOTATIONS_VALID             (annotations) 0x305
#define TEST_AMQP_MESSAGE_ANNOTATIONS_MAP_HANDLE_VALID  (AMQP_VALUE)  0x306

#define TEST_CONNECTION_STRING_HANDLE_INVALID           (STRING_HANDLE)0x400

#define TESTHOOK_STRING_BUFFER_SZ                       256

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

typedef struct TEST_HOOK_MAP_KVP_STRUCT_TAG
{
    const char*   key;
    const char*   value;
    STRING_HANDLE handle;
    STRING_HANDLE handleClone;
} TEST_HOOK_MAP_KVP_STRUCT;

typedef struct TEST_EVENTHUB_RECEIVER_ASYNC_CALLBACK_TAG
{
    EVENTHUBRECEIVER_LL_HANDLE eventHubRxHandle;
    EVENTHUBRECEIVER_ASYNC_CALLBACK rxCallback;
    EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK rxErrorCallback;
    ON_MESSAGE_RECEIVER_STATE_CHANGED onMsgChangedCallback;
    ON_MESSAGE_RECEIVED onMsgReceivedCallback;
    void* rxCallbackCtxt;
    void* rxErrorCallbackCtxt;
    void* rxEndCallbackCtxt;
    const void* onMsgChangedCallbackCtxt;
    const void* onMsgReceivedCallbackCtxt;
    int rxCallbackCalled;
    int rxErrorCallbackCalled;
    int rxEndCallbackCalled;
    EVENTHUBRECEIVER_RESULT rxCallbackResult;
    EVENTHUBRECEIVER_RESULT rxErrorCallbackResult;
    EVENTHUBRECEIVER_RESULT rxEndCallbackResult;
    int messageApplicationPropertiesNULLTest;
    int messageAnnotationsNULLTest;
} TEST_EVENTHUB_RECEIVER_ASYNC_CALLBACK;

//#################################################################################################
// EventHubReceiver LL Test Data
//#################################################################################################
static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

static int IS_INVOKED_STRING_construct_sprintf = 0;
static int STRING_construct_sprintf_Negative_Test = 0;
static int ConnectionDowWorkCalled = 0;

static const char CONNECTION_STRING[]    = "Endpoint=sb://servicebusName.servicebus.windows.net/;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=ICT5KKSJR/DW7OZVEQ7OPSSXU5TRMR6AIWLGI5ZIT/8=";
static const char EVENTHUB_PATH[]        = "eventHubName";
static const char CONSUMER_GROUP[]       = "ConsumerGroup";
static const char PARTITION_ID[]         = "0";
static const char TEST_HOST_NAME_KEY[]   = "servicebusName.servicebus.windows.net/";
static const char TEST_HOST_NAME_VALUE[] = "servicebusName.servicebus.windows.net";
static const size_t TEST_HOST_NAME_VALUE_LEN  = sizeof(TEST_HOST_NAME_VALUE) - 1;
static const char FILTER_BY_TIMESTAMP_VALUE[] = "amqp.annotation.x-opt-enqueuedtimeutc";

static const char* KEY_NAME_ENQUEUED_TIME = "x-opt-enqueued-time";

static TEST_HOOK_MAP_KVP_STRUCT connectionStringKVP[] =
{
    { "Endpoint", "sb://servicebusName.servicebus.windows.net/", TEST_CONNECTION_ENDPOINT_HANDLE_VALID, NULL },
    { "SharedAccessKeyName", "RootManageSharedAccessKey", TEST_SHAREDACCESSKEYNAME_STRING_HANDLE_VALID, NULL },
    { "SharedAccessKey", "ICT5KKSJR/DW7OZVEQ7OPSSXU5TRMR6AIWLGI5ZIT/8=", TEST_SHAREDACCESSKEY_STRING_HANDLE_VALID, NULL },
    { CONNECTION_STRING, CONNECTION_STRING, TEST_CONNECTION_STRING_HANDLE_VALID, NULL },
    { CONSUMER_GROUP, CONSUMER_GROUP, TEST_CONSUMERGROUP_STRING_HANDLE_VALID, NULL },
    { EVENTHUB_PATH, EVENTHUB_PATH, TEST_EVENTHUBPATH_STRING_HANDLE_VALID, TEST_TARGETADDRESS_STRING_HANDLE_VALID },
    { PARTITION_ID, PARTITION_ID, TEST_PARTITIONID_STRING_HANDLE_VALID, NULL },
    { TEST_HOST_NAME_KEY, TEST_HOST_NAME_VALUE, TEST_HOSTNAME_STRING_HANDLE_VALID, NULL },
    { "FilterByTimestamp", FILTER_BY_TIMESTAMP_VALUE, TEST_FILTER_QUERY_STRING_HANDLE_VALID, NULL },
    { "TargetAddress", "servicebusName.servicebus.windows.net", TEST_TARGETADDRESS_STRING_HANDLE_VALID, NULL },
    { NULL, NULL, NULL }
};

static TEST_EVENTHUB_RECEIVER_ASYNC_CALLBACK OnRxCBStruct;
static void* OnRxCBCtxt    = (void*)&OnRxCBStruct;
static void* OnRxEndCBCtxt = (void*)&OnRxCBStruct;
static void* OnErrCBCtxt   = (void*)&OnRxCBStruct;

//#################################################################################################
// EventHubReceiver LL Test Helper Implementations
//#################################################################################################
int TestHelper_Map_GetIndexByKey(const char* key)
{
    bool found = false;
    int idx = 0, result;

    while (connectionStringKVP[idx].key != NULL)
    {
        if (strcmp(connectionStringKVP[idx].key, key) == 0)
        {
            found = true;
            break;
        }
        idx++;
    }

    if (!found)
    {
        printf("Test Map, could not find by key:%s \r\n", key);
        result = -1;
    }
    else
    {
        result = idx;
    }
        
    return result;
}

int TestHelper_Map_GetIndexByValue(const char* value)
{
    bool found = false;
    int idx = 0, result;

    while (connectionStringKVP[idx].key != NULL)
    {
        if (strcmp(connectionStringKVP[idx].value, value) == 0)
        {
            found = true;
            break;
        }
        idx++;
    }

    if (!found)
    {
        printf("Test Map, could not find by value:%s \r\n", value);
        result = -1;
    }
    else
    {
        result = idx;
    }
    return result;
}

int TestHelper_Map_GetIndexByStringHandle(STRING_HANDLE h)
{
    bool found = false;
    int idx = 0, result;

    if (h)
    {
        while (connectionStringKVP[idx].key != NULL)
        {
            if (connectionStringKVP[idx].handle == h)
            {
                found = true;
                break;
            }
            idx++;
        }
    }

    if (!found)
    {
        printf("Test Map, could not find by string handle:%p \r\n", h);
        result = -1;
    }
    else
    {
        result = idx;
    }
    return result;
}

const char* TestHelper_Map_GetKey(int index)
{
    ASSERT_ARE_NOT_EQUAL(int, -1, index);
    return connectionStringKVP[index].key;
}

const char* TestHelper_Map_GetValue(int index)
{
    ASSERT_ARE_NOT_EQUAL(int, -1, index);
    return connectionStringKVP[index].value;
}

STRING_HANDLE TestHelper_Map_GetStringHandle(int index)
{
    ASSERT_ARE_NOT_EQUAL(int, -1, index);
    return connectionStringKVP[index].handle;
}

STRING_HANDLE TestHelper_Map_GetStringHandleClone(int index)
{
    ASSERT_ARE_NOT_EQUAL(int, -1, index);
    return connectionStringKVP[index].handleClone;
}

void TestHelper_SetNullMessageApplicationProperties(void)
{
    OnRxCBStruct.messageApplicationPropertiesNULLTest = 1;
}

int TestHelper_IsNullMessageApplicationProperties(void)
{
    return OnRxCBStruct.messageApplicationPropertiesNULLTest ? true : false;
}

void TestHelper_SetNullMessageAnnotations(void)
{
    OnRxCBStruct.messageAnnotationsNULLTest = 1;
}

int TestHelper_IsNullMessageAnnotations(void)
{
    return OnRxCBStruct.messageAnnotationsNULLTest ? true : false;
}

static void TestHelper_ResetTestGlobalData(void)
{
    memset(&OnRxCBStruct, 0, sizeof(TEST_EVENTHUB_RECEIVER_ASYNC_CALLBACK));
    IS_INVOKED_STRING_construct_sprintf = 0;
    STRING_construct_sprintf_Negative_Test = 0;
    ConnectionDowWorkCalled = 0;
}

static int TestHelper_isSTRING_construct_sprintfInvoked(void)
{
    return IS_INVOKED_STRING_construct_sprintf;
}

void TestHelper_SetNegativeTestSTRING_construct_sprintf(void)
{
    STRING_construct_sprintf_Negative_Test = 1;
}

static int TestHelper_isConnectionDoWorkInvoked(void)
{
    return ConnectionDowWorkCalled;
}
//#################################################################################################
// EventHubReceiver LL Test Hook Implementations
//#################################################################################################
static void TestHook_OnUMockCError(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

#ifdef __cplusplus
extern "C"
{
#endif
    STRING_HANDLE STRING_construct_sprintf(const char* format, ...)
    {
        (void)format;
        IS_INVOKED_STRING_construct_sprintf = 1;
        if (STRING_construct_sprintf_Negative_Test)
        {
            return NULL;
        }
        return TEST_FILTER_QUERY_STRING_HANDLE_VALID;
    }
#ifdef __cplusplus
}
#endif

static const char* TestHoook_Map_GetValueFromKey(MAP_HANDLE handle, const char* key)
{
    (void)handle;
    return TestHelper_Map_GetValue(TestHelper_Map_GetIndexByKey(key));
}

static STRING_HANDLE TestHook_STRING_construct(const char* psz)
{
    return TestHelper_Map_GetStringHandle(TestHelper_Map_GetIndexByValue(psz));
}

static STRING_HANDLE TestHook_STRING_construct_n(const char* psz, size_t n)
{
    int idx;
    char valueString[TESTHOOK_STRING_BUFFER_SZ];
    ASSERT_ARE_NOT_EQUAL(size_t, 0, n);
    // n + 1 because this API expects to allocate n + 1 bytes with the nth being null term char
    ASSERT_IS_FALSE(n + 1 > TESTHOOK_STRING_BUFFER_SZ);
    memcpy(valueString, psz, n);
    // always null term
    valueString[n] = 0;
    idx = TestHelper_Map_GetIndexByValue(valueString);
    size_t valueLen = strlen(TestHelper_Map_GetValue(idx));
    ASSERT_ARE_EQUAL(size_t, valueLen, n);
    return TestHelper_Map_GetStringHandle(idx);
}

static STRING_HANDLE TestHook_STRING_clone(STRING_HANDLE handle)
{
    return TestHelper_Map_GetStringHandleClone(TestHelper_Map_GetIndexByStringHandle(handle));
}

static const char* TestHook_STRING_c_str(STRING_HANDLE handle)
{
    return TestHelper_Map_GetValue(TestHelper_Map_GetIndexByStringHandle(handle));
}

static XIO_HANDLE TestHook_xio_create(const IO_INTERFACE_DESCRIPTION* io_interface_description, const void* xio_create_parameters)
{
    (void)xio_create_parameters;
    XIO_HANDLE result = NULL; 
    if (io_interface_description == TEST_TLS_IO_INTERFACE_DESCRPTION_HANDLE_VALID)
    {
        result = TEST_TLS_XIO_VALID_HANDLE;
    }
    else if (io_interface_description == TEST_SASL_CLIENT_IO_HANDLE_VALID)
    {
        result = TEST_SASL_XIO_VALID_HANDLE;
    }
    ASSERT_IS_NOT_NULL(result);

    return result;
}

static void TestHook_connection_dowork(CONNECTION_HANDLE h)
{
    (void)h;
    ConnectionDowWorkCalled = 1;
}

static MESSAGE_RECEIVER_HANDLE TestHook_messagereceiver_create(LINK_HANDLE link, ON_MESSAGE_RECEIVER_STATE_CHANGED on_message_receiver_state_changed, void* context)
{
    (void)link;
    OnRxCBStruct.onMsgChangedCallback = on_message_receiver_state_changed;
    OnRxCBStruct.onMsgChangedCallbackCtxt = context;
    return TEST_MESSAGE_RECEIVER_HANDLE_VALID;
}

static int TestHook_messagereceiver_open(MESSAGE_RECEIVER_HANDLE message_receiver, ON_MESSAGE_RECEIVED on_message_received, const void* callback_context)
{
    (void)message_receiver;
    OnRxCBStruct.onMsgReceivedCallback = on_message_received;
    OnRxCBStruct.onMsgReceivedCallbackCtxt = callback_context;
    return 0;
}

static int TestHook_message_get_message_annotations(MESSAGE_HANDLE message, annotations* message_annotations)
{
    (void)message;
    if (OnRxCBStruct.messageAnnotationsNULLTest)
    {
        *message_annotations = NULL;
    }
    else
    {
        *message_annotations = TEST_AMQP_MESSAGE_ANNOTATIONS_VALID;
    }

    return 0;
}

static int TestHook_message_get_application_properties(MESSAGE_HANDLE message, AMQP_VALUE* application_properties)
{
    (void)message;
    if (OnRxCBStruct.messageApplicationPropertiesNULLTest)
    {
        *application_properties = NULL;
    }
    else
    {
        *application_properties = TEST_MESSAGE_APP_PROPS_HANDLE_VALID;
    }

    return 0;
}

static int TestHook_amqpvalue_get_map(AMQP_VALUE value, AMQP_VALUE* map_value)
{
    if (value == TEST_AMQP_INPLACE_DESCRIBED_DESCRIPTOR)
    {
        *map_value = TEST_AMQP_MESSAGE_APP_PROPS_MAP_HANDLE_VALID;
    }
    else
    {
        *map_value = TEST_AMQP_MESSAGE_ANNOTATIONS_MAP_HANDLE_VALID;
    }
    
    return 0;
}

static int TestHook_amqpvalue_get_map_pair_count(AMQP_VALUE map, uint32_t* pair_count)
{
    (void)map;
    *pair_count = 1;
    return 0;
}

static int TestHook_amqpvalue_get_map_key_value_pair(AMQP_VALUE map, uint32_t index, AMQP_VALUE* key, AMQP_VALUE* value)
{
    (void)map;
    (void)index;
    *key = TEST_AMQP_DUMMY_KVP_KEY_VALUE;
    *value = TEST_AMQP_DUMMY_KVP_VAL_VALUE;
    return 0;
}

static int TestHook_amqpvalue_get_symbol(AMQP_VALUE value, const char** symbol_value)
{
    (void)value;
    *symbol_value = KEY_NAME_ENQUEUED_TIME;
    return 0;
}

/**
    @note this function is truly only capable of 1 sec precision and thus is accurate only at second transitions.
    The implementation here prefers portability over accuracy as sub second precision is not a requirement for 
    these tests.
*/
static int TestHook_tickcounter_get_current_ms(TICK_COUNTER_HANDLE tick_counter, tickcounter_ms_t* current_ms)
{
    int result;
    if (tick_counter == TEST_TICK_COUNTER_HANDLE_VALID)
    {
        time_t nowTS = time(NULL);
        if (nowTS == (time_t)(-1))
        {
            result = -1;
        }
        else
        {
            *current_ms = ((tickcounter_ms_t)nowTS) * 1000;
            result = 0;
        }
    }
    else
    {
        result = -2;
    }

    return result;
}

//#################################################################################################
// EventHubReceiver LL Callback Implementations
//#################################################################################################
static void EventHubHReceiver_LL_OnRxCB(EVENTHUBRECEIVER_RESULT result, EVENTDATA_HANDLE eventDataHandle, void* userContext)
{
    (void)userContext;
    if (result == EVENTHUBRECEIVER_OK) ASSERT_IS_NOT_NULL_WITH_MSG(eventDataHandle, "Unexpected NULL Data Handle");
    if (result != EVENTHUBRECEIVER_OK) ASSERT_IS_NULL_WITH_MSG(eventDataHandle, "Unexpected Non NULL Data Handle");
    OnRxCBStruct.rxCallbackCalled = 1;
    OnRxCBStruct.rxCallbackResult = result;
}

static void EventHubHReceiver_LL_OnRxNoTimeoutCB(EVENTHUBRECEIVER_RESULT result, EVENTDATA_HANDLE eventDataHandle, void* userContext)
{
    (void)eventDataHandle;
    (void)userContext;
    ASSERT_ARE_NOT_EQUAL(int, EVENTHUBRECEIVER_TIMEOUT, result);
    OnRxCBStruct.rxCallbackCalled = 1;
    OnRxCBStruct.rxCallbackResult = result;
}

static void EventHubHReceiver_LL_OnErrCB(EVENTHUBRECEIVER_RESULT errorCode, void* userContext)
{
    (void)userContext;
    OnRxCBStruct.rxErrorCallbackCalled = 1;
    OnRxCBStruct.rxErrorCallbackResult = errorCode;
}

static void EventHubHReceiver_LL_OnRxEndCB(EVENTHUBRECEIVER_RESULT result, void* userContext)
{
    (void)userContext;
    OnRxCBStruct.rxEndCallbackCalled = 1;
    OnRxCBStruct.rxEndCallbackResult = result;
}

//#################################################################################################
// EventHubReceiver LL Common Callstack Test Setup Functions
//#################################################################################################
static uint64_t TestSetupCallStack_Create(void)
{
    uint64_t failedFunctionBitmask = 0;
    int i = 0;

    // arrange
    umock_c_reset_all_calls();

    EXPECTED_CALL(EventHubClient_GetVersionString());
    i++; // this function is not expected to fail and thus does not included in the bitmask

    // LL internal data structure allocation
    EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    EXPECTED_CALL(tickcounter_create());
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    // create string handle for temp connection string handling
    STRICT_EXPECTED_CALL(STRING_construct(CONNECTION_STRING));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    // parse connection string
    STRICT_EXPECTED_CALL(connectionstringparser_parse(TEST_CONNECTION_STRING_HANDLE_VALID));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    // create string handle for event hub path 
    STRICT_EXPECTED_CALL(STRING_construct(EVENTHUB_PATH));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    // create string handle for (hostName) "Endpoint"
    STRICT_EXPECTED_CALL(Map_GetValueFromKey(TEST_MAP_HANDLE_VALID, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    // create string handle for host name TEST_HOST_NAME_VALUE
    STRICT_EXPECTED_CALL(STRING_construct_n(IGNORED_PTR_ARG, TEST_HOST_NAME_VALUE_LEN))
        .IgnoreArgument(1);
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    // create string handle for Key:SharedAccessKeyName value in connection string
    STRICT_EXPECTED_CALL(Map_GetValueFromKey(TEST_MAP_HANDLE_VALID, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    // create string handle for Key:SharedAccessKey value in connection string
    STRICT_EXPECTED_CALL(Map_GetValueFromKey(TEST_MAP_HANDLE_VALID, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    // destroy map as connection string processing is done
    STRICT_EXPECTED_CALL(Map_Destroy(TEST_MAP_HANDLE_VALID));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    // destroy temp connection string handle
    STRICT_EXPECTED_CALL(STRING_delete(TEST_CONNECTION_STRING_HANDLE_VALID));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    // create string handle for consumer group
    STRICT_EXPECTED_CALL(STRING_construct(CONSUMER_GROUP));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    // create string handle for partitionId
    STRICT_EXPECTED_CALL(STRING_construct(PARTITION_ID));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    // creation of string handle for partition URI 
    STRICT_EXPECTED_CALL(STRING_clone(TEST_EVENTHUBPATH_STRING_HANDLE_VALID));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(STRING_concat(TEST_TARGETADDRESS_STRING_HANDLE_VALID, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(TEST_TARGETADDRESS_STRING_HANDLE_VALID, TEST_CONSUMERGROUP_STRING_HANDLE_VALID));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(STRING_concat(TEST_TARGETADDRESS_STRING_HANDLE_VALID, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(TEST_TARGETADDRESS_STRING_HANDLE_VALID, TEST_PARTITIONID_STRING_HANDLE_VALID));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    // ensure that we do not have more that 64 mocked functions
    ASSERT_IS_FALSE_WITH_MSG((i > 64), "More Mocked Functions than permitted bitmask width");

    return failedFunctionBitmask;
}

static void TestSetupCallStack_ReceiveFromStartTimestampCommon(unsigned int waitTimeInMS)
{
    TestHelper_ResetTestGlobalData();
    umock_c_reset_all_calls();

    if (waitTimeInMS) STRICT_EXPECTED_CALL(tickcounter_get_current_ms(TEST_TICK_COUNTER_HANDLE_VALID, IGNORED_PTR_ARG)).IgnoreArgument(2);

    //STRICT_EXPECTED_CALL(STRING_construct_sprintf(IGNORED_PTR_ARG, nowTS)).IgnoreArgument(1);
    // Since the above is not possible to do with the mocking framework 
    // use TestHelper_isSTRING_construct_sprintfInvoked() instead to see if STRING_construct_sprintf was invoked at all
    // Ex: ASSERT_ARE_EQUAL_WITH_MSG(int, 1, TestHelper_isSTRING_construct_sprintfInvoked(), "Failed STRING_consturct_sprintf Value Test");
}

static void TestSetupCallStack_ReceiveEndAsync(void)
{
    TestHelper_ResetTestGlobalData();
    umock_c_reset_all_calls();
}

static uint64_t TestSetupCallStack_DoWorkInActiveTearDown(void)
{
    uint64_t failedFunctionBitmask = 0;
    int i = 0;

    TestHelper_ResetTestGlobalData();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(messagereceiver_close(TEST_MESSAGE_RECEIVER_HANDLE_VALID));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    STRICT_EXPECTED_CALL(messagereceiver_destroy(TEST_MESSAGE_RECEIVER_HANDLE_VALID));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    STRICT_EXPECTED_CALL(link_destroy(TEST_LINK_HANDLE_VALID));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    STRICT_EXPECTED_CALL(session_destroy(TEST_SESSION_HANDLE_VALID));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    STRICT_EXPECTED_CALL(connection_destroy(TEST_CONNECTION_HANDLE_VALID));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    STRICT_EXPECTED_CALL(xio_destroy(TEST_SASL_XIO_VALID_HANDLE));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    STRICT_EXPECTED_CALL(xio_destroy(TEST_TLS_XIO_VALID_HANDLE));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    EXPECTED_CALL(saslmechanism_destroy(IGNORED_PTR_ARG));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    STRICT_EXPECTED_CALL(STRING_delete(TEST_FILTER_QUERY_STRING_HANDLE_VALID));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    // ensure that we do not have more that 64 mocked functions
    ASSERT_IS_FALSE_WITH_MSG((i > 64), "More Mocked Functions than permitted bitmask width");

    return failedFunctionBitmask;
}

static uint64_t TestSetupCallStack_DoWorkActiveStackBringup(void)
{
    uint64_t failedFunctionBitmask = 0;
    int i = 0;

    TestHelper_ResetTestGlobalData();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(saslplain_get_interface());
    failedFunctionBitmask |= ((uint64_t)1 << i++);
    
    // SharedAccessKeyName
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_SHAREDACCESSKEYNAME_STRING_HANDLE_VALID));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    // SharedAccessKey
    STRICT_EXPECTED_CALL(STRING_c_str(TEST_SHAREDACCESSKEY_STRING_HANDLE_VALID));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    STRICT_EXPECTED_CALL(saslmechanism_create(TEST_SASL_PLAIN_INTERFACE_HANDLE, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(STRING_c_str(TEST_HOSTNAME_STRING_HANDLE_VALID));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    STRICT_EXPECTED_CALL(platform_get_default_tlsio());
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(xio_create(TEST_TLS_IO_INTERFACE_DESCRPTION_HANDLE_VALID, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(saslclientio_get_interface_description());
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(xio_create(TEST_SASL_CLIENT_IO_HANDLE_VALID, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(connection_create(TEST_SASL_XIO_VALID_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2).IgnoreArgument(3).IgnoreArgument(4).IgnoreArgument(5);
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(connection_set_trace(TEST_CONNECTION_HANDLE_VALID, 0));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    STRICT_EXPECTED_CALL(session_create(TEST_CONNECTION_HANDLE_VALID, NULL, NULL));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(session_set_incoming_window(TEST_SESSION_HANDLE_VALID, IGNORED_NUM_ARG))
        .IgnoreArgument(2);
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(amqpvalue_create_map());
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    EXPECTED_CALL(amqpvalue_create_symbol(IGNORED_PTR_ARG));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    EXPECTED_CALL(amqpvalue_create_symbol(IGNORED_PTR_ARG));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(STRING_c_str(TEST_FILTER_QUERY_STRING_HANDLE_VALID));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    STRICT_EXPECTED_CALL(amqpvalue_create_string(FILTER_BY_TIMESTAMP_VALUE));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    EXPECTED_CALL(amqpvalue_create_described(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    EXPECTED_CALL(amqpvalue_set_map_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(STRING_c_str(TEST_TARGETADDRESS_STRING_HANDLE_VALID));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    STRICT_EXPECTED_CALL(source_create());
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    EXPECTED_CALL(amqpvalue_create_string(IGNORED_PTR_ARG));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(source_set_address(TEST_SOURCE_HANDLE_VALID, IGNORED_PTR_ARG)).IgnoreArgument(2);
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(source_set_filter(TEST_SOURCE_HANDLE_VALID, IGNORED_PTR_ARG)).IgnoreArgument(2);
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(amqpvalue_create_source(TEST_SOURCE_HANDLE_VALID));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    EXPECTED_CALL(messaging_create_target(IGNORED_PTR_ARG)).IgnoreAllArguments(); // TEST_MESSAGING_TARGET_VALID
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(link_create(TEST_SESSION_HANDLE_VALID, IGNORED_PTR_ARG, 1, TEST_AMQP_SOURCE_HANDLE_VALID, TEST_MESSAGING_TARGET_VALID))
        .IgnoreArgument(2);
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(link_set_rcv_settle_mode(TEST_LINK_HANDLE_VALID, 0));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(link_set_max_message_size(TEST_LINK_HANDLE_VALID, (256 * 1024)));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(messagereceiver_create(TEST_LINK_HANDLE_VALID, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).IgnoreArgument(2).IgnoreArgument(3);
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(messagereceiver_open(TEST_MESSAGE_RECEIVER_HANDLE_VALID, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).IgnoreArgument(2).IgnoreArgument(3);
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(source_destroy(TEST_SOURCE_HANDLE_VALID)); //TEST_AMQP_SOURCE_HANDLE_VALID
    i++; // this function is not expected to fail and thus does not included in the bitmask

    EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    STRICT_EXPECTED_CALL(connection_dowork(TEST_CONNECTION_HANDLE_VALID));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    // ensure that we do not have more that 64 mocked functions
    ASSERT_IS_FALSE_WITH_MSG((i > 64), "More Mocked Functions than permitted bitmask width");

    return failedFunctionBitmask;
}

static uint64_t TestSetupCallStack_OnMessageReceived(void)
{
    uint64_t failedFunctionBitmask = 0;
    int i = 0;

    // arrange
    umock_c_reset_all_calls();

    //////////////////////////////////////////////////////////////////////////////////////////
    //   Event Data                                                                         //
    //////////////////////////////////////////////////////////////////////////////////////////
    STRICT_EXPECTED_CALL(message_get_body_amqp_data(TEST_MESSAGE_HANDLE_VALID, 0, IGNORED_PTR_ARG))
        .IgnoreArgument(3);
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    EXPECTED_CALL(EventData_CreateWithNewMemory(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    //////////////////////////////////////////////////////////////////////////////////////////
    //   Event Partition Key                                                                //
    //////////////////////////////////////////////////////////////////////////////////////////
    STRICT_EXPECTED_CALL(message_get_message_annotations(TEST_MESSAGE_HANDLE_VALID, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    if (!TestHelper_IsNullMessageAnnotations())
    {
        STRICT_EXPECTED_CALL(amqpvalue_get_map(TEST_AMQP_MESSAGE_ANNOTATIONS_VALID, IGNORED_PTR_ARG))
            .IgnoreArgument(2);
        failedFunctionBitmask |= ((uint64_t)1 << i++);

        STRICT_EXPECTED_CALL(amqpvalue_get_map_pair_count(TEST_AMQP_MESSAGE_ANNOTATIONS_MAP_HANDLE_VALID, IGNORED_PTR_ARG))
            .IgnoreArgument(2);
        failedFunctionBitmask |= ((uint64_t)1 << i++);

        STRICT_EXPECTED_CALL(amqpvalue_get_map_key_value_pair(TEST_AMQP_MESSAGE_ANNOTATIONS_MAP_HANDLE_VALID, 0, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3).IgnoreArgument(4);
        failedFunctionBitmask |= ((uint64_t)1 << i++);

        EXPECTED_CALL(amqpvalue_get_symbol(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        failedFunctionBitmask |= ((uint64_t)1 << i++);

        EXPECTED_CALL(amqpvalue_get_timestamp(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        failedFunctionBitmask |= ((uint64_t)1 << i++);

        STRICT_EXPECTED_CALL(EventData_SetEnqueuedTimestampUTCInMs(TEST_EVENTDATA_HANDLE_VALID, IGNORED_NUM_ARG))
            .IgnoreArgument(2);
        failedFunctionBitmask |= ((uint64_t)1 << i++);

        EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        i++; // this function is not expected to fail and thus does not included in the bitmask

        EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        i++; // this function is not expected to fail and thus does not included in the bitmask

        EXPECTED_CALL(annotations_destroy(IGNORED_PTR_ARG));
        i++; // this function is not expected to fail and thus does not included in the bitmask
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    //   Event Application Properties                                                       //
    //////////////////////////////////////////////////////////////////////////////////////////
    STRICT_EXPECTED_CALL(EventData_Properties(TEST_EVENTDATA_HANDLE_VALID));
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    STRICT_EXPECTED_CALL(message_get_application_properties(TEST_MESSAGE_HANDLE_VALID, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
    failedFunctionBitmask |= ((uint64_t)1 << i++);

    if (!TestHelper_IsNullMessageApplicationProperties())
    {
        STRICT_EXPECTED_CALL(amqpvalue_get_inplace_descriptor(TEST_MESSAGE_APP_PROPS_HANDLE_VALID));
        failedFunctionBitmask |= ((uint64_t)1 << i++);

        STRICT_EXPECTED_CALL(amqpvalue_get_inplace_described_value(TEST_MESSAGE_APP_PROPS_HANDLE_VALID));
        failedFunctionBitmask |= ((uint64_t)1 << i++);

        STRICT_EXPECTED_CALL(amqpvalue_get_map(TEST_AMQP_INPLACE_DESCRIBED_DESCRIPTOR, IGNORED_PTR_ARG))
            .IgnoreArgument(2);
        failedFunctionBitmask |= ((uint64_t)1 << i++);

        STRICT_EXPECTED_CALL(amqpvalue_get_map_pair_count(TEST_AMQP_MESSAGE_APP_PROPS_MAP_HANDLE_VALID, IGNORED_PTR_ARG))
            .IgnoreArgument(2);
        failedFunctionBitmask |= ((uint64_t)1 << i++);

        STRICT_EXPECTED_CALL(amqpvalue_get_map_key_value_pair(TEST_AMQP_MESSAGE_APP_PROPS_MAP_HANDLE_VALID, 0, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3).IgnoreArgument(4);
        failedFunctionBitmask |= ((uint64_t)1 << i++);

        EXPECTED_CALL(amqpvalue_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        failedFunctionBitmask |= ((uint64_t)1 << i++);

        EXPECTED_CALL(amqpvalue_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        failedFunctionBitmask |= ((uint64_t)1 << i++);

        STRICT_EXPECTED_CALL(Map_Add(TEST_EVENT_DATA_PROPS_MAP_HANDLE_VALID, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2).IgnoreArgument(3);
        failedFunctionBitmask |= ((uint64_t)1 << i++);

        EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        i++; // this function is not expected to fail and thus does not included in the bitmask

        EXPECTED_CALL(amqpvalue_destroy(IGNORED_PTR_ARG));
        i++; // this function is not expected to fail and thus does not included in the bitmask

        EXPECTED_CALL(application_properties_destroy(IGNORED_PTR_ARG));
        i++; // this function is not expected to fail and thus does not included in the bitmask
    }

    STRICT_EXPECTED_CALL(EventData_Destroy(TEST_EVENTDATA_HANDLE_VALID));
    i++; // this function is not expected to fail and thus does not included in the bitmask

    EXPECTED_CALL(messaging_delivery_accepted());
    i++; // this function is not expected to fail and thus does not included in the bitmask

    // ensure that we do not have more that 64 mocked functions
    ASSERT_IS_FALSE_WITH_MSG((i > 64), "More Mocked Functions than permitted bitmask width");

    return failedFunctionBitmask;
}

//#################################################################################################
// EventHubReceiver LL Tests
//#################################################################################################
BEGIN_TEST_SUITE(eventhubreceiver_ll_unittests)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(TestHook_OnUMockCError);

    REGISTER_UMOCK_ALIAS_TYPE(bool, unsigned char);
    REGISTER_UMOCK_ALIAS_TYPE(uint32_t, unsigned int);
    REGISTER_UMOCK_ALIAS_TYPE(uint64_t, unsigned long long);

    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MAP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TICK_COUNTER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MAP_RESULT, unsigned int);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_RECEIVER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LINK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SESSION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IO_INTERFACE_DESCRIPTION*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(CONNECTION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(XIO_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SASL_MECHANISM_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const IO_INTERFACE_DESCRIPTION*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(AMQP_VALUE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(filter_set, void*);
    REGISTER_UMOCK_ALIAS_TYPE(annotations, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SOURCE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(role, unsigned char);
    REGISTER_UMOCK_ALIAS_TYPE(receiver_settle_mode, unsigned char);
    REGISTER_UMOCK_ALIAS_TYPE(ON_NEW_ENDPOINT, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_LINK_ATTACHED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_CONNECTION_STATE_CHANGED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_MESSAGE_RECEIVER_STATE_CHANGED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_MESSAGE_RECEIVED, void*);
    
    REGISTER_UMOCK_ALIAS_TYPE(EVENTHUBRECEIVER_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(EVENTDATA_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(EVENTHUBRECEIVER_ASYNC_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(EVENTHUBRECEIVER_ASYNC_ERROR_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(EVENTHUBRECEIVER_ASYNC_END_CALLBACK, void*);

    REGISTER_UMOCK_ALIAS_TYPE(SASL_MECHANISM_INTERFACE_DESCRIPTION*, void*);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, TestHook_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, TestHook_free);

    REGISTER_GLOBAL_MOCK_RETURN(EventData_CreateWithNewMemory, TEST_EVENTDATA_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(EventData_CreateWithNewMemory, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(EventData_Clone, TEST_EVENTDATA_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(EventData_Clone, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(EventHubClient_GetVersionString, "Version Test");
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(EventHubClient_GetVersionString, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, TestHook_STRING_construct);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct_n, TestHook_STRING_construct_n);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct_n, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_clone, TestHook_STRING_clone);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_clone, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(STRING_concat, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat, -1);
    REGISTER_GLOBAL_MOCK_RETURN(STRING_concat_with_STRING, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat_with_STRING, -1);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_c_str, TestHook_STRING_c_str);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_c_str, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(connectionstringparser_parse, TEST_MAP_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(connectionstringparser_parse, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(Map_GetValueFromKey, TestHoook_Map_GetValueFromKey);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_GetValueFromKey, NULL);
	
    REGISTER_GLOBAL_MOCK_RETURN(Map_Add, (MAP_RESULT)0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_Add, (MAP_RESULT)1);

    REGISTER_GLOBAL_MOCK_RETURN(tickcounter_create, TEST_TICK_COUNTER_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(tickcounter_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(tickcounter_get_current_ms, TestHook_tickcounter_get_current_ms);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(tickcounter_get_current_ms, -1);

    REGISTER_GLOBAL_MOCK_RETURN(saslplain_get_interface, TEST_SASL_PLAIN_INTERFACE_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(saslplain_get_interface, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(saslmechanism_create, TEST_SASL_MECHANISM_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(saslmechanism_create, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(platform_get_default_tlsio, TEST_TLS_IO_INTERFACE_DESCRPTION_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(platform_get_default_tlsio, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(xio_create, TestHook_xio_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(xio_create, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(saslclientio_get_interface_description, TEST_SASL_CLIENT_IO_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(saslclientio_get_interface_description, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(connection_create, TEST_CONNECTION_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(connection_create, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(session_create, TEST_SESSION_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(session_create, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(session_set_incoming_window, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(session_set_incoming_window, -1);
    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_map, TEST_AMQP_VALUE_MAP_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_map, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_symbol, TEST_AMQP_VALUE_SYMBOL_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_symbol, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_string, TEST_AMQP_VALUE_FILTER_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_string, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_described, TEST_AMQP_VALUE_DESCRIBED_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_described, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_set_map_value, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_set_map_value, -1);
    REGISTER_GLOBAL_MOCK_RETURN(source_create, TEST_SOURCE_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(source_create, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(source_set_address, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(source_set_address, -1);
    REGISTER_GLOBAL_MOCK_RETURN(source_set_filter, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(source_set_filter, -1);
    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_create_source, TEST_AMQP_SOURCE_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_create_source, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(messaging_create_target, TEST_MESSAGING_TARGET_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messaging_create_target, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(link_create, TEST_LINK_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(link_create, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(link_set_rcv_settle_mode, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(link_set_rcv_settle_mode, -1);
    REGISTER_GLOBAL_MOCK_RETURN(link_set_max_message_size, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(link_set_max_message_size, -1);

    REGISTER_GLOBAL_MOCK_HOOK(messagereceiver_create, TestHook_messagereceiver_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagereceiver_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(messagereceiver_open, TestHook_messagereceiver_open);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(messagereceiver_open, -1);

	REGISTER_GLOBAL_MOCK_RETURN(message_get_body_amqp_data, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_get_body_amqp_data, -1);
    REGISTER_GLOBAL_MOCK_HOOK(message_get_message_annotations, TestHook_message_get_message_annotations);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_get_message_annotations, -1);
    REGISTER_GLOBAL_MOCK_HOOK(message_get_application_properties, TestHook_message_get_application_properties);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(message_get_application_properties, -1);

	REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_get_inplace_descriptor, TEST_AMQP_INPLACE_DESCRIPTOR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_inplace_descriptor, NULL);

	REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_get_inplace_described_value, TEST_AMQP_INPLACE_DESCRIBED_DESCRIPTOR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_inplace_described_value, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(amqpvalue_get_map, TestHook_amqpvalue_get_map);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_map, -1);
    REGISTER_GLOBAL_MOCK_HOOK(amqpvalue_get_map_pair_count, TestHook_amqpvalue_get_map_pair_count);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_map_pair_count, -1);
    REGISTER_GLOBAL_MOCK_HOOK(amqpvalue_get_map_key_value_pair, TestHook_amqpvalue_get_map_key_value_pair);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_map_key_value_pair, -1);
	REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_get_string, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_string, -1);
    REGISTER_GLOBAL_MOCK_RETURN(amqpvalue_get_timestamp, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_timestamp, -1);
    REGISTER_GLOBAL_MOCK_HOOK(amqpvalue_get_symbol, TestHook_amqpvalue_get_symbol);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(amqpvalue_get_symbol, -1);

    REGISTER_GLOBAL_MOCK_RETURN(EventData_CreateWithNewMemory, TEST_EVENTDATA_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(EventData_CreateWithNewMemory, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(EventData_Properties, TEST_EVENT_DATA_PROPS_MAP_HANDLE_VALID);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(EventData_Properties, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(EventData_SetEnqueuedTimestampUTCInMs, EVENTDATA_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(EventData_SetEnqueuedTimestampUTCInMs, EVENTDATA_INVALID_ARG);

    REGISTER_GLOBAL_MOCK_HOOK(connection_dowork, TestHook_connection_dowork);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();
    TEST_MUTEX_DESTROY(g_testByTest);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("Mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

//#################################################################################################
// EventHubReceiver_LL_Create Tests
//#################################################################################################
TEST_FUNCTION(EventHubReceiver_LL_Create_NULL_Param_ConnectionString)
{
    // arrange
    EVENTHUBRECEIVER_LL_HANDLE h;

    // act
    h = EventHubReceiver_LL_Create(NULL, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    // assert
    ASSERT_IS_NULL(h);
}

TEST_FUNCTION(EventHubReceiver_LL_Create_NULL_Param_EventHubPath)
{
    // arrange
    EVENTHUBRECEIVER_LL_HANDLE h;

    // act
    h = EventHubReceiver_LL_Create(CONNECTION_STRING, NULL, CONSUMER_GROUP, PARTITION_ID);

    // assert
    ASSERT_IS_NULL(h);
}

TEST_FUNCTION(EventHubReceiver_LL_Create_NULL_Param_ConsumerGroup)
{
    // arrange
    EVENTHUBRECEIVER_LL_HANDLE h;

    // act
    h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, NULL, PARTITION_ID);

    // assert
    ASSERT_IS_NULL(h);
}

TEST_FUNCTION(EventHubReceiver_LL_Create_NULL_Param_PartitionId)
{
    // arrange
    EVENTHUBRECEIVER_LL_HANDLE h;

    // act
    h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, NULL);

    // assert
    ASSERT_IS_NULL(h);
}

TEST_FUNCTION(EventHubReceiver_LL_Create_Success)
{
    // arrange
    (void)TestSetupCallStack_Create();

    // act
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test #1");
    ASSERT_IS_NOT_NULL_WITH_MSG(h, "Failed Return Value Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_LL_Create_Negative)
{
    // arrange
    int testResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, testResult);

    uint64_t failedCallBitmask = TestSetupCallStack_Create();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);
        if (failedCallBitmask & ((uint64_t)1 << i))
        {
            // act
            EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
            // assert
            ASSERT_IS_NULL(h);
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
}

//#################################################################################################
// EventHubReceiver_LL_SetConnectionTracing Tests
//#################################################################################################
TEST_FUNCTION(EventHubReceiver_LL_SetConnectionTracing_NULL_Params)
{
    // arrange
    EVENTHUBRECEIVER_RESULT result;

    // act
    result = EventHubReceiver_LL_SetConnectionTracing(NULL, false);

    // assert
    ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_INVALID_ARG, result);
}

TEST_FUNCTION(EventHubReceiver_LL_SetConnectionTracing_NoActiveConnection_Success)
{
    EVENTHUBRECEIVER_RESULT result;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    // arrange, 1
    umock_c_reset_all_calls();

    // act, 1
    result = EventHubReceiver_LL_SetConnectionTracing(h, false);
    
    // assert, 1
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test #1");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, result, "Failed Return Value Test #1");

    // arrange, 2
    umock_c_reset_all_calls();

    // act, 2
    result = EventHubReceiver_LL_SetConnectionTracing(h, true);
    
    // assert, 2
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test #2");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, result, "Failed Return Value Test #2");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_LL_SetConnectionTracing_ActiveConnection_Success)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    result = EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);

    EventHubReceiver_LL_DoWork(h);

    // arrange, 1
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(connection_set_trace(TEST_CONNECTION_HANDLE_VALID, 0));

    // act, 1
    result = EventHubReceiver_LL_SetConnectionTracing(h, false);

    // assert, 1
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test #1");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, result, "Failed Return Value Test #1");

    // arrange, 2
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(connection_set_trace(TEST_CONNECTION_HANDLE_VALID, 1));

    // act, 2
    result = EventHubReceiver_LL_SetConnectionTracing(h, true);

    // assert, 2
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test #2");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, result, "Failed Return Value Test #2");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
}

//#################################################################################################
// EventHubReceiver_LL_ReceiveFromStartTimestampAsync Tests
//#################################################################################################
TEST_FUNCTION(EventHubReceiver_LL_ReceiveFromStartTimestampAsync_NULL_Param_Handle)
{
    // arrange
    EVENTHUBRECEIVER_RESULT result;

    // act
    result = EventHubReceiver_LL_ReceiveFromStartTimestampAsync(NULL, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, 0);

    // assert
    ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_INVALID_ARG, result);
}

TEST_FUNCTION(EventHubReceiver_LL_ReceiveFromStartTimestampAsync_NULL_Param_onEventReceiveCallback)
{
    // arrange
    EVENTHUBRECEIVER_RESULT result;

    // act
    result = EventHubReceiver_LL_ReceiveFromStartTimestampAsync(TEST_EVENTHUB_RECEIVER_LL_VALID, NULL, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, 0);

    // assert
    ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_INVALID_ARG, result);
}

TEST_FUNCTION(EventHubReceiver_LL_ReceiveFromStartTimestampAsync_NULL_Param_onEventReceiveErrorCallback)
{
    // arrange
    EVENTHUBRECEIVER_RESULT result;

    // act
    result = EventHubReceiver_LL_ReceiveFromStartTimestampAsync(TEST_EVENTHUB_RECEIVER_LL_VALID, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, NULL, OnErrCBCtxt, 0);

    // assert
    ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_INVALID_ARG, result);
}

TEST_FUNCTION(EventHubReceiver_LL_ReceiveFromStartTimestampAsync_Success)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    // arrange
    TestSetupCallStack_ReceiveFromStartTimestampCommon(0);

    // act
    result = EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, result, "Failed Return Value Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, TestHelper_isSTRING_construct_sprintfInvoked(), "Failed STRING_consturct_sprintf Value Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_LL_ReceiveFromStartTimestampAsync_Mulitple_Receive_WithEnd_Success)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    (void)EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);

    (void)EventHubReceiver_LL_ReceiveEndAsync(h, EventHubHReceiver_LL_OnRxEndCB, OnRxEndCBCtxt);

    EventHubReceiver_LL_DoWork(h);

    // arrange
    TestSetupCallStack_ReceiveFromStartTimestampCommon(0);

    // act
    result = EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, result, "Failed Return Value Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, TestHelper_isSTRING_construct_sprintfInvoked(), "Failed STRING_consturct_sprintf Value Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_LL_ReceiveFromStartTimestampAsync_Mulitple_Receive_Failure)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    (void)EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);

    // arrange
    TestSetupCallStack_ReceiveFromStartTimestampCommon(0);

    // act
    result = EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_NOT_ALLOWED, result, "Failed Return Value Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, TestHelper_isSTRING_construct_sprintfInvoked(), "Failed STRING_consturct_sprintf Value Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_LL_ReceiveFromStartTimestampAsync_Negative)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    int testResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, testResult);

    // arrange, 1
    TestSetupCallStack_ReceiveFromStartTimestampCommon(0);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);
        // act, 1
        result = EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);
        // assert, 1
        ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_ERROR, result, "Failed Return Value Test #1");
    }

    // arrange, 2
    TestHelper_SetNegativeTestSTRING_construct_sprintf();
    
    // act, 2
    result = EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);

    // assert, 2
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_ERROR, result, "Failed Return Value Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, TestHelper_isSTRING_construct_sprintfInvoked(), "Failed STRING_consturct_sprintf Value Test");

    // cleanup
    umock_c_negative_tests_deinit();
    EventHubReceiver_LL_Destroy(h);
    TestHelper_ResetTestGlobalData();
}

//#################################################################################################
// EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync Tests
//#################################################################################################
TEST_FUNCTION(EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync_NULL_Param_Handle)
{
    // arrange
    EVENTHUBRECEIVER_RESULT result;

    // act
    result = EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync(NULL, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, 0, TEST_EVENTHUB_RECEIVER_TIMEOUT_MS);

    // assert
    ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_INVALID_ARG, result);
}

TEST_FUNCTION(EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync_NULL_Param_onEventReceiveCallback)
{
    // arrange
    EVENTHUBRECEIVER_RESULT result;

    // act
    result = EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync(TEST_EVENTHUB_RECEIVER_LL_VALID, NULL, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, 0, TEST_EVENTHUB_RECEIVER_TIMEOUT_MS);

    // assert
    ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_INVALID_ARG, result);
}

TEST_FUNCTION(EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync_NULL_Param_onEventReceiveErrorCallback)
{
    // arrange
    EVENTHUBRECEIVER_RESULT result;

    // act
    result = EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync(TEST_EVENTHUB_RECEIVER_LL_VALID, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, NULL, OnErrCBCtxt, 0, TEST_EVENTHUB_RECEIVER_TIMEOUT_MS);

    // assert
    ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_INVALID_ARG, result);
}

TEST_FUNCTION(EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync_Success)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    // arrange
    TestSetupCallStack_ReceiveFromStartTimestampCommon(TEST_EVENTHUB_RECEIVER_TIMEOUT_MS);

    // act
    result = EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT_MS);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, result, "Failed Return Value Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, TestHelper_isSTRING_construct_sprintfInvoked(), "Failed STRING_consturct_sprintf Value Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync_Mulitple_Receive_WithEnd_Success)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    (void)EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT_MS);

    (void)EventHubReceiver_LL_ReceiveEndAsync(h, EventHubHReceiver_LL_OnRxEndCB, OnRxEndCBCtxt);

    EventHubReceiver_LL_DoWork(h);

    // arrange
    TestSetupCallStack_ReceiveFromStartTimestampCommon(TEST_EVENTHUB_RECEIVER_TIMEOUT_MS);

    // act
    result = EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT_MS);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, result, "Failed Return Value Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, TestHelper_isSTRING_construct_sprintfInvoked(), "Failed STRING_consturct_sprintf Value Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync_Mulitple_Receive_Failure)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    (void)EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT_MS);

    // arrange
    TestSetupCallStack_ReceiveFromStartTimestampCommon(TEST_EVENTHUB_RECEIVER_TIMEOUT_MS);

    // act
    result = EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT_MS);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_NOT_ALLOWED, result, "Failed Return Value Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, TestHelper_isSTRING_construct_sprintfInvoked(), "Failed STRING_consturct_sprintf Value Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync_Negative)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    int testResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, testResult);

    // arrange, 1
    TestSetupCallStack_ReceiveFromStartTimestampCommon(TEST_EVENTHUB_RECEIVER_TIMEOUT_MS);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);
        // act, 1
        result = EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT_MS);
        // assert, 1
        ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_ERROR, result, "Failed Return Value Test #1");
    }

    // arrange, 2
    TestHelper_SetNegativeTestSTRING_construct_sprintf();

    // act, 2
    result = EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS, TEST_EVENTHUB_RECEIVER_TIMEOUT_MS);

    // assert, 2
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_ERROR, result, "Failed Return Value Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, TestHelper_isSTRING_construct_sprintfInvoked(), "Failed STRING_consturct_sprintf Value Test");

    // cleanup
    umock_c_negative_tests_deinit();
    EventHubReceiver_LL_Destroy(h);
    TestHelper_ResetTestGlobalData();
}

//#################################################################################################
// EventHubReceiver_LL_ReceiveEndAsync Tests
//#################################################################################################
TEST_FUNCTION(EventHubReceiver_LL_ReceiveEndAsync_NULL_Param_Handle)
{
    // arrange
    EVENTHUBRECEIVER_RESULT result;

    // act
    result = EventHubReceiver_LL_ReceiveEndAsync(NULL, NULL, NULL);

    // assert
    ASSERT_ARE_EQUAL(int, EVENTHUBRECEIVER_INVALID_ARG, result);
}

TEST_FUNCTION(EventHubReceiver_LL_ReceiveEndAsync_WithActiveReceiver_Success)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    (void)EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);

    // arrange
    TestSetupCallStack_ReceiveEndAsync();

    // act
    result = EventHubReceiver_LL_ReceiveEndAsync(h, EventHubHReceiver_LL_OnRxEndCB, OnRxEndCBCtxt);
    
    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, result, "Failed Return Value Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_LL_ReceiveEndAsync_WithInActiveReceiver_Failure)
{
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    // arrange
    TestSetupCallStack_ReceiveEndAsync();

    // act
    result = EventHubReceiver_LL_ReceiveEndAsync(h, EventHubHReceiver_LL_OnRxEndCB, OnRxEndCBCtxt);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_NOT_ALLOWED, result, "Failed Return Value Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_LL_ReceiveEndAsync_WithActiveReceiverAndAsyncEnd_Failure)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    EVENTHUBRECEIVER_RESULT result;

    (void)EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);

    (void)EventHubReceiver_LL_ReceiveEndAsync(h, EventHubHReceiver_LL_OnRxEndCB, OnRxEndCBCtxt);

    EventHubReceiver_LL_DoWork(h);

    // arrange
    TestSetupCallStack_ReceiveEndAsync();

    // act
    result = EventHubReceiver_LL_ReceiveEndAsync(h, EventHubHReceiver_LL_OnRxEndCB, OnRxEndCBCtxt);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_NOT_ALLOWED, result, "Failed Return Value Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
}

//#################################################################################################
// EventHubReceiver_LL_DoWork Tests
//#################################################################################################
TEST_FUNCTION(EventHubReceiver_LL_DoWork_NULL_Param)
{
    // arrange

    // act
    EventHubReceiver_LL_DoWork(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(EventHubReceiver_LL_DoWork_NoActiveReceiver_Success)
{
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    // arrange
    TestHelper_ResetTestGlobalData();
    umock_c_reset_all_calls();

    // act
    EventHubReceiver_LL_DoWork(h);
    
    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_LL_DoWork_ActiveReceiverStackBringup_Success)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    (void)EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);

    // arrange
    (void)TestSetupCallStack_DoWorkActiveStackBringup();
        
    EventHubReceiver_LL_DoWork(h);

    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_LL_DoWork_ActiveReceiver_MultipleDowWork_Success)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    (void)EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);

    EventHubReceiver_LL_DoWork(h);

    // arrange
    TestHelper_ResetTestGlobalData();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(connection_dowork(TEST_CONNECTION_HANDLE_VALID));

    EventHubReceiver_LL_DoWork(h);

    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_LL_DoWork_ActiveReceiverStackBringup_Negative)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);
    
    // setup a receiver
    (void)EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);

    int testResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, testResult);

    // arrange
    uint64_t failedCallBitmask = TestSetupCallStack_DoWorkActiveStackBringup();
    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        TestHelper_ResetTestGlobalData();
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);
        if (failedCallBitmask & ((uint64_t)1 << i))
        {
            // act
            EventHubReceiver_LL_DoWork(h);
            // assert
            ASSERT_ARE_EQUAL(int, 0, TestHelper_isConnectionDoWorkInvoked());
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
    EventHubReceiver_LL_Destroy(h);
    TestHelper_ResetTestGlobalData();
}

TEST_FUNCTION(EventHubReceiver_LL_DoWork_ActiveReceiverStackTeardown_Success)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    // setup a receiver
    (void)EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);

    // perform stack bring up
    EventHubReceiver_LL_DoWork(h);

    // request stack tear down
    (void)EventHubReceiver_LL_ReceiveEndAsync(h, EventHubHReceiver_LL_OnRxEndCB, OnRxEndCBCtxt);

    // arrange
    (void)TestSetupCallStack_DoWorkInActiveTearDown();

    // act (perform actual tear down)
    EventHubReceiver_LL_DoWork(h);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, OnRxCBStruct.rxEndCallbackResult, "Failed Receive Error Callback Result Value Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_LL_DoWork_ActiveReceiverStackTeardown_Negative)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    // setup a receiver
    (void)EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);

    // perform stack bring up
    EventHubReceiver_LL_DoWork(h);

    // request stack tear down
    (void)EventHubReceiver_LL_ReceiveEndAsync(h, EventHubHReceiver_LL_OnRxEndCB, OnRxEndCBCtxt);

    // arrange
    int testResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, testResult);

    uint64_t failedCallBitmask = TestSetupCallStack_DoWorkInActiveTearDown();
    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        TestHelper_ResetTestGlobalData();
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);
        if (failedCallBitmask & ((uint64_t)1 << i))
        {
            // act
            EventHubReceiver_LL_DoWork(h);
            // assert
            ASSERT_ARE_EQUAL(int, 0, TestHelper_isConnectionDoWorkInvoked());
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
    EventHubReceiver_LL_Destroy(h);
}

//#################################################################################################
// EventHubReceiver_LL_DoWork OnStateChanged Callback Tests
//#################################################################################################
TEST_FUNCTION(EventHubReceiver_LL_DoWork_OnStateChanged_Callback_Success)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    TestHelper_ResetTestGlobalData();

    (void)EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);
    EventHubReceiver_LL_DoWork(h);
    
    // arrange
    umock_c_reset_all_calls();

    // assert
    ASSERT_IS_NOT_NULL(OnRxCBStruct.onMsgChangedCallback);
    ASSERT_IS_NOT_NULL(OnRxCBStruct.onMsgReceivedCallback);

    // MSG_RECEIVER idle to MSG_RECEIVER open transition no error callback expected
    OnRxCBStruct.onMsgChangedCallback(OnRxCBStruct.onMsgChangedCallbackCtxt, MESSAGE_RECEIVER_STATE_OPEN, MESSAGE_RECEIVER_STATE_IDLE);
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test #1");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test #1");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test #1");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test #1");

    // MSG_RECEIVER open to MSG_RECEIVER error transition error callback expected
    OnRxCBStruct.onMsgChangedCallback(OnRxCBStruct.onMsgChangedCallbackCtxt, MESSAGE_RECEIVER_STATE_ERROR, MESSAGE_RECEIVER_STATE_OPEN);
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test #2");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test #2");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test #2");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test #2");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_CONNECTION_RUNTIME_ERROR, OnRxCBStruct.rxErrorCallbackResult, "Failed Receive Error Callback Result Value #2");

    // reset error callback data
    OnRxCBStruct.rxErrorCallbackCalled = 0;
    OnRxCBStruct.rxErrorCallbackResult = EVENTHUBRECEIVER_OK;

    // MSG_RECEIVER error to MSG_RECEIVER error transition no error callback expected
    OnRxCBStruct.onMsgChangedCallback(OnRxCBStruct.onMsgChangedCallbackCtxt, MESSAGE_RECEIVER_STATE_ERROR, MESSAGE_RECEIVER_STATE_ERROR);
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test #3");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test #3");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test #3");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test #3");

    // MSG_RECEIVER error to MSG_RECEIVER open transition no error callback expected
    OnRxCBStruct.onMsgChangedCallback(OnRxCBStruct.onMsgChangedCallbackCtxt, MESSAGE_RECEIVER_STATE_OPEN, MESSAGE_RECEIVER_STATE_ERROR);
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test #4");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test #4");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test #4");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test #4");

    // MSG_RECEIVER open to MSG_RECEIVER error transition error callback expected
    OnRxCBStruct.onMsgChangedCallback(OnRxCBStruct.onMsgChangedCallbackCtxt, MESSAGE_RECEIVER_STATE_ERROR, MESSAGE_RECEIVER_STATE_OPEN);
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test #5");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test #5");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test #5");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test #5");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_CONNECTION_RUNTIME_ERROR, OnRxCBStruct.rxErrorCallbackResult, "Failed Receive Error Callback Result Value #5");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
    TestHelper_ResetTestGlobalData();
}

//#################################################################################################
// EventHubReceiver_LL_DoWork OnMessageReceived Callback Tests
//#################################################################################################
TEST_FUNCTION(EventHubReceiver_LL_DoWork_OnMessageReceived_Callback_Success)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    TestHelper_ResetTestGlobalData();

    (void)EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);
    EventHubReceiver_LL_DoWork(h);

    // arrange
    (void)TestSetupCallStack_OnMessageReceived();

    // act
    OnRxCBStruct.onMsgReceivedCallback(OnRxCBStruct.onMsgReceivedCallbackCtxt, TEST_MESSAGE_HANDLE_VALID);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, OnRxCBStruct.rxCallbackResult, "Failed Receive Callback Result Value Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
    TestHelper_ResetTestGlobalData();
}

TEST_FUNCTION(EventHubReceiver_LL_DoWork_OnMessageReceived_Callback_NullApplicationProperties_Success)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    TestHelper_ResetTestGlobalData();
    (void)EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);
    EventHubReceiver_LL_DoWork(h);

    // arrange
    TestHelper_SetNullMessageApplicationProperties();
    (void)TestSetupCallStack_OnMessageReceived();

    // act
    OnRxCBStruct.onMsgReceivedCallback(OnRxCBStruct.onMsgReceivedCallbackCtxt, TEST_MESSAGE_HANDLE_VALID);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, OnRxCBStruct.rxCallbackResult, "Failed Receive Callback Result Value Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
    TestHelper_ResetTestGlobalData();
}

TEST_FUNCTION(EventHubReceiver_LL_DoWork_OnMessageReceived_Callback_NullMessageAnnotation_Success)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    TestHelper_ResetTestGlobalData();
    (void)EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);
    EventHubReceiver_LL_DoWork(h);

    // arrange
    TestHelper_SetNullMessageAnnotations();
    (void)TestSetupCallStack_OnMessageReceived();

    // act
    OnRxCBStruct.onMsgReceivedCallback(OnRxCBStruct.onMsgReceivedCallbackCtxt, TEST_MESSAGE_HANDLE_VALID);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, OnRxCBStruct.rxCallbackResult, "Failed Receive Callback Result Value Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
    TestHelper_ResetTestGlobalData();
}

TEST_FUNCTION(EventHubReceiver_LL_DoWork_OnMessageReceived_Callback_NullMessageAnnotationAndAppProps_Success)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    TestHelper_ResetTestGlobalData();
    (void)EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);
    EventHubReceiver_LL_DoWork(h);

    // arrange
    TestHelper_SetNullMessageAnnotations();
    TestHelper_SetNullMessageApplicationProperties();
    (void)TestSetupCallStack_OnMessageReceived();

    // act
    OnRxCBStruct.onMsgReceivedCallback(OnRxCBStruct.onMsgReceivedCallbackCtxt, TEST_MESSAGE_HANDLE_VALID);

    // assert
    ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Failed CallStack Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_OK, OnRxCBStruct.rxCallbackResult, "Failed Receive Callback Result Value Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
    TestHelper_ResetTestGlobalData();
}

TEST_FUNCTION(EventHubReceiver_LL_DoWork_OnMessageReceived_Callback_Negative)
{
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    TestHelper_ResetTestGlobalData();

    (void)EventHubReceiver_LL_ReceiveFromStartTimestampAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS);
    EventHubReceiver_LL_DoWork(h);

    // arrange
    int testResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, testResult);

    uint64_t failedCallBitmask = TestSetupCallStack_OnMessageReceived();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);
        if (failedCallBitmask & ((uint64_t)1 << i))
        {
            // act
            OnRxCBStruct.onMsgReceivedCallback(OnRxCBStruct.onMsgReceivedCallbackCtxt, TEST_MESSAGE_HANDLE_VALID);

            // assert
            ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
            ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
            ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");
        }
    }

    // cleanup
    umock_c_negative_tests_deinit();
    EventHubReceiver_LL_Destroy(h);
}

//#################################################################################################
// EventHubReceiver_LL_DoWork Timeout Callback Tests
//#################################################################################################
TEST_FUNCTION(EventHubReceiver_LL_DoWork_Timeout_Callback_Success)
{
    unsigned int timeoutMs = TEST_EVENTHUB_RECEIVER_TIMEOUT_MS, done = 0;
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;    
    time_t nowTimestamp, prevTimestamp;
    double timespan;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    TestHelper_ResetTestGlobalData();

    (void)EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS, timeoutMs);
    EventHubReceiver_LL_DoWork(h);

    // arrange
    prevTimestamp = time(NULL);
    do
    {
        nowTimestamp = time(NULL);
        timespan = difftime(nowTimestamp, prevTimestamp);
        if (timespan * 1000 >= (double)timeoutMs)
        {
            done = 1;
        }
        // act
        EventHubReceiver_LL_DoWork(h);
    } while (!done);

    // assert
    // ensure callback was invoked
    ASSERT_ARE_EQUAL_WITH_MSG(int, 1, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, EVENTHUBRECEIVER_TIMEOUT, OnRxCBStruct.rxCallbackResult, "Failed Receive Callback Result Value Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_LL_DoWork_Timeout_Callback_ZeroTimeoutValue_Success)
{
    unsigned int timeoutMs = 0, done = 0;
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    time_t nowTimestamp, prevTimestamp;
    double timespan;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    TestHelper_ResetTestGlobalData();

    (void)EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS, timeoutMs);
    EventHubReceiver_LL_DoWork(h);

    // arrange
    prevTimestamp = time(NULL);
    do
    {
        nowTimestamp = time(NULL);
        timespan = difftime(nowTimestamp, prevTimestamp);
        if (timespan * 1000 >= (double)timeoutMs)
        {
            done = 1;
        }
        // act
        EventHubReceiver_LL_DoWork(h);
    } while (!done);

    // assert
    // ensure no callback was invoked
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");

    // cleanup
    EventHubReceiver_LL_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_LL_DoWork_Timeout_Callback_WithMessageReceive_Success)
{
    unsigned int timeoutMs = TEST_EVENTHUB_RECEIVER_TIMEOUT_MS, done = 0;
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    time_t nowTimestamp, prevTimestamp;
    double timespan;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    TestHelper_ResetTestGlobalData();

    // No timeout CB registered thus if a timeout is ever called we will assert
    (void)EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync(h, EventHubHReceiver_LL_OnRxNoTimeoutCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS, timeoutMs);

    // required here to set up onMsgReceivedCallback
    EventHubReceiver_LL_DoWork(h);

    // arrange
    prevTimestamp = time(NULL);
    do
    {
        nowTimestamp = time(NULL);
        timespan = difftime(nowTimestamp, prevTimestamp);
        if (timespan * 1000 >= (double)timeoutMs)
        {
            done = 1;
        }
        // act
        OnRxCBStruct.onMsgReceivedCallback(OnRxCBStruct.onMsgReceivedCallbackCtxt, TEST_MESSAGE_HANDLE_VALID);
        EventHubReceiver_LL_DoWork(h);
    } while (!done);

    // cleanup
    EventHubReceiver_LL_Destroy(h);
}

TEST_FUNCTION(EventHubReceiver_LL_DoWork_Timeout_Callback_Negative)
{
    unsigned int timeoutMs = TEST_EVENTHUB_RECEIVER_TIMEOUT_MS;
    uint64_t nowTS = TEST_EVENTHUB_RECEIVER_UTC_TIMESTAMP;
    EVENTHUBRECEIVER_LL_HANDLE h = EventHubReceiver_LL_Create(CONNECTION_STRING, EVENTHUB_PATH, CONSUMER_GROUP, PARTITION_ID);

    (void)EventHubReceiver_LL_ReceiveFromStartTimestampWithTimeoutAsync(h, EventHubHReceiver_LL_OnRxCB, OnRxCBCtxt, EventHubHReceiver_LL_OnErrCB, OnErrCBCtxt, nowTS, timeoutMs);
    EventHubReceiver_LL_DoWork(h);

    // arrange
    TestHelper_ResetTestGlobalData();
    int testResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, testResult);
    umock_c_negative_tests_snapshot();

    STRICT_EXPECTED_CALL(tickcounter_get_current_ms(TEST_TICK_COUNTER_HANDLE_VALID, IGNORED_PTR_ARG)).IgnoreArgument(2);

    umock_c_negative_tests_snapshot();
    // act
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);
        // act
        EventHubReceiver_LL_DoWork(h);
        // assert
        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxCallbackCalled, "Failed Receive Callback Invoked Test");
        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxErrorCallbackCalled, "Failed Receive Error Callback Invoked Test");
        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, OnRxCBStruct.rxEndCallbackCalled, "Failed Receive Async End Callback Invoked Test");
    }

    // cleanup
    umock_c_negative_tests_deinit();
    EventHubReceiver_LL_Destroy(h);
}

END_TEST_SUITE(eventhubreceiver_ll_unittests)