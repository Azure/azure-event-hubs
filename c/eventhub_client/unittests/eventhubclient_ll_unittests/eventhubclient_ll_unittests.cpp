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
#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"

#include "eventhubclient_ll.h"
#include "proton/message.h"
#include "proton/messenger.h"
#include "proton/codec.h"
#include "strings.h"
#include "string_tokenizer.h"
#include "urlencode.h"
#include "version.h"
#include "lock.h"
#include "doublylinkedlist.h"

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

#define TEST_EVENTDATA_HANDLE (EVENTDATA_HANDLE)0x45
#define TEST_STRING_HANDLE (STRING_HANDLE)0x46
#define TEST_ENCODED_STRING_HANDLE (STRING_HANDLE)0x47
#define TEST_PN_DATA (pn_data_t*)0x43

#define TEST_STRING_TOKENIZER_HANDLE (STRING_TOKENIZER_HANDLE)0x48

#define TEST_SIZE 7
#define TEST_BYTES "MyTestBytes"
#define NULL_PN_MESSENGER_NAME NULL
#define ALL_MESSAGE_IN_OUTGOING_QUEUE -1

#define EXPECTED_OUTGOING_WINDOW_SIZE 10
#define EXPECTED_TIMEOUT 10000

#define MICROSOFT_MESSAGE_FORMAT 0x80013700

#define EVENT_HANDLE_COUNT  3

#define OVER_MAX_PACKET_SIZE    256*1024

static const char* CONNECTION_STRING = "Endpoint=sb://servicebusName.servicebus.windows.net/;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";
static const char* EVENTHUB_PATH = "eventHubName";
static const char* AMQP_ENCODED_ADDRESS = "amqps://RootManageSharedAccessKey:icT5kKsJr%2fDw7oZveq7OPsSxu5Trmr6aiWlgI5zIT%2f8%3d@servicebusName.servicebus.windows.net/eventHubName";
static const char ENDPOINT_SUBSTRING[] = "Endpoint=sb://";
static const size_t ENDPOINT_SUBSTRING_LENGTH = sizeof(ENDPOINT_SUBSTRING) / sizeof(ENDPOINT_SUBSTRING[0]) - 1;

static const STRING_HANDLE KEYNAME_ENCODED_STRING_HANDLE = (STRING_HANDLE)"RootManageSharedAccessKey";
static const STRING_HANDLE KEY_ENCODED_STRING_HANDLE = (STRING_HANDLE)"icT5kKsJr%2fDw7oZveq7OPsSxu5Trmr6aiWlgI5zIT%2f8%3d";
static const STRING_HANDLE NAMESPACE_ENCODED_STRING_HANDLE = (STRING_HANDLE)"servicebusName";
static const STRING_HANDLE EVENTHUBPATH_ENCODED_STRING_HANDLE = (STRING_HANDLE)"eventHubName";

static const char* TEXT_MESSAGE = "Hello From EventHubClient Unit Tests";
static const char* TEST_CHAR = "TestChar";
static const char* PARTITION_KEY_VALUE = "PartitionKeyValue";
static const char* PARTITION_KEY_EMPTY_VALUE = "";
static const char* PROPERTY_NAME = "PropertyName";
static const char TEST_STRING_VALUE[] = "Property_String_Value_1";
static const char TEST_STRING_VALUE2[] = "Property_String_Value_2";

const int BUFFER_SIZE = 8;
bool g_setProperty = false;
static bool g_lockInitFail = false;

static size_t currentmalloc_call;
static size_t whenShallmalloc_fail;

static size_t g_currentlock_call;
static size_t g_whenShalllock_fail;

static size_t g_current_pn_messenger_call;
static size_t g_when_pn_messenger_fail;

static size_t g_currentEventClone_call;
static size_t g_whenShallEventClone_fail;

static size_t g_currentEventDataGetData_call;
static size_t g_whenEventDataGetData_fail;
//static const char* g_textDataMsg = NULL;
//static size_t g_textDataSize = 0;

typedef struct LOCK_TEST_STRUCT_TAG
{
    char* dummy;
} LOCK_TEST_STRUCT;

typedef struct CALLBACK_CONFIRM_INFO
{
    sig_atomic_t successCounter;
    sig_atomic_t totalCalls;
} CALLBACK_CONFIRM_INFO;

static pn_bytes_t test_pn_bytes =
{
    TEST_SIZE,
    TEST_BYTES
};

std::ostream& operator<<(std::ostream& left, const pn_bytes_t& pnBytes)
{
    std::ios::fmtflags f(left.flags());
    left << std::hex;
    for (size_t i = 0; i < pnBytes.size; i++)
    {
        left << pnBytes.start[i];
    }
    left.flags(f);
    return left;
}

static bool operator==(const pn_bytes_t left, const pn_bytes_t& right)
{
    if (left.size != right.size)
    {
        return false;
    }
    else
    {
        return memcmp(left.start, right.start, left.size) == 0;
    }
}

static void fakeCallBack(EVENTHUBCLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    EVENTHUBCLIENT_CONFIRMATION_RESULT* expectedResult = (EVENTHUBCLIENT_CONFIRMATION_RESULT*)userContextCallback;
    ASSERT_ARE_EQUAL(EVENTHUBCLIENT_CONFIRMATION_RESULT, *expectedResult, result);
}

static void CounterCallBack(EVENTHUBCLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    CALLBACK_CONFIRM_INFO* contextInfo = (CALLBACK_CONFIRM_INFO*)userContextCallback;
    contextInfo->totalCalls++;
    // Only count the successfull callbacks
    if (result == EVENTHUBCLIENT_CONFIRMATION_OK)
    {
        contextInfo->successCounter++;
    }
}

