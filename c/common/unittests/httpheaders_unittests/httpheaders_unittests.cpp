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

#include "testrunnerswitcher.h"
#include "micromock.h"

#include "httpheaders.h"

#include <climits>

/*Below tags exists for traceability reasons only, they canot be really tested by automated means, except "this file compiles"*/
/*Tests_SRS_HTTP_HEADERS_99_001:[ HttpHeaders shall have the following interface]*/

static MICROMOCK_MUTEX_HANDLE g_testByTest;

DEFINE_MICROMOCK_ENUM_TO_STRING(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT_VALUES);

/*test assets*/
#define NAME1 "name1"
#define VALUE1 "value1"
#define HEADER1 NAME1 ": " VALUE1
const char *NAME1_TRICK1 = "name1:";
const char *NAME1_TRICK2 = "name1: ";
const char *NAME1_TRICK3 = "name1: value1";

#define NAME2 "name2"
#define VALUE2 "value2"
#define HEADER2 NAME2 ": " VALUE2

#define TEMP_BUFFER_SIZE 1024
char tempBuffer[TEMP_BUFFER_SIZE];

#define MAX_NAME_VALUE_PAIR 100

static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

BEGIN_TEST_SUITE(HTTPHeaders_UnitTests)

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
        }

        TEST_FUNCTION_CLEANUP(TestMethodCleanup)
        {
            if (!MicroMockReleaseMutex(g_testByTest))
            {
                ASSERT_FAIL("failure in test framework at ReleaseMutex");
            }
        }

        /*Tests_SRS_HTTP_HEADERS_99_003:[ The function shall return NULL when the function cannot execute properly]*/
        TEST_FUNCTION(HTTPHeaders_Alloc_fails_when_malloc_fails)
        {
            /*TODO when malloc is sim'd*/
            ///arrange

            ///act

            ///assert
            
        }

        /*Tests_SRS_HTTP_HEADERS_99_004:[ After a successful init, HTTPHeaders_GetHeaderCount shall report 0 existing headers.]*/
        TEST_FUNCTION(HTTPHeaders_Alloc_succeeds_and_GetHeaderCount_returns_0)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();

            ///act
            size_t nHeaders;
            auto res = HTTPHeaders_GetHeaderCount(httpHandle, &nHeaders);

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_OK, res);
            ASSERT_ARE_EQUAL(size_t, (size_t)0, nHeaders);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_012:[ Calling this API shall record a header from name and value parameters.]*/
        /*Tests_SRS_HTTP_HEADERS_99_013:[ The function shall return HTTP_HEADERS_OK when execution is successful.]*/
        /*Tests_SRS_HTTP_HEADERS_99_016:[ The function shall store the name:value pair in such a way that when later retrieved by a call to GetHeader it will return a string that shall strcmp equal to the name+": "+value.]*/
        TEST_FUNCTION(HTTPHeaders_AddHeaderNameValuePair_succeeds)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();

            ///act
            auto res = HTTPHeaders_AddHeaderNameValuePair(httpHandle, NAME1, VALUE1);

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_OK, res);

            //checking content
            size_t nHeaders;
            (void)HTTPHeaders_GetHeaderCount(httpHandle, &nHeaders);
            ASSERT_ARE_EQUAL(size_t, (size_t)1, nHeaders);
            (void)HTTPHeaders_GetHeader(httpHandle, 0, tempBuffer, TEMP_BUFFER_SIZE);
            ASSERT_ARE_EQUAL(char_ptr, (std::string(NAME1)+": "+VALUE1).c_str(), tempBuffer);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_014:[ The function shall return when the handle is not valid or when name parameter is NULL or when value parameter is NULL.]*/
        TEST_FUNCTION(HTTPHeaders_AddHeaderNameValuePair_with_NULL_handle_fails)
        {
            ///arrange

            ///act
            auto res = HTTPHeaders_AddHeaderNameValuePair(NULL, NAME1, VALUE1);

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_INVALID_ARG, res);
        }

        /*Tests_SRS_HTTP_HEADERS_99_014:[ The function shall return when the handle is not valid or when name parameter is NULL or when value parameter is NULL.]*/
        TEST_FUNCTION(HTTPHeaders_AddHeaderNameValuePair_with_NULL_name_fails)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();

            ///act
            auto res = HTTPHeaders_AddHeaderNameValuePair(httpHandle, NULL, VALUE1);

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_INVALID_ARG, res);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_014:[ The function shall return when the handle is not valid or when name parameter is NULL or when value parameter is NULL.]*/
        TEST_FUNCTION(HTTPHeaders_AddHeaderNameValuePair_with_NULL_value_fails)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();

            ///act
            auto res = HTTPHeaders_AddHeaderNameValuePair(httpHandle, NAME1, NULL);

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_INVALID_ARG, res);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_017:[ If the name already exists in the collection of headers, the function shall concatenate the new value after the existing value, separated by a comma and a space as in: old-value+", "+new-value.]*/
        TEST_FUNCTION(HTTPHeaders_AddHeaderNameValuePair_with_same_Name_appends_to_existing_value_succeeds)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();
            (void)HTTPHeaders_AddHeaderNameValuePair(httpHandle, NAME1, VALUE1);

            ///act
            auto res = HTTPHeaders_AddHeaderNameValuePair(httpHandle, NAME1, VALUE1);

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_OK, res);

            //checking content
            size_t nHeaders;
            (void)HTTPHeaders_GetHeaderCount(httpHandle, &nHeaders);
            ASSERT_ARE_EQUAL(size_t, 1U, nHeaders);
            (void)HTTPHeaders_GetHeader(httpHandle, 0, tempBuffer, TEMP_BUFFER_SIZE);
            ASSERT_ARE_EQUAL(char_ptr, (std::string(NAME1) + ": " + VALUE1 + ", " + VALUE1).c_str(), tempBuffer);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_012:[ Calling this API shall record a header from name and value parameters.]*/
        TEST_FUNCTION(HTTPHeaders_AddHeaderNameValuePair_add_two_headers_produces_two_headers)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();
            (void)HTTPHeaders_AddHeaderNameValuePair(httpHandle, NAME1, VALUE1);

            ///act
            auto res = HTTPHeaders_AddHeaderNameValuePair(httpHandle, NAME2, VALUE2);

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_OK, res);

            size_t nHeaders;
            bool foundHeader1 = false, foundHeader2=false;
            (void)HTTPHeaders_GetHeaderCount(httpHandle, &nHeaders);

            for(size_t i=0;i<nHeaders;i++)
            {
                (void)HTTPHeaders_GetHeader(httpHandle, i, tempBuffer, TEMP_BUFFER_SIZE);
                if (strcmp(HEADER1, tempBuffer) == 0)
                {
                    foundHeader1 = true;
                }

                if(strcmp(HEADER2,tempBuffer)==0)
                {
                    foundHeader2 = true;
                }
            }

            ASSERT_IS_TRUE(foundHeader1);
            ASSERT_IS_TRUE(foundHeader2);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_017:[ If the name already exists in the collection of headers, the function shall concatenate the new value after the existing value, separated by a comma and a space as in: old-value+", "+new-value.]*/
        TEST_FUNCTION(HTTPHeaders_When_Second_Added_Header_Is_A_Substring_Of_An_Existing_Header_2_Headers_Are_Added)
        {
            ///arrange
            HTTP_HEADERS_HANDLE httpHandle = HTTPHeaders_Alloc();
            (void)HTTPHeaders_AddHeaderNameValuePair(httpHandle, "ab", VALUE1);

            ///act
            HTTP_HEADERS_RESULT result = HTTPHeaders_AddHeaderNameValuePair(httpHandle, "a", VALUE1);

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_OK, result);

            //checking content
            size_t nHeaders;
            (void)HTTPHeaders_GetHeaderCount(httpHandle, &nHeaders);
            ASSERT_ARE_EQUAL(size_t, 2U, nHeaders);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_022:[ The return value shall be NULL if name parameter is NULL or if httpHeadersHandle is NULL]*/
        TEST_FUNCTION(HTTPHeaders_FindHeaderValue_with_NULL_handle_returns_NULL)
        {
            ///arrange

            ///act
            auto res = HTTPHeaders_FindHeaderValue(NULL, NAME1);

            ///assert
            ASSERT_IS_NULL(res);
        }

        /*Tests_SRS_HTTP_HEADERS_99_022:[ The return value shall be NULL if name parameter is NULL or if httpHeadersHandle is NULL]*/
        TEST_FUNCTION(HTTPHeaders_FindHeaderValue_with_NULL_name_returns_NULL)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();

            ///act
            auto res = HTTPHeaders_FindHeaderValue(httpHandle, NULL);

            ///assert
            ASSERT_IS_NULL(res);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_018:[ Calling this API shall retrieve the value for a previously stored name.]*/
        /*Tests_SRS_HTTP_HEADERS_99_021:[ In this case the return value shall point to a string that shall strcmp equal to the original stored string.]*/
        TEST_FUNCTION(HTTPHeaders_FindHeaderValue_retrieves_previously_stored_value_succeeds)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();
            (void)HTTPHeaders_AddHeaderNameValuePair(httpHandle, NAME1, VALUE1);

            ///act
            auto res1 = HTTPHeaders_FindHeaderValue(httpHandle, NAME1);

            ///assert
            ASSERT_ARE_EQUAL(char_ptr, VALUE1, res1);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_018:[ Calling this API shall retrieve the value for a previously stored name.]*/
        /*Tests_SRS_HTTP_HEADERS_99_021:[ In this case the return value shall point to a string that shall strcmp equal to the original stored string.]*/
        TEST_FUNCTION(HTTPHeaders_FindHeaderValue_retrieves_previously_stored_value_for_two_headers_succeeds)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();
            (void)HTTPHeaders_AddHeaderNameValuePair(httpHandle, NAME1, VALUE1);
            (void)HTTPHeaders_AddHeaderNameValuePair(httpHandle, NAME2, VALUE2);

            ///act
            auto res1 = HTTPHeaders_FindHeaderValue(httpHandle, NAME1);
            auto res2 = HTTPHeaders_FindHeaderValue(httpHandle, NAME2);

            ///assert
            ASSERT_ARE_EQUAL(char_ptr, VALUE1, res1);
            ASSERT_ARE_EQUAL(char_ptr, VALUE2, res2);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_018:[ Calling this API shall retrieve the value for a previously stored name.]*/
        /*Tests_SRS_HTTP_HEADERS_99_021:[ In this case the return value shall point to a string that shall strcmp equal to the original stored string.]*/
        TEST_FUNCTION(HTTPHeaders_FindHeaderValue_retrieves_concatenation_of_previously_stored_values_for_header_name_succeeds)
        {
            auto httpHandle = HTTPHeaders_Alloc();
            (void)HTTPHeaders_AddHeaderNameValuePair(httpHandle, NAME1, VALUE1);
            (void)HTTPHeaders_AddHeaderNameValuePair(httpHandle, NAME1, VALUE2);

            ///act
            auto res = HTTPHeaders_FindHeaderValue(httpHandle, NAME1);

            ///assert
            ASSERT_ARE_EQUAL(char_ptr, (std::string(VALUE1) + ", " + VALUE2).c_str(), res);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_020:[ The return value shall be different than NULL when the name matches the name of a previously stored name:value pair.]*/
        /*actually we are trying to see that finding a nonexisting value produces NULL*/
        TEST_FUNCTION(HTTPHeaders_FindHeaderValue_returns_NULL_for_nonexistent_value)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();
            (void)HTTPHeaders_AddHeaderNameValuePair(httpHandle, NAME1, VALUE1);

            ///act
            auto res2 = HTTPHeaders_FindHeaderValue(httpHandle, NAME2);

            ///assert
            ASSERT_IS_NULL(res2);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*trying to catch some errors here*/
        /*Tests_SRS_HTTP_HEADERS_99_020:[ The return value shall be different than NULL when the name matches the name of a previously stored name:value pair.]*/
        TEST_FUNCTION(HTTPHeaders_FindHeaderValue_with_nonexistent_header_succeeds)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();
            (void)HTTPHeaders_AddHeaderNameValuePair(httpHandle, NAME1, VALUE1);

            ///act
            auto res1 = HTTPHeaders_FindHeaderValue(httpHandle, NAME1_TRICK1);
            auto res2 = HTTPHeaders_FindHeaderValue(httpHandle, NAME1_TRICK2);
            auto res3 = HTTPHeaders_FindHeaderValue(httpHandle, NAME1_TRICK3);

            ///assert
            ASSERT_IS_NULL(res1);
            ASSERT_IS_NULL(res2);
            ASSERT_IS_NULL(res3);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /* Tests_SRS_HTTP_HEADERS_06_001: [This API will perform exactly as HTTPHeaders_AddHeaderNameValuePair except that if the header name already exists the already existing value will be replaced as opposed to concatenated to.] */
        TEST_FUNCTION(HTTPHeaders_ReplaceHeaderNameValuePair_succeeds)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();
            (void)HTTPHeaders_AddHeaderNameValuePair(httpHandle, NAME1, VALUE1);
            size_t nHeaders;

            ///act
            auto res3 = HTTPHeaders_ReplaceHeaderNameValuePair(httpHandle, NAME1, VALUE2);
            auto res = HTTPHeaders_GetHeaderCount(httpHandle, &nHeaders);
            auto res2 = HTTPHeaders_FindHeaderValue(httpHandle, NAME1);

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_OK, res);
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_OK, res3);
            ASSERT_ARE_EQUAL(size_t, (size_t)1, nHeaders);
            ASSERT_ARE_EQUAL(char_ptr, (std::string(VALUE2)).c_str(), res2);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /* Tests_SRS_HTTP_HEADERS_06_001: [This API will perform exactly as HTTPHeaders_AddHeaderNameValuePair except that if the header name already exists the already existing value will be replaced as opposed to concatenated to.] */
        TEST_FUNCTION(HTTPHeaders_ReplaceHeaderNameValuePair_for_none_existing_header_succeeds)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();
            size_t nHeaders;

            ///act
            auto res3 = HTTPHeaders_ReplaceHeaderNameValuePair(httpHandle, NAME1, VALUE2);
            auto res = HTTPHeaders_GetHeaderCount(httpHandle, &nHeaders);
            auto res2 = HTTPHeaders_FindHeaderValue(httpHandle, NAME1);

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_OK, res);
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_OK, res3);
            ASSERT_ARE_EQUAL(size_t, (size_t)1, nHeaders);
            ASSERT_ARE_EQUAL(char_ptr, (std::string(VALUE2)).c_str(), res2);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_024:[ The function shall return HTTP_HEADERS_INVALID_ARG when an invalid handle is passed.]*/
        TEST_FUNCTION(HTTPHeaders_GetHeaderCount_with_NULL_handle_fails)
        {
            ///arrange
            size_t nHeaders;

            ///act
            auto res1 = HTTPHeaders_GetHeaderCount(NULL, &nHeaders);

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_INVALID_ARG, res1);
        }

        /*Tests_SRS_HTTP_HEADERS_99_025:[ The function shall return HTTP_HEADERS_INVALID_ARG when headersCount is NULL.]*/
        TEST_FUNCTION(HTTPHeaders_GetHeaderCount_with_NULL_headersCount_fails)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();

            ///act
            auto res1 = HTTPHeaders_GetHeaderCount(httpHandle, NULL);

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_INVALID_ARG, res1);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_026:[ The function shall write in *headersCount the number of currently stored headers and shall return HTTP_HEADERS_OK]*/
        /*Tests_SRS_HTTP_HEADERS_99_023:[ Calling this API shall provide the number of stored headers.]*/
        TEST_FUNCTION(HTTPHeaders_GetHeaderCount_with_1_header_produces_1)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();
            (void)HTTPHeaders_AddHeaderNameValuePair(httpHandle, NAME1, VALUE1);
            size_t nHeaders;

            ///act
            auto res = HTTPHeaders_GetHeaderCount(httpHandle, &nHeaders);

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_OK, res);
            ASSERT_ARE_EQUAL(size_t, (size_t)1, nHeaders);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_026:[ The function shall write in *headersCount the number of currently stored headers and shall return HTTP_HEADERS_OK]*/
        /*Tests_SRS_HTTP_HEADERS_99_023:[ Calling this API shall provide the number of stored headers.]*/
        TEST_FUNCTION(HTTPHeaders_GetHeaderCount_with_2_header_produces_2)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();
            (void)HTTPHeaders_AddHeaderNameValuePair(httpHandle, NAME1, VALUE1);
            (void)HTTPHeaders_AddHeaderNameValuePair(httpHandle, NAME2, VALUE2);
            size_t nHeaders;

            ///act
            auto res = HTTPHeaders_GetHeaderCount(httpHandle, &nHeaders);

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_OK, res);
            ASSERT_ARE_EQUAL(size_t, (size_t)2, nHeaders);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_028:[ The function shall return NULL if the handle is invalid.]*/
        TEST_FUNCTION(HTTPHeaders_GetHeader_with_NULL_handle_fails)
        {
            ///arrange

            ///act
            auto res = HTTPHeaders_GetHeader(NULL, 0, tempBuffer, TEMP_BUFFER_SIZE);

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_INVALID_ARG, res);
        }

        /*Tests_SRS_HTTP_HEADERS_99_032:[ The function shall return HTTP_HEADERS_INVALID_ARG if the buffer is NULL]*/
        TEST_FUNCTION(HTTPHeaders_GetHeader_with_NULL_buffer_fails)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();
            (void)HTTPHeaders_AddHeaderNameValuePair(httpHandle, NAME1, VALUE1);

            ///act
            auto res = HTTPHeaders_GetHeader(httpHandle, 0, NULL, TEMP_BUFFER_SIZE);

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_INVALID_ARG, res);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_033:[ The function shall return HTTP_HEADERS_INSUFFICIENT_BUFFER if bufferSize is not big enough to accommodate the resulting string]*/
        TEST_FUNCTION(HTTPHeaders_GetHeader_with_insufficient_bufferSize_fails)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();
            (void)HTTPHeaders_AddHeaderNameValuePair(httpHandle, "a", "b");

            ///act
            auto res1 = HTTPHeaders_GetHeader(httpHandle, 0, tempBuffer, 4); /*4 is not enough to hold "a: b\0", however, 5 is*/

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_INSUFFICIENT_BUFFER, res1);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_029:[ The function shall return HTTP_HEADERS_INVALID_ARG if index is not valid (for example, out of range) for the currently stored headers.]*/
        /*Tests that not adding something to the collection fails tso retrieve for index 0*/
        TEST_FUNCTION(HTTPHeaders_GetHeader_with_index_too_big_fails_1)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();

            ///act
            auto res1 = HTTPHeaders_GetHeader(httpHandle, 0, tempBuffer, TEMP_BUFFER_SIZE);

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_INVALID_ARG, res1);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_029:[ The function shall return HTTP_HEADERS_INVALID_ARG if index is not valid (for example, out of range) for the currently stored headers.]*/
        /*Tests that not adding something to the collection fails tso retrieve for index 0*/
        TEST_FUNCTION(HTTPHeaders_GetHeader_with_index_too_big_fails_2)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();
            (void)HTTPHeaders_AddHeaderNameValuePair(httpHandle, NAME1, VALUE1);

            ///act
            auto res1 = HTTPHeaders_GetHeader(httpHandle, 1, tempBuffer, TEMP_BUFFER_SIZE);

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_INVALID_ARG, res1);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_027:[ Calling this API shall produce the string value+": "+pair) for the index header in the buffer pointed to by buffer.]*/
        /*Tests_SRS_HTTP_HEADERS_99_035:[ The function shall return HTTP_HEADERS_OK when the function executed without error.]*/
        TEST_FUNCTION(HTTPHeaders_GetHeader_succeeds_1)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();
            (void)HTTPHeaders_AddHeaderNameValuePair(httpHandle, "a", "b");

            ///act
            auto res1 = HTTPHeaders_GetHeader(httpHandle, 0, tempBuffer, 5); /*4 is not enough to hold "a: b\0", however, 5 is*/

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_OK, res1);
            ASSERT_ARE_EQUAL(char_ptr, tempBuffer, "a: b");

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_027:[ Calling this API shall produce the string value+": "+pair) for the index header in the buffer pointed to by buffer.]*/
        /*the test will put 100 of name:value pair in the form of "0":"0", "1":"2", ... "99":"198" and will retrieve those back*/
        TEST_FUNCTION(HTTPHeaders_GetHeader_succeeds_2)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();
            bool wasFound[MAX_NAME_VALUE_PAIR];
            for(int i=0;i<MAX_NAME_VALUE_PAIR;i++)
            {
                char tempName[100];
                char tempValue[100];
                sprintf(tempName, "%d",i);
                sprintf(tempValue, "%d",i*2);
                (void)HTTPHeaders_AddHeaderNameValuePair(httpHandle, tempName, tempValue);
                wasFound[i]=false;
            }
            size_t nHeaders;
            
            ///act
            (void)HTTPHeaders_GetHeaderCount(httpHandle, &nHeaders);
            for(size_t h=0;h<nHeaders;h++)
            {
                int name, value;
                (void)HTTPHeaders_GetHeader(httpHandle, h, tempBuffer, TEMP_BUFFER_SIZE);

                ///assert
                ASSERT_ARE_EQUAL(int, 2, sscanf(tempBuffer, "%d: %d", &name, &value));
                ASSERT_ARE_EQUAL(int, name*2, value);
                wasFound[name]=true;
            }

            for(size_t h=0;h<nHeaders;h++)
            {
                ASSERT_IS_TRUE(wasFound[h]);
            }

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_031:[ If name contains the character ":" then the return value shall be HTTP_HEADERS_INVALID_ARG.]*/
        TEST_FUNCTION(HTTPHeaders_AddHeaderNameValuePair_with_colon_in_name_fails)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();

            ///act
            auto res = HTTPHeaders_AddHeaderNameValuePair(httpHandle, "a:", "b");

            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_INVALID_ARG, res);

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }

        /*Tests_SRS_HTTP_HEADERS_99_031:[ If name contains the character ":" then the return value shall be HTTP_HEADERS_INVALID_ARG.]*/
        TEST_FUNCTION(HTTPHeaders_AddHeaderNameVAluePair_with_colon_in_value_succeeds_1)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();

            ///act
            auto res1 = HTTPHeaders_AddHeaderNameValuePair(httpHandle, "a", ":");
            auto res2 = HTTPHeaders_FindHeaderValue(httpHandle, "a");
            auto res3 = HTTPHeaders_GetHeader(httpHandle, 0, tempBuffer, TEMP_BUFFER_SIZE);


            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_OK, res1);

            ASSERT_ARE_EQUAL(char_ptr, ":", res2);

            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_OK, res3);
            ASSERT_ARE_EQUAL(char_ptr, "a: :", tempBuffer);

            ///cleanup
            HTTPHeaders_Free(httpHandle);

        }

        /*Tests_SRS_HTTP_HEADERS_99_031:[ If name contains the character ":" then the return value shall be HTTP_HEADERS_INVALID_ARG.]*/
        TEST_FUNCTION(HTTPHeaders_AddHeaderNameVAluePair_with_colon_in_value_succeeds_2)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();

            ///act
            auto res1 = HTTPHeaders_AddHeaderNameValuePair(httpHandle, "a", ":b");
            auto res2 = HTTPHeaders_FindHeaderValue(httpHandle, "a");
            auto res3 = HTTPHeaders_GetHeader(httpHandle, 0, tempBuffer, TEMP_BUFFER_SIZE);


            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_OK, res1);

            ASSERT_ARE_EQUAL(char_ptr, ":b", res2);

            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_OK, res3);
            ASSERT_ARE_EQUAL(char_ptr, "a: :b", tempBuffer);

            ///cleanup
            HTTPHeaders_Free(httpHandle);

        }

        /*Tests_SRS_HTTP_HEADERS_99_031:[ If name contains the character ":" then the return value shall be HTTP_HEADERS_INVALID_ARG.]*/
        TEST_FUNCTION(HTTPHeaders_AddHeaderNameVAluePair_with_colon_in_value_succeeds_3)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();

            ///act
            auto res1 = HTTPHeaders_AddHeaderNameValuePair(httpHandle, "a", "b:");
            auto res2 = HTTPHeaders_FindHeaderValue(httpHandle, "a");
            auto res3 = HTTPHeaders_GetHeader(httpHandle, 0, tempBuffer, TEMP_BUFFER_SIZE);


            ///assert
            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_OK, res1);

            ASSERT_ARE_EQUAL(char_ptr, "b:", res2);

            ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_OK, res3);
            ASSERT_ARE_EQUAL(char_ptr, "a: b:", tempBuffer);

            ///cleanup
            HTTPHeaders_Free(httpHandle);

        }

        /*Tests_SRS_HTTP_HEADERS_99_036:[ If name contains the characters outside character codes 33 to 126 then the return value shall be HTTP_HEADERS_INVALID_ARG]*/
        /*so says http://tools.ietf.org/html/rfc822#section-3.1*/
        TEST_FUNCTION(HTTP_HEADERS_AddHeaderNameValuePair_fails_for_every_invalid_character)
        {
            ///arrange
            auto httpHandle = HTTPHeaders_Alloc();
            char unacceptableString[2]={'\0', '\0'};

            
            for(int c=SCHAR_MIN;c <=SCHAR_MAX; c++)
            {
                if(c=='\0') continue;

                if((c<33) ||( 126<c)|| (c==':'))
                {
                    /*so it is an unacceptable character*/
                    unacceptableString[0]=(char)c;

                    ///act
                    auto res = HTTPHeaders_AddHeaderNameValuePair(httpHandle, unacceptableString, VALUE1);

                    ///assert
                    ASSERT_ARE_EQUAL(HTTP_HEADERS_RESULT, HTTP_HEADERS_INVALID_ARG, res);
                }
            }

            ///cleanup
            HTTPHeaders_Free(httpHandle);
        }



END_TEST_SUITE(HTTPHeaders_UnitTests)
