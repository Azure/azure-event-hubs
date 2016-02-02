/*
Microsoft Azure IoT Device Libraries
Copyright (c) Microsoft Corporation
All rights reserved. 
MIT License
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
documentation files (the Software), to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED 
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
IN THE SOFTWARE.
*/

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <signal.h>
#include <string.h>
#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"

#include "eventhubclient_ll.h"
#include "strings.h"
#include "connection_string_parser.h"
#include "xio.h"
#include "connection.h"
#include "session.h"
#include "link.h"
#include "messaging.h"
#include "message.h"
#include "message_sender.h"
#include "sasl_mechanism.h"
#include "sasl_plain.h"
#include "saslclientio.h"
#include "version.h"
#include "lock.h"
#include "amqp_definitions.h"
#include "amqpvalue.h"
#include "platform.h"
#include "doublylinkedlist.h"
#include "tlsio.h"
#include "map.h"

DEFINE_MICROMOCK_ENUM_TO_STRING(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_RESULT_VALUES);

DEFINE_MICROMOCK_ENUM_TO_STRING(EVENTHUBCLIENT_CONFIRMATION_RESULT, EVENTHUBCLIENT_CONFIRMATION_RESULT_VALUES);

#define GBALLOC_H

extern "C" int gballoc_init(void);
extern "C" void gballoc_deinit(void);
extern "C" void* gballoc_malloc(size_t size);
extern "C" void* gballoc_calloc(size_t nmemb, size_t size);
extern "C" void* gballoc_realloc(void* ptr, size_t size);
extern "C" void gballoc_free(void* ptr);

namespace BASEIMPLEMENTATION
{
    /*if malloc is defined as gballoc_malloc at this moment, there'd be serious trouble*/
#define Lock(x) (LOCK_OK + gballocState - gballocState) /*compiler warning about constant in if condition*/
#define Unlock(x) (LOCK_OK + gballocState - gballocState)
#define Lock_Init() (LOCK_HANDLE)0x42
#define Lock_Deinit(x) (LOCK_OK + gballocState - gballocState)
#include "gballoc.c"
#undef Lock
#undef Unlock
#undef Lock_Init
#undef Lock_Deinit

#include "doublylinkedlist.c"
};

static MICROMOCK_MUTEX_HANDLE g_testByTest;
EVENTHUBCLIENT_CONFIRMATION_RESULT g_confirmationResult;

#define TEST_EVENTDATA_HANDLE       (EVENTDATA_HANDLE)0x45
#define TEST_CONNSTR_HANDLE         (STRING_HANDLE)0x46
#define DUMMY_STRING_HANDLE         (STRING_HANDLE)0x47
#define TEST_HOSTNAME_STRING_HANDLE (STRING_HANDLE)0x48
#define TEST_KEYNAME_STRING_HANDLE  (STRING_HANDLE)0x49
#define TEST_KEY_STRING_HANDLE      (STRING_HANDLE)0x4A
#define TEST_TARGET_STRING_HANDLE   (STRING_HANDLE)0x4B
#define TEST_CONNSTR_MAP_HANDLE     (MAP_HANDLE)0x50

#define TEST_STRING_TOKENIZER_HANDLE (STRING_TOKENIZER_HANDLE)0x48
#define TEST_MAP_HANDLE (MAP_HANDLE)0x49
#define MICROSOFT_MESSAGE_FORMAT 0x80013700
#define EVENT_HANDLE_COUNT  3
#define OVER_MAX_PACKET_SIZE    256*1024

#define CONNECTION_STRING "Endpoint=sb://servicebusName.servicebus.windows.net/;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8="
#define TEST_EVENTHUB_PATH "eventHubName"
#define TEST_ENDPOINT "sb://servicebusName.servicebus.windows.net"
#define TEST_HOSTNAME "servicebusName.servicebus.windows.net"
#define TEST_KEYNAME "RootManageSharedAccessKey"
#define TEST_KEY "icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8="

static const char* TEXT_MESSAGE = "Hello From EventHubClient Unit Tests";
static const char* TEST_CHAR = "TestChar";
static const char* PARTITION_KEY_VALUE = "PartitionKeyValue";
static const char* PARTITION_KEY_EMPTY_VALUE = "";
static const char* PROPERTY_NAME = "PropertyName";
static const char TEST_STRING_VALUE[] = "Property_String_Value_1";
static const char TEST_STRING_VALUE2[] = "Property_String_Value_2";

static const char* TEST_PROPERTY_KEY[] = {"Key1", "Key2"};
static const char* TEST_PROPERTY_VALUE[] = {"Value1", "Value2"};

static const int BUFFER_SIZE = 8;
static bool g_setProperty = false;
static bool g_includeProperties = false;

static size_t currentmalloc_call;
static size_t whenShallmalloc_fail;

static size_t g_current_pn_messenger_call;
static size_t g_when_pn_messenger_fail;

static size_t g_currentEventClone_call;
static size_t g_whenShallEventClone_fail;

static size_t g_currentEventDataGetData_call;
static size_t g_whenEventDataGetData_fail;

typedef struct CALLBACK_CONFIRM_INFO
{
    sig_atomic_t successCounter;
    sig_atomic_t totalCalls;
} CALLBACK_CONFIRM_INFO;

static const XIO_HANDLE DUMMY_IO_HANDLE = (XIO_HANDLE)0x4342;
static const XIO_HANDLE TEST_TLSIO_HANDLE = (XIO_HANDLE)0x4343;
static const XIO_HANDLE TEST_SASLCLIENTIO_HANDLE = (XIO_HANDLE)0x4344;
static const CONNECTION_HANDLE TEST_CONNECTION_HANDLE = (CONNECTION_HANDLE)0x4243;
static const SESSION_HANDLE TEST_SESSION_HANDLE = (SESSION_HANDLE)0x4244;
static const LINK_HANDLE TEST_LINK_HANDLE = (LINK_HANDLE)0x4245;
static const AMQP_VALUE TEST_SOURCE_AMQP_VALUE = (AMQP_VALUE)0x4246;
static const AMQP_VALUE TEST_TARGET_AMQP_VALUE = (AMQP_VALUE)0x4247;
static const MESSAGE_HANDLE TEST_MESSAGE_HANDLE = (MESSAGE_HANDLE)0x4248;
static const MESSAGE_SENDER_HANDLE TEST_MESSAGE_SENDER_HANDLE = (MESSAGE_SENDER_HANDLE)0x4249;
static const SASL_MECHANISM_HANDLE TEST_SASL_MECHANISM_HANDLE = (SASL_MECHANISM_HANDLE)0x4250;
static const IO_INTERFACE_DESCRIPTION* TEST_SASLCLIENTIO_INTERFACE_DESCRIPTION = (const IO_INTERFACE_DESCRIPTION*)0x4251;
static const SASL_MECHANISM_INTERFACE_DESCRIPTION* TEST_SASLPLAIN_INTERFACE_DESCRIPTION = (const SASL_MECHANISM_INTERFACE_DESCRIPTION*)0x4252;
static const IO_INTERFACE_DESCRIPTION* TEST_TLSIO_INTERFACE_DESCRIPTION = (const IO_INTERFACE_DESCRIPTION*)0x4253;
static const AMQP_VALUE TEST_MAP_AMQP_VALUE = (AMQP_VALUE)0x4254;
static const AMQP_VALUE TEST_STRING_AMQP_VALUE = (AMQP_VALUE)0x4255;

static SASL_PLAIN_CONFIG* saved_sasl_mechanism_create_parameters;
static TLSIO_CONFIG* saved_tlsio_parameters;
static SASLCLIENTIO_CONFIG* saved_saslclientio_parameters;

std::ostream& operator<<(std::ostream& left, const BINARY_DATA& binary_data)
{
    std::ios::fmtflags f(left.flags());
    left << std::hex;
    for (size_t i = 0; i < binary_data.length; i++)
    {
        left << binary_data.bytes[i];
    }
    left.flags(f);
    return left;
}

static bool operator==(const BINARY_DATA left, const BINARY_DATA& right)
{
    if (left.length != right.length)
    {
        return false;
    }
    else
    {
        return memcmp(left.bytes, right.bytes, left.length) == 0;
    }
}