// ** Mocks **
TYPED_MOCK_CLASS(CEventHubClientLLMocks, CGlobalMock)
{
public:
    /* Proton Mocks */
    MOCK_STATIC_METHOD_0(, pn_message_t*, pn_message)
    MOCK_METHOD_END(pn_message_t*, (pn_message_t*)BASEIMPLEMENTATION::gballoc_malloc(1))

    MOCK_STATIC_METHOD_2(, int, pn_message_set_address, pn_message_t*, msg, const char*, address)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_2(, int, pn_message_set_inferred, pn_message_t*, msg, bool, inferred)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_1(, pn_data_t*, pn_message_body, pn_message_t*, msg)
    MOCK_METHOD_END(pn_data_t*, TEST_PN_DATA)

    MOCK_STATIC_METHOD_2(, int, pn_data_put_binary, pn_data_t*, data, pn_bytes_t, bytes)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_1(, pn_messenger_t*, pn_messenger, const char*, name)
        pn_messenger_t* messenger;
        g_current_pn_messenger_call++;
        if (g_when_pn_messenger_fail > 0)
        {
            if (g_when_pn_messenger_fail == g_current_pn_messenger_call)
            {
                messenger = NULL;
            }
            else
            {
                messenger = (pn_messenger_t*)BASEIMPLEMENTATION::gballoc_malloc(1);
            }
        }
        else
        {
            messenger = (pn_messenger_t*)BASEIMPLEMENTATION::gballoc_malloc(1);
        }
    MOCK_METHOD_END(pn_messenger_t*, messenger)

    MOCK_STATIC_METHOD_1(, int, pn_messenger_start, pn_messenger_t*, messsenger)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_2(, int, pn_messenger_put, pn_messenger_t*, messenger, pn_message_t*, msg)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_2(, int, pn_messenger_send, pn_messenger_t*, messenger, int, n)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_1(, int, pn_messenger_stop, pn_messenger_t*, messenger)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_1(, void, pn_messenger_free, pn_messenger_t*, messenger)
        free(messenger);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, void, pn_message_free, pn_message_t*, msg)
        free(msg);
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_2(, pn_bytes_t, pn_bytes, size_t, size, const char*, start)
    MOCK_METHOD_END(pn_bytes_t, test_pn_bytes)

    MOCK_STATIC_METHOD_2(, int, pn_messenger_set_blocking, pn_messenger_t*, messenger, bool, blocking)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_2(, int, pn_messenger_set_timeout, pn_messenger_t*, messenger, int, timeout)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_2(, int, pn_message_set_format, pn_message_t*, msg, uint32_t, format)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_2(, int, pn_messenger_set_outgoing_window, pn_messenger_t*, messenger, int, window)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_2(, pn_status_t, pn_messenger_status, pn_messenger_t*, messenger, pn_tracker_t, tracker)
    MOCK_METHOD_END(pn_status_t, PN_STATUS_ACCEPTED)

    MOCK_STATIC_METHOD_1(, pn_tracker_t, pn_messenger_outgoing_tracker, pn_messenger_t*, messenger)
    MOCK_METHOD_END(pn_tracker_t, 0)

    MOCK_STATIC_METHOD_3(, int, pn_messenger_settle, pn_messenger_t *, messenger, pn_tracker_t, tracker, int, flags)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_1(, void, pn_message_clear, pn_message_t*, msg)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, bool, pn_messenger_stopped, pn_messenger_t*, messenger)
    MOCK_METHOD_END(bool, true)

    MOCK_STATIC_METHOD_2(, int, pn_messenger_work, pn_messenger_t*, messenger, int, timeout)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_1(, pn_data_t*, pn_data, size_t, capacity)
    MOCK_METHOD_END(pn_data_t*, (pn_data_t*)BASEIMPLEMENTATION::gballoc_malloc(1))

    MOCK_STATIC_METHOD_1(, bool, pn_data_enter, pn_data_t*, data)
    MOCK_METHOD_END(bool, true)

    MOCK_STATIC_METHOD_1(, bool, pn_data_exit, pn_data_t*, data)
    MOCK_METHOD_END(bool, true)

    MOCK_STATIC_METHOD_1(, int, pn_data_put_map, pn_data_t*, data)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_2(, int, pn_data_put_string, pn_data_t*, data, pn_bytes_t, bytes)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_2(, int, pn_data_put_symbol, pn_data_t*, data, pn_bytes_t, bytes)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_1(, pn_data_t*, pn_message_annotations, pn_message_t*, msg)
    MOCK_METHOD_END(pn_data_t*, (pn_data_t*)TEST_PN_DATA)

    MOCK_STATIC_METHOD_1(, pn_data_t*, pn_message_properties, pn_message_t*, msg)
    MOCK_METHOD_END(pn_data_t*, (pn_data_t*)TEST_PN_DATA)

    MOCK_STATIC_METHOD_3(, ssize_t, pn_data_encode, pn_data_t *, data, char *, bytes, size_t, size)
    MOCK_METHOD_END(ssize_t, 10)

    MOCK_STATIC_METHOD_1(, void, pn_data_free, pn_data_t*, data);
        free(data);
    MOCK_VOID_METHOD_END()

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

    MOCK_STATIC_METHOD_4(, EVENTDATA_RESULT, EventData_GetPropertyByIndex, EVENTDATA_HANDLE, eventDataHandle, size_t, propertyIndex, const char**, propertyName, const char**, propertyValue)
        EVENTDATA_RESULT eventdataResult = EVENTDATA_MISSING_PROPERTY_NAME;
        if (g_setProperty)
        {
            *propertyName = PROPERTY_NAME;
            *propertyValue = TEST_STRING_VALUE;
            eventdataResult = EVENTDATA_OK;
        }
    MOCK_METHOD_END(EVENTDATA_RESULT, eventdataResult)

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

    MOCK_STATIC_METHOD_1(, size_t, EventData_GetPropertyCount, EVENTDATA_HANDLE, eventDataHandle)
    MOCK_METHOD_END(size_t, 0)

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

    /* Version Mocks */
    MOCK_STATIC_METHOD_0(, const char*, EventHubClient_GetVersionString)
    MOCK_METHOD_END(const char*, nullptr);

    /* URL_Encode Mocks */
    MOCK_STATIC_METHOD_1(, STRING_HANDLE, URL_Encode, STRING_HANDLE, input)
    MOCK_METHOD_END(STRING_HANDLE, TEST_ENCODED_STRING_HANDLE)

       
    /* STRING Mocks */
    MOCK_STATIC_METHOD_1(, STRING_HANDLE, STRING_clone, STRING_HANDLE, handle);
    MOCK_METHOD_END(STRING_HANDLE, TEST_STRING_HANDLE)

    MOCK_STATIC_METHOD_1(, STRING_HANDLE, STRING_construct, const char*, psz)
    MOCK_METHOD_END(STRING_HANDLE, TEST_STRING_HANDLE)

    MOCK_STATIC_METHOD_2(, int, STRING_concat, STRING_HANDLE, handle, const char*, s2)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_2(, int, STRING_concat_with_STRING, STRING_HANDLE, s1, STRING_HANDLE, s2)
    MOCK_METHOD_END(int, 0)

    MOCK_STATIC_METHOD_1(, void, STRING_delete, STRING_HANDLE, handle)
    MOCK_VOID_METHOD_END()

    MOCK_STATIC_METHOD_1(, const char*, STRING_c_str, STRING_HANDLE, s)
    MOCK_METHOD_END(const char*, TEST_CHAR)

    MOCK_STATIC_METHOD_0(, STRING_HANDLE, STRING_new)
    MOCK_METHOD_END(STRING_HANDLE, TEST_STRING_HANDLE);

    /* STRING_TOKENIZER Mocks */
    MOCK_STATIC_METHOD_1(, STRING_TOKENIZER_HANDLE, STRING_TOKENIZER_create, STRING_HANDLE, handle)
    MOCK_METHOD_END(STRING_TOKENIZER_HANDLE, TEST_STRING_TOKENIZER_HANDLE);

    MOCK_STATIC_METHOD_3(, int, STRING_TOKENIZER_get_next_token, STRING_TOKENIZER_HANDLE, t, STRING_HANDLE, output, const char*, delimiters)
    MOCK_METHOD_END(int, 0);

    MOCK_STATIC_METHOD_1(, void, STRING_TOKENIZER_destroy, STRING_TOKENIZER_HANDLE, handle)
    MOCK_VOID_METHOD_END()
    
    MOCK_STATIC_METHOD_0(, LOCK_HANDLE, Lock_Init)
        LOCK_HANDLE handle;
        if (g_lockInitFail)
        {
            handle = NULL;
        }
        else
        {
            LOCK_TEST_STRUCT* lockTest = (LOCK_TEST_STRUCT*)malloc(sizeof(LOCK_TEST_STRUCT) );
            handle = lockTest;
        }
    MOCK_METHOD_END(LOCK_HANDLE, handle);

    MOCK_STATIC_METHOD_1(, LOCK_RESULT, Lock, LOCK_HANDLE, handle)
        LOCK_RESULT lockResult;

        g_currentlock_call++;
        if (g_whenShalllock_fail > 0)
        {
            if (g_currentlock_call == g_whenShalllock_fail)
            {
                lockResult = LOCK_ERROR;
            }
            else
            {
                lockResult = LOCK_OK;
            }
        }
        else
        {
            lockResult = LOCK_OK;
        }
        if (lockResult == LOCK_OK && handle != NULL)
        {
            LOCK_TEST_STRUCT* lockTest = (LOCK_TEST_STRUCT*)handle;
            lockTest->dummy = (char*)malloc(1);
        }
    MOCK_METHOD_END(LOCK_RESULT, lockResult);
    MOCK_STATIC_METHOD_1(, LOCK_RESULT, Unlock, LOCK_HANDLE, handle)
        if (handle != NULL)
        {
            LOCK_TEST_STRUCT* lockTest = (LOCK_TEST_STRUCT*)handle;
            free(lockTest->dummy);
        }
    MOCK_METHOD_END(LOCK_RESULT, LOCK_OK);

    MOCK_STATIC_METHOD_1(, LOCK_RESULT, Lock_Deinit, LOCK_HANDLE, handle)
        free(handle);
    MOCK_METHOD_END(LOCK_RESULT, LOCK_OK);

    MOCK_STATIC_METHOD_2(, void, sendAsyncConfirmationCallback, EVENTHUBCLIENT_CONFIRMATION_RESULT, result2, void*, userContextCallback)
        EVENTHUBCLIENT_CONFIRMATION_RESULT* returnConfirm = (EVENTHUBCLIENT_CONFIRMATION_RESULT*)userContextCallback;
        *returnConfirm = result2;
    MOCK_VOID_METHOD_END()
};

