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
#include "azure_c_shared_utility/strings.h"
#include "connection_string_parser.h"
#include "azure_c_shared_utility/xio.h"
#include "azure_uamqp_c/connection.h"
#include "azure_uamqp_c/session.h"
#include "azure_uamqp_c/link.h"
#include "azure_uamqp_c/messaging.h"
#include "azure_uamqp_c/message.h"
#include "azure_uamqp_c/message_sender.h"
#include "azure_uamqp_c/sasl_mechanism.h"
#include "azure_uamqp_c/sasl_plain.h"
#include "azure_uamqp_c/saslclientio.h"
#include "version.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_uamqp_c/amqp_definitions.h"
#include "azure_uamqp_c/amqpvalue.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/map.h"

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

#define TEST_CONNSTR_HANDLE         (STRING_HANDLE)0x46
#define DUMMY_STRING_HANDLE         (STRING_HANDLE)0x47
#define TEST_HOSTNAME_STRING_HANDLE (STRING_HANDLE)0x48
#define TEST_KEYNAME_STRING_HANDLE  (STRING_HANDLE)0x49
#define TEST_KEY_STRING_HANDLE      (STRING_HANDLE)0x4A
#define TEST_TARGET_STRING_HANDLE   (STRING_HANDLE)0x4B
#define TEST_CONNSTR_MAP_HANDLE     (MAP_HANDLE)0x50

#define TEST_CLONED_EVENTDATA_HANDLE_1     (EVENTDATA_HANDLE)0x4240
#define TEST_CLONED_EVENTDATA_HANDLE_2     (EVENTDATA_HANDLE)0x4241

#define TEST_EVENTDATA_HANDLE     (EVENTDATA_HANDLE)0x4243
#define TEST_EVENTDATA_HANDLE_1   (EVENTDATA_HANDLE)0x4244
#define TEST_EVENTDATA_HANDLE_2   (EVENTDATA_HANDLE)0x4245

#define TEST_STRING_TOKENIZER_HANDLE (STRING_TOKENIZER_HANDLE)0x48
#define TEST_MAP_HANDLE (MAP_HANDLE)0x49
#define MICROSOFT_MESSAGE_FORMAT 0x80013700

#define CONNECTION_STRING "Endpoint=sb://servicebusName.servicebus.windows.net/;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8="
#define TEST_EVENTHUB_PATH "eventHubName"
#define TEST_ENDPOINT "sb://servicebusName.servicebus.windows.net"
#define TEST_HOSTNAME "servicebusName.servicebus.windows.net"
#define TEST_KEYNAME "RootManageSharedAccessKey"
#define TEST_KEY "icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8="

static ON_MESSAGE_SENDER_STATE_CHANGED saved_on_message_sender_state_changed;
static void* saved_message_sender_context;

static const AMQP_VALUE TEST_PROPERTY_1_KEY_AMQP_VALUE = (AMQP_VALUE)0x5001;
static const AMQP_VALUE TEST_PROPERTY_1_VALUE_AMQP_VALUE = (AMQP_VALUE)0x5002;
static const AMQP_VALUE TEST_PROPERTY_2_KEY_AMQP_VALUE = (AMQP_VALUE)0x5003;
static const AMQP_VALUE TEST_PROPERTY_2_VALUE_AMQP_VALUE = (AMQP_VALUE)0x5004;
static const AMQP_VALUE TEST_UAMQP_MAP = (AMQP_VALUE)0x5004;

static const AMQP_VALUE DUMMY_APPLICATION_PROPERTIES = (AMQP_VALUE)0x6001;
static const AMQP_VALUE TEST_APPLICATION_PROPERTIES_1 = (AMQP_VALUE)0x6002;
static const AMQP_VALUE TEST_APPLICATION_PROPERTIES_2 = (AMQP_VALUE)0x6003;
static const AMQP_VALUE DUMMY_DATA = (AMQP_VALUE)0x7001;
static const AMQP_VALUE TEST_DATA_1 = (AMQP_VALUE)0x7002;
static const AMQP_VALUE TEST_DATA_2 = (AMQP_VALUE)0x7003;

static const char* const no_property_keys[] = { "test_property_key" };
static const char* const no_property_values[] = { "test_property_value" };
static const char* const* no_property_keys_ptr = no_property_keys;
static const char* const* no_property_values_ptr = no_property_values;
static size_t no_property_size = 0;

static unsigned char* g_expected_encoded_buffer[3];
static size_t g_expected_encoded_length[3];
static size_t g_expected_encoded_counter;

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
static DLIST_ENTRY* saved_pending_list;

static size_t g_currentEventClone_call;
static size_t g_whenShallEventClone_fail;

typedef struct CALLBACK_CONFIRM_INFO
{
    sig_atomic_t successCounter;
    sig_atomic_t totalCalls;
} CALLBACK_CONFIRM_INFO;

static ON_MESSAGE_SEND_COMPLETE saved_on_message_send_complete;
static void* saved_on_message_send_complete_context;

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

std::ostream& operator<<(std::ostream& left, const amqp_binary& binary)
{
    std::ios::fmtflags f(left.flags());
    left << std::hex;
    for (size_t i = 0; i < binary.length; i++)
    {
        left << ((const unsigned char*)binary.bytes)[i];
    }
    left.flags(f);
    return left;
}

static bool operator==(const amqp_binary left, const amqp_binary& right)
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
    MOCK_STATIC_METHOD_2(, XIO_HANDLE, xio_create, const IO_INTERFACE_DESCRIPTION*, io_interface_description, const void*, io_create_parameters)
        if (saved_tlsio_parameters == NULL)
        {
            saved_tlsio_parameters = (TLSIO_CONFIG*)malloc(sizeof(TLSIO_CONFIG));
            saved_tlsio_parameters->port = ((TLSIO_CONFIG*)io_create_parameters)->port;
            saved_tlsio_parameters->hostname = (char*)malloc(strlen(((TLSIO_CONFIG*)io_create_parameters)->hostname) + 1);
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
    MOCK_STATIC_METHOD_2(, void, connection_set_trace, CONNECTION_HANDLE, connection, bool, traceOn)
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
    MOCK_STATIC_METHOD_1(, void, message_destroy, MESSAGE_HANDLE, message)
    MOCK_VOID_METHOD_END();
    MOCK_STATIC_METHOD_2(, int, message_set_application_properties, MESSAGE_HANDLE, message, AMQP_VALUE, application_properties);
    MOCK_METHOD_END(int, 0);
    MOCK_STATIC_METHOD_2(, int, message_add_body_amqp_data, MESSAGE_HANDLE, message, BINARY_DATA, binary_data);
    MOCK_METHOD_END(int, 0);
    MOCK_STATIC_METHOD_2(, int, message_set_message_format, MESSAGE_HANDLE, message, uint32_t, message_format)
    MOCK_METHOD_END(int, 0);

    /* messagesender mocks */
    MOCK_STATIC_METHOD_3(, MESSAGE_SENDER_HANDLE, messagesender_create, LINK_HANDLE, link, ON_MESSAGE_SENDER_STATE_CHANGED, on_message_sender_state_changed, void*, context);
        saved_on_message_sender_state_changed = on_message_sender_state_changed;
        saved_message_sender_context = context;
    MOCK_METHOD_END(MESSAGE_SENDER_HANDLE, TEST_MESSAGE_SENDER_HANDLE);
    MOCK_STATIC_METHOD_1(, void, messagesender_destroy, MESSAGE_SENDER_HANDLE, message_sender);
    MOCK_VOID_METHOD_END();
    MOCK_STATIC_METHOD_1(, int, messagesender_open, MESSAGE_SENDER_HANDLE, message_sender);
    MOCK_METHOD_END(int, 0);
    MOCK_STATIC_METHOD_1(, int, messagesender_close, MESSAGE_SENDER_HANDLE, message_sender);
    MOCK_METHOD_END(int, 0);
    MOCK_STATIC_METHOD_4(, int, messagesender_send, MESSAGE_SENDER_HANDLE, message_sender, MESSAGE_HANDLE, message, ON_MESSAGE_SEND_COMPLETE, on_message_send_complete, void*, callback_context);
        saved_on_message_send_complete = on_message_send_complete;
        saved_on_message_send_complete_context = callback_context;
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
            saved_sasl_mechanism_create_parameters->authcid = (char*)malloc(strlen(((SASL_PLAIN_CONFIG*)sasl_mechanism_create_parameters)->authcid) + 1);
            (void)strcpy((char*)saved_sasl_mechanism_create_parameters->authcid, ((SASL_PLAIN_CONFIG*)sasl_mechanism_create_parameters)->authcid);
        }
        if (((SASL_PLAIN_CONFIG*)sasl_mechanism_create_parameters)->authzid == NULL)
        {
            saved_sasl_mechanism_create_parameters->authzid = NULL;
        }
        else
        {
            saved_sasl_mechanism_create_parameters->authzid = (char*)malloc(strlen(((SASL_PLAIN_CONFIG*)sasl_mechanism_create_parameters)->authzid) + 1);
            (void)strcpy((char*)saved_sasl_mechanism_create_parameters->authzid, ((SASL_PLAIN_CONFIG*)sasl_mechanism_create_parameters)->authzid);
        }
        if (((SASL_PLAIN_CONFIG*)sasl_mechanism_create_parameters)->passwd == NULL)
        {
            saved_sasl_mechanism_create_parameters->passwd = NULL;
        }
        else
        {
            saved_sasl_mechanism_create_parameters->passwd = (char*)malloc(strlen(((SASL_PLAIN_CONFIG*)sasl_mechanism_create_parameters)->passwd) + 1);
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
    MOCK_STATIC_METHOD_1(, void, amqpvalue_destroy, AMQP_VALUE, value);
    MOCK_VOID_METHOD_END();
    MOCK_STATIC_METHOD_3(, int, amqpvalue_encode, AMQP_VALUE, value, AMQPVALUE_ENCODER_OUTPUT, encoder_output, void*, context)
        encoder_output(context, g_expected_encoded_buffer[g_expected_encoded_counter], g_expected_encoded_length[g_expected_encoded_counter]);
        g_expected_encoded_counter++;
    MOCK_METHOD_END(int, 0);
    MOCK_STATIC_METHOD_2(, int, amqpvalue_get_encoded_size, AMQP_VALUE, value, size_t*, encoded_size)
    MOCK_METHOD_END(int, 0);
    MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_create_application_properties, AMQP_VALUE, value)
    MOCK_METHOD_END(AMQP_VALUE, DUMMY_APPLICATION_PROPERTIES);
    MOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_create_data, data, value)
    MOCK_METHOD_END(AMQP_VALUE, DUMMY_DATA);

    /* map mocks */
    MOCK_STATIC_METHOD_2(, const char*, Map_GetValueFromKey, MAP_HANDLE, handle, const char*, key)
    MOCK_METHOD_END(const char*, NULL);
    MOCK_STATIC_METHOD_1(, void, Map_Destroy, MAP_HANDLE, handle)
    MOCK_VOID_METHOD_END();

    /* connectionstringparser mocks */
    MOCK_STATIC_METHOD_1(, MAP_HANDLE, connectionstringparser_parse, STRING_HANDLE, connection_string)
    MOCK_METHOD_END(MAP_HANDLE, TEST_CONNSTR_MAP_HANDLE);

    /* gballoc mocks */
    MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
    MOCK_METHOD_END(void*, BASEIMPLEMENTATION::gballoc_malloc(size));

    MOCK_STATIC_METHOD_2(, void*, gballoc_realloc, void*, ptr, size_t, size)
        MOCK_METHOD_END(void*, BASEIMPLEMENTATION::gballoc_realloc(ptr, size));

    MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
        BASEIMPLEMENTATION::gballoc_free(ptr);
    MOCK_VOID_METHOD_END()

        /* DoublyLinkedList mocks */
    MOCK_STATIC_METHOD_1(, void, DList_InitializeListHead, PDLIST_ENTRY, listHead)
        saved_pending_list = listHead;
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
    MOCK_METHOD_END(EVENTDATA_RESULT, EVENTDATA_OK)

    MOCK_STATIC_METHOD_1(, void, EventData_Destroy, EVENTDATA_HANDLE, eventDataHandle)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, const char*, EventData_GetPartitionKey, EVENTDATA_HANDLE, eventDataHandle)
    MOCK_METHOD_END(const char*, NULL)

    MOCK_STATIC_METHOD_1(, MAP_HANDLE, EventData_Properties, EVENTDATA_HANDLE, eventDataHandle)
    MOCK_METHOD_END(MAP_HANDLE, TEST_MAP_HANDLE)

    MOCK_STATIC_METHOD_1(, EVENTDATA_HANDLE, EventData_Clone, EVENTDATA_HANDLE, eventDataHandle)
    MOCK_METHOD_END(EVENTDATA_HANDLE, TEST_CLONED_EVENTDATA_HANDLE_1)

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

    MOCK_STATIC_METHOD_2(, STRING_HANDLE, STRING_construct_n, const char*, psz, size_t, n)
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
    MOCK_VOID_METHOD_END()
};

DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , XIO_HANDLE, xio_create, const IO_INTERFACE_DESCRIPTION*, io_interface_description, const void*, io_create_parameters);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, xio_destroy, XIO_HANDLE, xio);

DECLARE_GLOBAL_MOCK_METHOD_5(CEventHubClientLLMocks, , CONNECTION_HANDLE, connection_create, XIO_HANDLE, xio, const char*, hostname, const char*, container_id, ON_NEW_ENDPOINT, on_new_endpoint, void*, callback_context);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, connection_destroy, CONNECTION_HANDLE, connection);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, connection_dowork, CONNECTION_HANDLE, connection);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , void, connection_set_trace, CONNECTION_HANDLE, connection, bool, traceOn);

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
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, message_destroy, MESSAGE_HANDLE, message);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, message_set_application_properties, MESSAGE_HANDLE, message, AMQP_VALUE, application_properties);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, message_add_body_amqp_data, MESSAGE_HANDLE, message, BINARY_DATA, binary_data);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, message_set_message_format, MESSAGE_HANDLE, message, uint32_t, message_format);

DECLARE_GLOBAL_MOCK_METHOD_3(CEventHubClientLLMocks, , MESSAGE_SENDER_HANDLE, messagesender_create, LINK_HANDLE, link, ON_MESSAGE_SENDER_STATE_CHANGED, on_message_sender_state_changed, void*, context);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, messagesender_destroy, MESSAGE_SENDER_HANDLE, message_sender);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , int, messagesender_open, MESSAGE_SENDER_HANDLE, message_sender);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , int, messagesender_close, MESSAGE_SENDER_HANDLE, message_sender);
DECLARE_GLOBAL_MOCK_METHOD_4(CEventHubClientLLMocks, , int, messagesender_send, MESSAGE_SENDER_HANDLE, message_sender, MESSAGE_HANDLE, message, ON_MESSAGE_SEND_COMPLETE, on_message_send_complete, void*, callback_context);

DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , SASL_MECHANISM_HANDLE, saslmechanism_create, const SASL_MECHANISM_INTERFACE_DESCRIPTION*, sasl_mechanism_interface_description, void*, sasl_mechanism_create_parameters);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, saslmechanism_destroy, SASL_MECHANISM_HANDLE, sasl_mechanism);

DECLARE_GLOBAL_MOCK_METHOD_0(CEventHubClientLLMocks, , const IO_INTERFACE_DESCRIPTION*, saslclientio_get_interface_description);