TYPED_MOCK_CLASS(CEventHubClientLLMocks, CGlobalMock)
{
public:
    /* xio mocks */
    MOCK_STATIC_METHOD_3(, XIO_HANDLE, xio_create, const IO_INTERFACE_DESCRIPTION*, io_interface_description, const void*, io_create_parameters, LOGGER_LOG, logger_log)
        if (saved_tlsio_parameters == NULL)
        {
            saved_tlsio_parameters = (TLSIO_CONFIG*)malloc(sizeof(TLSIO_CONFIG));
            saved_tlsio_parameters->port = ((TLSIO_CONFIG*)io_create_parameters)->port;
            saved_tlsio_parameters->hostname = (char*)malloc(strlen(((TLSIO_CONFIG*)io_create_parameters)->hostname));
            (void)strcpy((char*)saved_tlsio_parameters->hostname, ((TLSIO_CONFIG*)io_create_parameters)->hostname);
        }
        else
        {
            saved_saslclientio_parameters = (SASLCLIENTIO_CONFIG*)malloc(sizeof(SASLCLIENTIO_CONFIG));
            saved_saslclientio_parameters->sasl_mechanism = ((SASLCLIENTIO_CONFIG*)io_create_parameters)->sasl_mechanism;
            saved_saslclientio_parameters->underlying_io = ((SASLCLIENTIO_CONFIG*)io_create_parameters)->underlying_io;
        }
    MOCK_METHOD_END(XIO_HANDLE, DUMMY_IO_HANDLE);
    MOCK_STATIC_METHOD_1(, void, xio_destroy, XIO_HANDLE, xio)
    MOCK_VOID_METHOD_END();

    /* connection mocks */
    MOCK_STATIC_METHOD_5(, CONNECTION_HANDLE, connection_create, XIO_HANDLE, xio, const char*, hostname, const char*, container_id, ON_NEW_ENDPOINT, on_new_endpoint, void*, callback_context)
    MOCK_METHOD_END(CONNECTION_HANDLE, TEST_CONNECTION_HANDLE);
    MOCK_STATIC_METHOD_1(, void, connection_destroy, CONNECTION_HANDLE, connection)
    MOCK_VOID_METHOD_END();
    MOCK_STATIC_METHOD_1(, void, connection_dowork, CONNECTION_HANDLE, connection)
    MOCK_VOID_METHOD_END();

    /* session mocks */
    MOCK_STATIC_METHOD_3(, SESSION_HANDLE, session_create, CONNECTION_HANDLE, connection, ON_LINK_ATTACHED, on_link_attached, void*, callback_context)
    MOCK_METHOD_END(SESSION_HANDLE, TEST_SESSION_HANDLE);
    MOCK_STATIC_METHOD_1(, void, session_destroy, SESSION_HANDLE, session)
    MOCK_VOID_METHOD_END();
    MOCK_STATIC_METHOD_2(, int, session_set_outgoing_window, SESSION_HANDLE, session, uint32_t, outgoing_window)
    MOCK_METHOD_END(int, 0);

    /* link mocks */
    MOCK_STATIC_METHOD_5(, LINK_HANDLE, link_create, SESSION_HANDLE, session, const char*, name, role, _role, AMQP_VALUE, source, AMQP_VALUE, target)
    MOCK_METHOD_END(LINK_HANDLE, TEST_LINK_HANDLE);
    MOCK_STATIC_METHOD_1(, void, link_destroy, LINK_HANDLE, handle)
    MOCK_VOID_METHOD_END();
    MOCK_STATIC_METHOD_2(, int, link_set_snd_settle_mode, LINK_HANDLE, link, sender_settle_mode, snd_settle_mode)
    MOCK_METHOD_END(int, 0);
    MOCK_STATIC_METHOD_2(, int, link_set_max_message_size, LINK_HANDLE, link, uint64_t, max_message_size)
    MOCK_METHOD_END(int, 0);

    /* messaging mocks */
    MOCK_STATIC_METHOD_1(, AMQP_VALUE, messaging_create_source, const char*, address)
    MOCK_METHOD_END(AMQP_VALUE, TEST_SOURCE_AMQP_VALUE);
    MOCK_STATIC_METHOD_1(, AMQP_VALUE, messaging_create_target, const char*, address)
    MOCK_METHOD_END(AMQP_VALUE, TEST_TARGET_AMQP_VALUE);

    /* message mocks */
    MOCK_STATIC_METHOD_0(, MESSAGE_HANDLE, message_create);
    MOCK_METHOD_END(MESSAGE_HANDLE, TEST_MESSAGE_HANDLE);
    MOCK_STATIC_METHOD_2(, int, message_set_message_annotations, MESSAGE_HANDLE, message, annotations, delivery_annotations);
    MOCK_METHOD_END(int, 0);
    MOCK_STATIC_METHOD_2(, int, message_add_body_amqp_data, MESSAGE_HANDLE, message, BINARY_DATA, binary_data);
    MOCK_METHOD_END(int, 0);

    /* messagesender mocks */
    MOCK_STATIC_METHOD_4(, MESSAGE_SENDER_HANDLE, messagesender_create, LINK_HANDLE, link, ON_MESSAGE_SENDER_STATE_CHANGED, on_message_sender_state_changed, void*, context, LOGGER_LOG, logger_log);
    MOCK_METHOD_END(MESSAGE_SENDER_HANDLE, TEST_MESSAGE_SENDER_HANDLE);
    MOCK_STATIC_METHOD_1(, void, messagesender_destroy, MESSAGE_SENDER_HANDLE, message_sender);
    MOCK_VOID_METHOD_END();
    MOCK_STATIC_METHOD_1(, int, messagesender_open, MESSAGE_SENDER_HANDLE, message_sender);
    MOCK_METHOD_END(int, 0);
    MOCK_STATIC_METHOD_1(, int, messagesender_close, MESSAGE_SENDER_HANDLE, message_sender);
    MOCK_METHOD_END(int, 0);
    MOCK_STATIC_METHOD_4(, int, messagesender_send, MESSAGE_SENDER_HANDLE, message_sender, MESSAGE_HANDLE, message, ON_MESSAGE_SEND_COMPLETE, on_message_send_complete, const void*, callback_context);
    MOCK_METHOD_END(int, 0);

    /* saslmechanism mocks */
    MOCK_STATIC_METHOD_2(, SASL_MECHANISM_HANDLE, saslmechanism_create, const SASL_MECHANISM_INTERFACE_DESCRIPTION*, sasl_mechanism_interface_description, void*, sasl_mechanism_create_parameters);
        saved_sasl_mechanism_create_parameters = (SASL_PLAIN_CONFIG*)malloc(sizeof(SASL_PLAIN_CONFIG));
        if (((SASL_PLAIN_CONFIG*)sasl_mechanism_create_parameters)->authcid == NULL)
        {
            saved_sasl_mechanism_create_parameters->authcid = NULL;
        }
        else
        {
            saved_sasl_mechanism_create_parameters->authcid = (char*)malloc(strlen(((SASL_PLAIN_CONFIG*)sasl_mechanism_create_parameters)->authcid));
            (void)strcpy((char*)saved_sasl_mechanism_create_parameters->authcid, ((SASL_PLAIN_CONFIG*)sasl_mechanism_create_parameters)->authcid);
        }
        if (((SASL_PLAIN_CONFIG*)sasl_mechanism_create_parameters)->authzid == NULL)
        {
            saved_sasl_mechanism_create_parameters->authzid = NULL;
        }
        else
        {
            saved_sasl_mechanism_create_parameters->authzid = (char*)malloc(strlen(((SASL_PLAIN_CONFIG*)sasl_mechanism_create_parameters)->authzid));
            (void)strcpy((char*)saved_sasl_mechanism_create_parameters->authzid, ((SASL_PLAIN_CONFIG*)sasl_mechanism_create_parameters)->authzid);
        }
        if (((SASL_PLAIN_CONFIG*)sasl_mechanism_create_parameters)->passwd == NULL)
        {
            saved_sasl_mechanism_create_parameters->passwd = NULL;
        }
        else
        {
            saved_sasl_mechanism_create_parameters->passwd = (char*)malloc(strlen(((SASL_PLAIN_CONFIG*)sasl_mechanism_create_parameters)->passwd));
            (void)strcpy((char*)saved_sasl_mechanism_create_parameters->passwd, ((SASL_PLAIN_CONFIG*)sasl_mechanism_create_parameters)->passwd);
        }
    MOCK_METHOD_END(SASL_MECHANISM_HANDLE, TEST_SASL_MECHANISM_HANDLE);
    MOCK_STATIC_METHOD_1(, void, saslmechanism_destroy, SASL_MECHANISM_HANDLE, sasl_mechanism);
    MOCK_VOID_METHOD_END();

    /* saslclientio mocks */
    MOCK_STATIC_METHOD_0(, const IO_INTERFACE_DESCRIPTION*, saslclientio_get_interface_description);
    MOCK_METHOD_END(const IO_INTERFACE_DESCRIPTION*, TEST_SASLCLIENTIO_INTERFACE_DESCRIPTION);

    /* saslplain mocks */
    MOCK_STATIC_METHOD_0(, const SASL_MECHANISM_INTERFACE_DESCRIPTION*, saslplain_get_interface);
    MOCK_METHOD_END(const SASL_MECHANISM_INTERFACE_DESCRIPTION*, TEST_SASLPLAIN_INTERFACE_DESCRIPTION);

    /* platform mocks */
    MOCK_STATIC_METHOD_0(, const IO_INTERFACE_DESCRIPTION*, platform_get_default_tlsio);
    MOCK_METHOD_END(const IO_INTERFACE_DESCRIPTION*, TEST_TLSIO_INTERFACE_DESCRIPTION);

    /* amqpvalue mocks */
    MOCK_STATIC_METHOD_0(, AMQP_VALUE, amqpvalue_create_map);
    MOCK_METHOD_END(AMQP_VALUE, TEST_MAP_AMQP_VALUE);
    MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_create_string, const char*, value);
    MOCK_METHOD_END(AMQP_VALUE, TEST_STRING_AMQP_VALUE);
    MOCK_STATIC_METHOD_3(, int, amqpvalue_set_map_value, AMQP_VALUE, map, AMQP_VALUE, key, AMQP_VALUE, value);
    MOCK_METHOD_END(int, 0);

    /* map mocks */
    MOCK_STATIC_METHOD_2(, const char*, Map_GetValueFromKey, MAP_HANDLE, handle, const char*, key)
    MOCK_METHOD_END(const char*, NULL);

    /* connectionstringparser mocks */
    MOCK_STATIC_METHOD_1(, MAP_HANDLE, connectionstringparser_parse, STRING_HANDLE, connection_string)
    MOCK_METHOD_END(MAP_HANDLE, TEST_CONNSTR_MAP_HANDLE);

    /* gballoc mocks */
    MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
        void* result2;
        currentmalloc_call++;
        if (whenShallmalloc_fail>0)
        {
            if (currentmalloc_call == whenShallmalloc_fail)
            {
                result2 = (void*)NULL;
            }
            else
            {
                result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
            }
        }
        else
        {
            result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
        }
    MOCK_METHOD_END(void*, result2);

    MOCK_STATIC_METHOD_2(, void*, gballoc_realloc, void*, ptr, size_t, size)
        MOCK_METHOD_END(void*, BASEIMPLEMENTATION::gballoc_realloc(ptr, size));

    MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
        BASEIMPLEMENTATION::gballoc_free(ptr);
    MOCK_VOID_METHOD_END()

    /* DoublyLinkedList mocks */
    MOCK_STATIC_METHOD_1(, void, DList_InitializeListHead, PDLIST_ENTRY, listHead)
        BASEIMPLEMENTATION::DList_InitializeListHead(listHead);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, int, DList_IsListEmpty, PDLIST_ENTRY, listHead)
        int result2 = BASEIMPLEMENTATION::DList_IsListEmpty(listHead);
    MOCK_METHOD_END(int, result2)

    MOCK_STATIC_METHOD_2(, void, DList_InsertTailList, PDLIST_ENTRY, listHead, PDLIST_ENTRY, listEntry)
        BASEIMPLEMENTATION::DList_InsertTailList(listHead, listEntry);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_2(, void, DList_AppendTailList, PDLIST_ENTRY, listHead, PDLIST_ENTRY, ListToAppend)
        BASEIMPLEMENTATION::DList_AppendTailList(listHead, ListToAppend);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, int, DList_RemoveEntryList, PDLIST_ENTRY, listEntry)
        int result2 = BASEIMPLEMENTATION::DList_RemoveEntryList(listEntry);
    MOCK_METHOD_END(int, result2)

    MOCK_STATIC_METHOD_1(, PDLIST_ENTRY, DList_RemoveHeadList, PDLIST_ENTRY, listHead)
        PDLIST_ENTRY entry = BASEIMPLEMENTATION::DList_RemoveHeadList(listHead);
    MOCK_METHOD_END(PDLIST_ENTRY, entry)

    /* EventData Mocks */
    MOCK_STATIC_METHOD_2(, EVENTDATA_HANDLE, EventData_CreateWithNewMemory, const unsigned char*, data, size_t, length)
    MOCK_METHOD_END(EVENTDATA_HANDLE, (EVENTDATA_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1))

    MOCK_STATIC_METHOD_3(, EVENTDATA_RESULT, EventData_GetData, EVENTDATA_HANDLE, eventDataHandle, const unsigned char**, data, size_t*, dataLength)
    {
        g_currentEventDataGetData_call++;
        if (g_whenEventDataGetData_fail > 0 && g_whenEventDataGetData_fail == g_currentEventDataGetData_call)
        {
            *data = (unsigned char*)TEXT_MESSAGE;
            *dataLength = OVER_MAX_PACKET_SIZE;
        }
        else
        {
            *data = (unsigned char*)TEXT_MESSAGE;
            *dataLength = strlen((const char*)TEXT_MESSAGE);
        }
    }
    MOCK_METHOD_END(EVENTDATA_RESULT, EVENTDATA_OK)

    MOCK_STATIC_METHOD_1(, void, EventData_Destroy, EVENTDATA_HANDLE, eventDataHandle)
        free(eventDataHandle);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, const char*, EventData_GetPartitionKey, EVENTDATA_HANDLE, eventDataHandle)
    MOCK_METHOD_END(const char*, NULL)

    MOCK_STATIC_METHOD_1(, MAP_HANDLE, EventData_Properties, EVENTDATA_HANDLE, eventDataHandle)
    MOCK_METHOD_END(MAP_HANDLE, TEST_MAP_HANDLE)

    MOCK_STATIC_METHOD_1(, EVENTDATA_HANDLE, EventData_Clone, EVENTDATA_HANDLE, eventDataHandle)
        EVENTDATA_HANDLE evHandle;
        g_currentEventClone_call++;
        if (g_whenShallEventClone_fail > 0 && g_whenShallEventClone_fail == g_currentEventClone_call)
        {
            evHandle = NULL;
        }
        else
        {
            evHandle = (EVENTDATA_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
        }
    MOCK_METHOD_END(EVENTDATA_HANDLE, evHandle)

    MOCK_STATIC_METHOD_4(, MAP_RESULT, Map_GetInternals, MAP_HANDLE, handle, const char*const**, keys, const char*const**, values, size_t*, count)
        if (g_includeProperties)
        {
            *keys = TEST_PROPERTY_KEY;
            *values = TEST_PROPERTY_VALUE;
            *count = 2;
        }
    MOCK_METHOD_END(MAP_RESULT, MAP_OK)

    /* Version Mocks */
    MOCK_STATIC_METHOD_0(, const char*, EventHubClient_GetVersionString)
    MOCK_METHOD_END(const char*, nullptr);

    /* STRING Mocks */
    MOCK_STATIC_METHOD_1(, STRING_HANDLE, STRING_clone, STRING_HANDLE, handle);
    MOCK_METHOD_END(STRING_HANDLE, DUMMY_STRING_HANDLE)

    MOCK_STATIC_METHOD_1(, STRING_HANDLE, STRING_construct, const char*, psz)
    MOCK_METHOD_END(STRING_HANDLE, DUMMY_STRING_HANDLE)

    MOCK_STATIC_METHOD_2(, int, STRING_concat, STRING_HANDLE, handle, const char*, s2)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_2(, int, STRING_concat_with_STRING, STRING_HANDLE, s1, STRING_HANDLE, s2)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_1(, void, STRING_delete, STRING_HANDLE, handle)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, const char*, STRING_c_str, STRING_HANDLE, s)
    MOCK_METHOD_END(const char*, TEST_CHAR)

    MOCK_STATIC_METHOD_0(, STRING_HANDLE, STRING_new)
    MOCK_METHOD_END(STRING_HANDLE, DUMMY_STRING_HANDLE);

    MOCK_STATIC_METHOD_2(, void, sendAsyncConfirmationCallback, EVENTHUBCLIENT_CONFIRMATION_RESULT, result2, void*, userContextCallback)
        EVENTHUBCLIENT_CONFIRMATION_RESULT* returnConfirm = (EVENTHUBCLIENT_CONFIRMATION_RESULT*)userContextCallback;
        *returnConfirm = result2;
    MOCK_VOID_METHOD_END()
};