DECLARE_GLOBAL_MOCK_METHOD_0(CEventHubClientLLMocks, , pn_message_t*, pn_message);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, pn_message_set_address, pn_message_t*, msg, const char*, address);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, pn_message_set_inferred, pn_message_t*, msg, bool, inferred);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , pn_data_t*, pn_message_body, pn_message_t*, msg);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, pn_data_put_binary, pn_data_t*, data, pn_bytes_t, bytes);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , pn_messenger_t*, pn_messenger, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , int, pn_messenger_start, pn_messenger_t*, messsenger);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, pn_messenger_put, pn_messenger_t*, messenger, pn_message_t*, msg);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, pn_messenger_send, pn_messenger_t*, messenger, int, n);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , int, pn_messenger_stop, pn_messenger_t*, messenger);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, pn_messenger_free, pn_messenger_t*, messenger);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, pn_message_free, pn_message_t*, msg);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , pn_bytes_t, pn_bytes, size_t, size, const char*, start);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, pn_messenger_set_blocking, pn_messenger_t*, messenger, bool, blocking);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, pn_messenger_set_timeout, pn_messenger_t*, messenger, int, timeout);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, pn_message_set_format, pn_message_t*, messenger, uint32_t, format);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , pn_data_t*, pn_data, size_t, capacity);
DECLARE_GLOBAL_MOCK_METHOD_3(CEventHubClientLLMocks, , ssize_t, pn_data_encode, pn_data_t *, data, char *, bytes, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, pn_data_free, pn_data_t*, data);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, pn_messenger_set_outgoing_window, pn_messenger_t*, messenger, int, window);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , pn_status_t, pn_messenger_status, pn_messenger_t*, messenger, pn_tracker_t, tracker);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , pn_tracker_t, pn_messenger_outgoing_tracker, pn_messenger_t*, messenger);
DECLARE_GLOBAL_MOCK_METHOD_3(CEventHubClientLLMocks, , int, pn_messenger_settle, pn_messenger_t *, messenger, pn_tracker_t, tracker, int, flags);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, pn_message_clear, pn_message_t*, msg);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , bool, pn_messenger_stopped, pn_messenger_t*, messenger);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, pn_messenger_work, pn_messenger_t*, messenger, int, timeout);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , bool, pn_data_enter, pn_data_t*, data);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , bool, pn_data_exit, pn_data_t*, data);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , int, pn_data_put_map, pn_data_t*, data);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, pn_data_put_string, pn_data_t*, data, pn_bytes_t, bytes);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, pn_data_put_symbol, pn_data_t*, data, pn_bytes_t, bytes);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , pn_data_t*, pn_message_annotations, pn_message_t*, msg);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , pn_data_t*, pn_message_properties, pn_message_t*, msg);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, DList_InitializeListHead, PDLIST_ENTRY, listHead);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , int, DList_IsListEmpty, PDLIST_ENTRY, listHead);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , void, DList_InsertTailList, PDLIST_ENTRY, listHead, PDLIST_ENTRY, listEntry);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , void, DList_AppendTailList, PDLIST_ENTRY, listHead, PDLIST_ENTRY, ListToAppend);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , int, DList_RemoveEntryList, PDLIST_ENTRY, listEntry);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , PDLIST_ENTRY, DList_RemoveHeadList, PDLIST_ENTRY, listHead);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , void*, gballoc_realloc, void*, ptr, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, gballoc_free, void*, ptr)

DECLARE_GLOBAL_MOCK_METHOD_0(CEventHubClientLLMocks, , LOCK_HANDLE, Lock_Init);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , LOCK_RESULT, Lock, LOCK_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , LOCK_RESULT, Unlock, LOCK_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , LOCK_RESULT, Lock_Deinit, LOCK_HANDLE, handle)


DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , EVENTDATA_HANDLE, EventData_CreateWithNewMemory, const unsigned char*, data, size_t, length);
DECLARE_GLOBAL_MOCK_METHOD_3(CEventHubClientLLMocks, , EVENTDATA_RESULT, EventData_GetData, EVENTDATA_HANDLE, eventDataHandle, const unsigned char**, data, size_t*, dataLength);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, EventData_Destroy, EVENTDATA_HANDLE, eventDataHandle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , const char*, EventData_GetPartitionKey, EVENTDATA_HANDLE, eventDataHandle);
DECLARE_GLOBAL_MOCK_METHOD_4(CEventHubClientLLMocks, , EVENTDATA_RESULT, EventData_GetPropertyByIndex, EVENTDATA_HANDLE, eventDataHandle, size_t, propertyIndex, const char**, propertyName, const char**, propertyValue)
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , size_t, EventData_GetPropertyCount, EVENTDATA_HANDLE, eventDataHandle);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , EVENTDATA_HANDLE, EventData_Clone, EVENTDATA_HANDLE, eventDataHandle);

DECLARE_GLOBAL_MOCK_METHOD_0(CEventHubClientLLMocks, , const char*, EventHubClient_GetVersionString);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , STRING_HANDLE, URL_Encode, STRING_HANDLE, input);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , STRING_HANDLE, STRING_construct, const char*, psz);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , STRING_HANDLE, STRING_clone, STRING_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, STRING_concat, STRING_HANDLE, handle, const char*, s2);
DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , int, STRING_concat_with_STRING, STRING_HANDLE, s1, STRING_HANDLE, s2);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, STRING_delete, STRING_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , const char*, STRING_c_str, STRING_HANDLE, s);
DECLARE_GLOBAL_MOCK_METHOD_0(CEventHubClientLLMocks, , STRING_HANDLE, STRING_new);

DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , STRING_TOKENIZER_HANDLE, STRING_TOKENIZER_create, STRING_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CEventHubClientLLMocks, , int, STRING_TOKENIZER_get_next_token, STRING_TOKENIZER_HANDLE, t, STRING_HANDLE, output, const char*, delimiters);
DECLARE_GLOBAL_MOCK_METHOD_1(CEventHubClientLLMocks, , void, STRING_TOKENIZER_destroy, STRING_TOKENIZER_HANDLE, handle);

DECLARE_GLOBAL_MOCK_METHOD_2(CEventHubClientLLMocks, , void, sendAsyncConfirmationCallback, EVENTHUBCLIENT_CONFIRMATION_RESULT, result2, void*, userContextCallback);


// ** End of Mocks **
static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