DECLARE_GLOBAL_MOCK_METHOD_0(CEventHubClientLLMocks, , const SASL_MECHANISM_INTERFACE_DESCRIPTION*, saslplain_get_interface);

DECLARE_GLOBAL_MOCK_METHOD_0(CEventHubClientLLMocks, , const IO_INTERFACE_DESCRIPTION*, platform_get_default_tlsio);

DECLARE_GLOBAL_MOCK_METHOD_0(CEventHubClientLLMocks, , AMQP_VALUE, amqpvalue_create_map);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , AMQP_VALUE, amqpvalue_create_string, const char*, value);
DECLARE_GLOBAL_MOCK_METHOD_3(CEventHubClientLLMocks, , int, amqpvalue_set_map_value, AMQP_VALUE, map, AMQP_VALUE, key, AMQP_VALUE, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, amqpvalue_destroy, AMQP_VALUE, value);
DECLARE_GLOBAL_MOCK_METHOD_3(CEventHubClientLLMocks, , int, amqpvalue_encode, AMQP_VALUE, value, AMQPVALUE_ENCODER_OUTPUT, encoder_output, void*, context);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, amqpvalue_get_encoded_size, AMQP_VALUE, value, size_t*, encoded_size);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , AMQP_VALUE, amqpvalue_create_application_properties, AMQP_VALUE, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , AMQP_VALUE, amqpvalue_create_data, data, value);

DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , const char*, Map_GetValueFromKey, MAP_HANDLE, handle, const char*, key);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, Map_Destroy, MAP_HANDLE, handle);

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
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , STRING_HANDLE, STRING_construct_n, const char*, psz, size_t, n);
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
        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = MicroMockCreateMutex();
        ASSERT_IS_NOT_NULL(g_testByTest);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        MicroMockDestroyMutex(g_testByTest);
        TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
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

        g_setProperty = false;

        g_currentEventClone_call = 0;
        g_whenShallEventClone_fail = 0;
        g_confirmationResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;

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

    static void eventhub_state_change_callback(EVENTHUBCLIENT_STATE eventhub_state, void* userContextCallback)
    {
        //printf("eventhub_state_change_callback %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_STATE, eventhub_state) );
    }

    static void eventhub_error_callback(EVENTHUBCLIENT_ERROR_RESULT eventhub_failure, void* userContextCallback)
    {
        //printf("eventhub_error_callback %s\r\n", ENUM_TO_STRING(EVENTHUBCLIENT_ERROR_RESULT, eventhub_failure) );
    }

    static void setup_createfromconnectionstring_success(CEventHubClientLLMocks* mocks)
    {
        STRICT_EXPECTED_CALL((*mocks), EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL((*mocks), STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        STRICT_EXPECTED_CALL((*mocks), connectionstringparser_parse(TEST_CONNSTR_HANDLE));
        EXPECTED_CALL((*mocks), gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL((*mocks), Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "Endpoint"))
            .SetReturn(TEST_ENDPOINT);
        STRICT_EXPECTED_CALL((*mocks), STRING_construct_n(TEST_HOSTNAME, strlen(TEST_HOSTNAME)))
            .SetReturn(TEST_HOSTNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL((*mocks), Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKeyName"))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL((*mocks), STRING_construct(TEST_KEYNAME))
            .SetReturn(TEST_KEYNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL((*mocks), Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKey"))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL((*mocks), STRING_construct(TEST_KEY))
            .SetReturn(TEST_KEY_STRING_HANDLE);
        STRICT_EXPECTED_CALL((*mocks), STRING_construct("amqps://"))
            .SetReturn(TEST_TARGET_STRING_HANDLE);
        STRICT_EXPECTED_CALL((*mocks), STRING_concat_with_STRING(TEST_TARGET_STRING_HANDLE, TEST_HOSTNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL((*mocks), STRING_concat(TEST_TARGET_STRING_HANDLE, "/"));
        STRICT_EXPECTED_CALL((*mocks), STRING_concat(TEST_TARGET_STRING_HANDLE, TEST_EVENTHUB_PATH));
        EXPECTED_CALL((*mocks), DList_InitializeListHead(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL((*mocks), Map_Destroy(TEST_CONNSTR_MAP_HANDLE));
        STRICT_EXPECTED_CALL((*mocks), STRING_delete(TEST_CONNSTR_HANDLE));
    }

    static void setup_messenger_initialize_success(CEventHubClientLLMocks* mocks)
    {
        STRICT_EXPECTED_CALL((*mocks), STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(TEST_HOSTNAME);
        STRICT_EXPECTED_CALL((*mocks), STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH);
        STRICT_EXPECTED_CALL((*mocks), STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL((*mocks), STRING_c_str(TEST_KEY_STRING_HANDLE))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL((*mocks), saslplain_get_interface());
        STRICT_EXPECTED_CALL((*mocks), saslmechanism_create(TEST_SASLPLAIN_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL((*mocks), platform_get_default_tlsio());
        STRICT_EXPECTED_CALL((*mocks), xio_create(TEST_TLSIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_TLSIO_HANDLE);
        STRICT_EXPECTED_CALL((*mocks), saslclientio_get_interface_description());
        STRICT_EXPECTED_CALL((*mocks), xio_create(TEST_SASLCLIENTIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_SASLCLIENTIO_HANDLE);
        STRICT_EXPECTED_CALL((*mocks), connection_create(TEST_SASLCLIENTIO_HANDLE, TEST_HOSTNAME, "eh_client_connection", NULL, NULL));
        STRICT_EXPECTED_CALL((*mocks), session_create(TEST_CONNECTION_HANDLE, NULL, NULL));
        STRICT_EXPECTED_CALL((*mocks), session_set_outgoing_window(TEST_SESSION_HANDLE, 10));
        STRICT_EXPECTED_CALL((*mocks), messaging_create_source("ingress"));
        STRICT_EXPECTED_CALL((*mocks), messaging_create_target("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH));
        STRICT_EXPECTED_CALL((*mocks), link_create(TEST_SESSION_HANDLE, "sender-link", role_sender, TEST_SOURCE_AMQP_VALUE, TEST_TARGET_AMQP_VALUE));
        STRICT_EXPECTED_CALL((*mocks), link_set_snd_settle_mode(TEST_LINK_HANDLE, sender_settle_mode_unsettled));
        STRICT_EXPECTED_CALL((*mocks), link_set_max_message_size(TEST_LINK_HANDLE, 256 * 1024));
        STRICT_EXPECTED_CALL((*mocks), messagesender_create(TEST_LINK_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2).IgnoreArgument(3);
        STRICT_EXPECTED_CALL((*mocks), amqpvalue_destroy(TEST_SOURCE_AMQP_VALUE));
        STRICT_EXPECTED_CALL((*mocks), amqpvalue_destroy(TEST_TARGET_AMQP_VALUE));
    }

    /*** EventHubClient_LL_CreateFromConnectionString ***/

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_016: [EventHubClient_LL_CreateFromConnectionString shall return a non-NULL handle value upon success.] */
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
    /* Tests_SRS_EVENTHUBCLIENT_LL_04_016: [EventHubClient_LL_CreateFromConnectionString shall initialize the pending list that will be used to send Events.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_valid_args_yields_a_valid_eventhub_client)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        setup_createfromconnectionstring_success(&mocks);

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);

        // assert
        ASSERT_IS_NOT_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(result);
    }

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
    /* Tests_SRS_EVENTHUBCLIENT_LL_04_016: [EventHubClient_LL_CreateFromConnectionString shall initialize the pending list that will be used to send Events.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_valid_args_second_set_of_args_yields_a_valid_eventhub_client)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connectionstringparser_parse(TEST_CONNSTR_HANDLE));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "Endpoint"))
            .SetReturn("sb://Host2");
        STRICT_EXPECTED_CALL(mocks, STRING_construct_n("Host2", 5))
            .SetReturn(TEST_HOSTNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKeyName"))
            .SetReturn("Mwahaha");
        STRICT_EXPECTED_CALL(mocks, STRING_construct("Mwahaha"))
            .SetReturn(TEST_KEYNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKey"))
            .SetReturn("Secret");
        STRICT_EXPECTED_CALL(mocks, STRING_construct("Secret"))
            .SetReturn(TEST_KEY_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, STRING_construct("amqps://"))
            .SetReturn(TEST_TARGET_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, STRING_concat_with_STRING(TEST_TARGET_STRING_HANDLE, TEST_HOSTNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_concat(TEST_TARGET_STRING_HANDLE, "/"));
        STRICT_EXPECTED_CALL(mocks, STRING_concat(TEST_TARGET_STRING_HANDLE, "AnotherOne"));
        EXPECTED_CALL(mocks, DList_InitializeListHead(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(TEST_CONNSTR_MAP_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_CONNSTR_HANDLE));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, "AnotherOne");

        // assert
        ASSERT_IS_NOT_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_003: [EventHubClient_LL_CreateFromConnectionString shall return a NULL value if connectionString or eventHubPath is NULL.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_NULL_connectionString_Fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(NULL, TEST_EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_003: [EventHubClient_LL_CreateFromConnectionString shall return a NULL value if connectionString or eventHubPath is NULL.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_NULL_path_Fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString("connectionString", NULL);

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_066: [If connection_string_parser_parse fails then EventHubClient_LL_CreateFromConnectionString shall fail and return NULL.] */
    TEST_FUNCTION(when_creating_the_string_for_the_connection_string_fails_then_EventHubClient_LL_CreateFromConnectionString_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn((STRING_HANDLE)NULL);

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.AssertActualAndExpectedCalls();

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_066: [If connection_string_parser_parse fails then EventHubClient_LL_CreateFromConnectionString shall fail and return NULL.] */
    TEST_FUNCTION(when_connection_string_parser_parse_fails_then_EventHubClient_LL_CreateFromConnectionString_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connectionstringparser_parse(TEST_CONNSTR_HANDLE))
            .SetReturn((MAP_HANDLE)NULL);
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_CONNSTR_HANDLE));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.AssertActualAndExpectedCalls();

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_004: [For all other errors, EventHubClient_LL_CreateFromConnectionString shall return NULL.] */
    TEST_FUNCTION(when_allocating_memory_for_the_event_hub_client_fails_then_EventHubClient_LL_CreateFromConnectionString_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connectionstringparser_parse(TEST_CONNSTR_HANDLE));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .SetReturn((void*)NULL);
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(TEST_CONNSTR_MAP_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_CONNSTR_HANDLE));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.AssertActualAndExpectedCalls();

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_018: [EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.] */
    TEST_FUNCTION(when_getting_the_endpoint_from_the_map_fails_then_EventHubClient_LL_CreateFromConnectionString_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connectionstringparser_parse(TEST_CONNSTR_HANDLE));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        EXPECTED_CALL(mocks, DList_InitializeListHead(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "Endpoint"))
            .SetReturn((const char*)NULL);
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(TEST_CONNSTR_MAP_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_CONNSTR_HANDLE));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.AssertActualAndExpectedCalls();

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_004: [For all other errors, EventHubClient_LL_CreateFromConnectionString shall return NULL.] */
    TEST_FUNCTION(when_constructing_the_hostname_string_fails_then_EventHubClient_LL_CreateFromConnectionString_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connectionstringparser_parse(TEST_CONNSTR_HANDLE));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        EXPECTED_CALL(mocks, DList_InitializeListHead(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "Endpoint"))
            .SetReturn(TEST_ENDPOINT);
        STRICT_EXPECTED_CALL(mocks, STRING_construct_n(TEST_HOSTNAME, strlen(TEST_HOSTNAME)))
            .SetReturn((STRING_HANDLE)NULL);
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(TEST_CONNSTR_MAP_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_CONNSTR_HANDLE));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.AssertActualAndExpectedCalls();

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_018: [EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.] */
    TEST_FUNCTION(when_getting_the_keyname_from_the_map_fails_then_EventHubClient_LL_CreateFromConnectionString_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connectionstringparser_parse(TEST_CONNSTR_HANDLE));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        EXPECTED_CALL(mocks, DList_InitializeListHead(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "Endpoint"))
            .SetReturn(TEST_ENDPOINT);
        STRICT_EXPECTED_CALL(mocks, STRING_construct_n(TEST_HOSTNAME, strlen(TEST_HOSTNAME)))
            .SetReturn(TEST_HOSTNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKeyName"))
            .SetReturn((char*)NULL);
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(TEST_CONNSTR_MAP_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_HOSTNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_CONNSTR_HANDLE));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.AssertActualAndExpectedCalls();

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_004: [For all other errors, EventHubClient_LL_CreateFromConnectionString shall return NULL.] */
    TEST_FUNCTION(when_constructing_the_keyname_string_fails_then_EventHubClient_LL_CreateFromConnectionString_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connectionstringparser_parse(TEST_CONNSTR_HANDLE));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        EXPECTED_CALL(mocks, DList_InitializeListHead(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "Endpoint"))
            .SetReturn(TEST_ENDPOINT);
        STRICT_EXPECTED_CALL(mocks, STRING_construct_n(TEST_HOSTNAME, strlen(TEST_HOSTNAME)))
            .SetReturn(TEST_HOSTNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKeyName"))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_construct(TEST_KEYNAME))
            .SetReturn((STRING_HANDLE)NULL);
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(TEST_CONNSTR_MAP_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_HOSTNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_CONNSTR_HANDLE));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.AssertActualAndExpectedCalls();

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_018: [EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.] */
    TEST_FUNCTION(when_getting_the_key_from_the_map_fails_then_EventHubClient_LL_CreateFromConnectionString_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connectionstringparser_parse(TEST_CONNSTR_HANDLE));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        EXPECTED_CALL(mocks, DList_InitializeListHead(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "Endpoint"))
            .SetReturn(TEST_ENDPOINT);
        STRICT_EXPECTED_CALL(mocks, STRING_construct_n(TEST_HOSTNAME, strlen(TEST_HOSTNAME)))
            .SetReturn(TEST_HOSTNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKeyName"))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_construct(TEST_KEYNAME))
            .SetReturn(TEST_KEYNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKey"))
            .SetReturn((char*)NULL);
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(TEST_CONNSTR_MAP_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_HOSTNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_KEYNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_CONNSTR_HANDLE));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.AssertActualAndExpectedCalls();

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_004: [For all other errors, EventHubClient_LL_CreateFromConnectionString shall return NULL.] */
    TEST_FUNCTION(when_constructing_the_key_string_fails_then_EventHubClient_LL_CreateFromConnectionString_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connectionstringparser_parse(TEST_CONNSTR_HANDLE));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        EXPECTED_CALL(mocks, DList_InitializeListHead(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "Endpoint"))
            .SetReturn(TEST_ENDPOINT);
        STRICT_EXPECTED_CALL(mocks, STRING_construct_n(TEST_HOSTNAME, strlen(TEST_HOSTNAME)))
            .SetReturn(TEST_HOSTNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKeyName"))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_construct(TEST_KEYNAME))
            .SetReturn(TEST_KEYNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKey"))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, STRING_construct(TEST_KEY))
            .SetReturn((STRING_HANDLE)NULL);
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(TEST_CONNSTR_MAP_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_HOSTNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_KEYNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_CONNSTR_HANDLE));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.AssertActualAndExpectedCalls();

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_004: [For all other errors, EventHubClient_LL_CreateFromConnectionString shall return NULL.] */
    TEST_FUNCTION(when_constructing_the_target_address_fails_then_EventHubClient_LL_CreateFromConnectionString_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connectionstringparser_parse(TEST_CONNSTR_HANDLE));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        EXPECTED_CALL(mocks, DList_InitializeListHead(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "Endpoint"))
            .SetReturn(TEST_ENDPOINT);
        STRICT_EXPECTED_CALL(mocks, STRING_construct_n(TEST_HOSTNAME, strlen(TEST_HOSTNAME)))
            .SetReturn(TEST_HOSTNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKeyName"))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_construct(TEST_KEYNAME))
            .SetReturn(TEST_KEYNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKey"))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, STRING_construct(TEST_KEY))
            .SetReturn((STRING_HANDLE)TEST_KEY_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, STRING_construct("amqps://"))
            .SetReturn((STRING_HANDLE)NULL);
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(TEST_CONNSTR_MAP_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_HOSTNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_KEYNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_KEY_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_CONNSTR_HANDLE));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.AssertActualAndExpectedCalls();

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_004: [For all other errors, EventHubClient_LL_CreateFromConnectionString shall return NULL.] */
    TEST_FUNCTION(when_concatenating_the_hostname_in_the_target_address_fails_then_EventHubClient_LL_CreateFromConnectionString_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connectionstringparser_parse(TEST_CONNSTR_HANDLE));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        EXPECTED_CALL(mocks, DList_InitializeListHead(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "Endpoint"))
            .SetReturn(TEST_ENDPOINT);
        STRICT_EXPECTED_CALL(mocks, STRING_construct_n(TEST_HOSTNAME, strlen(TEST_HOSTNAME)))
            .SetReturn(TEST_HOSTNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKeyName"))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_construct(TEST_KEYNAME))
            .SetReturn(TEST_KEYNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKey"))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, STRING_construct(TEST_KEY))
            .SetReturn((STRING_HANDLE)TEST_KEY_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, STRING_construct("amqps://"))
            .SetReturn(TEST_TARGET_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, STRING_concat_with_STRING(TEST_TARGET_STRING_HANDLE, TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(1);
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(TEST_CONNSTR_MAP_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_HOSTNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_KEYNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_KEY_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_CONNSTR_HANDLE));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.AssertActualAndExpectedCalls();

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_004: [For all other errors, EventHubClient_LL_CreateFromConnectionString shall return NULL.] */
    TEST_FUNCTION(when_concatenating_the_slash_in_the_target_address_fails_then_EventHubClient_LL_CreateFromConnectionString_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connectionstringparser_parse(TEST_CONNSTR_HANDLE));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        EXPECTED_CALL(mocks, DList_InitializeListHead(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "Endpoint"))
            .SetReturn(TEST_ENDPOINT);
        STRICT_EXPECTED_CALL(mocks, STRING_construct_n(TEST_HOSTNAME, strlen(TEST_HOSTNAME)))
            .SetReturn(TEST_HOSTNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKeyName"))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_construct(TEST_KEYNAME))
            .SetReturn(TEST_KEYNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKey"))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, STRING_construct(TEST_KEY))
            .SetReturn((STRING_HANDLE)TEST_KEY_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, STRING_construct("amqps://"))
            .SetReturn(TEST_TARGET_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, STRING_concat_with_STRING(TEST_TARGET_STRING_HANDLE, TEST_HOSTNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_concat(TEST_TARGET_STRING_HANDLE, "/"))
            .SetReturn(1);
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(TEST_CONNSTR_MAP_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_HOSTNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_KEYNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_KEY_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_CONNSTR_HANDLE));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.AssertActualAndExpectedCalls();

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_004: [For all other errors, EventHubClient_LL_CreateFromConnectionString shall return NULL.] */
    TEST_FUNCTION(when_concatenating_the_event_hub_path_in_the_target_address_fails_then_EventHubClient_LL_CreateFromConnectionString_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connectionstringparser_parse(TEST_CONNSTR_HANDLE));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "Endpoint"))
            .SetReturn(TEST_ENDPOINT);
        STRICT_EXPECTED_CALL(mocks, STRING_construct_n(TEST_HOSTNAME, strlen(TEST_HOSTNAME)))
            .SetReturn(TEST_HOSTNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKeyName"))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_construct(TEST_KEYNAME))
            .SetReturn(TEST_KEYNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKey"))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, STRING_construct(TEST_KEY))
            .SetReturn((STRING_HANDLE)TEST_KEY_STRING_HANDLE);
        EXPECTED_CALL(mocks, DList_InitializeListHead(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, STRING_construct("amqps://"))
            .SetReturn(TEST_TARGET_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, STRING_concat_with_STRING(TEST_TARGET_STRING_HANDLE, TEST_HOSTNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_concat(TEST_TARGET_STRING_HANDLE, "/"));
        STRICT_EXPECTED_CALL(mocks, STRING_concat(TEST_TARGET_STRING_HANDLE, TEST_EVENTHUB_PATH))
            .SetReturn(1);
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(TEST_CONNSTR_MAP_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_HOSTNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_KEYNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_KEY_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_CONNSTR_HANDLE));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.AssertActualAndExpectedCalls();

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_018: [EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.] */
    TEST_FUNCTION(when_the_endpoint_has_only_sb_slash_slash_EventHubClient_LL_CreateFromConnectionString_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connectionstringparser_parse(TEST_CONNSTR_HANDLE));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        EXPECTED_CALL(mocks, DList_InitializeListHead(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "Endpoint"))
            .SetReturn("sb://");
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(TEST_CONNSTR_MAP_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_CONNSTR_HANDLE));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.AssertActualAndExpectedCalls();

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_018: [EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.] */
    TEST_FUNCTION(when_the_endpoint_does_not_start_with_sb_slash_slash_EventHubClient_LL_CreateFromConnectionString_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connectionstringparser_parse(TEST_CONNSTR_HANDLE));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        EXPECTED_CALL(mocks, DList_InitializeListHead(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "Endpoint"))
            .SetReturn("sb:/5test");
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(TEST_CONNSTR_MAP_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_CONNSTR_HANDLE));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.AssertActualAndExpectedCalls();

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_018: [EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.] */
    TEST_FUNCTION(when_the_endpoint_ends_in_a_slash_the_slash_is_stripped)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connectionstringparser_parse(TEST_CONNSTR_HANDLE));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        EXPECTED_CALL(mocks, DList_InitializeListHead(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "Endpoint"))
            .SetReturn(TEST_ENDPOINT "/");
        STRICT_EXPECTED_CALL(mocks, STRING_construct_n(IGNORED_PTR_ARG, strlen(TEST_HOSTNAME)))
            .IgnoreArgument(1)
            .SetReturn(TEST_HOSTNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKeyName"))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_construct(TEST_KEYNAME))
            .SetReturn(TEST_KEYNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKey"))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, STRING_construct(TEST_KEY))
            .SetReturn(TEST_KEY_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, STRING_construct("amqps://"))
            .SetReturn(TEST_TARGET_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, STRING_concat_with_STRING(TEST_TARGET_STRING_HANDLE, TEST_HOSTNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_concat(TEST_TARGET_STRING_HANDLE, "/"));
        STRICT_EXPECTED_CALL(mocks, STRING_concat(TEST_TARGET_STRING_HANDLE, TEST_EVENTHUB_PATH));

        STRICT_EXPECTED_CALL(mocks, Map_Destroy(TEST_CONNSTR_MAP_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_CONNSTR_HANDLE));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);

        // assert
        ASSERT_IS_NOT_NULL(result);
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_018: [EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.] */
    TEST_FUNCTION(when_the_keyname_is_empty_EventHubClient_LL_CreateFromConnectionString_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connectionstringparser_parse(TEST_CONNSTR_HANDLE));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        EXPECTED_CALL(mocks, DList_InitializeListHead(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "Endpoint"))
            .SetReturn(TEST_ENDPOINT "/");
        STRICT_EXPECTED_CALL(mocks, STRING_construct_n(IGNORED_PTR_ARG, strlen(TEST_HOSTNAME)))
            .IgnoreArgument(1)
            .SetReturn(TEST_HOSTNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKeyName"))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_construct(TEST_KEYNAME))
            .SetReturn(TEST_KEYNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKey"))
            .SetReturn("");
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(TEST_CONNSTR_MAP_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_HOSTNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_KEYNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_CONNSTR_HANDLE));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_018: [EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.] */
    TEST_FUNCTION(when_the_key_is_empty_EventHubClient_LL_CreateFromConnectionString_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        STRICT_EXPECTED_CALL(mocks, EventHubClient_GetVersionString());
        STRICT_EXPECTED_CALL(mocks, STRING_construct(CONNECTION_STRING))
            .SetReturn(TEST_CONNSTR_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connectionstringparser_parse(TEST_CONNSTR_HANDLE));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        EXPECTED_CALL(mocks, DList_InitializeListHead(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "Endpoint"))
            .SetReturn(TEST_ENDPOINT "/");
        STRICT_EXPECTED_CALL(mocks, STRING_construct_n(IGNORED_PTR_ARG, strlen(TEST_HOSTNAME)))
            .IgnoreArgument(1)
            .SetReturn(TEST_HOSTNAME_STRING_HANDLE);
        STRICT_EXPECTED_CALL(mocks, Map_GetValueFromKey(TEST_CONNSTR_MAP_HANDLE, "SharedAccessKeyName"))
            .SetReturn("");
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, Map_Destroy(TEST_CONNSTR_MAP_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_HOSTNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_CONNSTR_HANDLE));

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    /* EventHubClient_LL_SendAsync */

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_011: [EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_INVALID_ARG if parameter eventHubClientLLHandle or eventDataHandle is NULL.] */
    TEST_FUNCTION(EventHubClient_LL_SendAsync_with_NULL_eventHubLLHandle_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendAsync(NULL, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_011: [EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_INVALID_ARG if parameter eventHubClientLLHandle or eventDataHandle is NULL.] */
    TEST_FUNCTION(EventHubClient_LL_SendAsync_with_NULL_eventDataHandle_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendAsync(eventHubHandle, NULL, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_012: [EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_INVALID_ARG if parameter sendAsyncConfirmationCallback is NULL and userContextCallBack is not NULL.] */
    TEST_FUNCTION(EventHubClient_LL_SendAsync_with_NULL_sendAsyncConfirmationCallbackandNonNullUSerContext_fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> mocks;

        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, NULL, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_013: [EventHubClient_LL_SendAsync shall add to the pending events list a new record cloning the information from eventDataHandle, sendAsyncConfirmationCallback and userContextCallBack.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_04_015: [Otherwise EventHubClient_LL_SendAsync shall succeed and return EVENTHUBCLIENT_OK.] */
    TEST_FUNCTION(EventHubClient_LL_SendAsync_succeeds)
    {
        ///arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE dataEventHandle = (EVENTDATA_HANDLE)1;

        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .ExpectedTimesExactly(2);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(dataEventHandle));
        EXPECTED_CALL(mocks, DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_014: [If cloning and/or adding the information fails for any reason, EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_ERROR.] */
    TEST_FUNCTION(when_allocating_memory_for_the_pending_entry_fails_then_EventHubClient_LL_SendAsync_fails)
    {
        ///arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE dataEventHandle = (EVENTDATA_HANDLE)1;

        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .SetReturn((void*)NULL);

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_014: [If cloning and/or adding the information fails for any reason, EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_ERROR.] */
    TEST_FUNCTION(when_allocating_memory_for_the_event_data_list_of_the_entry_fails_then_EventHubClient_LL_SendAsync_fails)
    {
        ///arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE dataEventHandle = (EVENTDATA_HANDLE)1;

        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .SetReturn((void*)NULL);
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_014: [If cloning and/or adding the information fails for any reason, EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_ERROR.] */
    TEST_FUNCTION(when_cloning_the_payload_fails_then_EventHubClient_LL_SendAsync_fails)
    {
        ///arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE dataEventHandle = (EVENTDATA_HANDLE)1;

        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .ExpectedTimesExactly(2);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(dataEventHandle))
            .SetReturn((EVENTDATA_HANDLE)NULL);
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
            .ExpectedTimesExactly(2);

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* EventHubClient_LL_Destroy */

    /* Tests_SRS_EVENTHUBCLIENT_03_010: [If the eventHubHandle is NULL, EventHubClient_LL_Destroy shall not do anything.] */
    TEST_FUNCTION(EventHubClient_LL_Destroy_with_NULL_eventHubHandle_Does_Nothing)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        // act
        EventHubClient_LL_Destroy(NULL);

        // assert
        // Implicit
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_03_009: [EventHubClient_LL_Destroy shall terminate the usage of this EventHubClient_LL specified by the eventHubLLHandle and cleanup all associated resources.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_081: [The key host name, key name and key allocated in EventHubClient_LL_CreateFromConnectionString shall be freed.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_042: [The message sender shall be freed by calling messagesender_destroy.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_043: [The link shall be freed by calling link_destroy.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_044: [The session shall be freed by calling session_destroy.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_045: [The connection shall be freed by calling connection_destroy.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_046: [The SASL client IO shall be freed by calling xio_destroy.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_047: [The TLS IO shall be freed by calling xio_destroy.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_048: [The SASL plain mechanism shall be freed by calling saslmechanism_destroy.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_041: [All pending message data shall be freed.] */
    TEST_FUNCTION(EventHubClient_LL_Destroy_with_valid_handle_frees_all_the_resources)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, messagesender_destroy(TEST_MESSAGE_SENDER_HANDLE));
        STRICT_EXPECTED_CALL(mocks, link_destroy(TEST_LINK_HANDLE));
        STRICT_EXPECTED_CALL(mocks, session_destroy(TEST_SESSION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, connection_destroy(TEST_CONNECTION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_SASLCLIENTIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_TLSIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, saslmechanism_destroy(TEST_SASL_MECHANISM_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_TARGET_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_KEYNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_KEY_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_HOSTNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, DList_RemoveHeadList(saved_pending_list));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        // act
        EventHubClient_LL_Destroy(eventHubHandle);

        // assert
        // Implicit
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_081: [The key host name, key name and key allocated in EventHubClient_LL_CreateFromConnectionString shall be freed.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_041: [All pending message data shall be freed.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_040: [All the pending messages shall be indicated as error by calling the associated callback with EVENTHUBCLIENT_CONFIRMATION_DESTROY.] */
    TEST_FUNCTION(EventHubClient_LL_Destroy_frees_2_pending_messages)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE dataEventHandle = (EVENTDATA_HANDLE)1;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        EXPECTED_CALL(mocks, EventData_Clone(IGNORED_PTR_ARG))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, sendAsyncConfirmationCallback, (void*)0x4242);
        EXPECTED_CALL(mocks, EventData_Clone(IGNORED_PTR_ARG))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, sendAsyncConfirmationCallback, (void*)0x4243);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_TARGET_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_KEYNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_KEY_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_HOSTNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, DList_RemoveHeadList(saved_pending_list));

        /* 1st item */
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_DESTROY, (void*)0x4242));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, DList_RemoveHeadList(saved_pending_list));
        /* 2nd item */
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_DESTROY, (void*)0x4243));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_2));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, DList_RemoveHeadList(saved_pending_list));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        // act
        EventHubClient_LL_Destroy(eventHubHandle);

        // assert
        // Implicit
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_060: [When on_messagesender_state_changed is called with MESSAGE_SENDER_STATE_ERROR, the uAMQP stack shall be brough down so that it can be created again if needed in dowork:] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_03_009: [EventHubClient_LL_Destroy shall terminate the usage of this EventHubClient_LL specified by the eventHubLLHandle and cleanup all associated resources.] */
    TEST_FUNCTION(when_destroy_is_Called_after_the_stack_has_been_brought_down_then_it_is_not_brought_down_again)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_ERROR, MESSAGE_SENDER_STATE_OPEN);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, messagesender_destroy(TEST_MESSAGE_SENDER_HANDLE));
        STRICT_EXPECTED_CALL(mocks, link_destroy(TEST_LINK_HANDLE));
        STRICT_EXPECTED_CALL(mocks, session_destroy(TEST_SESSION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, connection_destroy(TEST_CONNECTION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(DUMMY_IO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(DUMMY_IO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, saslmechanism_destroy(TEST_SASL_MECHANISM_HANDLE));

        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_TARGET_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_KEYNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_KEY_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, STRING_delete(TEST_HOSTNAME_STRING_HANDLE));
        STRICT_EXPECTED_CALL(mocks, DList_RemoveHeadList(saved_pending_list));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));


        // act
        EventHubClient_LL_Destroy(eventHubHandle);

        // assert
        // Implicit
    }

    /* EventHubClient_LL_DoWork */

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_018: [if parameter eventHubClientLLHandle is NULL EventHubClient_LL_DoWork shall immediately return.]  */
    TEST_FUNCTION(EventHubClient_LL_DoWork_with_NullHandle_Do_Not_Work)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        // act
        EventHubClient_LL_DoWork(NULL);

        // assert
        mocks.AssertActualAndExpectedCalls();
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_038: [EventHubClient_LL_DoWork shall perform a messagesender_open if the state of the message_sender is not OPEN.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_064: [EventHubClient_LL_DoWork shall call connection_dowork while passing as argument the connection handle obtained in EventHubClient_LL_Create.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_079: [EventHubClient_LL_DoWork shall bring up the uAMQP stack if it has not already brought up:] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_03_030: [A TLS IO shall be created by calling xio_create.] */
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
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_079: [EventHubClient_LL_DoWork shall bring up the uAMQP stack if it has not already brought up:] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_when_message_sender_is_not_open_opens_the_message_sender)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        setup_messenger_initialize_success(&mocks);
        STRICT_EXPECTED_CALL(mocks, messagesender_open(TEST_MESSAGE_SENDER_HANDLE));
        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, connection_set_trace(TEST_CONNECTION_HANDLE, false));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        ASSERT_ARE_EQUAL(char_ptr, TEST_KEYNAME, saved_sasl_mechanism_create_parameters->authcid);
        ASSERT_ARE_EQUAL(char_ptr, TEST_KEY, saved_sasl_mechanism_create_parameters->passwd);
        ASSERT_ARE_EQUAL(char_ptr, TEST_HOSTNAME, saved_tlsio_parameters->hostname);
        ASSERT_ARE_EQUAL(int, 5671, saved_tlsio_parameters->port);
        ASSERT_ARE_EQUAL(void_ptr, TEST_SASL_MECHANISM_HANDLE, saved_saslclientio_parameters->sasl_mechanism);
        ASSERT_ARE_EQUAL(void_ptr, TEST_TLSIO_HANDLE, saved_saslclientio_parameters->underlying_io);
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_038: [EventHubClient_LL_DoWork shall perform a messagesender_open if the state of the message_sender is not OPEN.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_064: [EventHubClient_LL_DoWork shall call connection_dowork while passing as argument the connection handle obtained in EventHubClient_LL_Create.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_079: [EventHubClient_LL_DoWork shall bring up the uAMQP stack if it has not already brought up:] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_03_030: [A TLS IO shall be created by calling xio_create.] */
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
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_079: [EventHubClient_LL_DoWork shall bring up the uAMQP stack if it has not already brought up:] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_when_message_sender_is_not_open_opens_the_message_sender_different_args)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn("Host2");
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" "Host2" "/" "AnotherOne");
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn("Mwahaha");
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEY_STRING_HANDLE))
            .SetReturn("Secret");
        STRICT_EXPECTED_CALL(mocks, saslplain_get_interface());
        STRICT_EXPECTED_CALL(mocks, saslmechanism_create(TEST_SASLPLAIN_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_TLSIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_TLSIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, saslclientio_get_interface_description());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_SASLCLIENTIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_SASLCLIENTIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connection_create(TEST_SASLCLIENTIO_HANDLE, "Host2", "eh_client_connection", NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, session_create(TEST_CONNECTION_HANDLE, NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, session_set_outgoing_window(TEST_SESSION_HANDLE, 10));
        STRICT_EXPECTED_CALL(mocks, messaging_create_source("ingress"));
        STRICT_EXPECTED_CALL(mocks, messaging_create_target("amqps://" "Host2" "/" "AnotherOne"));
        STRICT_EXPECTED_CALL(mocks, link_create(TEST_SESSION_HANDLE, "sender-link", role_sender, TEST_SOURCE_AMQP_VALUE, TEST_TARGET_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, link_set_snd_settle_mode(TEST_LINK_HANDLE, sender_settle_mode_unsettled));
        STRICT_EXPECTED_CALL(mocks, link_set_max_message_size(TEST_LINK_HANDLE, 256 * 1024));
        STRICT_EXPECTED_CALL(mocks, messagesender_create(TEST_LINK_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2).IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_SOURCE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_TARGET_AMQP_VALUE));

        STRICT_EXPECTED_CALL(mocks, messagesender_open(TEST_MESSAGE_SENDER_HANDLE));
        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, connection_set_trace(TEST_CONNECTION_HANDLE, false));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        ASSERT_ARE_EQUAL(char_ptr, "Mwahaha", saved_sasl_mechanism_create_parameters->authcid);
        ASSERT_ARE_EQUAL(char_ptr, "Secret", saved_sasl_mechanism_create_parameters->passwd);
        ASSERT_ARE_EQUAL(char_ptr, "Host2", saved_tlsio_parameters->hostname);
        ASSERT_ARE_EQUAL(int, 5671, saved_tlsio_parameters->port);
        ASSERT_ARE_EQUAL(void_ptr, TEST_SASL_MECHANISM_HANDLE, saved_saslclientio_parameters->sasl_mechanism);
        ASSERT_ARE_EQUAL(void_ptr, TEST_TLSIO_HANDLE, saved_saslclientio_parameters->underlying_io);
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_080: [If any other error happens while bringing up the uAMQP stack, EventHubClient_LL_DoWork shall not attempt to open the message_sender and return without sending any messages.] */
    TEST_FUNCTION(when_getting_the_hostname_raw_string_fails_then_EventHubClient_LL_DoWork_does_not_proceed)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn((char*)NULL);

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_080: [If any other error happens while bringing up the uAMQP stack, EventHubClient_LL_DoWork shall not attempt to open the message_sender and return without sending any messages.] */
    TEST_FUNCTION(when_getting_the_target_address_string_content_fails_then_EventHubClient_LL_DoWork_does_not_proceed)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(TEST_HOSTNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn((char*)NULL);

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_080: [If any other error happens while bringing up the uAMQP stack, EventHubClient_LL_DoWork shall not attempt to open the message_sender and return without sending any messages.] */
    TEST_FUNCTION(when_getting_the_string_content_for_the_keyname_fails_then_EventHubClient_LL_DoWork_does_not_proceed)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(TEST_HOSTNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn((char*)NULL);

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_080: [If any other error happens while bringing up the uAMQP stack, EventHubClient_LL_DoWork shall not attempt to open the message_sender and return without sending any messages.] */
    TEST_FUNCTION(when_getting_the_string_content_for_the_key_fails_then_EventHubClient_LL_DoWork_does_not_proceed)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(TEST_HOSTNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEY_STRING_HANDLE))
            .SetReturn((char*)NULL);

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_006: [If saslplain_get_interface fails then EventHubClient_LL_DoWork shall not proceed with sending any messages.] */
    TEST_FUNCTION(when_getting_the_interface_for_SASL_plain_fails_then_EventHubClient_LL_DoWork_does_not_proceed)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(TEST_HOSTNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEY_STRING_HANDLE))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, saslplain_get_interface())
            .SetReturn((const SASL_MECHANISM_INTERFACE_DESCRIPTION*)NULL);

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_011: [If sasl_mechanism_create fails then EventHubClient_LL_DoWork shall not proceed with sending any messages.] */
    TEST_FUNCTION(when_creating_the_SASL_mechanism_fails_then_EventHubClient_LL_DoWork_does_not_proceed)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(TEST_HOSTNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEY_STRING_HANDLE))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, saslplain_get_interface());
        STRICT_EXPECTED_CALL(mocks, saslmechanism_create(TEST_SASLPLAIN_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn((SASL_MECHANISM_HANDLE)NULL);

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_001: [If platform_get_default_tlsio_interface fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages. ] */
    TEST_FUNCTION(when_getting_the_default_TLS_IO_fails_then_EventHubClient_LL_DoWork_does_not_proceed)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(TEST_HOSTNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEY_STRING_HANDLE))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, saslplain_get_interface());
        STRICT_EXPECTED_CALL(mocks, saslmechanism_create(TEST_SASLPLAIN_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, platform_get_default_tlsio())
            .SetReturn((const IO_INTERFACE_DESCRIPTION*)NULL);
        STRICT_EXPECTED_CALL(mocks, saslmechanism_destroy(TEST_SASL_MECHANISM_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_003: [If xio_create fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.] */
    TEST_FUNCTION(when_creating_the_TLS_IO_fails_then_EventHubClient_LL_DoWork_does_not_proceed)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(TEST_HOSTNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEY_STRING_HANDLE))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, saslplain_get_interface());
        STRICT_EXPECTED_CALL(mocks, saslmechanism_create(TEST_SASLPLAIN_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_TLSIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn((XIO_HANDLE)NULL);
        STRICT_EXPECTED_CALL(mocks, saslmechanism_destroy(TEST_SASL_MECHANISM_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_014: [If saslclientio_get_interface_description fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.] */
    TEST_FUNCTION(when_getting_the_SASL_client_IO_interface_description_fails_then_EventHubClient_LL_DoWork_does_not_proceed)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(TEST_HOSTNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEY_STRING_HANDLE))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, saslplain_get_interface());
        STRICT_EXPECTED_CALL(mocks, saslmechanism_create(TEST_SASLPLAIN_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_TLSIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_TLSIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, saslclientio_get_interface_description())
            .SetReturn((const IO_INTERFACE_DESCRIPTION*)NULL);
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_TLSIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, saslmechanism_destroy(TEST_SASL_MECHANISM_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_018: [If xio_create fails creating the SASL client IO then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.] */
    TEST_FUNCTION(when_creating_the_saslclientio_fails_then_EventHubClient_LL_DoWork_does_not_proceed)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(TEST_HOSTNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEY_STRING_HANDLE))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, saslplain_get_interface());
        STRICT_EXPECTED_CALL(mocks, saslmechanism_create(TEST_SASLPLAIN_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_TLSIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_TLSIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, saslclientio_get_interface_description());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_SASLCLIENTIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn((XIO_HANDLE)NULL);
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_TLSIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, saslmechanism_destroy(TEST_SASL_MECHANISM_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_020: [If connection_create fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.] */
    TEST_FUNCTION(when_creating_the_connection_fails_then_EventHubClient_LL_DoWork_does_not_proceed)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(TEST_HOSTNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEY_STRING_HANDLE))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, saslplain_get_interface());
        STRICT_EXPECTED_CALL(mocks, saslmechanism_create(TEST_SASLPLAIN_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_TLSIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_TLSIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, saslclientio_get_interface_description());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_SASLCLIENTIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_SASLCLIENTIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connection_create(TEST_SASLCLIENTIO_HANDLE, TEST_HOSTNAME, "eh_client_connection", NULL, NULL))
            .SetReturn((CONNECTION_HANDLE)NULL);
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_SASLCLIENTIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_TLSIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, saslmechanism_destroy(TEST_SASL_MECHANISM_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_029: [If session_create fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.] */
    TEST_FUNCTION(when_creating_the_sesion_fails_then_EventHubClient_LL_DoWork_does_not_proceed)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(TEST_HOSTNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEY_STRING_HANDLE))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, saslplain_get_interface());
        STRICT_EXPECTED_CALL(mocks, saslmechanism_create(TEST_SASLPLAIN_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_TLSIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_TLSIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, saslclientio_get_interface_description());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_SASLCLIENTIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_SASLCLIENTIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connection_create(TEST_SASLCLIENTIO_HANDLE, TEST_HOSTNAME, "eh_client_connection", NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, session_create(TEST_CONNECTION_HANDLE, NULL, NULL))
            .SetReturn((SESSION_HANDLE)NULL);
        STRICT_EXPECTED_CALL(mocks, connection_destroy(TEST_CONNECTION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_SASLCLIENTIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_TLSIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, saslmechanism_destroy(TEST_SASL_MECHANISM_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_031: [If setting the outgoing window fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.] */
    TEST_FUNCTION(when_setting_the_outgoing_window_fails_then_EventHubClient_LL_DoWork_does_not_proceed)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(TEST_HOSTNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEY_STRING_HANDLE))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, saslplain_get_interface());
        STRICT_EXPECTED_CALL(mocks, saslmechanism_create(TEST_SASLPLAIN_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_TLSIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_TLSIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, saslclientio_get_interface_description());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_SASLCLIENTIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_SASLCLIENTIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connection_create(TEST_SASLCLIENTIO_HANDLE, TEST_HOSTNAME, "eh_client_connection", NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, session_create(TEST_CONNECTION_HANDLE, NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, session_set_outgoing_window(TEST_SESSION_HANDLE, 10))
            .SetReturn(1);
        STRICT_EXPECTED_CALL(mocks, session_destroy(TEST_SESSION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, connection_destroy(TEST_CONNECTION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_SASLCLIENTIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_TLSIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, saslmechanism_destroy(TEST_SASL_MECHANISM_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_025: [If creating the source or target values fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.] */
    TEST_FUNCTION(when_creating_the_source_for_the_link_fails_then_EventHubClient_LL_DoWork_does_not_proceed)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(TEST_HOSTNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEY_STRING_HANDLE))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, saslplain_get_interface());
        STRICT_EXPECTED_CALL(mocks, saslmechanism_create(TEST_SASLPLAIN_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_TLSIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_TLSIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, saslclientio_get_interface_description());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_SASLCLIENTIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_SASLCLIENTIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connection_create(TEST_SASLCLIENTIO_HANDLE, TEST_HOSTNAME, "eh_client_connection", NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, session_create(TEST_CONNECTION_HANDLE, NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, session_set_outgoing_window(TEST_SESSION_HANDLE, 10));
        STRICT_EXPECTED_CALL(mocks, messaging_create_source("ingress"))
            .SetReturn((AMQP_VALUE)NULL);
        STRICT_EXPECTED_CALL(mocks, session_destroy(TEST_SESSION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, connection_destroy(TEST_CONNECTION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_SASLCLIENTIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_TLSIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, saslmechanism_destroy(TEST_SASL_MECHANISM_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_025: [If creating the source or target values fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.] */
    TEST_FUNCTION(when_creating_the_target_for_the_link_fails_then_EventHubClient_LL_DoWork_does_not_proceed)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(TEST_HOSTNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEY_STRING_HANDLE))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, saslplain_get_interface());
        STRICT_EXPECTED_CALL(mocks, saslmechanism_create(TEST_SASLPLAIN_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_TLSIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_TLSIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, saslclientio_get_interface_description());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_SASLCLIENTIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_SASLCLIENTIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connection_create(TEST_SASLCLIENTIO_HANDLE, TEST_HOSTNAME, "eh_client_connection", NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, session_create(TEST_CONNECTION_HANDLE, NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, session_set_outgoing_window(TEST_SESSION_HANDLE, 10));
        STRICT_EXPECTED_CALL(mocks, messaging_create_source("ingress"))
            .SetReturn(TEST_SOURCE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, messaging_create_target("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH))
            .SetReturn((AMQP_VALUE)NULL);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_SOURCE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, session_destroy(TEST_SESSION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, connection_destroy(TEST_CONNECTION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_SASLCLIENTIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_TLSIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, saslmechanism_destroy(TEST_SASL_MECHANISM_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_027: [If creating the link fails then EventHubClient_LL_DoWork shall shall not proceed with sending any messages.] */
    TEST_FUNCTION(when_creating_the_link_fails_then_EventHubClient_LL_DoWork_does_not_proceed)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(TEST_HOSTNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEY_STRING_HANDLE))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, saslplain_get_interface());
        STRICT_EXPECTED_CALL(mocks, saslmechanism_create(TEST_SASLPLAIN_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_TLSIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_TLSIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, saslclientio_get_interface_description());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_SASLCLIENTIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_SASLCLIENTIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connection_create(TEST_SASLCLIENTIO_HANDLE, TEST_HOSTNAME, "eh_client_connection", NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, session_create(TEST_CONNECTION_HANDLE, NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, session_set_outgoing_window(TEST_SESSION_HANDLE, 10));
        STRICT_EXPECTED_CALL(mocks, messaging_create_source("ingress"))
            .SetReturn(TEST_SOURCE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, messaging_create_target("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH))
            .SetReturn(TEST_TARGET_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, link_create(TEST_SESSION_HANDLE, "sender-link", role_sender, TEST_SOURCE_AMQP_VALUE, TEST_TARGET_AMQP_VALUE))
            .SetReturn((LINK_HANDLE)NULL);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_SOURCE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_TARGET_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, session_destroy(TEST_SESSION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, connection_destroy(TEST_CONNECTION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_SASLCLIENTIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_TLSIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, saslmechanism_destroy(TEST_SASL_MECHANISM_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_033: [If link_set_snd_settle_mode fails then EventHubClient_LL_DoWork shall not proceed with sending any messages.] */
    TEST_FUNCTION(when_link_set_snd_settle_mode_fails_then_EventHubClient_LL_DoWork_does_not_proceed)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(TEST_HOSTNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEY_STRING_HANDLE))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, saslplain_get_interface());
        STRICT_EXPECTED_CALL(mocks, saslmechanism_create(TEST_SASLPLAIN_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_TLSIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_TLSIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, saslclientio_get_interface_description());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_SASLCLIENTIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_SASLCLIENTIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connection_create(TEST_SASLCLIENTIO_HANDLE, TEST_HOSTNAME, "eh_client_connection", NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, session_create(TEST_CONNECTION_HANDLE, NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, session_set_outgoing_window(TEST_SESSION_HANDLE, 10));
        STRICT_EXPECTED_CALL(mocks, messaging_create_source("ingress"))
            .SetReturn(TEST_SOURCE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, messaging_create_target("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH))
            .SetReturn(TEST_TARGET_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, link_create(TEST_SESSION_HANDLE, "sender-link", role_sender, TEST_SOURCE_AMQP_VALUE, TEST_TARGET_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, link_set_snd_settle_mode(TEST_LINK_HANDLE, sender_settle_mode_unsettled))
            .SetReturn(1);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_SOURCE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_TARGET_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, link_destroy(TEST_LINK_HANDLE));
        STRICT_EXPECTED_CALL(mocks, session_destroy(TEST_SESSION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, connection_destroy(TEST_CONNECTION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_SASLCLIENTIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_TLSIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, saslmechanism_destroy(TEST_SASL_MECHANISM_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_035: [If link_set_max_message_size fails then EventHubClient_LL_DoWork shall not proceed with sending any messages.] */
    TEST_FUNCTION(when_link_set_max_message_size_fails_then_EventHubClient_LL_DoWork_does_not_proceed)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(TEST_HOSTNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEY_STRING_HANDLE))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, saslplain_get_interface());
        STRICT_EXPECTED_CALL(mocks, saslmechanism_create(TEST_SASLPLAIN_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_TLSIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_TLSIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, saslclientio_get_interface_description());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_SASLCLIENTIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_SASLCLIENTIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connection_create(TEST_SASLCLIENTIO_HANDLE, TEST_HOSTNAME, "eh_client_connection", NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, session_create(TEST_CONNECTION_HANDLE, NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, session_set_outgoing_window(TEST_SESSION_HANDLE, 10));
        STRICT_EXPECTED_CALL(mocks, messaging_create_source("ingress"))
            .SetReturn(TEST_SOURCE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, messaging_create_target("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH))
            .SetReturn(TEST_TARGET_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, link_create(TEST_SESSION_HANDLE, "sender-link", role_sender, TEST_SOURCE_AMQP_VALUE, TEST_TARGET_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, link_set_snd_settle_mode(TEST_LINK_HANDLE, sender_settle_mode_unsettled));
        STRICT_EXPECTED_CALL(mocks, link_set_max_message_size(TEST_LINK_HANDLE, 256 * 1024))
            .SetReturn(1);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_SOURCE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_TARGET_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, link_destroy(TEST_LINK_HANDLE));
        STRICT_EXPECTED_CALL(mocks, session_destroy(TEST_SESSION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, connection_destroy(TEST_CONNECTION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_SASLCLIENTIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_TLSIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, saslmechanism_destroy(TEST_SASL_MECHANISM_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_037: [If creating the message sender fails then EventHubClient_LL_DoWork shall not proceed with sending any messages.] */
    TEST_FUNCTION(when_creating_the_messagesender_fails_then_EventHubClient_LL_DoWork_does_not_proceed)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_HOSTNAME_STRING_HANDLE))
            .SetReturn(TEST_HOSTNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_TARGET_STRING_HANDLE))
            .SetReturn("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEYNAME_STRING_HANDLE))
            .SetReturn(TEST_KEYNAME);
        STRICT_EXPECTED_CALL(mocks, STRING_c_str(TEST_KEY_STRING_HANDLE))
            .SetReturn(TEST_KEY);
        STRICT_EXPECTED_CALL(mocks, saslplain_get_interface());
        STRICT_EXPECTED_CALL(mocks, saslmechanism_create(TEST_SASLPLAIN_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(mocks, platform_get_default_tlsio());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_TLSIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_TLSIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, saslclientio_get_interface_description());
        STRICT_EXPECTED_CALL(mocks, xio_create(TEST_SASLCLIENTIO_INTERFACE_DESCRIPTION, NULL))
            .IgnoreArgument(2)
            .SetReturn(TEST_SASLCLIENTIO_HANDLE);
        STRICT_EXPECTED_CALL(mocks, connection_create(TEST_SASLCLIENTIO_HANDLE, TEST_HOSTNAME, "eh_client_connection", NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, session_create(TEST_CONNECTION_HANDLE, NULL, NULL));
        STRICT_EXPECTED_CALL(mocks, session_set_outgoing_window(TEST_SESSION_HANDLE, 10));
        STRICT_EXPECTED_CALL(mocks, messaging_create_source("ingress"))
            .SetReturn(TEST_SOURCE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, messaging_create_target("amqps://" TEST_HOSTNAME "/" TEST_EVENTHUB_PATH))
            .SetReturn(TEST_TARGET_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, link_create(TEST_SESSION_HANDLE, "sender-link", role_sender, TEST_SOURCE_AMQP_VALUE, TEST_TARGET_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, link_set_snd_settle_mode(TEST_LINK_HANDLE, sender_settle_mode_unsettled));
        STRICT_EXPECTED_CALL(mocks, link_set_max_message_size(TEST_LINK_HANDLE, 256 * 1024));
        STRICT_EXPECTED_CALL(mocks, messagesender_create(TEST_LINK_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2).IgnoreArgument(3)
            .SetReturn((MESSAGE_SENDER_HANDLE)NULL);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_SOURCE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_TARGET_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, link_destroy(TEST_LINK_HANDLE));
        STRICT_EXPECTED_CALL(mocks, session_destroy(TEST_SESSION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, connection_destroy(TEST_CONNECTION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_SASLCLIENTIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, xio_destroy(TEST_TLSIO_HANDLE));
        STRICT_EXPECTED_CALL(mocks, saslmechanism_destroy(TEST_SASL_MECHANISM_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_038: [EventHubClient_LL_DoWork shall perform a messagesender_open if the state of the message_sender is not OPEN.] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_when_message_sender_is_already_opening_no_messagesender_open_is_issued_again)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPENING, MESSAGE_SENDER_STATE_IDLE);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_039: [If messagesender_open fails, no further actions shall be carried out.] */
    TEST_FUNCTION(when_messagesender_open_fails_then_EventHubClient_LL_DoWork_does_not_do_anything_else)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        setup_messenger_initialize_success(&mocks);

        STRICT_EXPECTED_CALL(mocks, messagesender_open(TEST_MESSAGE_SENDER_HANDLE))
            .SetReturn(1);
        STRICT_EXPECTED_CALL(mocks, connection_set_trace(IGNORED_PTR_ARG, false))
            .IgnoreArgument(1);

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_049: [If the message has not yet been given to uAMQP then a new message shall be created by calling message_create.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_050: [If the number of event data entries for the message is 1 (not batched) then the message body shall be set to the event data payload by calling message_add_body_amqp_data.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_051: [The pointer to the payload and its length shall be obtained by calling EventData_GetData.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_069: [The AMQP message shall be given to uAMQP by calling messagesender_send, while passing as arguments the message sender handle, the message handle, a callback function and its context.] */
    TEST_FUNCTION(messages_that_are_pending_are_given_to_uAMQP_by_EventHubClient_LL_DoWork)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        BINARY_DATA binary_data = { test_data, length };

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &no_property_keys_ptr, sizeof(no_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &no_property_values_ptr, sizeof(no_property_values_ptr))
            .CopyOutArgumentBuffer(4, &no_property_size, sizeof(no_property_size));
        STRICT_EXPECTED_CALL(mocks, messagesender_send(TEST_MESSAGE_SENDER_HANDLE, TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3).IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_049: [If the message has not yet been given to uAMQP then a new message shall be created by calling message_create.] */
    TEST_FUNCTION(two_messages_that_are_pending_are_given_to_uAMQP_by_EventHubClient_LL_DoWork)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EXPECTED_CALL(mocks, EventData_Clone(IGNORED_PTR_ARG))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4242);
        EXPECTED_CALL(mocks, EventData_Clone(IGNORED_PTR_ARG))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4243);
        mocks.ResetAllCalls();

        unsigned char test_data_1[] = { 0x42 };
        unsigned char test_data_2[] = { 0x43, 0x44 };
        unsigned char* buffer = test_data_1;
        size_t length = sizeof(test_data_1);
        BINARY_DATA binary_data = { test_data_1, length };

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &no_property_keys_ptr, sizeof(no_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &no_property_values_ptr, sizeof(no_property_values_ptr))
            .CopyOutArgumentBuffer(4, &no_property_size, sizeof(no_property_size));
        STRICT_EXPECTED_CALL(mocks, messagesender_send(TEST_MESSAGE_SENDER_HANDLE, TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3).IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));

        buffer = test_data_2;
        length = sizeof(test_data_2);
        binary_data = { test_data_2, length };

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_2, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_2));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &no_property_keys_ptr, sizeof(no_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &no_property_values_ptr, sizeof(no_property_values_ptr))
            .CopyOutArgumentBuffer(4, &no_property_size, sizeof(no_property_size));
        STRICT_EXPECTED_CALL(mocks, messagesender_send(TEST_MESSAGE_SENDER_HANDLE, TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3).IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_052: [If EventData_GetData fails then the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_EventData_GetData_fails_then_the_message_is_indicated_as_ERROR_and_removed_from_pending_list)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        BINARY_DATA binary_data = { test_data, length };

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length))
            .SetReturn(EVENTDATA_ERROR);
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_071: [If message_add_body_amqp_data fails then the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_message_add_body_amqp_data_send_fails_then_the_message_is_indicated_as_ERROR_and_removed_from_pending_list)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        BINARY_DATA binary_data = { test_data, length };

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data))
            .SetReturn(1);
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_053: [If messagesender_send failed then the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_messagesender_send_fails_then_the_message_is_indicated_as_ERROR_and_removed_from_pending_list)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        BINARY_DATA binary_data = { test_data, length };

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &no_property_keys_ptr, sizeof(no_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &no_property_values_ptr, sizeof(no_property_values_ptr))
            .CopyOutArgumentBuffer(4, &no_property_size, sizeof(no_property_size));
        STRICT_EXPECTED_CALL(mocks, messagesender_send(TEST_MESSAGE_SENDER_HANDLE, TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3).IgnoreArgument(4)
            .SetReturn(1);
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_070: [If creating the message fails, then the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_message_create_fails_then_the_message_is_indicated_as_ERROR_and_removed_from_pending_list)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        BINARY_DATA binary_data = { test_data, length };

        STRICT_EXPECTED_CALL(mocks, message_create())
            .SetReturn((MESSAGE_HANDLE)NULL);
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_070: [If creating the message fails, then the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_the_first_message_fails_the_second_is_given_to_uAMQP)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EXPECTED_CALL(mocks, EventData_Clone(IGNORED_PTR_ARG))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4242);
        EXPECTED_CALL(mocks, EventData_Clone(IGNORED_PTR_ARG))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4243);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        BINARY_DATA binary_data = { test_data, length };

        STRICT_EXPECTED_CALL(mocks, message_create())
            .SetReturn((MESSAGE_HANDLE)NULL);
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_2, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_2));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &no_property_keys_ptr, sizeof(no_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &no_property_values_ptr, sizeof(no_property_values_ptr))
            .CopyOutArgumentBuffer(4, &no_property_size, sizeof(no_property_size));
        STRICT_EXPECTED_CALL(mocks, messagesender_send(TEST_MESSAGE_SENDER_HANDLE, TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3).IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_054: [If the number of event data entries for the message is 1 (not batched) the event data properties shall be added as application properties to the message.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_055: [A map shall be created to hold the application properties by calling amqpvalue_create_map.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_056: [For each property a key and value AMQP value shall be created by calling amqpvalue_create_string.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_057: [Then each property shall be added to the application properties map by calling amqpvalue_set_map_value.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_058: [The resulting map shall be set as the message application properties by calling message_set_application_properties.] */
    TEST_FUNCTION(one_property_is_added_to_a_non_batched_message)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        BINARY_DATA binary_data = { test_data, length };

        const char* const one_property_keys[] = { "test_property_key" };
        const char* const one_property_values[] = { "test_property_value" };
        const char* const* one_property_keys_ptr = one_property_keys;
        const char* const* one_property_values_ptr = one_property_values;
        size_t one_property_size = 1;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &one_property_keys_ptr, sizeof(one_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &one_property_values_ptr, sizeof(one_property_values_ptr))
            .CopyOutArgumentBuffer(4, &one_property_size, sizeof(one_property_size));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_map())
            .SetReturn(TEST_UAMQP_MAP);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_key"))
            .SetReturn(TEST_PROPERTY_1_KEY_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_value"))
            .SetReturn(TEST_PROPERTY_1_VALUE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_set_map_value(TEST_UAMQP_MAP, TEST_PROPERTY_1_KEY_AMQP_VALUE, TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_KEY_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, message_set_application_properties(TEST_MESSAGE_HANDLE, TEST_UAMQP_MAP));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_UAMQP_MAP));
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data));
        STRICT_EXPECTED_CALL(mocks, messagesender_send(TEST_MESSAGE_SENDER_HANDLE, TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3).IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_054: [If the number of event data entries for the message is 1 (not batched) the event data properties shall be added as application properties to the message.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_055: [A map shall be created to hold the application properties by calling amqpvalue_create_map.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_056: [For each property a key and value AMQP value shall be created by calling amqpvalue_create_string.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_057: [Then each property shall be added to the application properties map by calling amqpvalue_set_map_value.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_058: [The resulting map shall be set as the message application properties by calling message_set_application_properties.] */
    TEST_FUNCTION(two_properties_are_added_to_a_non_batched_message)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        BINARY_DATA binary_data = { test_data, length };

        const char* const two_property_keys[] = { "test_property_key", "prop_key_2" };
        const char* const two_property_values[] = { "test_property_value", "prop_value_2" };
        const char* const* two_property_keys_ptr = two_property_keys;
        const char* const* two_property_values_ptr = two_property_values;
        size_t two_properties_size = 2;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &two_property_keys_ptr, sizeof(two_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &two_property_values_ptr, sizeof(two_property_values_ptr))
            .CopyOutArgumentBuffer(4, &two_properties_size, sizeof(two_properties_size));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_map())
            .SetReturn(TEST_UAMQP_MAP);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_key"))
            .SetReturn(TEST_PROPERTY_1_KEY_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_value"))
            .SetReturn(TEST_PROPERTY_1_VALUE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_set_map_value(TEST_UAMQP_MAP, TEST_PROPERTY_1_KEY_AMQP_VALUE, TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("prop_key_2"))
            .SetReturn(TEST_PROPERTY_2_KEY_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("prop_value_2"))
            .SetReturn(TEST_PROPERTY_2_VALUE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_set_map_value(TEST_UAMQP_MAP, TEST_PROPERTY_2_KEY_AMQP_VALUE, TEST_PROPERTY_2_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_KEY_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_2_KEY_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_2_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, message_set_application_properties(TEST_MESSAGE_HANDLE, TEST_UAMQP_MAP));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_UAMQP_MAP));
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data));
        STRICT_EXPECTED_CALL(mocks, messagesender_send(TEST_MESSAGE_SENDER_HANDLE, TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3).IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_059: [If any error is encountered while creating the application properties the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_getting_the_properties_from_the_event_data_fails_the_event_is_indicated_as_errored)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        BINARY_DATA binary_data = { test_data, length };

        const char* const one_property_keys[] = { "test_property_key" };
        const char* const one_property_values[] = { "test_property_value" };
        const char* const* one_property_keys_ptr = one_property_keys;
        const char* const* one_property_values_ptr = one_property_values;
        size_t one_property_size = 1;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1))
            .SetReturn((MAP_HANDLE)NULL);
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_059: [If any error is encountered while creating the application properties the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_getting_the_properties_details_fails_the_event_is_indicated_as_errored)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        BINARY_DATA binary_data = { test_data, length };

        const char* const one_property_keys[] = { "test_property_key" };
        const char* const one_property_values[] = { "test_property_value" };
        const char* const* one_property_keys_ptr = one_property_keys;
        const char* const* one_property_values_ptr = one_property_values;
        size_t one_property_size = 1;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &one_property_keys_ptr, sizeof(one_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &one_property_values_ptr, sizeof(one_property_values_ptr))
            .CopyOutArgumentBuffer(4, &one_property_size, sizeof(one_property_size))
            .SetReturn(MAP_ERROR);
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_059: [If any error is encountered while creating the application properties the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_creating_the_AMQP_map_fails_the_event_is_indicated_as_errored)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        BINARY_DATA binary_data = { test_data, length };

        const char* const one_property_keys[] = { "test_property_key" };
        const char* const one_property_values[] = { "test_property_value" };
        const char* const* one_property_keys_ptr = one_property_keys;
        const char* const* one_property_values_ptr = one_property_values;
        size_t one_property_size = 1;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &one_property_keys_ptr, sizeof(one_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &one_property_values_ptr, sizeof(one_property_values_ptr))
            .CopyOutArgumentBuffer(4, &one_property_size, sizeof(one_property_size));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_map())
            .SetReturn((AMQP_VALUE)NULL);
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_059: [If any error is encountered while creating the application properties the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_creating_the_AMQP_property_key_value_fails_the_event_is_indicated_as_errored)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        BINARY_DATA binary_data = { test_data, length };

        const char* const one_property_keys[] = { "test_property_key" };
        const char* const one_property_values[] = { "test_property_value" };
        const char* const* one_property_keys_ptr = one_property_keys;
        const char* const* one_property_values_ptr = one_property_values;
        size_t one_property_size = 1;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &one_property_keys_ptr, sizeof(one_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &one_property_values_ptr, sizeof(one_property_values_ptr))
            .CopyOutArgumentBuffer(4, &one_property_size, sizeof(one_property_size));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_map())
            .SetReturn(TEST_UAMQP_MAP);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_key"))
            .SetReturn((AMQP_VALUE)NULL);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_UAMQP_MAP));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_059: [If any error is encountered while creating the application properties the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_creating_the_AMQP_property_value_value_fails_the_event_is_indicated_as_errored)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        BINARY_DATA binary_data = { test_data, length };

        const char* const one_property_keys[] = { "test_property_key" };
        const char* const one_property_values[] = { "test_property_value" };
        const char* const* one_property_keys_ptr = one_property_keys;
        const char* const* one_property_values_ptr = one_property_values;
        size_t one_property_size = 1;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &one_property_keys_ptr, sizeof(one_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &one_property_values_ptr, sizeof(one_property_values_ptr))
            .CopyOutArgumentBuffer(4, &one_property_size, sizeof(one_property_size));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_map())
            .SetReturn(TEST_UAMQP_MAP);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_key"))
            .SetReturn(TEST_PROPERTY_1_KEY_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_value"))
            .SetReturn((AMQP_VALUE)NULL);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_KEY_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_UAMQP_MAP));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_059: [If any error is encountered while creating the application properties the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_setting_the_property_in_the_uAMQP_map_fails_the_event_is_indicated_as_errored)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        BINARY_DATA binary_data = { test_data, length };

        const char* const one_property_keys[] = { "test_property_key" };
        const char* const one_property_values[] = { "test_property_value" };
        const char* const* one_property_keys_ptr = one_property_keys;
        const char* const* one_property_values_ptr = one_property_values;
        size_t one_property_size = 1;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &one_property_keys_ptr, sizeof(one_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &one_property_values_ptr, sizeof(one_property_values_ptr))
            .CopyOutArgumentBuffer(4, &one_property_size, sizeof(one_property_size));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_map())
            .SetReturn(TEST_UAMQP_MAP);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_key"))
            .SetReturn(TEST_PROPERTY_1_KEY_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_value"))
            .SetReturn(TEST_PROPERTY_1_VALUE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_set_map_value(TEST_UAMQP_MAP, TEST_PROPERTY_1_KEY_AMQP_VALUE, TEST_PROPERTY_1_VALUE_AMQP_VALUE))
            .SetReturn(1);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_KEY_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_UAMQP_MAP));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_059: [If any error is encountered while creating the application properties the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_setting_the_message_annotations_on_the_message_fails_the_event_is_indicated_as_errored)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        BINARY_DATA binary_data = { test_data, length };

        const char* const one_property_keys[] = { "test_property_key" };
        const char* const one_property_values[] = { "test_property_value" };
        const char* const* one_property_keys_ptr = one_property_keys;
        const char* const* one_property_values_ptr = one_property_values;
        size_t one_property_size = 1;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &one_property_keys_ptr, sizeof(one_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &one_property_values_ptr, sizeof(one_property_values_ptr))
            .CopyOutArgumentBuffer(4, &one_property_size, sizeof(one_property_size));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_map())
            .SetReturn(TEST_UAMQP_MAP);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_key"))
            .SetReturn(TEST_PROPERTY_1_KEY_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_value"))
            .SetReturn(TEST_PROPERTY_1_VALUE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_set_map_value(TEST_UAMQP_MAP, TEST_PROPERTY_1_KEY_AMQP_VALUE, TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, message_set_application_properties(TEST_MESSAGE_HANDLE, TEST_UAMQP_MAP))
            .SetReturn(1);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_KEY_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_UAMQP_MAP));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* on_message_send_complete */

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_061: [When on_message_send_complete is called with MESSAGE_SEND_OK the pending message shall be indicated as sent correctly by calling the callback associated with the pending message with EVENTHUBCLIENT_CONFIRMATION_OK.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_062: [The pending message shall be removed from the pending list.] */
    TEST_FUNCTION(on_message_send_complete_with_OK_on_one_message_triggers_the_user_callback_and_removes_the_pending_event)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4242);
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &no_property_keys_ptr, sizeof(no_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &no_property_values_ptr, sizeof(no_property_values_ptr))
            .CopyOutArgumentBuffer(4, &no_property_size, sizeof(no_property_size));
        EventHubClient_LL_DoWork(eventHubHandle);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        BINARY_DATA binary_data = { test_data, length };

        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_OK, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        // act
        saved_on_message_send_complete(saved_on_message_send_complete_context, MESSAGE_SEND_OK);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_063: [When on_message_send_complete is called with a result code different than MESSAGE_SEND_OK the pending message shall be indicated as having an error by calling the callback associated with the pending message with EVENTHUBCLIENT_CONFIRMATION_ERROR.]  */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_062: [The pending message shall be removed from the pending list.] */
    TEST_FUNCTION(on_message_send_complete_with_ERROR_on_one_message_triggers_the_user_callback_and_removes_the_pending_event)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        (void)EventHubClient_LL_SendAsync(eventHubHandle, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, (void*)0x4242);
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &no_property_keys_ptr, sizeof(no_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &no_property_values_ptr, sizeof(no_property_values_ptr))
            .CopyOutArgumentBuffer(4, &no_property_size, sizeof(no_property_size));
        EventHubClient_LL_DoWork(eventHubHandle);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        BINARY_DATA binary_data = { test_data, length };

        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        // act
        saved_on_message_send_complete(saved_on_message_send_complete_context, MESSAGE_SEND_ERROR);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* on_messagesender_state_changed */

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_060: [When on_messagesender_state_changed is called with MESSAGE_SENDER_STATE_ERROR] */
    TEST_FUNCTION(when_the_messagesender_state_changes_to_ERROR_then_the_uAMQP_stack_is_brought_down)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        mocks.ResetAllCalls();

        // act
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_ERROR, MESSAGE_SENDER_STATE_OPEN);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_082: [If the number of event data entries for the message is greater than 1 (batched) then the message format shall be set to 0x80013700 by calling message_set_message_format.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_084: [For each event in the batch:] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_085: [The event shall be added to the message by into a separate data section by calling message_add_body_amqp_data.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_086: [The buffer passed to message_add_body_amqp_data shall contain the properties and the binary event payload serialized as AMQP values.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_088: [The event payload shall be serialized as an AMQP message data section.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_090: [Enough memory shall be allocated to hold the properties and binary payload for each event part of the batch.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_091: [The size needed for the properties and data section shall be obtained by calling amqpvalue_get_encoded_size.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_092: [The properties and binary data shall be encoded by calling amqpvalue_encode and passing an encoding function that places the encoded data into the memory allocated for the event.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_093: [If the property count is 0 for an event part of the batch, then no property map shall be serialized for that event.] */
    TEST_FUNCTION(a_batched_message_with_2_events_and_no_properties_is_added_to_the_message_body)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        g_expected_encoded_counter = 0;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT));

        // 1st event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &no_property_keys, sizeof(no_property_keys))
            .CopyOutArgumentBuffer(3, &no_property_values, sizeof(no_property_values))
            .CopyOutArgumentBuffer(4, &no_property_size, sizeof(no_property_size));
        amqp_binary amqy_binary = { test_data, length };
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_data(amqy_binary))
            .SetReturn(TEST_DATA_1);
        unsigned char encoded_data_1[] = { 0x42 };
        size_t data_encoded_size = sizeof(encoded_data_1);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_DATA_1, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &data_encoded_size, sizeof(data_encoded_size));
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(data_encoded_size));
        g_expected_encoded_buffer[0] = encoded_data_1;
        g_expected_encoded_length[0] = data_encoded_size;
        STRICT_EXPECTED_CALL(mocks, amqpvalue_encode(TEST_DATA_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2).IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_DATA_1));
        BINARY_DATA binary_data = { encoded_data_1, data_encoded_size };
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        //2nd event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_2, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_2));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &no_property_keys, sizeof(no_property_keys))
            .CopyOutArgumentBuffer(3, &no_property_values, sizeof(no_property_values))
            .CopyOutArgumentBuffer(4, &no_property_size, sizeof(no_property_size));
        amqy_binary = { test_data, (uint32_t)length };
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_data(amqy_binary))
            .SetReturn(TEST_DATA_2);
        unsigned char encoded_data_2[] = { 0x43, 0x44 };
        data_encoded_size = sizeof(encoded_data_2);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_DATA_2, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &data_encoded_size, sizeof(data_encoded_size));
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(data_encoded_size));
        g_expected_encoded_buffer[1] = encoded_data_2;
        g_expected_encoded_length[1] = data_encoded_size;
        STRICT_EXPECTED_CALL(mocks, amqpvalue_encode(TEST_DATA_2, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2).IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_DATA_2));
        binary_data = { encoded_data_2, data_encoded_size };
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, messagesender_send(TEST_MESSAGE_SENDER_HANDLE, TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3).IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_083: [If message_set_message_format fails, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_message_set_message_format_fails_then_the_callback_is_triggered_with_ERROR)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT))
            .SetReturn(1);
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_2));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_094: [If any error occurs during serializing each event properties and data that are part of the batch, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_getting_the_event_data_fails_then_the_callback_is_triggered_with_ERROR)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT));

        // 1st event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length))
            .SetReturn(EVENTDATA_ERROR);

        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_2));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_094: [If any error occurs during serializing each event properties and data that are part of the batch, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_getting_the_event_data_properties_fails_then_the_callback_is_triggered_with_ERROR)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT));

        // 1st event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1))
            .SetReturn((MAP_HANDLE)NULL);

        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_2));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_094: [If any error occurs during serializing each event properties and data that are part of the batch, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_getting_the_properties_map_keys_and_values_fails_then_the_callback_is_triggered_with_ERROR)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT));

        // 1st event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &no_property_keys, sizeof(no_property_keys))
            .CopyOutArgumentBuffer(3, &no_property_values, sizeof(no_property_values))
            .CopyOutArgumentBuffer(4, &no_property_size, sizeof(no_property_size))
            .SetReturn(MAP_ERROR);

        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_2));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_094: [If any error occurs during serializing each event properties and data that are part of the batch, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_creating_the_data_section_fails_then_the_callback_is_triggered_with_ERROR)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT));

        // 1st event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &no_property_keys, sizeof(no_property_keys))
            .CopyOutArgumentBuffer(3, &no_property_values, sizeof(no_property_values))
            .CopyOutArgumentBuffer(4, &no_property_size, sizeof(no_property_size));
        amqp_binary amqy_binary = { test_data, length };
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_data(amqy_binary))
            .SetReturn((AMQP_VALUE)NULL);

        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_2));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_094: [If any error occurs during serializing each event properties and data that are part of the batch, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_getting_the_encoded_length_of_the_data_section_fails_then_the_callback_is_triggered_with_ERROR)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT));

        // 1st event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &no_property_keys, sizeof(no_property_keys))
            .CopyOutArgumentBuffer(3, &no_property_values, sizeof(no_property_values))
            .CopyOutArgumentBuffer(4, &no_property_size, sizeof(no_property_size));
        amqp_binary amqy_binary = { test_data, length };
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_data(amqy_binary))
            .SetReturn(TEST_DATA_1);
        unsigned char encoded_data_1[] = { 0x42 };
        size_t data_encoded_size = sizeof(encoded_data_1);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_DATA_1, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &data_encoded_size, sizeof(data_encoded_size))
            .SetReturn(1);

        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_DATA_1));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_2));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_094: [If any error occurs during serializing each event properties and data that are part of the batch, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_allocating_memory_for_the_data_section_fails_then_the_callback_is_triggered_with_ERROR)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT));

        // 1st event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &no_property_keys, sizeof(no_property_keys))
            .CopyOutArgumentBuffer(3, &no_property_values, sizeof(no_property_values))
            .CopyOutArgumentBuffer(4, &no_property_size, sizeof(no_property_size));
        amqp_binary amqy_binary = { test_data, length };
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_data(amqy_binary))
            .SetReturn(TEST_DATA_1);
        unsigned char encoded_data_1[] = { 0x42 };
        size_t data_encoded_size = sizeof(encoded_data_1);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_DATA_1, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &data_encoded_size, sizeof(data_encoded_size));
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(data_encoded_size))
            .SetReturn((void*)NULL);

        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_DATA_1));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_2));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_094: [If any error occurs during serializing each event properties and data that are part of the batch, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_encoding_the_event_in_a_batch_fails_then_the_callback_is_triggered_with_ERROR)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        g_expected_encoded_counter = 0;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT));

        // 1st event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &no_property_keys, sizeof(no_property_keys))
            .CopyOutArgumentBuffer(3, &no_property_values, sizeof(no_property_values))
            .CopyOutArgumentBuffer(4, &no_property_size, sizeof(no_property_size));
        amqp_binary amqy_binary = { test_data, length };
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_data(amqy_binary))
            .SetReturn(TEST_DATA_1);
        unsigned char encoded_data_1[] = { 0x42 };
        size_t data_encoded_size = sizeof(encoded_data_1);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_DATA_1, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &data_encoded_size, sizeof(data_encoded_size));
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(data_encoded_size));
        g_expected_encoded_buffer[0] = encoded_data_1;
        g_expected_encoded_length[0] = data_encoded_size;
        STRICT_EXPECTED_CALL(mocks, amqpvalue_encode(TEST_DATA_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2).IgnoreArgument(3)
            .SetReturn(1);

        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_DATA_1));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_2));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_089: [If message_add_body_amqp_data fails, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_message_add_body_amqp_data_for_an_event_in_a_batch_fails_then_the_callback_is_triggered_with_ERROR)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        g_expected_encoded_counter = 0;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT));

        // 1st event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &no_property_keys, sizeof(no_property_keys))
            .CopyOutArgumentBuffer(3, &no_property_values, sizeof(no_property_values))
            .CopyOutArgumentBuffer(4, &no_property_size, sizeof(no_property_size));
        amqp_binary amqy_binary = { test_data, length };
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_data(amqy_binary))
            .SetReturn(TEST_DATA_1);
        unsigned char encoded_data_1[] = { 0x42 };
        size_t data_encoded_size = sizeof(encoded_data_1);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_DATA_1, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &data_encoded_size, sizeof(data_encoded_size));
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(data_encoded_size));
        g_expected_encoded_buffer[0] = encoded_data_1;
        g_expected_encoded_length[0] = data_encoded_size;
        STRICT_EXPECTED_CALL(mocks, amqpvalue_encode(TEST_DATA_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2).IgnoreArgument(3);
        BINARY_DATA binary_data = { encoded_data_1, data_encoded_size };
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data))
            .SetReturn(1);

        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_DATA_1));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_2));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_086: [The buffer passed to message_add_body_amqp_data shall contain the properties and the binary event payload serialized as AMQP values.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_090: [Enough memory shall be allocated to hold the properties and binary payload for each event part of the batch.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_091: [The size needed for the properties and data section shall be obtained by calling amqpvalue_get_encoded_size.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_092: [The properties and binary data shall be encoded by calling amqpvalue_encode and passing an encoding function that places the encoded data into the memory allocated for the event.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_087: [The properties shall be serialized as AMQP application_properties.] */
    TEST_FUNCTION(a_batched_event_with_2_properties_gets_the_properties_added_to_the_payload)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        g_expected_encoded_counter = 0;

        const char* const two_property_keys[] = { "test_property_key", "prop_key_2" };
        const char* const two_property_values[] = { "test_property_value", "prop_value_2" };
        const char* const* two_property_keys_ptr = two_property_keys;
        const char* const* two_property_values_ptr = two_property_values;
        size_t two_properties_size = 2;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT));

        // 1st event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &two_property_keys_ptr, sizeof(two_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &two_property_values_ptr, sizeof(two_property_values_ptr))
            .CopyOutArgumentBuffer(4, &two_properties_size, sizeof(two_properties_size));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_map())
            .SetReturn(TEST_UAMQP_MAP);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_key"))
            .SetReturn(TEST_PROPERTY_1_KEY_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_value"))
            .SetReturn(TEST_PROPERTY_1_VALUE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_set_map_value(TEST_UAMQP_MAP, TEST_PROPERTY_1_KEY_AMQP_VALUE, TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("prop_key_2"))
            .SetReturn(TEST_PROPERTY_2_KEY_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("prop_value_2"))
            .SetReturn(TEST_PROPERTY_2_VALUE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_set_map_value(TEST_UAMQP_MAP, TEST_PROPERTY_2_KEY_AMQP_VALUE, TEST_PROPERTY_2_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_KEY_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_2_KEY_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_2_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_application_properties(TEST_UAMQP_MAP))
            .SetReturn(TEST_APPLICATION_PROPERTIES_1);
        unsigned char properties_encoded_data[] = { 0x42, 0x43, 0x44 };
        size_t properties_encoded_size = sizeof(properties_encoded_data);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_APPLICATION_PROPERTIES_1, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &properties_encoded_size, sizeof(properties_encoded_size));
        amqp_binary amqy_binary = { test_data, length };
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_data(amqy_binary))
            .SetReturn(TEST_DATA_1);
        unsigned char encoded_data_1[] = { 0x42 };
        size_t data_encoded_size = sizeof(encoded_data_1);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_DATA_1, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &data_encoded_size, sizeof(data_encoded_size));
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(data_encoded_size + properties_encoded_size));
        g_expected_encoded_buffer[0] = properties_encoded_data;
        g_expected_encoded_length[0] = properties_encoded_size;
        g_expected_encoded_buffer[1] = encoded_data_1;
        g_expected_encoded_length[1] = data_encoded_size;
        STRICT_EXPECTED_CALL(mocks, amqpvalue_encode(TEST_APPLICATION_PROPERTIES_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2).IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_encode(TEST_DATA_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2).IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_APPLICATION_PROPERTIES_1));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_UAMQP_MAP));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_DATA_1));
        unsigned char combined_encoded_data[] = { 0x42, 0x43, 0x44, 0x42 };
        BINARY_DATA binary_data = { combined_encoded_data, data_encoded_size + properties_encoded_size };
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        //2nd event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_2, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_2));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &no_property_keys, sizeof(no_property_keys))
            .CopyOutArgumentBuffer(3, &no_property_values, sizeof(no_property_values))
            .CopyOutArgumentBuffer(4, &no_property_size, sizeof(no_property_size));
        amqy_binary = { test_data, (uint32_t)length };
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_data(amqy_binary))
            .SetReturn(TEST_DATA_2);
        unsigned char encoded_data_2[] = { 0x43, 0x44 };
        data_encoded_size = sizeof(encoded_data_2);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_DATA_2, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &data_encoded_size, sizeof(data_encoded_size));
        STRICT_EXPECTED_CALL(mocks, gballoc_malloc(data_encoded_size));
        g_expected_encoded_buffer[2] = encoded_data_2;
        g_expected_encoded_length[2] = data_encoded_size;
        STRICT_EXPECTED_CALL(mocks, amqpvalue_encode(TEST_DATA_2, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2).IgnoreArgument(3);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_DATA_2));
        binary_data = { encoded_data_2, data_encoded_size };
        STRICT_EXPECTED_CALL(mocks, message_add_body_amqp_data(TEST_MESSAGE_HANDLE, binary_data));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, messagesender_send(TEST_MESSAGE_SENDER_HANDLE, TEST_MESSAGE_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(3).IgnoreArgument(4);
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_094: [If any error occurs during serializing each event properties and data that are part of the batch, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_creating_the_properties_map_fails_then_the_callback_is_triggered_with_ERROR)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        g_expected_encoded_counter = 0;

        const char* const two_property_keys[] = { "test_property_key", "prop_key_2" };
        const char* const two_property_values[] = { "test_property_value", "prop_value_2" };
        const char* const* two_property_keys_ptr = two_property_keys;
        const char* const* two_property_values_ptr = two_property_values;
        size_t two_properties_size = 2;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT));

        // 1st event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &two_property_keys_ptr, sizeof(two_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &two_property_values_ptr, sizeof(two_property_values_ptr))
            .CopyOutArgumentBuffer(4, &two_properties_size, sizeof(two_properties_size));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_map())
            .SetReturn((AMQP_VALUE)NULL);

        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_2));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_094: [If any error occurs during serializing each event properties and data that are part of the batch, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_creating_the_key_for_the_first_property_string_fails_then_the_callback_is_triggered_with_ERROR)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        g_expected_encoded_counter = 0;

        const char* const two_property_keys[] = { "test_property_key", "prop_key_2" };
        const char* const two_property_values[] = { "test_property_value", "prop_value_2" };
        const char* const* two_property_keys_ptr = two_property_keys;
        const char* const* two_property_values_ptr = two_property_values;
        size_t two_properties_size = 2;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT));

        // 1st event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &two_property_keys_ptr, sizeof(two_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &two_property_values_ptr, sizeof(two_property_values_ptr))
            .CopyOutArgumentBuffer(4, &two_properties_size, sizeof(two_properties_size));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_map())
            .SetReturn(TEST_UAMQP_MAP);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_key"))
            .SetReturn((AMQP_VALUE)NULL);

        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_UAMQP_MAP));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_2));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_094: [If any error occurs during serializing each event properties and data that are part of the batch, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_creating_the_value_for_the_first_property_string_fails_then_the_callback_is_triggered_with_ERROR)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        g_expected_encoded_counter = 0;

        const char* const two_property_keys[] = { "test_property_key", "prop_key_2" };
        const char* const two_property_values[] = { "test_property_value", "prop_value_2" };
        const char* const* two_property_keys_ptr = two_property_keys;
        const char* const* two_property_values_ptr = two_property_values;
        size_t two_properties_size = 2;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT));

        // 1st event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &two_property_keys_ptr, sizeof(two_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &two_property_values_ptr, sizeof(two_property_values_ptr))
            .CopyOutArgumentBuffer(4, &two_properties_size, sizeof(two_properties_size));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_map())
            .SetReturn(TEST_UAMQP_MAP);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_key"))
            .SetReturn(TEST_PROPERTY_1_KEY_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_value"))
            .SetReturn((AMQP_VALUE)NULL);

        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_KEY_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_UAMQP_MAP));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_2));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_094: [If any error occurs during serializing each event properties and data that are part of the batch, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_setting_the_first_property_in_the_properties_map_fails_then_the_callback_is_triggered_with_ERROR)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        g_expected_encoded_counter = 0;

        const char* const two_property_keys[] = { "test_property_key", "prop_key_2" };
        const char* const two_property_values[] = { "test_property_value", "prop_value_2" };
        const char* const* two_property_keys_ptr = two_property_keys;
        const char* const* two_property_values_ptr = two_property_values;
        size_t two_properties_size = 2;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT));

        // 1st event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &two_property_keys_ptr, sizeof(two_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &two_property_values_ptr, sizeof(two_property_values_ptr))
            .CopyOutArgumentBuffer(4, &two_properties_size, sizeof(two_properties_size));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_map())
            .SetReturn(TEST_UAMQP_MAP);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_key"))
            .SetReturn(TEST_PROPERTY_1_KEY_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_value"))
            .SetReturn(TEST_PROPERTY_1_VALUE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_set_map_value(TEST_UAMQP_MAP, TEST_PROPERTY_1_KEY_AMQP_VALUE, TEST_PROPERTY_1_VALUE_AMQP_VALUE))
            .SetReturn(1);

        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_KEY_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_UAMQP_MAP));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_2));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_094: [If any error occurs during serializing each event properties and data that are part of the batch, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_creating_the_key_for_the_second_property_in_the_properties_map_fails_then_the_callback_is_triggered_with_ERROR)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        g_expected_encoded_counter = 0;

        const char* const two_property_keys[] = { "test_property_key", "prop_key_2" };
        const char* const two_property_values[] = { "test_property_value", "prop_value_2" };
        const char* const* two_property_keys_ptr = two_property_keys;
        const char* const* two_property_values_ptr = two_property_values;
        size_t two_properties_size = 2;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT));

        // 1st event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &two_property_keys_ptr, sizeof(two_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &two_property_values_ptr, sizeof(two_property_values_ptr))
            .CopyOutArgumentBuffer(4, &two_properties_size, sizeof(two_properties_size));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_map())
            .SetReturn(TEST_UAMQP_MAP);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_key"))
            .SetReturn(TEST_PROPERTY_1_KEY_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_value"))
            .SetReturn(TEST_PROPERTY_1_VALUE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_set_map_value(TEST_UAMQP_MAP, TEST_PROPERTY_1_KEY_AMQP_VALUE, TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("prop_key_2"))
            .SetReturn((AMQP_VALUE)NULL);

        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_KEY_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_UAMQP_MAP));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_2));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_094: [If any error occurs during serializing each event properties and data that are part of the batch, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_creating_the_value_for_the_second_property_in_the_properties_map_fails_then_the_callback_is_triggered_with_ERROR)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        g_expected_encoded_counter = 0;

        const char* const two_property_keys[] = { "test_property_key", "prop_key_2" };
        const char* const two_property_values[] = { "test_property_value", "prop_value_2" };
        const char* const* two_property_keys_ptr = two_property_keys;
        const char* const* two_property_values_ptr = two_property_values;
        size_t two_properties_size = 2;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT));

        // 1st event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &two_property_keys_ptr, sizeof(two_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &two_property_values_ptr, sizeof(two_property_values_ptr))
            .CopyOutArgumentBuffer(4, &two_properties_size, sizeof(two_properties_size));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_map())
            .SetReturn(TEST_UAMQP_MAP);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_key"))
            .SetReturn(TEST_PROPERTY_1_KEY_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_value"))
            .SetReturn(TEST_PROPERTY_1_VALUE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_set_map_value(TEST_UAMQP_MAP, TEST_PROPERTY_1_KEY_AMQP_VALUE, TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("prop_key_2"))
            .SetReturn(TEST_PROPERTY_2_KEY_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("prop_value_2"))
            .SetReturn((AMQP_VALUE)NULL);

        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_KEY_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_2_KEY_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_UAMQP_MAP));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_2));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_094: [If any error occurs during serializing each event properties and data that are part of the batch, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_adding_the_second_property_to_the_map_fails_then_the_callback_is_triggered_with_ERROR)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        g_expected_encoded_counter = 0;

        const char* const two_property_keys[] = { "test_property_key", "prop_key_2" };
        const char* const two_property_values[] = { "test_property_value", "prop_value_2" };
        const char* const* two_property_keys_ptr = two_property_keys;
        const char* const* two_property_values_ptr = two_property_values;
        size_t two_properties_size = 2;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT));

        // 1st event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &two_property_keys_ptr, sizeof(two_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &two_property_values_ptr, sizeof(two_property_values_ptr))
            .CopyOutArgumentBuffer(4, &two_properties_size, sizeof(two_properties_size));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_map())
            .SetReturn(TEST_UAMQP_MAP);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_key"))
            .SetReturn(TEST_PROPERTY_1_KEY_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_value"))
            .SetReturn(TEST_PROPERTY_1_VALUE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_set_map_value(TEST_UAMQP_MAP, TEST_PROPERTY_1_KEY_AMQP_VALUE, TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("prop_key_2"))
            .SetReturn(TEST_PROPERTY_2_KEY_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("prop_value_2"))
            .SetReturn(TEST_PROPERTY_2_VALUE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_set_map_value(TEST_UAMQP_MAP, TEST_PROPERTY_2_KEY_AMQP_VALUE, TEST_PROPERTY_2_VALUE_AMQP_VALUE))
            .SetReturn(1);

        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_KEY_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_2_KEY_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_2_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_UAMQP_MAP));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_2));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_094: [If any error occurs during serializing each event properties and data that are part of the batch, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_creating_the_application_properties_object_fails_then_the_callback_is_triggered_with_ERROR)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        g_expected_encoded_counter = 0;

        const char* const two_property_keys[] = { "test_property_key", "prop_key_2" };
        const char* const two_property_values[] = { "test_property_value", "prop_value_2" };
        const char* const* two_property_keys_ptr = two_property_keys;
        const char* const* two_property_values_ptr = two_property_values;
        size_t two_properties_size = 2;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT));

        // 1st event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &two_property_keys_ptr, sizeof(two_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &two_property_values_ptr, sizeof(two_property_values_ptr))
            .CopyOutArgumentBuffer(4, &two_properties_size, sizeof(two_properties_size));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_map())
            .SetReturn(TEST_UAMQP_MAP);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_key"))
            .SetReturn(TEST_PROPERTY_1_KEY_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_value"))
            .SetReturn(TEST_PROPERTY_1_VALUE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_set_map_value(TEST_UAMQP_MAP, TEST_PROPERTY_1_KEY_AMQP_VALUE, TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("prop_key_2"))
            .SetReturn(TEST_PROPERTY_2_KEY_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("prop_value_2"))
            .SetReturn(TEST_PROPERTY_2_VALUE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_set_map_value(TEST_UAMQP_MAP, TEST_PROPERTY_2_KEY_AMQP_VALUE, TEST_PROPERTY_2_VALUE_AMQP_VALUE));
        amqp_binary amqy_binary = { test_data, length };
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_data(amqy_binary))
            .SetReturn(TEST_DATA_1);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_application_properties(TEST_UAMQP_MAP))
            .SetReturn((AMQP_VALUE)NULL);

        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_DATA_1));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_KEY_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_2_KEY_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_2_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_UAMQP_MAP));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_2));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_094: [If any error occurs during serializing each event properties and data that are part of the batch, the callback associated with the message shall be called with EVENTHUBCLIENT_CONFIRMATION_ERROR and the message shall be freed from the pending list.] */
    TEST_FUNCTION(when_getting_the_encoded_size_for_the_properties_fails_then_the_callback_is_triggered_with_ERROR)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        setup_messenger_initialize_success(&mocks);
        EventHubClient_LL_DoWork(eventHubHandle);
        saved_on_message_sender_state_changed(saved_message_sender_context, MESSAGE_SENDER_STATE_OPEN, MESSAGE_SENDER_STATE_IDLE);
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        (void)EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(EVENTDATA_HANDLE), sendAsyncConfirmationCallback, (void*)0x4242);
        mocks.ResetAllCalls();

        unsigned char test_data[] = { 0x42 };
        unsigned char* buffer = test_data;
        size_t length = sizeof(test_data);
        g_expected_encoded_counter = 0;

        const char* const two_property_keys[] = { "test_property_key", "prop_key_2" };
        const char* const two_property_values[] = { "test_property_value", "prop_value_2" };
        const char* const* two_property_keys_ptr = two_property_keys;
        const char* const* two_property_values_ptr = two_property_values;
        size_t two_properties_size = 2;

        STRICT_EXPECTED_CALL(mocks, message_create());
        STRICT_EXPECTED_CALL(mocks, message_set_message_format(TEST_MESSAGE_HANDLE, MICROSOFT_MESSAGE_FORMAT));

        // 1st event
        STRICT_EXPECTED_CALL(mocks, EventData_GetData(TEST_CLONED_EVENTDATA_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &buffer, sizeof(buffer))
            .CopyOutArgumentBuffer(3, &length, sizeof(length));
        STRICT_EXPECTED_CALL(mocks, EventData_Properties(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, Map_GetInternals(TEST_MAP_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &two_property_keys_ptr, sizeof(two_property_keys_ptr))
            .CopyOutArgumentBuffer(3, &two_property_values_ptr, sizeof(two_property_values_ptr))
            .CopyOutArgumentBuffer(4, &two_properties_size, sizeof(two_properties_size));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_map())
            .SetReturn(TEST_UAMQP_MAP);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_key"))
            .SetReturn(TEST_PROPERTY_1_KEY_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("test_property_value"))
            .SetReturn(TEST_PROPERTY_1_VALUE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_set_map_value(TEST_UAMQP_MAP, TEST_PROPERTY_1_KEY_AMQP_VALUE, TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("prop_key_2"))
            .SetReturn(TEST_PROPERTY_2_KEY_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_string("prop_value_2"))
            .SetReturn(TEST_PROPERTY_2_VALUE_AMQP_VALUE);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_set_map_value(TEST_UAMQP_MAP, TEST_PROPERTY_2_KEY_AMQP_VALUE, TEST_PROPERTY_2_VALUE_AMQP_VALUE));
        amqp_binary amqy_binary = { test_data, length };
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_data(amqy_binary))
            .SetReturn(TEST_DATA_1);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_create_application_properties(TEST_UAMQP_MAP))
            .SetReturn(TEST_APPLICATION_PROPERTIES_1);
        unsigned char properties_encoded_data[] = { 0x42, 0x43, 0x44 };
        size_t properties_encoded_size = sizeof(properties_encoded_data);
        STRICT_EXPECTED_CALL(mocks, amqpvalue_get_encoded_size(TEST_APPLICATION_PROPERTIES_1, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &properties_encoded_size, sizeof(properties_encoded_size))
            .SetReturn(1);

        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_DATA_1));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_APPLICATION_PROPERTIES_1));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_KEY_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_1_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_2_KEY_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_PROPERTY_2_VALUE_AMQP_VALUE));
        STRICT_EXPECTED_CALL(mocks, amqpvalue_destroy(TEST_UAMQP_MAP));
        STRICT_EXPECTED_CALL(mocks, message_destroy(TEST_MESSAGE_HANDLE));
        STRICT_EXPECTED_CALL(mocks, sendAsyncConfirmationCallback(EVENTHUBCLIENT_CONFIRMATION_ERROR, (void*)0x4242));
        EXPECTED_CALL(mocks, DList_RemoveEntryList(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_2));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mocks, connection_dowork(TEST_CONNECTION_HANDLE));

        // act
        EventHubClient_LL_DoWork(eventHubHandle);

        // assert
        mocks.AssertActualAndExpectedCalls();

        // cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* EventHubClient_LL_SendBatchAsync */

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_012: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_INVALID_ARG if eventhubClientLLHandle or eventDataList are NULL or if sendAsnycConfirmationCallback equals NULL and userContextCallback does not equal NULL.] */
    TEST_FUNCTION(EventHubClient_LL_SendBatchAsync_with_NULL_sendAsyncConfirmationCallbackandNonNullUSerContext_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };

        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(batch[0]), NULL, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_012: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_INVALID_ARG if eventhubClientLLHandle or eventDataList are NULL or if sendAsnycConfirmationCallback equals NULL and userContextCallback does not equal NULL.]  */
    TEST_FUNCTION(EventHubClient_LL_SendBatchAsync_with_NULL_eventHubLLHandle_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(NULL, batch, sizeof(batch) / sizeof(batch[0]), sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_012: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_INVALID_ARG if eventhubClientLLHandle or eventDataList are NULL or if sendAsnycConfirmationCallback equals NULL and userContextCallback does not equal NULL.] */
    TEST_FUNCTION(EventHubClient_LL_SendBatchAsync_with_NULL_event_data_list_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, NULL, sizeof(batch) / sizeof(batch[0]), sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_095: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_INVALID_ARG if the count argument is zero.] */
    TEST_FUNCTION(EventHubClient_LL_SendBatchAsync_with_zero_count_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, 0, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_014: [EventHubClient_LL_SendBatchAsync shall clone each item in the eventDataList by calling EventData_Clone.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_07_015: [On success EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_OK.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_097: [The partition key for each event shall be obtained by calling EventData_getPartitionKey.] */
    TEST_FUNCTION(EventHubClient_LL_SendBatchAsync_clones_the_event_data)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, EventData_GetPartitionKey(TEST_EVENTDATA_HANDLE_1))
            .SetReturn("partitionKey");
        STRICT_EXPECTED_CALL(mocks, EventData_GetPartitionKey(TEST_EVENTDATA_HANDLE_2))
            .SetReturn("partitionKey");
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_2);
        EXPECTED_CALL(mocks, DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(batch[0]), sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_01_097: [The partition key for each event shall be obtained by calling EventData_getPartitionKey.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_01_096: [If the partitionKey properties on the events in the batch are not the same then EventHubClient_LL_SendBatchAsync shall fail and return EVENTHUBCLIENT_ERROR.] */
    TEST_FUNCTION(when_the_partitions_on_the_events_do_not_match_then_EventHubClient_LL_SendBatchAsync_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, EventData_GetPartitionKey(TEST_EVENTDATA_HANDLE_1))
            .SetReturn("partitionKey1");
        STRICT_EXPECTED_CALL(mocks, EventData_GetPartitionKey(TEST_EVENTDATA_HANDLE_2))
            .SetReturn("partitionKey2");

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(batch[0]), sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_013: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_ERROR for any Error that is encountered.] */
    TEST_FUNCTION(when_allocating_memory_for_the_event_batch_fails_then_EventHubClient_LL_SendBatchAsync_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, EventData_GetPartitionKey(TEST_EVENTDATA_HANDLE_1))
            .SetReturn("partitionKey");
        STRICT_EXPECTED_CALL(mocks, EventData_GetPartitionKey(TEST_EVENTDATA_HANDLE_2))
            .SetReturn("partitionKey");
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .SetReturn((void*)NULL);

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(batch[0]), sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_013: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_ERROR for any Error that is encountered.]  */
    TEST_FUNCTION(when_allocating_memory_for_the_list_of_events_fails_then_EventHubClient_LL_SendBatchAsync_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, EventData_GetPartitionKey(TEST_EVENTDATA_HANDLE_1))
            .SetReturn("partitionKey");
        STRICT_EXPECTED_CALL(mocks, EventData_GetPartitionKey(TEST_EVENTDATA_HANDLE_2))
            .SetReturn("partitionKey");
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG))
            .SetReturn((void*)NULL);
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(batch[0]), sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_013: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_ERROR for any Error that is encountered.]  */
    TEST_FUNCTION(when_cloning_the_first_item_fails_EventHubClient_LL_SendBatchAsync_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, EventData_GetPartitionKey(TEST_EVENTDATA_HANDLE_1))
            .SetReturn("partitionKey");
        STRICT_EXPECTED_CALL(mocks, EventData_GetPartitionKey(TEST_EVENTDATA_HANDLE_2))
            .SetReturn("partitionKey");
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn((EVENTDATA_HANDLE)NULL);
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(batch[0]), sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_013: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_ERROR for any Error that is encountered.]  */
    TEST_FUNCTION(when_cloning_the_second_item_fails_EventHubClient_LL_SendBatchAsync_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;
        EVENTDATA_HANDLE batch[] = { TEST_EVENTDATA_HANDLE_1, TEST_EVENTDATA_HANDLE_2 };
        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(mocks, EventData_GetPartitionKey(TEST_EVENTDATA_HANDLE_1))
            .SetReturn("partitionKey");
        STRICT_EXPECTED_CALL(mocks, EventData_GetPartitionKey(TEST_EVENTDATA_HANDLE_2))
            .SetReturn("partitionKey");
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        EXPECTED_CALL(mocks, gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_1))
            .SetReturn(TEST_CLONED_EVENTDATA_HANDLE_1);
        STRICT_EXPECTED_CALL(mocks, EventData_Clone(TEST_EVENTDATA_HANDLE_2))
            .SetReturn((EVENTDATA_HANDLE)NULL);
        STRICT_EXPECTED_CALL(mocks, EventData_Destroy(TEST_CLONED_EVENTDATA_HANDLE_1));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));
        EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG));

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, batch, sizeof(batch) / sizeof(batch[0]), sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    TEST_FUNCTION(EventHubClient_LL_SetStateChangeCallback_EventHubClient_NULL_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SetStateChangeCallback(NULL, eventhub_state_change_callback, NULL);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
    }

    TEST_FUNCTION(EventHubClient_LL_SetStateChangeCallback_succeed)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SetStateChangeCallback(eventHubHandle, eventhub_state_change_callback, NULL);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    TEST_FUNCTION(EventHubClient_LL_SetErrorCallback_EventHubClient_NULL_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SetErrorCallback(NULL, eventhub_error_callback, NULL);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
    }

    TEST_FUNCTION(EventHubClient_LL_SetErrorCallback_succeed)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SetErrorCallback(eventHubHandle, eventhub_error_callback, NULL);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    TEST_FUNCTION(EventHubClient_LL_SetLogTrace_EventHubClient_NULL_fails)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        // act
        EventHubClient_LL_SetLogTrace(NULL, false);

        //assert
        mocks.AssertActualAndExpectedCalls();

        //cleanup
    }

    TEST_FUNCTION(EventHubClient_LL_SetLogTrace_succeed)
    {
        // arrange
        CEventHubClientLLMocks mocks;

        setup_createfromconnectionstring_success(&mocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, TEST_EVENTHUB_PATH);
        mocks.ResetAllCalls();

        // act
        EventHubClient_LL_SetLogTrace(eventHubHandle, true);

        //assert
        mocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

END_TEST_SUITE(eventhubclient_ll_unittests)