DECLARE_GLOBAL_MOCK_METHOD_3(CEventHubClientLLMocks, , XIO_HANDLE, xio_create, const IO_INTERFACE_DESCRIPTION*, io_interface_description, const void*, io_create_parameters, LOGGER_LOG, logger_log);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, xio_destroy, XIO_HANDLE, xio);

DECLARE_GLOBAL_MOCK_METHOD_5(CEventHubClientLLMocks, , CONNECTION_HANDLE, connection_create, XIO_HANDLE, xio, const char*, hostname, const char*, container_id, ON_NEW_ENDPOINT, on_new_endpoint, void*, callback_context);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, connection_destroy, CONNECTION_HANDLE, connection);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, connection_dowork, CONNECTION_HANDLE, connection);

DECLARE_GLOBAL_MOCK_METHOD_3(CEventHubClientLLMocks, , SESSION_HANDLE, session_create, CONNECTION_HANDLE, connection, ON_LINK_ATTACHED, on_link_attached, void*, callback_context);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, session_destroy, SESSION_HANDLE, session);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, session_set_outgoing_window, SESSION_HANDLE, session, uint32_t, outgoing_window);

DECLARE_GLOBAL_MOCK_METHOD_5(CEventHubClientLLMocks, , LINK_HANDLE, link_create, SESSION_HANDLE, session, const char*, name, role, role, AMQP_VALUE, source, AMQP_VALUE, target);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, link_destroy, LINK_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, link_set_snd_settle_mode, LINK_HANDLE, link, sender_settle_mode, snd_settle_mode);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, link_set_max_message_size, LINK_HANDLE, link, uint64_t, max_message_size);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , AMQP_VALUE, messaging_create_source, const char*, address);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , AMQP_VALUE, messaging_create_target, const char*, address);

DECLARE_GLOBAL_MOCK_METHOD_0(CEventHubClientLLMocks, , MESSAGE_HANDLE, message_create);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, message_set_message_annotations, MESSAGE_HANDLE, message, annotations, delivery_annotations);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, message_add_body_amqp_data, MESSAGE_HANDLE, message, BINARY_DATA, binary_data);

DECLARE_GLOBAL_MOCK_METHOD_4(CEventHubClientLLMocks, , MESSAGE_SENDER_HANDLE, messagesender_create, LINK_HANDLE, link, ON_MESSAGE_SENDER_STATE_CHANGED, on_message_sender_state_changed, void*, context, LOGGER_LOG, logger_log);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, messagesender_destroy, MESSAGE_SENDER_HANDLE, message_sender);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , int, messagesender_open, MESSAGE_SENDER_HANDLE, message_sender);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , int, messagesender_close, MESSAGE_SENDER_HANDLE, message_sender);
DECLARE_GLOBAL_MOCK_METHOD_4(CEventHubClientLLMocks, , int, messagesender_send, MESSAGE_SENDER_HANDLE, message_sender, MESSAGE_HANDLE, message, ON_MESSAGE_SEND_COMPLETE, on_message_send_complete, const void*, callback_context);

DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , SASL_MECHANISM_HANDLE, saslmechanism_create, const SASL_MECHANISM_INTERFACE_DESCRIPTION*, sasl_mechanism_interface_description, void*, sasl_mechanism_create_parameters);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, saslmechanism_destroy, SASL_MECHANISM_HANDLE, sasl_mechanism);

DECLARE_GLOBAL_MOCK_METHOD_0(CEventHubClientLLMocks, , const IO_INTERFACE_DESCRIPTION*, saslclientio_get_interface_description);

DECLARE_GLOBAL_MOCK_METHOD_0(CEventHubClientLLMocks, , const SASL_MECHANISM_INTERFACE_DESCRIPTION*, saslplain_get_interface);

DECLARE_GLOBAL_MOCK_METHOD_0(CEventHubClientLLMocks, , const IO_INTERFACE_DESCRIPTION*, platform_get_default_tlsio);

DECLARE_GLOBAL_MOCK_METHOD_0(CEventHubClientLLMocks, , AMQP_VALUE, amqpvalue_create_map);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , AMQP_VALUE, amqpvalue_create_string, const char*, value);
DECLARE_GLOBAL_MOCK_METHOD_3(CEventHubClientLLMocks, , int, amqpvalue_set_map_value, AMQP_VALUE, map, AMQP_VALUE, key, AMQP_VALUE, value);

DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , const char*, Map_GetValueFromKey, MAP_HANDLE, handle, const char*, key);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , MAP_HANDLE, connectionstringparser_parse, STRING_HANDLE, connection_string);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, DList_InitializeListHead, PDLIST_ENTRY, listHead);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , int, DList_IsListEmpty, PDLIST_ENTRY, listHead);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , void, DList_InsertTailList, PDLIST_ENTRY, listHead, PDLIST_ENTRY, listEntry);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , void, DList_AppendTailList, PDLIST_ENTRY, listHead, PDLIST_ENTRY, ListToAppend);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , int, DList_RemoveEntryList, PDLIST_ENTRY, listEntry);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , PDLIST_ENTRY, DList_RemoveHeadList, PDLIST_ENTRY, listHead);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , void*, gballoc_realloc, void*, ptr, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, gballoc_free, void*, ptr)

DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , EVENTDATA_HANDLE, EventData_CreateWithNewMemory, const unsigned char*, data, size_t, length);
DECLARE_GLOBAL_MOCK_METHOD_3(CEventHubClientLLMocks, , EVENTDATA_RESULT, EventData_GetData, EVENTDATA_HANDLE, eventDataHandle, const unsigned char**, data, size_t*, dataLength);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, EventData_Destroy, EVENTDATA_HANDLE, eventDataHandle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , const char*, EventData_GetPartitionKey, EVENTDATA_HANDLE, eventDataHandle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , MAP_HANDLE, EventData_Properties, EVENTDATA_HANDLE, eventDataHandle);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , EVENTDATA_HANDLE, EventData_Clone, EVENTDATA_HANDLE, eventDataHandle);

DECLARE_GLOBAL_MOCK_METHOD_0(CEventHubClientLLMocks, , const char*, EventHubClient_GetVersionString);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , STRING_HANDLE, STRING_construct, const char*, psz);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , STRING_HANDLE, STRING_clone, STRING_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, STRING_concat, STRING_HANDLE, handle, const char*, s2);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, STRING_concat_with_STRING, STRING_HANDLE, s1, STRING_HANDLE, s2);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, STRING_delete, STRING_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , const char*, STRING_c_str, STRING_HANDLE, s);
DECLARE_GLOBAL_MOCK_METHOD_0(CEventHubClientLLMocks, , STRING_HANDLE, STRING_new);

DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , void, sendAsyncConfirmationCallback, EVENTHUBCLIENT_CONFIRMATION_RESULT, result2, void*, userContextCallback);

DECLARE_GLOBAL_MOCK_METHOD_4(CEventHubClientLLMocks, , MAP_RESULT, Map_GetInternals, MAP_HANDLE, handle, const char*const**, keys, const char*const**, values, size_t*, count);

// ** End of Mocks **
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