static void prepareCreateEventHubWithConnectionStringToPass(CNiceCallComparer<CEventHubClientLLMocks> &ehMocks)
{
    (void)ehMocks;
    EXPECTED_CALL(ehMocks, STRING_clone(IGNORED_PTR_ARG));

    EXPECTED_CALL(ehMocks, STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("servicebus.windows.net/");

    EXPECTED_CALL(ehMocks, STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("SharedAccessKeyName");

    EXPECTED_CALL(ehMocks, STRING_clone(IGNORED_PTR_ARG));

    EXPECTED_CALL(ehMocks, STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("SharedAccessKey");

    EXPECTED_CALL(ehMocks, STRING_clone(IGNORED_PTR_ARG));
}

static void prepareCreateEventHubWithConnectionStringToPassFullMock(CEventHubClientLLMocks &ehMocks)
{
    (void)ehMocks;
    EXPECTED_CALL(ehMocks, STRING_clone(IGNORED_PTR_ARG));

    EXPECTED_CALL(ehMocks, STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("servicebus.windows.net/");

    EXPECTED_CALL(ehMocks, STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("SharedAccessKeyName");

    EXPECTED_CALL(ehMocks, STRING_clone(IGNORED_PTR_ARG));

    EXPECTED_CALL(ehMocks, STRING_c_str(IGNORED_PTR_ARG))
        .SetReturn("SharedAccessKey");

    EXPECTED_CALL(ehMocks, STRING_clone(IGNORED_PTR_ARG));
}

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

        g_currentlock_call = 0;
        g_whenShalllock_fail = 0;
        g_lockInitFail = false;

        currentmalloc_call = 0;
        whenShallmalloc_fail = 0;
        g_setProperty = false;

        g_current_pn_messenger_call = 0;
        g_when_pn_messenger_fail = 0;

        g_currentEventClone_call = 0;
        g_whenShallEventClone_fail = 0;
        g_confirmationResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;
        g_whenEventDataGetData_fail = 0;
        g_currentEventDataGetData_call = 0;
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        if (!MicroMockReleaseMutex(g_testByTest))
        {
            ASSERT_FAIL("failure in test framework at ReleaseMutex");
        }
    }

    /*** EventHubClient_LL_CreateFromConnectionString ***/

    /* Tests_SRS_EVENTHUBCLIENT_05_001: [EventHubClient_LL_CreateFromConnectionString shall obtain the version string by a call to EventHubClient_GetVersionString.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_gets_the_version_string)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        EXPECTED_CALL(ehMocks, EventHubClient_GetVersionString());


        // act
        (void)EventHubClient_LL_CreateFromConnectionString(NULL, NULL);

        // assert
        // Implicit
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_003: [EventHubClient_ CreateFromConnectionString shall return a NULL value if connectionString or path is NULL.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_NULL_connectionString_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(NULL, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_003: [EventHubClient_ CreateFromConnectionString shall return a NULL value if connectionString or path is NULL.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_NULL_path_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString("connectionString", NULL);

        // assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_018: [EventHubClient_LL_CreateFromConnectionString shall return NULL if the connectionString format is invalid.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_Invalid_Formatted_connectionString_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
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
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);

        EXPECTED_CALL(ehMocks, DList_InitializeListHead(NULL));

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
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);

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
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
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
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
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
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        const char* connectionString = "ENdpoint=sb://servicebusName.servicebus.windows.net;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_missing_servicebusName_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        const char* connectionString = "Endpoint=sb://.servicebus.windows.net/;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_missing_servicebusDomain_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        const char* connectionString = "Endpoint=sb://servicebusName;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_missing_part_of_servicebusDomain_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.net;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_missing_SharedAccessKeyName_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.windows.net/;=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_missing_SharedAccessKeyNameValue_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.windows.net/;SharedAccessKeyName=;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_missing_SharedAccessKey_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.windows.net/;SharedAccessKeyName=RootManageSharedAccessKey;=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_missing_SharedAccessKeyValue_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.windows.net/;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_missing_delimiters_0_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.windows.netSharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_missing_delimiters_1_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.windows.net/;SharedAccessKeyNameRootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_noneAlphaNumericInHost_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servi(cebus.windows.net/;SharedAccessKeyNameRootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_in_different_order_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        const char* connectionString = "Endpoint=sb://SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8;servicebusName.servicebus.windows.net/;";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_connectString_differentDelimiter_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.windows.net(;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";

        // act
        EVENTHUBCLIENT_LL_HANDLE result = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(result);
    }

    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_doubleSlash_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
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
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;

        const STRING_HANDLE eventHubPathHandle = (STRING_HANDLE)0x04;

        prepareCreateEventHubWithConnectionStringToPass(ehMocks);
        

        EXPECTED_CALL(ehMocks, EventHubClient_GetVersionString());

        STRICT_EXPECTED_CALL(ehMocks, STRING_construct(EVENTHUB_PATH))
            .SetReturn(eventHubPathHandle);

        STRICT_EXPECTED_CALL(ehMocks, URL_Encode(TEST_STRING_HANDLE));
        STRICT_EXPECTED_CALL(ehMocks, URL_Encode(TEST_STRING_HANDLE));
        STRICT_EXPECTED_CALL(ehMocks, URL_Encode(TEST_STRING_HANDLE));
        STRICT_EXPECTED_CALL(ehMocks, URL_Encode(eventHubPathHandle));

        // act
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NOT_NULL(eventHubHandle);
        ehMocks.AssertActualAndExpectedCalls();

        // Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_021: [EventHubClient_ CreateFromConnectionString shall return EVENTHUBCLIENT_URL_ENCODING_FAILURE if any failure is encountered during the URL encoding.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_with_URL_Encode_1_Failing_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;

        STRICT_EXPECTED_CALL(ehMocks, URL_Encode(TEST_STRING_HANDLE))
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
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;

        STRICT_EXPECTED_CALL(ehMocks, URL_Encode(TEST_STRING_HANDLE))
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
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;

        prepareCreateEventHubWithConnectionStringToPass(ehMocks);

        STRICT_EXPECTED_CALL(ehMocks, URL_Encode(TEST_STRING_HANDLE));
        STRICT_EXPECTED_CALL(ehMocks, URL_Encode(TEST_STRING_HANDLE));
        STRICT_EXPECTED_CALL(ehMocks, URL_Encode(TEST_STRING_HANDLE))
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
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;

        const STRING_HANDLE eventHubPathHandle = (STRING_HANDLE)0x04;

        prepareCreateEventHubWithConnectionStringToPass(ehMocks);

        STRICT_EXPECTED_CALL(ehMocks, STRING_construct(EVENTHUB_PATH))
            .SetReturn(eventHubPathHandle);

        STRICT_EXPECTED_CALL(ehMocks, URL_Encode(TEST_STRING_HANDLE));
        STRICT_EXPECTED_CALL(ehMocks, URL_Encode(TEST_STRING_HANDLE));
        STRICT_EXPECTED_CALL(ehMocks, URL_Encode(TEST_STRING_HANDLE));
        STRICT_EXPECTED_CALL(ehMocks, URL_Encode(eventHubPathHandle))
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
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;

        prepareCreateEventHubWithConnectionStringToPass(ehMocks);


        STRICT_EXPECTED_CALL(ehMocks, pn_messenger(IGNORED_PTR_ARG));

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
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);

        STRICT_EXPECTED_CALL(ehMocks, pn_messenger(IGNORED_PTR_ARG));

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
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);


        STRICT_EXPECTED_CALL(ehMocks, pn_messenger(NULL));
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_set_outgoing_window(IGNORED_PTR_ARG, EXPECTED_OUTGOING_WINDOW_SIZE))
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
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);


        STRICT_EXPECTED_CALL(ehMocks, pn_messenger(NULL));
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_set_outgoing_window(IGNORED_PTR_ARG, EXPECTED_OUTGOING_WINDOW_SIZE))
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
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        const STRING_HANDLE eventHubPathHandle = (STRING_HANDLE)0x04;
        
        EXPECTED_CALL(ehMocks, EventHubClient_GetVersionString());

        prepareCreateEventHubWithConnectionStringToPass(ehMocks);


        STRICT_EXPECTED_CALL(ehMocks, STRING_construct(EVENTHUB_PATH))
            .SetReturn(eventHubPathHandle);

        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_start(NULL))
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
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        const STRING_HANDLE eventHubPathHandle = (STRING_HANDLE)0x04;

        prepareCreateEventHubWithConnectionStringToPass(ehMocks);

        STRICT_EXPECTED_CALL(ehMocks, STRING_construct(EVENTHUB_PATH))
            .SetReturn(eventHubPathHandle);

        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_set_blocking(IGNORED_PTR_ARG, false))
            .IgnoreArgument(1)
            .SetReturn(-1);
        
        // act
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(eventHubHandle);
        ehMocks.AssertActualAndExpectedCalls();

        // Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_004: [For all other errors, EventHubClient_LL_CreateFromConnectionString shall return NULL.] */
    TEST_FUNCTION(EventHubClient_LL_CreateFromConnectionString_invokes_pn_messenger_set_timeout_Fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        const STRING_HANDLE eventHubPathHandle = (STRING_HANDLE)0x04;

        EXPECTED_CALL(ehMocks, EventHubClient_GetVersionString());

        prepareCreateEventHubWithConnectionStringToPass(ehMocks);

        STRICT_EXPECTED_CALL(ehMocks, STRING_construct(EVENTHUB_PATH))
            .SetReturn(eventHubPathHandle);

        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_set_timeout(IGNORED_PTR_ARG, EXPECTED_TIMEOUT))
            .IgnoreArgument(1)
            .SetReturn(-1);

        // act
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        // assert
        ASSERT_IS_NULL(eventHubHandle);
        ehMocks.AssertActualAndExpectedCalls();

        // Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /*** EventHubClient_LL_SendAsync ***/
    /* Tests_SRS_EVENTHUBCLIENT_LL_04_011: [EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_INVALID_ARG if parameter eventHubClientLLHandle or eventDataHandle is NULL.] */
    TEST_FUNCTION(EventHubClient_LL_SendAsync_with_NULL_eventHubLLHandle_fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendAsync(NULL, TEST_EVENTDATA_HANDLE, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_011: [EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_INVALID_ARG if parameter eventHubClientLLHandle or eventDataHandle is NULL.] */
    TEST_FUNCTION(EventHubClient_LL_SendAsync_with_NULL_eventDataHandle_fails)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        ehMocks.ResetAllCalls();

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
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;


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
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        ehMocks.ResetAllCalls();

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
        CEventHubClientLLMocks ehMocks;
        auto dataEventHandle = (EVENTDATA_HANDLE)1;

        prepareCreateEventHubWithConnectionStringToPassFullMock(ehMocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        ehMocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(ehMocks, gballoc_malloc(IGNORE))
            .ExpectedTimesExactly(2)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(ehMocks, DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(ehMocks, EventData_Clone(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);
        ehMocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_014: [If cloning and/or adding the information fails for any reason, EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_ERROR.] */
    TEST_FUNCTION(EventHubClient_LL_SendAsync_AllocFail_fail)
    {
        ///arrange
        CEventHubClientLLMocks ehMocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.windows.net/;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";
        auto dataEventHandle = (EVENTDATA_HANDLE)1;
        
        prepareCreateEventHubWithConnectionStringToPassFullMock(ehMocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        ehMocks.ResetAllCalls();

        whenShallmalloc_fail = currentmalloc_call + 1;

        STRICT_EXPECTED_CALL(ehMocks, gballoc_malloc(IGNORE))
            .IgnoreArgument(1);

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
        ehMocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }
    

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_014: [If cloning and/or adding the information fails for any reason, EventHubClient_LL_SendAsync shall fail and return EVENTHUBCLIENT_ERROR.] */
    TEST_FUNCTION(EventHubClient_LL_SendAsync_EventData_Clonefail_fail)
    {
        ///arrange
        CEventHubClientLLMocks ehMocks;
        const char* connectionString = "Endpoint=sb://servicebusName.servicebus.windows.net/;SharedAccessKeyName=RootManageSharedAccessKey;SharedAccessKey=icT5kKsJr/Dw7oZveq7OPsSxu5Trmr6aiWlgI5zIT/8=";
        auto dataEventHandle = (EVENTDATA_HANDLE)1;

        prepareCreateEventHubWithConnectionStringToPassFullMock(ehMocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(connectionString, EVENTHUB_PATH);

        ehMocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(ehMocks, gballoc_malloc(IGNORE))
            .ExpectedTimesExactly(2)
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(ehMocks, EventData_Clone(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(ehMocks, gballoc_free(IGNORED_PTR_ARG))
            .ExpectedTimesExactly(2)
            .IgnoreArgument(1);

        // act
        g_whenShallEventClone_fail = 1;
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
        ehMocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }
    /*** EventHubClient_LL_Destroy ***/

    /* Tests_SRS_EVENTHUBCLIENT_03_010: [If the eventHubHandle is NULL, EventHubClient_LL_Destroy shall not do anything.] */
    TEST_FUNCTION(EventHubClient_LL_Destroy_with_NULL_eventHubHandle_Does_Nothing)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;

        // act
        EventHubClient_LL_Destroy(NULL);

        // assert
        // Implicit
    }

    /* Tests_SRS_EVENTHUBCLIENT_03_009: [EventHubClient_LL_Destroy shall terminate the usage of this EventHubClient specified by the eventHubHandle and cleanup all associated resources.] */
    TEST_FUNCTION(EventHubClient_LL_Destroy_with_valid_handle_Succeeds)
    {
        // arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        ASSERT_IS_NOT_NULL(eventHubHandle);

        EXPECTED_CALL(ehMocks, pn_messenger_free(IGNORED_PTR_ARG));

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
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_stop(NULL))
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
        CEventHubClientLLMocks ehMocks;
        //act
        EventHubClient_LL_DoWork(NULL);
        //assert
        ehMocks.AssertActualAndExpectedCalls();
    }

    /* Test_SRS_EVENTHUBCLIENT_LL_07_016: [If the total data size of the message exceeds the AMQP_MAX_MESSAGE_SIZE, then the message shall fail and the callback will be called with EVENTHUBCLIENT_CONFIRMATION_EXCEED_MAX_SIZE.] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_Exceed_PayloadSize)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        auto dataEventHandle = (EVENTDATA_HANDLE)1;

        g_whenEventDataGetData_fail = 1;
        g_confirmationResult = EVENTHUBCLIENT_CONFIRMATION_EXCEED_MAX_SIZE;
        EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, fakeCallBack, &g_confirmationResult);

        ehMocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_address(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_inferred(IGNORED_PTR_ARG, true))
            .IgnoreArgument(1);
        EXPECTED_CALL(ehMocks, EventData_GetData(dataEventHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        EventHubClient_LL_DoWork(eventHubHandle);

        //assert
        ehMocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Test_SRS_EVENTHUBCLIENT_LL_07_016: [If the total data size of the message exceeds the AMQP_MAX_MESSAGE_SIZE, then the message shall fail and the callback will be called with EVENTHUBCLIENT_CONFIRMATION_EXCEED_MAX_SIZE.] */
    /* Test_SRS_EVENTHUBCLIENT_LL_04_020: [If EventHubClient_LL_DoWork fails to create/put and get tracker of a proton message, it shall fail the payload and send the callback EVENTHUBCLIENT_CONFIRMATION_ERROR.] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_100_Send_With_Succeed)
    {
        const int NUMBER_OF_ITEMS = 100;
        size_t index = 0;
        CALLBACK_CONFIRM_INFO context;
        context.successCounter = 0;
        context.totalCalls = 0; // Ensure we get 100 calls

        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        for (index = 0; index < NUMBER_OF_ITEMS; index++)
        {
            auto dataEventHandle = (EVENTDATA_HANDLE)(index+1);
            EVENTHUBCLIENT_RESULT eventResult = EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, CounterCallBack, &context);
            ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, eventResult);
        }

        ehMocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_address(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_inferred(IGNORED_PTR_ARG, true))
            .IgnoreArgument(1);
        EXPECTED_CALL(ehMocks, EventData_GetData(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        for (index = 0; index <= NUMBER_OF_ITEMS && context.totalCalls < NUMBER_OF_ITEMS; index++)
        {
            EventHubClient_LL_DoWork(eventHubHandle); //At this point the message is marked as WAITFORACK
        }

        //assert
        ASSERT_ARE_EQUAL(int, NUMBER_OF_ITEMS, context.totalCalls);
        ASSERT_ARE_EQUAL(int, NUMBER_OF_ITEMS, context.successCounter);

        ehMocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Test_SRS_EVENTHUBCLIENT_LL_07_016: [If the total data size of the message exceeds the AMQP_MAX_MESSAGE_SIZE, then the message shall fail and the callback will be called with EVENTHUBCLIENT_CONFIRMATION_EXCEED_MAX_SIZE.] */
    /* Test_SRS_EVENTHUBCLIENT_LL_04_020: [If EventHubClient_LL_DoWork fails to create/put and get tracker of a proton message, it shall fail the payload and send the callback EVENTHUBCLIENT_CONFIRMATION_ERROR.] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_Multiple_Send_With_1_Failure_Succeed)
    {
        const int NUMBER_OF_ITEMS = 11;
        const int NUMBER_OF_ITEMS_FAIL = 1;
        size_t index = 0;
        CALLBACK_CONFIRM_INFO context;
        context.successCounter = 0;
        context.totalCalls = 0;

        g_whenEventDataGetData_fail = 3;

        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        for (index = 0; index < NUMBER_OF_ITEMS; index++)
        {
            auto dataEventHandle = (EVENTDATA_HANDLE)(index+1);
            EVENTHUBCLIENT_RESULT eventResult = EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, CounterCallBack, &context);
            ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, eventResult);
        }

        ehMocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_address(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_inferred(IGNORED_PTR_ARG, true))
            .IgnoreArgument(1);
        EXPECTED_CALL(ehMocks, EventData_GetData(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        for (index = 0; index <= NUMBER_OF_ITEMS && context.totalCalls < NUMBER_OF_ITEMS; index++)
        {
            EventHubClient_LL_DoWork(eventHubHandle); //At this point the message is marked as WAITFORACK
        }

        ASSERT_ARE_EQUAL(int, NUMBER_OF_ITEMS, context.totalCalls);
        ASSERT_ARE_EQUAL(int, NUMBER_OF_ITEMS-NUMBER_OF_ITEMS_FAIL, context.successCounter);

        //assert
        ehMocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    TEST_FUNCTION(EventHubClient_LL_DoWork_Encode_Failure)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        auto dataEventHandle = (EVENTDATA_HANDLE)1;

        g_confirmationResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;
        EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, fakeCallBack, &g_confirmationResult);

        ehMocks.ResetAllCalls();

        EXPECTED_CALL(ehMocks, pn_message_set_address(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_inferred(IGNORED_PTR_ARG, true))
            .IgnoreArgument(1);
        EXPECTED_CALL(ehMocks, pn_data_encode(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORE))
            .SetReturn(-1);

        //act
        EventHubClient_LL_DoWork(eventHubHandle);

        //assert
        ehMocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_019: [If the current status of the entry is WAITINGTOBESENT and there is available spots on proton, defined by OUTGOING_WINDOW_SIZE, EventHubClient_LL_DoWork shall call create pn_message and put the message into messenger by calling pn_messenger_put.]  */
    TEST_FUNCTION(EventHubClient_LL_DoWork_validEventHubClient_NoOutgoignMessages_Do_Not_Work)
    {
        //arrange
        CEventHubClientLLMocks ehMocks;
        prepareCreateEventHubWithConnectionStringToPassFullMock(ehMocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        ehMocks.ResetAllCalls();
        //act
        EventHubClient_LL_DoWork(eventHubHandle);
        //assert
        ehMocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_019: [If the current status of the entry is WAITINGTOBESENT and there is available spots on proton, defined by OUTGOING_WINDOW_SIZE, EventHubClient_LL_DoWork shall call create pn_message and put the message into messenger by calling pn_messenger_put.]  */
    /* Tests_SRS_EVENTHUBCLIENT_LL_04_021: [If there are message to be sent, EventHubClient_LL_DoWork shall call pn_messenger_send with parameter -1.] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_validEventHubClient_OneOutgoingMessages_Succeed)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        auto dataEventHandle = (EVENTDATA_HANDLE)1;

        EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, NULL, NULL);

        ehMocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_address(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_inferred(NULL, true))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_put(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_outgoing_tracker(NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_send(NULL, ALL_MESSAGE_IN_OUTGOING_QUEUE))
            .IgnoreArgument(1);

        //act
        EventHubClient_LL_DoWork(eventHubHandle);

        //assert
        ehMocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_022: [If the current status of the entry is WAITINGFORACK, than EventHubClient_LL_DoWork shall check status of this entry by calling pn_messenger_status.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_04_023: [If the status returned is PN_STATUS_PENDING EventHubClient_LL_DoWork shall do nothing.] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_validEventHubClient_OneOutgoignMessages_callDoWork2Times_getStatusPN_STATUS_PENDING_DoNothing)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        auto dataEventHandle = (EVENTDATA_HANDLE)1;

        EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, NULL, NULL);

        ehMocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_address(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_inferred(NULL, true))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_put(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_outgoing_tracker(NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_send(NULL, ALL_MESSAGE_IN_OUTGOING_QUEUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_status(NULL, IGNORE)) 
            .IgnoreAllArguments()
            .SetReturn((pn_status_t)PN_STATUS_PENDING);

        //act
        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message is marked as WAITFORACK

        EventHubClient_LL_DoWork(eventHubHandle);

        //assert
        ehMocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_025: [If the status returned is any other EventHubClient_LL_DoWork shall call the callback (if exist) with CONFIRMATION_RESULT value of EVENTHUBCLIENT_CONFIRMATION_ERROR and remove the event from the list.]*/
    /* Testst_SRS_EVENTHUBCLIENT_LL_04_026: [EventHubClient_LL_DoWork shall free the tracker before removing the event from the list by calling pn_messenger_settle] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_validEventHubClient_OneOutgoignMessages_callDoWork3Times_getStatusPN_STATUS_UNKNOWN_Remove_Message)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        auto dataEventHandle = (EVENTDATA_HANDLE)1;
        g_confirmationResult = EVENTHUBCLIENT_CONFIRMATION_UNKNOWN;

        EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, fakeCallBack, &g_confirmationResult);

        ehMocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_address(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_inferred(NULL, true))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_put(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_outgoing_tracker(NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_send(NULL, ALL_MESSAGE_IN_OUTGOING_QUEUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_status(NULL, IGNORE)) 
            .IgnoreAllArguments()
            .SetReturn((pn_status_t)PN_STATUS_UNKNOWN);

        //act
        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message is marked as WAITFORACK

        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message has to be removed from the list (Called Callback). 

        EventHubClient_LL_DoWork(eventHubHandle); //This Shall Do nothing.

        //assert
        ehMocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_025: [If the status returned is any other EventHubClient_LL_DoWork shall call the callback (if exist) with CONFIRMATION_RESULT value of EVENTHUBCLIENT_CONFIRMATION_ERROR and remove the event from the list.]*/
    /* Testst_SRS_EVENTHUBCLIENT_LL_04_026: [EventHubClient_LL_DoWork shall free the tracker before removing the event from the list by calling pn_messenger_settle] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_validEventHubClient_OneOutgoignMessages_callDoWork3Times_getStatusPN_STATUS_REJECTED_Remove_Message)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        auto dataEventHandle = (EVENTDATA_HANDLE)1;
        g_confirmationResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;

        EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, fakeCallBack, &g_confirmationResult);

        ehMocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_address(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_inferred(NULL, true))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_put(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_outgoing_tracker(NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_send(NULL, ALL_MESSAGE_IN_OUTGOING_QUEUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_status(NULL, IGNORE))
            .IgnoreAllArguments()
            .SetReturn((pn_status_t)PN_STATUS_REJECTED);
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_settle(NULL, IGNORE, 0))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        //act
        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message is marked as WAITFORACK

        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message has to be removed from the list (Called Callback). 

        EventHubClient_LL_DoWork(eventHubHandle); //This Shall Do nothing.

        //assert
        ehMocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_025: [If the status returned is any other EventHubClient_LL_DoWork shall call the callback (if exist) with CONFIRMATION_RESULT value of EVENTHUBCLIENT_CONFIRMATION_ERROR and remove the event from the list.]*/
    /* Testst_SRS_EVENTHUBCLIENT_LL_04_026: [EventHubClient_LL_DoWork shall free the tracker before removing the event from the list by calling pn_messenger_settle] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_validEventHubClient_OneOutgoignMessages_callDoWork3Times_getStatusPN_STATUS_ABORTED_Remove_Message)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        auto dataEventHandle = (EVENTDATA_HANDLE)1;
        g_confirmationResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;

        EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, fakeCallBack, &g_confirmationResult);

        ehMocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_address(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_inferred(NULL, true))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_put(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_outgoing_tracker(NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_send(NULL, ALL_MESSAGE_IN_OUTGOING_QUEUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_status(NULL, IGNORE))
            .IgnoreAllArguments()
            .SetReturn((pn_status_t)PN_STATUS_ABORTED);
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_settle(NULL, IGNORE, 0))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        //act
        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message is marked as WAITFORACK

        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message has to be removed from the list (Called Callback). 

        EventHubClient_LL_DoWork(eventHubHandle); //This Shall Do nothing.

        //assert
        ehMocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_04_024: [If the status returned is PN_STATUS_ACCEPTED EventHubClient_LL_DoWork shall call the call back (if exist) with CONFIRMATION_RESULT value of EVENTHUBCLIENT_CONFIRMATION_OK and remove the event from the list.]*/
    /* Testst_SRS_EVENTHUBCLIENT_LL_04_026: [EventHubClient_LL_DoWork shall free the tracker before removing the event from the list by calling pn_messenger_settle] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_validEventHubClient_OneOutgoignMessages_callDoWork3Times_getStatusPN_STATUS_ACCEPTED_CallCallback_Remove_Message)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        auto dataEventHandle = (EVENTDATA_HANDLE)1;

        g_confirmationResult = EVENTHUBCLIENT_CONFIRMATION_OK;

        EventHubClient_LL_SendAsync(eventHubHandle, dataEventHandle, fakeCallBack, &g_confirmationResult);

        ehMocks.ResetAllCalls();

        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_address(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_inferred(NULL, true))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_put(NULL, NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_outgoing_tracker(NULL))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_send(NULL, ALL_MESSAGE_IN_OUTGOING_QUEUE))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_status(NULL, IGNORE))
            .IgnoreAllArguments()
            .SetReturn((pn_status_t)PN_STATUS_ACCEPTED);
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_settle(NULL, IGNORE, 0))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        //act
        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message is marked as WAITFORACK

        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message has to be removed from the list (Called Callback). 

        EventHubClient_LL_DoWork(eventHubHandle); //This Shall Do nothing.

        //assert
        ehMocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_015: [On success EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_OK.] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_BatchSendAsync_SUCCEED)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        EVENTHUBCLIENT_RESULT result;
        EVENTDATA_HANDLE eventhandleList[3];

        eventhandleList[0] = (EVENTDATA_HANDLE)1;
        eventhandleList[1] = (EVENTDATA_HANDLE)2;
        eventhandleList[2] = (EVENTDATA_HANDLE)4;

        EVENTHUBCLIENT_CONFIRMATION_RESULT sendBatchAsyncResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;

        result = EventHubClient_LL_SendBatchAsync(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT, sendAsyncConfirmationCallback, &sendBatchAsyncResult);
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);

        ehMocks.ResetAllCalls();

        EXPECTED_CALL(ehMocks, pn_message_set_address(NULL, NULL));
        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_inferred(NULL, true))
            .IgnoreArgument(1);
        EXPECTED_CALL(ehMocks, pn_messenger_put(NULL, NULL));
        EXPECTED_CALL(ehMocks, pn_messenger_outgoing_tracker(NULL));
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_send(NULL, ALL_MESSAGE_IN_OUTGOING_QUEUE))
            .IgnoreArgument(1);
        EXPECTED_CALL(ehMocks, pn_messenger_status(NULL, IGNORE))
            .SetReturn((pn_status_t)PN_STATUS_ACCEPTED);
        EXPECTED_CALL(ehMocks, pn_messenger_settle(NULL, IGNORE, 0));

        //act
        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message is marked as WAITFORACK

        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message has to be removed from the list (Called Callback). 

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_CONFIRMATION_RESULT, EVENTHUBCLIENT_CONFIRMATION_OK, sendBatchAsyncResult);

        ehMocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_015: [On success EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_OK.] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_BatchSendAsync_With_PartitionKey_SUCCEED)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        EVENTHUBCLIENT_RESULT result;
        EVENTDATA_HANDLE eventhandleList[EVENT_HANDLE_COUNT];

        eventhandleList[0] = (EVENTDATA_HANDLE)1;
        eventhandleList[1] = (EVENTDATA_HANDLE)2;
        eventhandleList[2] = (EVENTDATA_HANDLE)4;

        EVENTHUBCLIENT_CONFIRMATION_RESULT expectedResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;

        result = EventHubClient_LL_SendBatchAsync(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT, sendAsyncConfirmationCallback, &expectedResult);
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);

        ehMocks.ResetAllCalls();

        EXPECTED_CALL(ehMocks, pn_message_set_address(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_inferred(IGNORED_PTR_ARG, true))
            .IgnoreArgument(1);
        EXPECTED_CALL(ehMocks, pn_messenger_put(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        EXPECTED_CALL(ehMocks, pn_messenger_outgoing_tracker(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_send(IGNORED_PTR_ARG, ALL_MESSAGE_IN_OUTGOING_QUEUE))
            .IgnoreArgument(1);
        EXPECTED_CALL(ehMocks, pn_messenger_status(IGNORED_PTR_ARG, IGNORE))
            .SetReturn((pn_status_t)PN_STATUS_ACCEPTED);
        EXPECTED_CALL(ehMocks, pn_messenger_settle(IGNORED_PTR_ARG, IGNORE, 0));
        EXPECTED_CALL(ehMocks, EventData_GetPartitionKey(IGNORED_PTR_ARG))
            .SetReturn(PARTITION_KEY_VALUE);
        EXPECTED_CALL(ehMocks, pn_message_annotations(IGNORED_PTR_ARG));
        EXPECTED_CALL(ehMocks, pn_data_put_map(IGNORED_PTR_ARG));
        EXPECTED_CALL(ehMocks, pn_data_enter(IGNORED_PTR_ARG));
        EXPECTED_CALL(ehMocks, pn_bytes(IGNORE, IGNORED_PTR_ARG))
            .ExpectedTimesExactly(5);

        //act
        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message is marked as WAITFORACK

        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message has to be removed from the list (Called Callback). 

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_CONFIRMATION_RESULT, EVENTHUBCLIENT_CONFIRMATION_OK, expectedResult);

        ehMocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_015: [On success EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_OK.] */
    TEST_FUNCTION(EventHubClient_LL_DoWork_BatchSend_With_Property_SUCCEED)
    {
        //arrange
        CNiceCallComparer<CEventHubClientLLMocks> ehMocks;
        prepareCreateEventHubWithConnectionStringToPass(ehMocks);
        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);
        EVENTHUBCLIENT_RESULT result;
        EVENTDATA_HANDLE eventhandleList[EVENT_HANDLE_COUNT];

        eventhandleList[0] = (EVENTDATA_HANDLE)1;
        eventhandleList[1] = (EVENTDATA_HANDLE)2;
        eventhandleList[2] = (EVENTDATA_HANDLE)4;

        EVENTHUBCLIENT_CONFIRMATION_RESULT expectedResult = EVENTHUBCLIENT_CONFIRMATION_ERROR;

        result = EventHubClient_LL_SendBatchAsync(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT, sendAsyncConfirmationCallback, &expectedResult);
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);

        ehMocks.ResetAllCalls();

        EXPECTED_CALL(ehMocks, pn_message_set_address(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(ehMocks, pn_message_set_inferred(IGNORED_PTR_ARG, true))
            .IgnoreArgument(1);
        EXPECTED_CALL(ehMocks, pn_messenger_put(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        EXPECTED_CALL(ehMocks, pn_messenger_outgoing_tracker(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(ehMocks, pn_messenger_send(IGNORED_PTR_ARG, ALL_MESSAGE_IN_OUTGOING_QUEUE))
            .IgnoreArgument(1);
        EXPECTED_CALL(ehMocks, pn_messenger_status(IGNORED_PTR_ARG, IGNORE))
            .SetReturn((pn_status_t)PN_STATUS_ACCEPTED);
        EXPECTED_CALL(ehMocks, pn_messenger_settle(IGNORED_PTR_ARG, IGNORE, 0));
        EXPECTED_CALL(ehMocks, EventData_GetPropertyCount(IGNORED_PTR_ARG))
            .SetReturn(1);
        EXPECTED_CALL(ehMocks, EventData_GetPropertyByIndex(IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(ehMocks, pn_data(0));
        EXPECTED_CALL(ehMocks, pn_data_put_map(IGNORED_PTR_ARG));
        EXPECTED_CALL(ehMocks, pn_data_enter(IGNORED_PTR_ARG));
        EXPECTED_CALL(ehMocks, pn_bytes(IGNORE, IGNORED_PTR_ARG))
            .ExpectedTimesExactly(5);
        
        g_setProperty = true;

        //act
        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message is marked as WAITFORACK

        EventHubClient_LL_DoWork(eventHubHandle); //At this point the message has to be removed from the list (Called Callback). 

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_CONFIRMATION_RESULT, EVENTHUBCLIENT_CONFIRMATION_OK, expectedResult);

        ehMocks.AssertActualAndExpectedCalls();

        //Cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_015: [On success EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_OK.] */
    /* Tests_SRS_EVENTHUBCLIENT_LL_07_014: [EventHubClient_LL_SendBatchAsync shall clone each item in the eventDataList and by calling EVENTDATA_Clone.] */
    TEST_FUNCTION(EventHubClient_LL_SendBatchAsync_SUCCEED)
    {
        ///arrange
        CEventHubClientLLMocks ehMocks;
        EVENTDATA_HANDLE eventhandleList[3];
        EVENTHUBCLIENT_CONFIRMATION_RESULT confirmationResult;

        eventhandleList[0] = (EVENTDATA_HANDLE)1;
        eventhandleList[1] = (EVENTDATA_HANDLE)2;
        eventhandleList[2] = (EVENTDATA_HANDLE)4;

        prepareCreateEventHubWithConnectionStringToPassFullMock(ehMocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        ehMocks.ResetAllCalls();

        EXPECTED_CALL(ehMocks, gballoc_malloc(IGNORE))
            .ExpectedTimesExactly(2);
        EXPECTED_CALL(ehMocks, DList_InsertTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        EXPECTED_CALL(ehMocks, EventData_GetPartitionKey(IGNORED_PTR_ARG))
            .ExpectedTimesExactly(EVENT_HANDLE_COUNT);
        EXPECTED_CALL(ehMocks, EventData_Clone(IGNORED_PTR_ARG))
            .ExpectedTimesExactly(EVENT_HANDLE_COUNT);

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT, sendAsyncConfirmationCallback, &confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_OK, result);
        ehMocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_012: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_INVALLID_ARG if eventhubClientLLHandle or eventDataList are NULL or if sendAsnycConfirmationCallback equals NULL and userContextCallback does not equal NULL.] */
    TEST_FUNCTION(EventHubClient_LL_SendBatchAsync_EVENTHUBHANDLE_NULL_Fail)
    {
        ///arrange
        CEventHubClientLLMocks ehMocks;
        EVENTDATA_HANDLE eventhandleList[3];
        eventhandleList[0] = (EVENTDATA_HANDLE)1;
        eventhandleList[1] = (EVENTDATA_HANDLE)2;
        eventhandleList[2] = (EVENTDATA_HANDLE)4;

        ehMocks.ResetAllCalls();

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(NULL, eventhandleList, EVENT_HANDLE_COUNT, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
        ehMocks.AssertActualAndExpectedCalls();

        //cleanup
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_012: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_INVALID_ARG if eventhubClientLLHandle or eventDataList are NULL or if sendAsnycConfirmationCallback equals NULL and userContextCallback does not equal NULL.] */
    TEST_FUNCTION(EventHubClient_LL_SendBatchAsync_EVENTDATA_List_NULL_Fail)
    {
        ///arrange
        CEventHubClientLLMocks ehMocks;

        prepareCreateEventHubWithConnectionStringToPassFullMock(ehMocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        ehMocks.ResetAllCalls();

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, NULL, EVENT_HANDLE_COUNT, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
        ehMocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_012: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_INVALID_ARG if eventhubClientLLHandle or eventDataList are NULL or if sendAsnycConfirmationCallback equals NULL and userContextCallback does not equal NULL.] */
    TEST_FUNCTION(EventHubClient_LL_SendBatchAsync_Count_0_Fail)
    {
        ///arrange
        CEventHubClientLLMocks ehMocks;
        EVENTDATA_HANDLE eventhandleList[3];

        eventhandleList[0] = (EVENTDATA_HANDLE)1;
        eventhandleList[1] = (EVENTDATA_HANDLE)2;
        eventhandleList[2] = (EVENTDATA_HANDLE)4;

        prepareCreateEventHubWithConnectionStringToPassFullMock(ehMocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        ehMocks.ResetAllCalls();

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, eventhandleList, 0, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
        ehMocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_012: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_INVALID_ARG if eventhubClientLLHandle or eventDataList are NULL or if sendAsnycConfirmationCallback equals NULL and userContextCallback does not equal NULL.] */
    TEST_FUNCTION(EventHubClient_LL_SendBatchAsync_sendAsyncConfirmationCallback_NULL_userContext_NonNULL_Fail)
    {
        ///arrange
        CEventHubClientLLMocks ehMocks;
        EVENTDATA_HANDLE eventhandleList[3];

        eventhandleList[0] = (EVENTDATA_HANDLE)1;
        eventhandleList[1] = (EVENTDATA_HANDLE)2;
        eventhandleList[2] = (EVENTDATA_HANDLE)4;

        prepareCreateEventHubWithConnectionStringToPassFullMock(ehMocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        ehMocks.ResetAllCalls();

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT, NULL, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_INVALID_ARG, result);
        ehMocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_013: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_ERROR for any Error that is encountered.] */
    TEST_FUNCTION(EventHubClient_LL_SendBatchAsync_EVENTDATAT_HANDLE_NULL_Fail)
    {
        ///arrange
        CEventHubClientLLMocks ehMocks;
        EVENTDATA_HANDLE eventhandleList[3];

        eventhandleList[0] = (EVENTDATA_HANDLE)1;
        eventhandleList[1] = (EVENTDATA_HANDLE)2;
        eventhandleList[2] = (EVENTDATA_HANDLE)NULL;

        prepareCreateEventHubWithConnectionStringToPassFullMock(ehMocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        ehMocks.ResetAllCalls();

        EXPECTED_CALL(ehMocks, EventData_GetPartitionKey(IGNORED_PTR_ARG))
            .ExpectedTimesExactly(2);

        // act
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
        ehMocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

    /* Tests_SRS_EVENTHUBCLIENT_LL_07_013: [EventHubClient_LL_SendBatchAsync shall return EVENTHUBCLIENT_ERROR for any Error that is encountered.] */
    TEST_FUNCTION(EventHubClient_LL_SendBatchAsync_EVENTDATA_Clone_NULL_Fail)
    {
        ///arrange
        CEventHubClientLLMocks ehMocks;
        EVENTDATA_HANDLE eventhandleList[3];

        eventhandleList[0] = (EVENTDATA_HANDLE)1;
        eventhandleList[1] = (EVENTDATA_HANDLE)2;
        eventhandleList[2] = (EVENTDATA_HANDLE)4;

        prepareCreateEventHubWithConnectionStringToPassFullMock(ehMocks);

        EVENTHUBCLIENT_LL_HANDLE eventHubHandle = EventHubClient_LL_CreateFromConnectionString(CONNECTION_STRING, EVENTHUB_PATH);

        ehMocks.ResetAllCalls();

        EXPECTED_CALL(ehMocks, gballoc_malloc(IGNORE))
            .ExpectedTimesExactly(2);
        EXPECTED_CALL(ehMocks, EventData_GetPartitionKey(IGNORED_PTR_ARG))
            .ExpectedTimesExactly(EVENT_HANDLE_COUNT);
        EXPECTED_CALL(ehMocks, EventData_Clone(IGNORED_PTR_ARG))
            .ExpectedTimesExactly(2);
        EXPECTED_CALL(ehMocks, EventData_Destroy(IGNORED_PTR_ARG));
        EXPECTED_CALL(ehMocks, gballoc_free(IGNORE))
            .ExpectedTimesExactly(2);
        
        // act
        g_whenShallEventClone_fail = 2;
        EVENTHUBCLIENT_RESULT result = EventHubClient_LL_SendBatchAsync(eventHubHandle, eventhandleList, EVENT_HANDLE_COUNT, sendAsyncConfirmationCallback, &g_confirmationResult);

        //assert
        ASSERT_ARE_EQUAL(EVENTHUBCLIENT_RESULT, EVENTHUBCLIENT_ERROR, result);
        ehMocks.AssertActualAndExpectedCalls();

        //cleanup
        EventHubClient_LL_Destroy(eventHubHandle);
    }

END_TEST_SUITE(eventhubclient_ll_unittests)
