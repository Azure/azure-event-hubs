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

//
// PUT NO INCLUDES BEFORE HERE !!!!
//
#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <stddef.h>

//
// PUT NO CLIENT LIBRARY INCLUDES BEFORE HERE !!!!
//
#include "testrunnerswitcher.h"
#include "strings.h"
#include "micromock.h"


#ifdef _MSC_VER
#pragma warning(disable:4505)
#endif

static STRING_HANDLE g_hString = NULL;
static char TEST_STRING_VALUE []= "DataValueTest";
static char INITAL_STRING_VALUE []= "Initial_";
static char MULTIPLE_TEST_STRING_VALUE[] = "DataValueTestDataValueTest";
static const char* COMBINED_STRING_VALUE = "Initial_DataValueTest";
static const char* QUOTED_TEST_STRING_VALUE = "\"DataValueTest\"";
static const char* EMPTY_STRING = "";

#define NUMBER_OF_CHAR_TOCOPY               8

static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

BEGIN_TEST_SUITE(strings_unittests)

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);

    }

    TEST_SUITE_INITIALIZE(setsBufferTempSize)
    {
        INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    TEST_FUNCTION_CLEANUP(cleans)
    {
        if (g_hString != NULL)
        {
            /* Tests_SRS_STRING_07_010: [STRING_delete will free the memory allocated by the STRING_HANDLE.] */
            STRING_delete(g_hString);
            g_hString = NULL;
        }
    }

    /* STRING_Tests BEGIN */
    /* Tests_SRS_STRING_07_001: [STRING_new shall allocate a new STRING_HANDLE pointing to an empty string.] */
    TEST_FUNCTION(STRING_new_Succeed)
    {
        ///arrange

        ///act
        g_hString = STRING_new();

        ///assert
        ASSERT_IS_NOT_NULL(g_hString);
    }

    /* Tests_SRS_STRING_07_007: [STRING_new_with_memory shall return a NULL STRING_HANDLE if the supplied char* is empty.] */
    TEST_FUNCTION(STRING_new_With_Memory_NULL_Memory_Fail)
    {
        ///arrange

        ///act
        g_hString = STRING_new_with_memory(NULL);

        ///assert
        ASSERT_IS_NULL(g_hString);
    }

    /* Tests_SRS_STRING_07_006: [STRING_new_with_memory shall return a STRING_HANDLE by using the supplied char* memory.] */
    TEST_FUNCTION(STRING_new_With_Memory_Succeed)
    {
        ///arrange

        ///act
        size_t nLen = strlen(TEST_STRING_VALUE)+1;
        char* szTestString = (char*)malloc(nLen);
        strncpy(szTestString, TEST_STRING_VALUE, nLen);
        g_hString = STRING_new_with_memory(szTestString);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, TEST_STRING_VALUE, STRING_c_str(g_hString) );
    }

    /* Tests_SRS_STRING_07_003: [STRING_construct shall allocate a new string with the value of the specified const char*.] */
    TEST_FUNCTION(STRING_construct_Succeed)
    {
        ///arrange

        ///act
        g_hString = STRING_construct(TEST_STRING_VALUE);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, TEST_STRING_VALUE, STRING_c_str(g_hString) );
    }

    /* Tests_SRS_STRING_07_005: [If the supplied const char* is NULL STRING_construct shall return a NULL value.] */
    TEST_FUNCTION(STRING_construct_With_NULL_HANDLE_Fail)
    {
        ///arrange

        ///act
        g_hString = STRING_construct(NULL);

        ///assert
        ASSERT_IS_NULL(g_hString);
    }

    /* Tests_SRS_STRING_07_008: [STRING_new_quoted shall return a valid STRING_HANDLE Copying the supplied const char* value surrounded by quotes.] */
    TEST_FUNCTION(STRING_new_quoted_Succeed)
    {
        ///arrange

        ///act
        g_hString = STRING_new_quoted(TEST_STRING_VALUE);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, QUOTED_TEST_STRING_VALUE, STRING_c_str(g_hString) );
    }

    /* Tests_SRS_STRING_07_009: [STRING_new_quoted shall return a NULL STRING_HANDLE if the supplied const char* is NULL.] */
    TEST_FUNCTION(STRING_new_quoted_NULL_Fail)
    {
        ///arrange

        ///act
        g_hString = STRING_new_quoted(NULL);

        ///assert
        ASSERT_IS_NULL(g_hString);
    }

    /* Tests_ */
    TEST_FUNCTION(STRING_Concat_Succeed)
    {
        ///arrange

        ///act
        g_hString = STRING_construct(INITAL_STRING_VALUE);
        int nResult = STRING_concat(g_hString, TEST_STRING_VALUE);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, COMBINED_STRING_VALUE, STRING_c_str(g_hString) );
        ASSERT_ARE_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_STRING_07_013: [STRING_concat shall return a nonzero number if the STRING_HANDLE and const char* is NULL.] */
    TEST_FUNCTION(STRING_Concat_HANDLE_NULL_Fail)
    {
        ///arrange

        ///act
        g_hString = STRING_construct(INITAL_STRING_VALUE);
        int nResult = STRING_concat(NULL, TEST_STRING_VALUE);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_STRING_07_013: [STRING_concat shall return a nonzero number if the STRING_HANDLE and const char* is NULL.] */
    TEST_FUNCTION(STRING_Concat_CharPtr_NULL_Fail)
    {
        ///arrange

        ///act
        g_hString = STRING_construct(INITAL_STRING_VALUE);
        int nResult = STRING_concat(g_hString, NULL);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_STRING_07_013: [STRING_concat shall return a nonzero number if the STRING_HANDLE and const char* is NULL.] */
    TEST_FUNCTION(STRING_Concat_HANDLE_and_CharPtr_NULL_Fail)
    {
        ///arrange

        ///act
        g_hString = STRING_construct(INITAL_STRING_VALUE);
        int nResult = STRING_concat(NULL, NULL);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_STRING_07_013: [STRING_concat shall return a nonzero number if the STRING_HANDLE and const char* is NULL.] */
    TEST_FUNCTION(STRING_Concat_Copy_Multiple_Succeed)
    {
        ///arrange

        ///act
        g_hString = STRING_new();
        STRING_copy(g_hString, TEST_STRING_VALUE);
        STRING_concat(g_hString, TEST_STRING_VALUE);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, MULTIPLE_TEST_STRING_VALUE, STRING_c_str(g_hString) );
    }

    /* Tests_SRS_STRING_07_034: [String_Concat_with_STRING shall concatenate a given STRING_HANDLE variable with a source STRING_HANDLE.] */
    TEST_FUNCTION(STRING_Concat_With_STRING_SUCCEED)
    {
        ///arrange
        g_hString = STRING_construct(INITAL_STRING_VALUE);
        STRING_HANDLE hAppend = STRING_construct(TEST_STRING_VALUE);

        ///act
        int nResult = STRING_concat_with_STRING(g_hString, hAppend);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, COMBINED_STRING_VALUE, STRING_c_str(g_hString) );
        ASSERT_ARE_EQUAL(int, nResult, 0);

        // Clean up
        STRING_delete(hAppend);
    }

    /* Tests_SRS_STRING_07_035: [String_Concat_with_STRING shall return a nonzero number if an error is encountered.] */
    TEST_FUNCTION(STRING_Concat_With_STRING_HANDLE_NULL_Fail)
    {
        ///arrange

        ///act
        STRING_HANDLE hAppend = STRING_construct(TEST_STRING_VALUE);

        int nResult = STRING_concat_with_STRING(NULL, hAppend);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);

        // Clean up
        STRING_delete(hAppend);
    }

    /* Tests_SRS_STRING_07_035: [String_Concat_with_STRING shall return a nonzero number if an error is encountered.] */
    TEST_FUNCTION(STRING_Concat_With_STRING_Append_HANDLE_NULL_Fail)
    {
        ///arrange
        g_hString = STRING_construct(INITAL_STRING_VALUE);

        ///act
        int nResult = STRING_concat_with_STRING(g_hString, NULL);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_STRING_07_035: [String_Concat_with_STRING shall return a nonzero number if an error is encountered.] */
    TEST_FUNCTION(STRING_Concat_With_STRING_All_HANDLE_NULL_Fail)
    {
        ///arrange

        ///act
        int nResult = STRING_concat_with_STRING(NULL, NULL);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_STRING_07_016: [STRING_copy shall copy the const char* into the supplied STRING_HANDLE.] */
    TEST_FUNCTION(STRING_Copy_Succeed)
    {
        ///arrange

        ///act
        g_hString = STRING_construct(INITAL_STRING_VALUE);
        int nResult = STRING_copy(g_hString, TEST_STRING_VALUE);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, TEST_STRING_VALUE, STRING_c_str(g_hString) );
        ASSERT_ARE_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_STRING_07_017: [STRING_copy shall return a nonzero value if any of the supplied parameters are NULL.] */
    TEST_FUNCTION(STRING_Copy_NULL_Fail)
    {
        ///arrange

        ///act
        g_hString = STRING_construct(INITAL_STRING_VALUE);
        int nResult = STRING_copy(g_hString, NULL);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_STRING_07_018: [STRING_copy_n shall copy the number of characters defined in size_t.] */
    TEST_FUNCTION(STRING_Copy_n_Succeed)
    {
        ///arrange

        ///act
        g_hString = STRING_construct(INITAL_STRING_VALUE);
        int nResult = STRING_copy_n(g_hString, COMBINED_STRING_VALUE, NUMBER_OF_CHAR_TOCOPY);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, INITAL_STRING_VALUE, STRING_c_str(g_hString) );
        ASSERT_ARE_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_STRING_07_019: [STRING_copy_n shall return a nonzero value if STRING_HANDLE or const char* is NULL.] */
    TEST_FUNCTION(STRING_Copy_n_With_HANDLE_NULL_Fail)
    {
        ///arrange

        ///act
        int nResult = STRING_copy_n(NULL, COMBINED_STRING_VALUE, NUMBER_OF_CHAR_TOCOPY);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_STRING_07_019: [STRING_copy_n shall return a nonzero value if STRING_HANDLE or const char* is NULL.] */
    TEST_FUNCTION(STRING_Copy_n_With_CONST_CHAR_NULL_Fail)
    {
        ///arrange

        ///act
        g_hString = STRING_construct(INITAL_STRING_VALUE);
        int nResult = STRING_copy_n(g_hString, NULL, NUMBER_OF_CHAR_TOCOPY);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_STRING_07_018: [STRING_copy_n shall copy the number of characters defined in size_t.] */
    TEST_FUNCTION(STRING_Copy_n_With_Size_0_Succeed)
    {
        ///arrange

        ///act
        g_hString = STRING_construct(INITAL_STRING_VALUE);
        int nResult = STRING_copy_n(g_hString, COMBINED_STRING_VALUE, 0);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, EMPTY_STRING, STRING_c_str(g_hString) );
        ASSERT_ARE_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_STRING_07_014: [STRING_quote shall “quote” the supplied STRING_HANDLE and return 0 on success.] */
    TEST_FUNCTION(STRING_quote_Succeed)
    {
        ///arrange

        ///act
        g_hString = STRING_construct(TEST_STRING_VALUE);
        int nResult = STRING_quote(g_hString);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, QUOTED_TEST_STRING_VALUE, STRING_c_str(g_hString) );
        ASSERT_ARE_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_STRING_07_015: [STRING_quote shall return a nonzero value if any of the supplied parameters are NULL.] */
    TEST_FUNCTION(STRING_quote_NULL_HANDLE_Fail)
    {
        ///arrange

        ///act
        int nResult = STRING_quote(NULL);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_STRING_07_021: [STRING_c_str shall return NULL if the STRING_HANDLE is NULL.] */
    TEST_FUNCTION(STRING_c_str_NULL_HANDLE_Fail)
    {
        ///arrange

        ///act
        const char* s = STRING_c_str(NULL);

        ///assert
        ASSERT_IS_NULL(s);
    }

    /* Tests_SRS_STRING_07_020: [STRING_c_str shall return the const char* associated with the given STRING_HANDLE.] */
    TEST_FUNCTION(STRING_c_str_Success)
    {
        ///arrange

        ///act
        g_hString = STRING_construct(TEST_STRING_VALUE);
        const char* s = STRING_c_str(g_hString);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, s, TEST_STRING_VALUE);
    }

    /* Tests_SRS_STRING_07_022: [STRING_empty shall revert the STRING_HANDLE to an empty state.] */
    TEST_FUNCTION(STRING_empty_Succeed)
    {
        ///arrange

        ///act
        g_hString = STRING_construct(TEST_STRING_VALUE);
        int nResult = STRING_empty(g_hString);

        ///assert
        ASSERT_ARE_EQUAL(int, nResult, 0);
        ASSERT_ARE_EQUAL(char_ptr, EMPTY_STRING, STRING_c_str(g_hString) );
    }

    /* Tests_SRS_STRING_07_023: [STRING_empty shall return a nonzero value if the STRING_HANDLE is NULL.] */
    TEST_FUNCTION(STRING_empty_NULL_HANDLE_Fail)
    {
        ///arrange

        ///act
        int nResult = STRING_empty(NULL);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_STRING_07_011: [STRING_delete will not attempt to free anything with a NULL STRING_HANDLE.] */
    TEST_FUNCTION(STRING_delete_NULL_Succeed)
    {
        ///arrange

        ///act
        STRING_delete(NULL);

        ///assert
        // Just checking for exception here
    }

    /* Tests_SRS_STRING_07_011: [STRING_delete will not attempt to free anything with a NULL STRING_HANDLE.] */
    TEST_FUNCTION(STRING_delete_Succeed)
    {
        ///arrange

        ///act
        g_hString = STRING_new();
        STRING_delete(g_hString);
        g_hString = NULL;
        ///assert
    }

    TEST_FUNCTION(STRING_length_Succeed)
    {
        ///arrange

        ///act
        g_hString = STRING_construct(TEST_STRING_VALUE);
        int nResult = STRING_length(g_hString);

        ///assert
        ASSERT_ARE_EQUAL(int, nResult, strlen(TEST_STRING_VALUE) );
    }

    TEST_FUNCTION(STRING_length_NULL_HANDLE_Fail)
    {
        ///arrange

        ///act
        int nResult = STRING_length(NULL);

        ///assert
        ASSERT_ARE_EQUAL(int, nResult, 0);
    }

    /*Tests_SRS_STRING_02_002: [If parameter handle is NULL then STRING_clone shall return NULL.]*/
    TEST_FUNCTION(STRING_clone_NULL_HANDLE_return_NULL)
    {
        ///arrange

        ///act
        STRING_HANDLE result = STRING_clone(NULL);

        ///assert
        ASSERT_IS_NULL( result);
    }

    /*Tests_SRS_STRING_02_001: [STRING_clone shall produce a new string having the same content as the handle string.]*/
    TEST_FUNCTION(STRING_clone_succeeds)
    {
        ///arrange
        auto hSource = STRING_construct("aa");

        ///act
        auto result = STRING_clone(hSource);

        ///assert
        ASSERT_ARE_NOT_EQUAL(void_ptr, NULL, result);
        ASSERT_ARE_NOT_EQUAL(void_ptr, STRING_c_str(hSource), STRING_c_str(result));
        ASSERT_ARE_EQUAL    (char_ptr, STRING_c_str(hSource), STRING_c_str(result));

        ///cleanup
        STRING_delete(hSource);
        STRING_delete(result);
    }

    /*Codes_SRS_STRING_02_008: [If psz is NULL then STRING_construct_n shall return NULL*/
    TEST_FUNCTION(STRING_construct_n_with_NULL_argument_fails)
    {
        ///arrange

        ///act
        auto result = STRING_construct_n(NULL, 3);

        ///assert
        ASSERT_IS_NULL(result);
    }

    /*Codes_SRS_STRING_02_009: [If n is bigger than the size of the string psz, then STRING_construct_n shall return NULL.] */
    TEST_FUNCTION(STRING_construct_n_with_too_big_size_fails)
    {
        ///arrange

        ///act
        auto result = STRING_construct_n("a", 2);

        ///assert
        ASSERT_IS_NULL(result);
    }

    /*Codes_SRS_STRING_02_007: [STRING_construct_n shall construct a STRING_HANDLE from first "n" characters of the string pointed to by psz parameter.] */
    TEST_FUNCTION(STRING_construct_n_succeeds_with_2_char)
    {
        ///arrange

        ///act
        auto result = STRING_construct_n("qq", 2);

        ///assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, "qq", STRING_c_str(result));

        ///cleanup
        STRING_delete(result);
    }

    /*Codes_SRS_STRING_02_007: [STRING_construct_n shall construct a STRING_HANDLE from first "n" characters of the string pointed to by psz parameter.] */
    TEST_FUNCTION(STRING_construct_n_succeeds_with_3_char_out_of_five)
    {
        ///arrange

        ///act
        auto result = STRING_construct_n("12345", 3);

        ///assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, "123", STRING_c_str(result));

        ///cleanup
        STRING_delete(result);
    }

    /* Tests_SRS_STRING_07_036: [If h1 is NULL and h2 is nonNULL then STRING_compare shall return 1.] */
    TEST_FUNCTION(STRING_compare_s1_NULL)
    {
        ///arrange
        auto h2 = STRING_construct("bb");

        ///act
        auto result = STRING_compare(NULL, h2);

        ///assert
        ASSERT_ARE_EQUAL(int, 1, result);

        ///cleanup
        STRING_delete(h2);
    }

    /* Tests_SRS_STRING_07_037: [If h2 is NULL and h1 is nonNULL then STRING_compare shall return -1.] */
    TEST_FUNCTION(STRING_compare_s2_NULL)
    {
        ///arrange
        auto h1 = STRING_construct("aa");

        ///act
        auto result = STRING_compare(h1, NULL);

        ///assert
        ASSERT_ARE_EQUAL(int, -1, result);

        ///cleanup
        STRING_delete(h1);
    }

    /* Codes_SRS_STRING_07_035: [If h1 and h2 are both NULL then STRING_compare shall return 0.] */
    TEST_FUNCTION(STRING_compare_s1_s2_NULL)
    {
        ///arrange

        ///act
        auto result = STRING_compare(NULL, NULL);

        ///assert
        ASSERT_ARE_EQUAL(int, 0, result);

        ///cleanup
    }

    /* Tests_SRS_STRING_07_034: [STRING_compare returns an integer greater than, equal to, or less than zero, accordingly as the string pointed to by s1 is greater than, equal to, or less than the string s2.] */
    TEST_FUNCTION(STRING_compare_s1_first_SUCCEED)
    {
        ///arrange
        auto h1 = STRING_construct("aa");
        auto h2 = STRING_construct("bb");

        ///act
        auto result = STRING_compare(h1, h2);

        ///assert
        ASSERT_ARE_EQUAL(int, -1, result);

        ///cleanup
        STRING_delete(h1);
        STRING_delete(h2);
    }

    /* Tests_SRS_STRING_07_034: [STRING_compare returns an integer greater than, equal to, or less than zero, accordingly as the string pointed to by s1 is greater than, equal to, or less than the string s2.] */
    TEST_FUNCTION(STRING_compare_s2_first_SUCCEED)
    {
        ///arrange
        auto h1 = STRING_construct("aa");
        auto h2 = STRING_construct("bb");

        ///act
        auto result = STRING_compare(h2, h1);

        ///assert
        ASSERT_ARE_EQUAL(int, 1, result);

        ///cleanup
        STRING_delete(h1);
        STRING_delete(h2);
    }

    /* Tests_SRS_STRING_07_034: [STRING_compare returns an integer greater than, equal to, or less than zero, accordingly as the string pointed to by s1 is greater than, equal to, or less than the string s2.] */
    /* Tests_SRS_STRING_07_038: [STRING_compare shall compare the char s variable using the strcmp function.] */
    TEST_FUNCTION(STRING_compare_Equal_SUCCEED)
    {
        ///arrange
        auto h1 = STRING_construct("a1234");
        auto h2 = STRING_construct("a1234");

        ///act
        auto result = STRING_compare(h1, h2);

        ///assert
        ASSERT_ARE_EQUAL(int, 0, result);

        ///cleanup
        STRING_delete(h1);
        STRING_delete(h2);
    }

END_TEST_SUITE(strings_unittests)