BEGIN_TEST_SUITE(eventhubclient_ll_unittests)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = MicroMockCreateMutex();
        ASSERT_IS_NOT_NULL(g_testByTest);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        MicroMockDestroyMutex(g_testByTest);

        DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
    {
        if (!MicroMockAcquireMutex(g_testByTest))
        {
            ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
        }

        saved_tlsio_parameters = NULL;
        saved_saslclientio_parameters = NULL;
        saved_sasl_mechanism_create_parameters = NULL;

        currentmalloc_call = 0;
        whenShallmalloc_fail = 0;
        g_setProperty = false;

        g_currentEventClone_call = 0;
        g_whenShallEventClone_fail = 0;
        g_confirmationResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;
        g_whenEventDataGetData_fail = 0;
        g_currentEventDataGetData_call = 0;

        g_includeProperties = false;
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        if (saved_tlsio_parameters != NULL)
        {
            free((char*)saved_tlsio_parameters->hostname);
            free(saved_tlsio_parameters);
        }

        if (saved_saslclientio_parameters != NULL)
        {
            free(saved_saslclientio_parameters);
        }

        if (saved_sasl_mechanism_create_parameters != NULL)
        {
            free((char*)saved_sasl_mechanism_create_parameters->authcid);
            free((char*)saved_sasl_mechanism_create_parameters->authzid);
            free((char*)saved_sasl_mechanism_create_parameters->passwd);
            free(saved_sasl_mechanism_create_parameters);
        }

        saved_tlsio_parameters = NULL;
        saved_saslclientio_parameters = NULL;
        saved_sasl_mechanism_create_parameters = NULL;

        if (!MicroMockReleaseMutex(g_testByTest))
        {
            ASSERT_FAIL("failure in test framework at ReleaseMutex");
        }
    }

    /*** EventHubClient_LL_CreateFromConnectionString ***/

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_002: [EventHubClient_LL_CreateFromConnectionString shall allocate a new event hub client LL instance.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_05_001: [EventHubClient_LL_CreateFromConnectionString shall obtain the version string by a call to EventHubClient_GetVersionString.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_05_002: [EventHubClient_LL_CreateFromConnectionString shall print the version string to standard output.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_03_017: [EventHubClient_LL expects a service bus connection string in one of the following formats:
    Endpoint=sb://[namespace].servicebus.windows.net/;SharedAccessKeyName=[keyname];SharedAccessKey=[keyvalue] 
    Endpoint=sb://[namespace].servicebus.windows.net;SharedAccessKeyName=[keyname];SharedAccessKey=[keyvalue]
    ] ]*/
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_065: [The connection string shall be parsed to a map of strings by using connection_string_parser_parse.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_067: [The endpoint shall be looked up in the resulting map and used to construct the host name to be used for connecting by removing the sb://.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_068: [The key name and key shall be looked up in the resulting map and they should be stored as is for later use in connecting.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_03_016: [EventHubClient_LL_CreateFromConnectionString shall return a non-NULL handle value upon success.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_03_030: [EventHubClient_LL_CreateFromConnectionString shall create a TLS IO by calling xio_create.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_002: [The TLS IO interface description passed to xio_create shall be obtained by calling platform_get_default_tlsio_interface.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_004: [A SASL plain mechanism shall be created by calling saslmechanism_create.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_005: [The interface passed to saslmechanism_create shall be obtained by calling saslplain_get_interface.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_007: [The creation parameters for the SASL plain mechanism shall be in the form of a SASL_PLAIN_CONFIG structure.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_008: [The authcid shall be set to the key name parsed earlier from the connection string.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_009: [The passwd members shall be set to the key value parsed earlier from the connection string.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_010: [The authzid shall be NULL.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_012: [A SASL client IO shall be created by calling xio_create.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_013: [The IO interface description for the SASL client IO shall be obtained by calling saslclientio_get_interface_description.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_015: [The IO creation parameters passed to xio_create shall be in the form of a SASLCLIENTIO_CONFIG.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_016: [The underlying_io members shall be set to the previously created TLS IO.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_017: [The sasl_mechanism shall be set to the previously created SASL PLAIN mechanism.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_019: [An AMQP connection shall be created by calling connection_create and passing as arguments the SASL client IO handle, eventhub hostname, "eh_client_connection" as container name and NULL for the new session handler and context.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_028: [An AMQP session shall be created by calling session_create and passing as arguments the connection handle, and NULL for the new link handler and context.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_030: [The outgoing window for the session shall be set to 10 by calling session_set_outgoing_window.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_021: [A source AMQP value shall be created by calling messaging_create_source.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_022: [The source address shall be "ingress".] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_023: [A target AMQP value shall be created by calling messaging_create_target.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_024: [The target address shall be "amqps://" {eventhub hostname} / {eventhub name}.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_026: [An AMQP link shall be created by calling link_create and passing as arguments the session handle, "sender-link" as link name, role_sender and the previously created source and target values.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_032: [The link sender settle mode shall be set to unsettled by calling link_set_snd_settle_mode.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_034: [The message size shall be set to 256K by calling link_set_max_message_size.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_036: [A message sender shall be created by calling messagesender_create and passing as arguments the link handle, a state changed callback, a context and NULL for the logging function.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_valid_args_yields_a_value_eventhub_client)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());

        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(mocks, connectionstringparser_parse(TEST_CONNSTR_HANDLE));
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "Endpoint"))
            .SetReturn(TEST_ENDPOINT);
        STRICT_EXPECTED_CALL(mocks, STRING_construct(TEST_HOSTNAME))
            .SetReturn(TEST_HOSTNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKeyName"))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_construct(TEST_KEYNAME))
            .SetReturn(TEST_KEYNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKey"))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, STRING_construct(TEST_KEY))
            .SetReturn(TEST_KEY_STRING_HANDLE);
        EXPECTED_CALL(mocks, DList_InitializeListHead(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEY_STRING_HANDLE))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, saslplain_get_interface());
        STRICT_EXPECTED_CALL(mocks, saslmechanism_create(TEST_SASLPLAIN_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_TLSIO_INTERFACE_DESCRIPTION, NULL, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_TLSIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, saslclientio_get_interface_description());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_SASLCLIENTIO_INTERFACE_DESCRIPTION, NULL, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_SASLCLIENTIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connection_create(TEST_SASLCLIENTIO_HANDLE, TEST_HOSTNAME, "eh_client_connection", NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, session_create(TEST_CONNECTION_HANDLE, NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, session_set_outgoing_window(TEST_SESSION_HANDLE, 10));
        STRICT_EXPECTED_CALL(mocks, messaging_create_source("ingress"));
        STRICT_EXPECTED_CALL(mocks, STRING_construct("amqps://"))
            .SetReturn(TEST_TARGET_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, STRING_concat(TEST_TARGET_STRING_HANDLE, TEST_HOSTNAME));
        STRICT_EXPECTED_CALL(mocks, STRING_concat(TEST_TARGET_STRING_HANDLE, "/"));
        STRICT_EXPECTED_CALL(mocks, STRING_concat(TEST_TARGET_STRING_HANDLE, TEST_EVENTHUB_PATH));
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH);
        STRICT_EXPECTED_CALL(mocks, messaging_create_target("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH));
        STRICT_EXPECTED_CALL(mocks, link_create(TEST_SESSION_HANDLE, "sender-link", role_sender, TEST_SOURCE_AMQP_VALUE, TEST_TARGET_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, link_set_snd_settle_mode(TEST_LINK_HANDLE, sender_settle_mode_unsettled));
        STRICT_EXPECTED_CALL(mocks, link_set_max_message_size(TEST_LINK_HANDLE, 256 * 1024));
        STRICT_EXPECTED_CALL(mocks, messagesender_create(TEST_LINK_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL))
            .IgnoreArgument(2).IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_CONNSTR_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_TARGET_STRING_HANDLE));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);

        // assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, TEST_KEYNAME, saved_sasl_mechanism_create_parameters->authcid);
        ASSERT_ARE_EQUAL(char_ptr, TEST_KEY, saved_sasl_mechanism_create_parameters->passwd);
        ASSERT_ARE_EQUAL(char_ptr, TEST_HOSTNAME, saved_tlsio_parameters->hostname);
        ASSERT_ARE_EQUAL(int, 5671, saved_tlsio_parameters->port);
        ASSERT_ARE_EQUAL(void_ptr, TEST_SASL_MECHANISM_HANDLE, saved_saslclientio_parameters->sasl_mechanism);
        ASSERT_ARE_EQUAL(void_ptr, TEST_TLSIO_HANDLE, saved_saslclientio_parameters->underlying_io);
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(result);
    }

#if 0
    /* Tests_SRS_EVENTHUBCLIENT_03_003: [EventHubClient_ CreateFromConnectionString shall return a NULL value if connectionString or path is NULL.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_NULL_connectionString_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(NULL, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_003: [EventHubClient_ CreateFromConnectionString shall return a NULL value if connectionString or path is NULL.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_NULL_path_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString("connectionString", NULL);

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_018: [EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_Invalid_Formatted_connectionString_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const char* connectionString = "MyBadConnectionString";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_017: [EventHubClient expects a service bus connection string in one of the following formats:
    Endpoint=sb://[namespace].servicebus.windows.net/;SharedAccessKeyName=[key name];SharedAccessKey=[key value]
    Endpoint=sb://[namespace].servicebus.windows.net;SharedAccessKeyName=[key name];SharedAccessKey=[key value] ]*/
    /* Tests_SRS_EVENTHUBCLIENT_03_016: [EventHubClient_LL_CreateFromConnectionString shall return a none-NULL handle value upon success.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_04_016: [EventHubClient_LL_CreateFromConnectionString shall initialize the outgoingEvents list that will be used to send Events.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_valid_parameters_1_Succeeds)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        prepareCreateEventHubWithConnectionStringToPass(mocks);

        EXPECTED_CALL(mocks, DList_InitializeListHead(NULL));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NOT_NULL(result);

        // cleanup
        EventHubClient_LL_Destroy(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_valid_parameters_2_Succeeds)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        prepareCreateEventHubWithConnectionStringToPass(mocks);

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NOT_NULL(result);

        // cleanup
        EventHubClient_LL_Destroy(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_017: [EventHubClient expects a service bus connection string in one of the following formats:
    Endpoint=sb://[namespace].servicebus.windows.net/;SharedAccessKeyName=[key name];SharedAccessKey=[key value]
    Endpoint=sb://[namespace].servicebus.windows.net;SharedAccessKeyName=[key name];SharedAccessKey=[key value] ]*/
    /* Tests_SRS_EVENTHUBCLIENT_03_018: [EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_missing_start_of_string_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const char* connectionString = "servicebusName.servicebus.windows.net/;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_017: [EventHubClient expects a service bus connection string in one of the following formats:
    Endpoint=sb://[namespace].servicebus.windows.net/;SharedAccessKeyName=[key name];SharedAccessKey=[key value]
    Endpoint=sb://[namespace].servicebus.windows.net;SharedAccessKeyName=[key name];SharedAccessKey=[key value] ]*/
    /* Tests_SRS_EVENTHUBCLIENT_03_018: [EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_partially_missing_start_of_string_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const char* connectionString = "Edpoint=sb://servicebusName.servicebus.windows.net;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_017: [EventHubClient expects a service bus connection string in one of the following formats:
    Endpoint=sb://[namespace].servicebus.windows.net/;SharedAccessKeyName=[key name];SharedAccessKey=[key value]
    Endpoint=sb://[namespace].servicebus.windows.net;SharedAccessKeyName=[key name];SharedAccessKey=[key value] ]*/
    /* Tests_SRS_EVENTHUBCLIENT_03_018: [EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_partially_missing2_start_of_string_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const char* connectionString = "ENdpoint=sb://servicebusName.servicebus.windows.net;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_missing_servicebusName_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const char* connectionString = "Endpoint=sb://.servicebus.windows.net/;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_missing_servicebusDomain_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const char* connectionString = "Endpoint=sb://servicebusName;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_missing_part_of_servicebusDomain_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.net;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_missing_SharedAccessKeyName_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.windows.net/;=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_missing_SharedAccessKeyNameValue_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.windows.net/;SharedAccessKeyName=;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_missing_SharedAccessKey_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.windows.net/;SharedAccessKeyName=RootManageSharedAccessKey;=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_missing_SharedAccessKeyValue_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.windows.net/;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_missing_delimiters_0_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.windows.netSharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_missing_delimiters_1_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.windows.net/;SharedAccessKeyNameRootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_noneAlphaNumericInHost_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servi(cebus.windows.net/;SharedAccessKeyNameRootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_in_different_order_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const char* connectionString = "Endpoint=sb://SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8;servicebusName.servicebus.windows.net/;";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_differentDelimiter_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.windows.net(;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_doubleSlash_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.windows.net//;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_020: [EventHubClient_LL_CreateFromConnectionString shall ensure that the keyname, keyvalue, namespace and eventhubpath are all URL encoded.] */
    /* Tests_SRS_EVENTHUBCLIENT_04_009: [EventHubClient_LL_CreateFromConnectionString shall specify a timeout value of 10 seconds to the proton messenger via a call to pn_messenger_set_timeout.] */
    /* Tests_SRS_EVENTHUBCLIENT_04_010: [EventHubClient_ CreateFromConnectionString shall setup the global messenger to be blocking via a call to pn_messenger_set_blocking.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_invokes_URL_Encode_Succeeds)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;

        const STRING_HANDLE eventHubPathHandle = (STRING_HANDLE)0x04;

        prepareCreateEventHubWithConnectionStringToPass(mocks);
        

        EXPECTED_CALL(mocks, EventHubClient_GetVersionString());

        STRICT_EXPECTED_CALL(mocks, STRING_construct(EVENTHUB_PATH))
            .SetReturn(eventHubPathHandle);

        STRICT_EXPECTED_CALL(mocks, URL_Encode(TEST_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, URL_Encode(TEST_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, URL_Encode(TEST_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, URL_Encode(eventHubPathHandle));

        // act
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NOT_NULL(eventHubHandle);
        mocks.AssertActualAndExpectedCalls();

        // Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_021: [EventHubClient_ CreateFromConnectionString shall return EVENTHUBCLIENT_URL_ENCODING_FAILURE if any failure is encountered during the URL encoding.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_URL_Encode_1_Failing_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;

        STRICT_EXPECTED_CALL(mocks, URL_Encode(TEST_STRING_HANDLE))
            .SetReturn((STRING_HANDLE)NULL);

        // act
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(eventHubHandle);

        // Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_021: [EventHubClient_ CreateFromConnectionString shall return EVENTHUBCLIENT_URL_ENCODING_FAILURE if any failure is encountered during the URL encoding.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_URL_Encode_2_Failing_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;

        STRICT_EXPECTED_CALL(mocks, URL_Encode(TEST_STRING_HANDLE))
            .SetReturn((STRING_HANDLE)NULL);

        // act
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(eventHubHandle);

        // Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_021: [EventHubClient_ CreateFromConnectionString shall return EVENTHUBCLIENT_URL_ENCODING_FAILURE if any failure is encountered during the URL encoding.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_URL_Encode_3_Failing_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;

        prepareCreateEventHubWithConnectionStringToPass(mocks);

        STRICT_EXPECTED_CALL(mocks, URL_Encode(TEST_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, URL_Encode(TEST_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, URL_Encode(TEST_STRING_HANDLE))
            .SetReturn((STRING_HANDLE)NULL);

        // act
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(eventHubHandle);

        // Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_021: [EventHubClient_ CreateFromConnectionString shall return EVENTHUBCLIENT_URL_ENCODING_FAILURE if any failure is encountered during the URL encoding.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_URL_Encode_4_Failing_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;

        const STRING_HANDLE eventHubPathHandle = (STRING_HANDLE)0x04;

        prepareCreateEventHubWithConnectionStringToPass(mocks);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(EVENTHUB_PATH))
            .SetReturn(eventHubPathHandle);

        STRICT_EXPECTED_CALL(mocks, URL_Encode(TEST_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, URL_Encode(TEST_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, URL_Encode(TEST_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, URL_Encode(eventHubPathHandle))
            .SetReturn((STRING_HANDLE)NULL);

        // act
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(eventHubHandle);

        // Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_029: [EventHubClient_ CreateFromConnectionString shall create a proton messenger via a call to pn_messenger.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_invokes_pn_messenger_Succeeds)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;

        prepareCreateEventHubWithConnectionStringToPass(mocks);


        STRICT_EXPECTED_CALL(mocks, pn_messenger(IGNORED_PTR_ARG));

        // act
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NOT_NULL(eventHubHandle);

        // Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_039: [EventHubClient_ CreateFromConnectionString shall return EVENTHUBCLIENT_ERROR if it fails to create a proton messenger.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_pn_messenger_Failing_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        prepareCreateEventHubWithConnectionStringToPass(mocks);

        STRICT_EXPECTED_CALL(mocks, pn_messenger(IGNORED_PTR_ARG));

        // act
        g_when_pn_messenger_fail = 1;
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(eventHubHandle);

        // Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }


    /* Tests_SRS_EVENTHUBCLIENT_04_001: [EventHubClient_LL_CreateFromConnectionString shall call pn_messenger_set_outgoing_window so it can get the status of the message sent]  */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_invokes_pn_messenger_set_outgoing_window_Succeeds)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        prepareCreateEventHubWithConnectionStringToPass(mocks);


        STRICT_EXPECTED_CALL(mocks, pn_messenger(NULL));
        STRICT_EXPECTED_CALL(mocks, pn_messenger_set_outgoing_window(IGNORED_PTR_ARG, EXPECTED_OUTGOING_WINDOW_SIZE))
            .IgnoreArgument(1);

        // act
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NOT_NULL(eventHubHandle);

        // Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_04_002: [If EventHubClient_LL_CreateFromConnectionString fail is shall return  NULL;]   */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_pn_messenger_set_outgoing_window_Failing_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        prepareCreateEventHubWithConnectionStringToPass(mocks);


        STRICT_EXPECTED_CALL(mocks, pn_messenger(NULL));
        STRICT_EXPECTED_CALL(mocks, pn_messenger_set_outgoing_window(IGNORED_PTR_ARG, EXPECTED_OUTGOING_WINDOW_SIZE))
            .IgnoreArgument(1).SetReturn((int)42);

        // act
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(eventHubHandle);

        // Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_030: [EventHubClient_ CreateFromConnectionString shall then start the messenger invoking pn_messenger_start.]  */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_invokes_pn_messenger_start_Succeeds)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const STRING_HANDLE eventHubPathHandle = (STRING_HANDLE)0x04;
        
        EXPECTED_CALL(mocks, EventHubClient_GetVersionString());

        prepareCreateEventHubWithConnectionStringToPass(mocks);


        STRICT_EXPECTED_CALL(mocks, STRING_construct(EVENTHUB_PATH))
            .SetReturn(eventHubPathHandle);

        STRICT_EXPECTED_CALL(mocks, pn_messenger_start(NULL))
            .IgnoreArgument(1);


        // act
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NOT_NULL(eventHubHandle);

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_004: [For all other errors, EventHubClient_LL_CreateFromConnectionString shall return NULL.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_invokes_pn_messenger_set_blocking_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const STRING_HANDLE eventHubPathHandle = (STRING_HANDLE)0x04;

        prepareCreateEventHubWithConnectionStringToPass(mocks);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(EVENTHUB_PATH))
            .SetReturn(eventHubPathHandle);

        STRICT_EXPECTED_CALL(mocks, pn_messenger_set_blocking(IGNORED_PTR_ARG, false))
            .IgnoreArgument(1)
            .SetReturn(-1);
        
        // act
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(eventHubHandle);
        mocks.AssertActualAndExpectedCalls();

        // Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_004: [For all other errors, EventHubClient_LL_CreateFromConnectionString shall return NULL.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_invokes_pn_messenger_set_timeout_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        const STRING_HANDLE eventHubPathHandle = (STRING_HANDLE)0x04;

        EXPECTED_CALL(mocks, EventHubClient_GetVersionString());

        prepareCreateEventHubWithConnectionStringToPass(mocks);

        STRICT_EXPECTED_CALL(mocks, STRING_construct(EVENTHUB_PATH))
            .SetReturn(eventHubPathHandle);

        STRICT_EXPECTED_CALL(mocks, pn_messenger_set_timeout(IGNORED_PTR_ARG, EXPECTED_TIMEOUT))
            .IgnoreArgument(1)
            .SetReturn(-1);

        // act
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(eventHubHandle);
        mocks.AssertActualAndExpectedCalls();

        // Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /*** EventHubClient_LL_SendAsync ***/
    /* Tests_SRS_EVENTHUBCLIENT_LL_04_011: [EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_INVALID_ARG if parameter eventHubClientLLHandle or eventDataHandle is NULL.] */
    TEST_FUNCTION(EventHubClient_LL_SendAsync_with_NULL_eventHubLLHandle_fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendAsync(NULL, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_011: [EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_INVALID_ARG if parameter eventHubClientLLHandle or eventDataHandle is NULL.] */
    TEST_FUNCTION(EventHubClient_LL_SendAsync_with_NULL_eventDataHandle_fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        prepareCreateEventHubWithConnectionStringToPass(mocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        mocks.ResetAllCalls();

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendAsync(eventHubHandle, NULL, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_011: [EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_INVALID_ARG if parameter eventHubClientLLHandle or eventDataHandle is NULL.] */
    TEST_FUNCTION(EventHubClient_LL_SendAsync_with_NULL_eventDataHandle_and_NULL_eventHubClientLLHandle_fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;


        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendAsync(NULL, NULL, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);

        //cleanup
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_012: [EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_INVALID_ARG if parameter sendAsyncConfirmationCallback is NULL and userContextCallBack is not NULL.] */
    TEST_FUNCTION(EventHubClient_LL_SendAsync_with_NULL_sendAsyncConfirmationCallbackandNonNullUSerContext_fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        prepareCreateEventHubWithConnectionStringToPass(mocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        mocks.ResetAllCalls();

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, NULL, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_013: [EventHubClient_LL_SendAsync shall add the DLIST outgoingEvents a new record cloning the information from eventDataHandle, sendAsyncConfirmationCallback and userContextCallBack.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_04_015: [Otherwise EventHubClient_LL_SendAsync shall succeed and return EVENTHUBCLIENT_OK.] */
    TEST_FUNCTION(EventHubClient_LL_SendAsync_succeeds)
    {
        ///arrange
        CEventHubClientLLMocks mocks;
        auto dataEventHandle = (EVENTDATA_HANDLE)1;

        prepareCreateEventHubWithConnectionStringToPassFullMock(mocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORE))
            .ExpectedTimesExactly(2)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(mocks, EventData_Clone(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_014: [If cloning and/or adding the information fails for any reason, EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_ERROR.] */
    TEST_FUNCTION(EventHubClient_LL_SendAsync_AllocFail_fail)
    {
        ///arrange
        CEventHubClientLLMocks mocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.windows.net/;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";
        auto dataEventHandle = (EVENTDATA_HANDLE)1;
        
        prepareCreateEventHubWithConnectionStringToPassFullMock(mocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        mocks.ResetAllCalls();

        whenShallmalloc_fail = currentmalloc_call + 1;

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORE))
            .IgnoreArgument(1);

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }
    

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_014: [If cloning and/or adding the information fails for any reason, EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_ERROR.] */
    TEST_FUNCTION(EventHubClient_LL_SendAsync_EventData_Clonefail_fail)
    {
        ///arrange
        CEventHubClientLLMocks mocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.windows.net/;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";
        auto dataEventHandle = (EVENTDATA_HANDLE)1;

        prepareCreateEventHubWithConnectionStringToPassFullMock(mocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(IGNORE))
            .ExpectedTimesExactly(2)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, EventData_Clone(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .ExpectedTimesExactly(2)
            .IgnoreArgument(1);

        // act
        g_whenShallEventClone_fail = 1;
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }
    /*** EventHubClient_LL_Destroy ***/

    /* Tests_SRS_EVENTHUBCLIENT_03_010: [If the eventHubHandle is NULL, EventHubClient_LL_Destroy shall not do anything.] */
    TEST_FUNCTION(EventHubClient_LL_Destroy_with_NULL_eventHubHandle_Does_Nothing)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;

        // act
        EventHubClient_LL_Destroy(NULL);

        // assert
        // Implicit
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_009: [EventHubClient_LL_Destroy shall terminate the usage of this EventHubClient specified by the eventHubHandle and cleanup all associated resources.] */
    TEST_FUNCTION(EventHubClient_LL_Destroy_with_valid_handle_Succeeds)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        prepareCreateEventHubWithConnectionStringToPass(mocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        ASSERT_IS_NOT_NULL(eventHubHandle);

        EXPECTED_CALL(mocks, pn_messenger_free(IGNORED_PTR_ARG));

        // act
        EventHubClient_LL_Destroy(eventHubHandle);

        // assert
        // Implicit
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_034: [EventHubClient_LL_Destroy shall then stop the messenger by invoking pn_messenger_stop.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_04_017: [EventHubClient_LL_Destroy shall complete all the event notifications callbacks that are in the outgoingdestroy the outgoingEvents with the result EVENTHUBCLIENT_CONFIRMATION_DESTROY.] */
    TEST_FUNCTION(EventHubClient_LL_Destroy_invokes_pn_messenger_stop_Succeeds)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        prepareCreateEventHubWithConnectionStringToPass(mocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        STRICT_EXPECTED_CALL(mocks, pn_messenger_stop(NULL))
            .IgnoreArgument(1);

        // act
        EventHubClient_LL_Destroy(eventHubHandle);

        // assert
        // cleanup
    }

    /* EventHubClient_LL_DoWork */

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_018: [if parameter eventHubClientLLHandle is NULL  EventHubClient_LL_DoWork shall immediately return.] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_with_NullHandle_Do_Not_Work)
    {
        //arrange
        CEventHubClientLLMocks mocks;
        //act
        EventHubClient_LL_DoWork(NULL);
        //assert
        mocks.AssertActualAndExpectedCalls();
    }

    TEST_FUNCTION(EventHubClient_LL_DoWork_Encode_Failure)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        prepareCreateEventHubWithConnectionStringToPass(mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        auto dataEventHandle = (EVENTDATA_HANDLE)1;

        g_confirmationResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;
        EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, fakeCallBack, &g_confirmationResult);

        mocks.ResetAllCalls();

        EXPECTED_CALL(mocks, pn_message_set_address(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, pn_message_set_inferred(IGNORED_PTR_ARG, true))
            .IgnoreArgument(1);
        EXPECTED_CALL(mocks, pn_data_encode(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORE))
            .SetReturn(-1);

        //act
        EventHubClient_LL_DoWork(eventHubHandle);

        //assert
        mocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_019: [If the current status of the entry is WAITINGTOBESENT and there is available spots on proton, defined by OUTGOING_WINDOW_SIZE, EventHubClient_LL_DoWork shall call create pn_message and put the message into messenger by calling pn_messenger_put.]  */
    TEST_FUNCTION(EventHubClient_LL_DoWork_validEventHubClient_NoOutgoignMessages_Do_Not_Work)
    {
        //arrange
        CEventHubClientLLMocks mocks;
        prepareCreateEventHubWithConnectionStringToPassFullMock(mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        mocks.ResetAllCalls();
        //act
        EventHubClient_LL_DoWork(eventHubHandle);
        //assert
        mocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_019: [If the current status of the entry is WAITINGTOBESENT and there is available spots on proton, defined by OUTGOING_WINDOW_SIZE, EventHubClient_LL_DoWork shall call create pn_message and put the message into messenger by calling pn_messenger_put.]  */
    /* Tests_SRS_EVENTHUBCLIENT_LL_04_021: [If there are message to be sent, EventHubClient_LL_DoWork shall call pn_messenger_send with parameter -1.] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_validEventHubClient_OneOutgoingMessages_Succeed)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        prepareCreateEventHubWithConnectionStringToPass(mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        auto dataEventHandle = (EVENTDATA_HANDLE)1;

        EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, NULL, NULL);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, pn_message_set_address(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, pn_message_set_inferred(NULL, true))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, pn_messenger_put(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, pn_messenger_outgoing_tracker(NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, pn_messenger_send(NULL, ALL_MESSAGE_IN_OUTGOING_QUEUE))
            .IgnoreArgument(1);

        //act
        EventHubClient_LL_DoWork(eventHubHandle);

        //assert
        mocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_022: [If the current status of the entry is WAITINGFORACK, than EventHubClient_LL_DoWork shall check status of this entry by calling pn_messenger_status.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_04_023: [If the status returned is PN_STATUS_PENDING EventHubClient_LL_DoWork shall do nothing.] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_validEventHubClient_OneOutgoignMessages_callDoWork2Times_getStatusPN_STATUS_PENDING_DoNothing)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        prepareCreateEventHubWithConnectionStringToPass(mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        auto dataEventHandle = (EVENTDATA_HANDLE)1;

        EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, NULL, NULL);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, pn_message_set_address(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, pn_message_set_inferred(NULL, true))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, pn_messenger_put(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, pn_messenger_outgoing_tracker(NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, pn_messenger_send(NULL, ALL_MESSAGE_IN_OUTGOING_QUEUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, pn_messenger_status(NULL, IGNORE)) 
            .IgnoreAllArguments()
            .SetReturn((pn_status_t)PN_STATUS_PENDING);

        //act
        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message is marked as WAITFORACK

        EventHubClient_LL_DoWork(eventHubHandle);

        //assert
        mocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_025: [If the status returned is any other EventHubClient_LL_DoWork shall call the callback (if exist) with CONFIRMATION_RESULT value of EVENTHUBCLIENT_CONFIRMATION_ERROR and remove the event from the list.]*/
    /* Testst_SRS_EVENTHUBCLIENT_LL_04_026: [EventHubClient_LL_DoWork shall free the tracker before removing the event from the list by calling pn_messenger_settle] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_validEventHubClient_OneOutgoignMessages_callDoWork3Times_getStatusPN_STATUS_UNKNOWN_Remove_Message)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        prepareCreateEventHubWithConnectionStringToPass(mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        auto dataEventHandle = (EVENTDATA_HANDLE)1;
        g_confirmationResult = EVENTHUBCLIENT_CONFIRMATION_UNKNOWN;

        EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, fakeCallBack, &g_confirmationResult);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, pn_message_set_address(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, pn_message_set_inferred(NULL, true))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, pn_messenger_put(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, pn_messenger_outgoing_tracker(NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, pn_messenger_send(NULL, ALL_MESSAGE_IN_OUTGOING_QUEUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, pn_messenger_status(NULL, IGNORE)) 
            .IgnoreAllArguments()
            .SetReturn((pn_status_t)PN_STATUS_UNKNOWN);

        //act
        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message is marked as WAITFORACK

        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message has to be removed from the list (Called Callback). 

        EventHubClient_LL_DoWork(eventHubHandle); //This Shall Do nothing.

        //assert
        mocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_025: [If the status returned is any other EventHubClient_LL_DoWork shall call the callback (if exist) with CONFIRMATION_RESULT value of EVENTHUBCLIENT_CONFIRMATION_ERROR and remove the event from the list.]*/
    /* Testst_SRS_EVENTHUBCLIENT_LL_04_026: [EventHubClient_LL_DoWork shall free the tracker before removing the event from the list by calling pn_messenger_settle] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_validEventHubClient_OneOutgoignMessages_callDoWork3Times_getStatusPN_STATUS_REJECTED_Remove_Message)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        prepareCreateEventHubWithConnectionStringToPass(mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        auto dataEventHandle = (EVENTDATA_HANDLE)1;
        g_confirmationResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;

        EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, fakeCallBack, &g_confirmationResult);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, pn_message_set_address(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, pn_message_set_inferred(NULL, true))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, pn_messenger_put(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, pn_messenger_outgoing_tracker(NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, pn_messenger_send(NULL, ALL_MESSAGE_IN_OUTGOING_QUEUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, pn_messenger_status(NULL, IGNORE))
            .IgnoreAllArguments()
            .SetReturn((pn_status_t)PN_STATUS_REJECTED);
        STRICT_EXPECTED_CALL(mocks, pn_messenger_settle(NULL, IGNORE, 0))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        //act
        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message is marked as WAITFORACK

        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message has to be removed from the list (Called Callback). 

        EventHubClient_LL_DoWork(eventHubHandle); //This Shall Do nothing.

        //assert
        mocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_025: [If the status returned is any other EventHubClient_LL_DoWork shall call the callback (if exist) with CONFIRMATION_RESULT value of EVENTHUBCLIENT_CONFIRMATION_ERROR and remove the event from the list.]*/
    /* Testst_SRS_EVENTHUBCLIENT_LL_04_026: [EventHubClient_LL_DoWork shall free the tracker before removing the event from the list by calling pn_messenger_settle] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_validEventHubClient_OneOutgoignMessages_callDoWork3Times_getStatusPN_STATUS_ABORTED_Remove_Message)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        prepareCreateEventHubWithConnectionStringToPass(mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        auto dataEventHandle = (EVENTDATA_HANDLE)1;
        g_confirmationResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;

        EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, fakeCallBack, &g_confirmationResult);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, pn_message_set_address(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, pn_message_set_inferred(NULL, true))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, pn_messenger_put(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, pn_messenger_outgoing_tracker(NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, pn_messenger_send(NULL, ALL_MESSAGE_IN_OUTGOING_QUEUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, pn_messenger_status(NULL, IGNORE))
            .IgnoreAllArguments()
            .SetReturn((pn_status_t)PN_STATUS_ABORTED);
        STRICT_EXPECTED_CALL(mocks, pn_messenger_settle(NULL, IGNORE, 0))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        //act
        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message is marked as WAITFORACK

        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message has to be removed from the list (Called Callback). 

        EventHubClient_LL_DoWork(eventHubHandle); //This Shall Do nothing.

        //assert
        mocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_024: [If the status returned is PN_STATUS_ACCEPTED EventHubClient_LL_DoWork shall call the call back (if exist) with CONFIRMATION_RESULT value of EVENTHUBCLIENT_CONFIRMATION_OK and remove the event from the list.]*/
    /* Testst_SRS_EVENTHUBCLIENT_LL_04_026: [EventHubClient_LL_DoWork shall free the tracker before removing the event from the list by calling pn_messenger_settle] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_validEventHubClient_OneOutgoignMessages_callDoWork3Times_getStatusPN_STATUS_ACCEPTED_CallCallback_Remove_Message)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        prepareCreateEventHubWithConnectionStringToPass(mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        auto dataEventHandle = (EVENTDATA_HANDLE)1;

        g_confirmationResult = EVENTHUBCLIENT_CONFIRMATION_OK;

        EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, fakeCallBack, &g_confirmationResult);

        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, pn_message_set_address(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, pn_message_set_inferred(NULL, true))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, pn_messenger_put(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, pn_messenger_outgoing_tracker(NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(mocks, pn_messenger_send(NULL, ALL_MESSAGE_IN_OUTGOING_QUEUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(mocks, pn_messenger_status(NULL, IGNORE))
            .IgnoreAllArguments()
            .SetReturn((pn_status_t)PN_STATUS_ACCEPTED);
        STRICT_EXPECTED_CALL(mocks, pn_messenger_settle(NULL, IGNORE, 0))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        //act
        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message is marked as WAITFORACK

        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message has to be removed from the list (Called Callback). 

        EventHubClient_LL_DoWork(eventHubHandle); //This Shall Do nothing.

        //assert
        mocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_015: [On success EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_OK.] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_BatchSendAsync_SUCCEED)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        prepareCreateEventHubWithConnectionStringToPass(mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        EVENTHUBCLIENT_RESULT result;
        EVENTDATA_HANDLE eventhandleList[3];

        eventhandleList[0] = (EVENTDATA_HANDLE)1;
        eventhandleList[1] = (EVENTDATA_HANDLE)2;
        eventhandleList[2] = (EVENTDATA_HANDLE)4;

        EVENTHUBCLIENT_CONFIRMATION_RESULT sendBatchAsyncResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;

        result = EventHubClient_LL_SendBatchAsync(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT, sendAsyncConfirmationCallback, &sendBatchAsyncResult);
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);

        mocks.ResetAllCalls();

        EXPECTED_CALL(mocks, pn_message_set_address(NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, pn_message_set_inferred(NULL, true))
            .IgnoreArgument(1);
        EXPECTED_CALL(mocks, pn_messenger_put(NULL, NULL));
        EXPECTED_CALL(mocks, pn_messenger_outgoing_tracker(NULL));
        STRICT_EXPECTED_CALL(mocks, pn_messenger_send(NULL, ALL_MESSAGE_IN_OUTGOING_QUEUE))
            .IgnoreArgument(1);
        EXPECTED_CALL(mocks, pn_messenger_status(NULL, IGNORE))
            .SetReturn((pn_status_t)PN_STATUS_ACCEPTED);
        EXPECTED_CALL(mocks, pn_messenger_settle(NULL, IGNORE, 0));

        //act
        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message is marked as WAITFORACK

        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message has to be removed from the list (Called Callback). 

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_CONFIRMATION_RESULT, EVENTHUBCLIENT_CONFIRMATION_OK, sendBatchAsyncResult);

        mocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_015: [On success EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_OK.] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_BatchSendAsync_With_PartitionKey_SUCCEED)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        prepareCreateEventHubWithConnectionStringToPass(mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        EVENTHUBCLIENT_RESULT result;
        EVENTDATA_HANDLE eventhandleList[EVENT_HANDLE_COUNT];

        eventhandleList[0] = (EVENTDATA_HANDLE)1;
        eventhandleList[1] = (EVENTDATA_HANDLE)2;
        eventhandleList[2] = (EVENTDATA_HANDLE)4;

        EVENTHUBCLIENT_CONFIRMATION_RESULT expectedResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;

        result = EventHubClient_LL_SendBatchAsync(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT, sendAsyncConfirmationCallback, &expectedResult);
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);

        mocks.ResetAllCalls();

        EXPECTED_CALL(mocks, pn_message_set_address(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, pn_message_set_inferred(IGNORED_PTR_ARG, true))
            .IgnoreArgument(1);
        EXPECTED_CALL(mocks, pn_messenger_put(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, pn_messenger_outgoing_tracker(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, pn_messenger_send(IGNORED_PTR_ARG, ALL_MESSAGE_IN_OUTGOING_QUEUE))
            .IgnoreArgument(1);
        EXPECTED_CALL(mocks, pn_messenger_status(IGNORED_PTR_ARG, IGNORE))
            .SetReturn((pn_status_t)PN_STATUS_ACCEPTED);
        EXPECTED_CALL(mocks, pn_messenger_settle(IGNORED_PTR_ARG, IGNORE, 0));
        EXPECTED_CALL(mocks, EventData_GetPartitionKey(IGNORED_PTR_ARG))
            .SetReturn(PARTITION_KEY_VALUE);
        EXPECTED_CALL(mocks, pn_message_annotations(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, pn_data_put_map(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, pn_data_enter(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, pn_bytes(IGNORE, IGNORED_PTR_ARG))
            .ExpectedTimesExactly(5);

        //act
        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message is marked as WAITFORACK

        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message has to be removed from the list (Called Callback). 

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_CONFIRMATION_RESULT, EVENTHUBCLIENT_CONFIRMATION_OK, expectedResult);

        mocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_015: [On success EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_OK.] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_BatchSend_With_Property_SUCCEED)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;
        prepareCreateEventHubWithConnectionStringToPass(mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        EVENTHUBCLIENT_RESULT result;
        EVENTDATA_HANDLE eventhandleList[EVENT_HANDLE_COUNT];

        eventhandleList[0] = (EVENTDATA_HANDLE)1;
        eventhandleList[1] = (EVENTDATA_HANDLE)2;
        eventhandleList[2] = (EVENTDATA_HANDLE)4;

        EVENTHUBCLIENT_CONFIRMATION_RESULT expectedResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;

        result = EventHubClient_LL_SendBatchAsync(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT, sendAsyncConfirmationCallback, &expectedResult);
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);

        mocks.ResetAllCalls();

        EXPECTED_CALL(mocks, pn_message_set_address(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, pn_message_set_inferred(IGNORED_PTR_ARG, true))
            .IgnoreArgument(1);
        EXPECTED_CALL(mocks, pn_messenger_put(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, pn_messenger_outgoing_tracker(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, pn_messenger_send(IGNORED_PTR_ARG, ALL_MESSAGE_IN_OUTGOING_QUEUE))
            .IgnoreArgument(1);
        EXPECTED_CALL(mocks, pn_messenger_status(IGNORED_PTR_ARG, IGNORE))
            .SetReturn((pn_status_t)PN_STATUS_ACCEPTED);
        EXPECTED_CALL(mocks, pn_messenger_settle(IGNORED_PTR_ARG, IGNORE, 0));
        EXPECTED_CALL(mocks, EventData_Properties(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, Map_GetInternals(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, pn_data(0));
        EXPECTED_CALL(mocks, pn_data_put_map(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, pn_data_enter(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, pn_bytes(IGNORE, IGNORED_PTR_ARG))
            .ExpectedTimesExactly(15);
        
        g_setProperty = true;
        g_includeProperties = true;

        //act
        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message is marked as WAITFORACK

        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message has to be removed from the list (Called Callback). 

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_CONFIRMATION_RESULT, EVENTHUBCLIENT_CONFIRMATION_OK, expectedResult);

        mocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_015: [On success EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_OK.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_07_014: [EventHubClient_LL_SendBatchAsync shall clone each item in the eventDataList by calling EventData_Clone.] */
    TEST_FUNCTION(EventHubClient_LL_SendBatchAsync_SUCCEED)
    {
        ///arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE eventhandleList[3];
        EVENTHUBCLIENT_CONFIRMATION_RESULT confirmationResult;

        eventhandleList[0] = (EVENTDATA_HANDLE)1;
        eventhandleList[1] = (EVENTDATA_HANDLE)2;
        eventhandleList[2] = (EVENTDATA_HANDLE)4;

        prepareCreateEventHubWithConnectionStringToPassFullMock(mocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        mocks.ResetAllCalls();

        EXPECTED_CALL(mocks, gballoc_malloc(IGNORE))
            .ExpectedTimesExactly(2);
        EXPECTED_CALL(mocks, DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, EventData_GetPartitionKey(IGNORED_PTR_ARG))
            .ExpectedTimesExactly(EVENT_HANDLE_COUNT);
        EXPECTED_CALL(mocks, EventData_Clone(IGNORED_PTR_ARG))
            .ExpectedTimesExactly(EVENT_HANDLE_COUNT);

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT, sendAsyncConfirmationCallback, &confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_012: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_INVALLID_ARG if eventhubClientLLHandle or eventDataList are NULL or if sendAsnycConfirmationCallback equals NULL and userContextCallback does not equal NULL.] */
    TEST_FUNCTION(EventHubClient_LL_SendBatchAsync_EVENTHUBHANDLE_NULL_Fail)
    {
        ///arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE eventhandleList[3];
        eventhandleList[0] = (EVENTDATA_HANDLE)1;
        eventhandleList[1] = (EVENTDATA_HANDLE)2;
        eventhandleList[2] = (EVENTDATA_HANDLE)4;

        mocks.ResetAllCalls();

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(NULL, eventhandleList, EVENT_HANDLE_COUNT, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_012: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_INVALID_ARG if eventhubClientLLHandle or eventDataList are NULL or if sendAsnycConfirmationCallback equals NULL and userContextCallback does not equal NULL.] */
    TEST_FUNCTION(EventHubClient_LL_SendBatchAsync_EVENTDATA_List_NULL_Fail)
    {
        ///arrange
        CEventHubClientLLMocks mocks;

        prepareCreateEventHubWithConnectionStringToPassFullMock(mocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        mocks.ResetAllCalls();

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, NULL, EVENT_HANDLE_COUNT, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_012: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_INVALID_ARG if eventhubClientLLHandle or eventDataList are NULL or if sendAsnycConfirmationCallback equals NULL and userContextCallback does not equal NULL.] */
    TEST_FUNCTION(EventHubClient_LL_SendBatchAsync_Count_0_Fail)
    {
        ///arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE eventhandleList[3];

        eventhandleList[0] = (EVENTDATA_HANDLE)1;
        eventhandleList[1] = (EVENTDATA_HANDLE)2;
        eventhandleList[2] = (EVENTDATA_HANDLE)4;

        prepareCreateEventHubWithConnectionStringToPassFullMock(mocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        mocks.ResetAllCalls();

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, eventhandleList, 0, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_012: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_INVALID_ARG if eventhubClientLLHandle or eventDataList are NULL or if sendAsnycConfirmationCallback equals NULL and userContextCallback does not equal NULL.] */
    TEST_FUNCTION(EventHubClient_LL_SendBatchAsync_sendAsyncConfirmationCallback_NULL_userContext_NonNULL_Fail)
    {
        ///arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE eventhandleList[3];

        eventhandleList[0] = (EVENTDATA_HANDLE)1;
        eventhandleList[1] = (EVENTDATA_HANDLE)2;
        eventhandleList[2] = (EVENTDATA_HANDLE)4;

        prepareCreateEventHubWithConnectionStringToPassFullMock(mocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        mocks.ResetAllCalls();

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT, NULL, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_013: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_ERROR for any Error that is encountered.] */
    TEST_FUNCTION(EventHubClient_LL_SendBatchAsync_EVENTDATAT_HANDLE_NULL_Fail)
    {
        ///arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE eventhandleList[3];

        eventhandleList[0] = (EVENTDATA_HANDLE)1;
        eventhandleList[1] = (EVENTDATA_HANDLE)2;
        eventhandleList[2] = (EVENTDATA_HANDLE)NULL;

        prepareCreateEventHubWithConnectionStringToPassFullMock(mocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        mocks.ResetAllCalls();

        EXPECTED_CALL(mocks, EventData_GetPartitionKey(IGNORED_PTR_ARG))
            .ExpectedTimesExactly(2);

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_013: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_ERROR for any Error that is encountered.] */
    TEST_FUNCTION(EventHubClient_LL_SendBatchAsync_EVENTDATA_Clone_NULL_Fail)
    {
        ///arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE eventhandleList[3];

        eventhandleList[0] = (EVENTDATA_HANDLE)1;
        eventhandleList[1] = (EVENTDATA_HANDLE)2;
        eventhandleList[2] = (EVENTDATA_HANDLE)4;

        prepareCreateEventHubWithConnectionStringToPassFullMock(mocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        mocks.ResetAllCalls();

        EXPECTED_CALL(mocks, gballoc_malloc(IGNORE))
            .ExpectedTimesExactly(2);
        EXPECTED_CALL(mocks, EventData_GetPartitionKey(IGNORED_PTR_ARG))
            .ExpectedTimesExactly(EVENT_HANDLE_COUNT);
        EXPECTED_CALL(mocks, EventData_Clone(IGNORED_PTR_ARG))
            .ExpectedTimesExactly(2);
        EXPECTED_CALL(mocks, EventData_Destroy(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORE))
            .ExpectedTimesExactly(2);
        
        // act
        g_whenShallEventClone_fail = 2;
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }
#endif

END_TEST_SUITE(eventhubclient_ll_unittests)
