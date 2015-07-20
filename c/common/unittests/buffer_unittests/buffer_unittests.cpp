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
#include "buffer_.h"
#include "micromock.h"


#ifdef _MSC_VER
#pragma warning(disable:4505)
#endif

static BUFFER_HANDLE g_hBuffer = NULL;

#define ALLOCATION_SIZE             16
#define TOTAL_ALLOCATION_SIZE       32

unsigned char BUFFER_TEST_VALUE[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15,0x16};
unsigned char ADDITIONAL_BUFFER[] = {0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26};
unsigned char TOTAL_BUFFER[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26};

static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;

BEGIN_TEST_SUITE(Buffer_UnitTests)

    TEST_SUITE_INITIALIZE(setsBufferTempSize)
    {
        INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
    }



    TEST_FUNCTION_CLEANUP(cleans)
    {
        if (g_hBuffer != NULL)
        {
            /* Tests_SRS_STRING_07_010: [STRING_delete will free the memory allocated by the STRING_HANDLE.] */
            BUFFER_delete(g_hBuffer);
            g_hBuffer = NULL;
        }
    }

    /* BUFFER_new Tests BEGIN */
    /* Tests_SRS_BUFFER_07_001: [BUFFER_new shall allocate a BUFFER_HANDLE that will contain a NULL unsigned char*.] */
    TEST_FUNCTION(BUFFER_new_Succeed)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();

        ///assert
        ASSERT_IS_NOT_NULL(g_hBuffer);
    }

    /* BUFFER_delete Tests BEGIN */
    /* Tests_SRS_BUFFER_07_003: [BUFFER_delete shall delete the data associated with the BUFFER_HANDLE.] */
    TEST_FUNCTION(BUFFER_delete_Succeed)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();
        BUFFER_delete(g_hBuffer);
        // Buffer_delete does not NULL the handle
        g_hBuffer = NULL;

        ///assert
    }

    /* Tests_SRS_BUFFER_07_003: [BUFFER_delete shall delete the data associated with the BUFFER_HANDLE.] */
    TEST_FUNCTION(BUFFER_delete_Alloc_Succeed)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();
        int nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE);
        ASSERT_ARE_EQUAL(int, nResult, 0);

        BUFFER_delete(g_hBuffer);
        // Buffer_delete does not NULL the handle
        g_hBuffer = NULL;

        ///assert
    }

    /* Tests_SRS_BUFFER_07_004: [BUFFER_delete shall not delete any BUFFER_HANDLE that is NULL.] */
    TEST_FUNCTION(BUFFER_delete_NULL_HANDLE_Succeed)
    {
        ///arrange

        ///act
        BUFFER_delete(NULL);

        ///assert
    }

    /* BUFFER_pre_Build Tests BEGIN */
    /* Tests_SRS_BUFFER_07_005: [BUFFER_pre_build allocates size_t bytes of BUFFER_HANDLE and returns zero on success.] */
    TEST_FUNCTION(BUFFER_pre_build_Succeed)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();

        int nResult = BUFFER_pre_build(g_hBuffer, ALLOCATION_SIZE);

        ///assert
        ASSERT_ARE_EQUAL(int, nResult, 0);
        ASSERT_ARE_EQUAL(int, BUFFER_length(g_hBuffer), ALLOCATION_SIZE);
    }

    /* Tests_SRS_BUFFER_07_006: [If handle is NULL or size is 0 then BUFFER_pre_build shall return a nonzero value.] */
    /* Tests_SRS_BUFFER_07_013: [BUFFER_pre_build shall return nonzero if any error is encountered.] */
    TEST_FUNCTION(BUFFER_pre_build_HANDLE_NULL_Fail)
    {
        ///arrange

        ///act
        int nResult = BUFFER_pre_build(NULL, ALLOCATION_SIZE);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_BUFFER_07_006: [If handle is NULL or size is 0 then BUFFER_pre_build shall return a nonzero value.] */
    TEST_FUNCTION(BUFFER_pre_Size_Zero_Fail)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();

        int nResult = BUFFER_pre_build(g_hBuffer, 0);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_BUFFER_07_013: [BUFFER_pre_build shall return nonzero if any error is encountered.] */
    TEST_FUNCTION(BUFFER_pre_build_HANDLE_NULL_Size_Zero_Fail)
    {
        ///arrange

        ///act
        int nResult = BUFFER_pre_build(NULL, 0);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_BUFFER_07_007: [BUFFER_pre_build shall return nonzero if the buffer has been previously allocated and is not NULL.] */
    /* Tests_SRS_BUFFER_07_013: [BUFFER_pre_build shall return nonzero if any error is encountered.] */
    TEST_FUNCTION(BUFFER_pre_build_Multiple_Alloc_Fail)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();
        int nResult = BUFFER_pre_build(g_hBuffer, ALLOCATION_SIZE);
        ASSERT_ARE_EQUAL(int, nResult, 0);

        nResult = BUFFER_pre_build(g_hBuffer, ALLOCATION_SIZE);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* BUFFER_build Tests BEGIN */

    /* Tests_SRS_BUFFER_07_008: [BUFFER_build allocates size_t bytes, copies the unsigned char* into the buffer and returns zero on success.] */
    TEST_FUNCTION(BUFFER_build_Succeed)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();

        int nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE);

        ///assert
        ASSERT_ARE_EQUAL(int, BUFFER_length(g_hBuffer), ALLOCATION_SIZE);
        ASSERT_ARE_EQUAL(int, 0, memcmp(BUFFER_u_char(g_hBuffer), BUFFER_TEST_VALUE, ALLOCATION_SIZE));
        ASSERT_ARE_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_BUFFER_07_009: [BUFFER_build shall return nonzero if handle is NULL ] */
    TEST_FUNCTION(BUFFER_build_NULL_HANDLE_Fail)
    {
        ///arrange

        ///act
        int nResult = BUFFER_build(NULL, BUFFER_TEST_VALUE, ALLOCATION_SIZE);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_BUFFER_01_001: [If size is positive and source is NULL, BUFFER_build shall return nonzero] */
    TEST_FUNCTION(BUFFER_build_Content_NULL_Fail)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();

        int nResult = BUFFER_build(g_hBuffer, NULL, ALLOCATION_SIZE);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_BUFFER_01_002: [The size argument can be zero, in which case the underlying buffer held by the buffer instance shall be freed.] */
    TEST_FUNCTION(BUFFER_build_Size_Zero_non_NULL_buffer_Succeeds)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();

        int nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, 0);

        ///assert
        ASSERT_ARE_EQUAL(int, nResult, 0);
    }

	/* Tests_SRS_BUFFER_01_002: [The size argument can be zero, in which case the underlying buffer held by the buffer instance shall be freed.] */
	TEST_FUNCTION(BUFFER_build_Size_Zero_NULL_buffer_Succeeds)
	{
		///arrange

		///act
		g_hBuffer = BUFFER_new();

		int nResult = BUFFER_build(g_hBuffer, NULL, 0);

		///assert
		ASSERT_ARE_EQUAL(int, nResult, 0);
	}

    /* Tests_SRS_BUFFER_07_011: [BUFFER_build shall overwrite previous contents if the buffer has been previously allocated.] */
    TEST_FUNCTION(BUFFER_build_when_the_buffer_is_already_allocated_and_the_same_amount_of_bytes_is_needed_succeeds)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();
        int nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE);
        ASSERT_ARE_EQUAL(int, nResult, 0);

        nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE);

        ///assert
        ASSERT_ARE_EQUAL(int, nResult, 0);
    }

	/* Tests_SRS_BUFFER_07_011: [BUFFER_build shall overwrite previous contents if the buffer has been previously allocated.] */
	TEST_FUNCTION(BUFFER_build_when_the_buffer_is_already_allocated_and_more_bytes_are_needed_succeeds)
	{
		///arrange

		///act
		g_hBuffer = BUFFER_new();
		int nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE - 1);
		ASSERT_ARE_EQUAL(int, nResult, 0);

		nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE);

		///assert
		ASSERT_ARE_EQUAL(int, nResult, 0);
	}

	/* Tests_SRS_BUFFER_07_011: [BUFFER_build shall overwrite previous contents if the buffer has been previously allocated.] */
	TEST_FUNCTION(BUFFER_build_when_the_buffer_is_already_allocated_and_less_bytes_are_needed_succeeds)
	{
		///arrange

		///act
		g_hBuffer = BUFFER_new();
		int nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE);
		ASSERT_ARE_EQUAL(int, nResult, 0);

		nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE - 1);

		///assert
		ASSERT_ARE_EQUAL(int, nResult, 0);
	}

    /* BUFFER_unbuild Tests BEGIN */
    /* Tests_SRS_BUFFER_07_012: [BUFFER_unbuild shall clear the underlying unsigned char* data associated with the BUFFER_HANDLE this will return zero on success.] */
    TEST_FUNCTION(BUFFER_unbuild_Succeed)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();
        int nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE);
        ASSERT_ARE_EQUAL(int, nResult, 0);

        nResult = BUFFER_unbuild(g_hBuffer);

        ///assert
        ASSERT_ARE_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_BUFFER_07_014: [BUFFER_unbuild shall return a nonzero value if BUFFER_HANDLE is NULL.] */
    TEST_FUNCTION(BUFFER_unbuild_HANDLE_NULL_Fail)
    {
        ///arrange

        ///act
        int nResult = BUFFER_unbuild(NULL);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_BUFFER_07_015: [BUFFER_unbuild shall return a nonzero value if the unsigned char* referenced by BUFFER_HANDLE is NULL.] */
    TEST_FUNCTION(BUFFER_unbuild_Multiple_Alloc_Fail)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();
        int nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE);
        ASSERT_ARE_EQUAL(int, nResult, 0);

        nResult = BUFFER_unbuild(g_hBuffer);
        ASSERT_ARE_EQUAL(int, nResult, 0);

        nResult = BUFFER_unbuild(g_hBuffer);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* BUFFER_enlarge Tests BEGIN */
    /* Tests_SRS_BUFFER_07_016: [BUFFER_enlarge shall increase the size of the unsigned char* referenced by BUFFER_HANDLE.] */
    TEST_FUNCTION(BUFFER_enlarge_Succeed)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();
        int nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE);
        ASSERT_ARE_EQUAL(int, nResult, 0);

        nResult = BUFFER_enlarge(g_hBuffer, ALLOCATION_SIZE);

        ///assert
        ASSERT_ARE_EQUAL(int, nResult, 0);
        ASSERT_ARE_EQUAL(int, TOTAL_ALLOCATION_SIZE, BUFFER_length(g_hBuffer) );
    }

    /* Tests_SRS_BUFFER_07_017: [BUFFER_enlarge shall return a nonzero result if any parameters are NULL or zero.] */
    /* Tests_SRS_BUFFER_07_018: [BUFFER_enlarge shall return a nonzero result if any error is encountered.] */
    TEST_FUNCTION(BUFFER_enlarge_NULL_HANDLE_Fail)
    {
        ///arrange

        ///act
        int nResult = BUFFER_enlarge(NULL, ALLOCATION_SIZE);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_BUFFER_07_017: [BUFFER_enlarge shall return a nonzero result if any parameters are NULL or zero.] */
    /* Tests_SRS_BUFFER_07_018: [BUFFER_enlarge shall return a nonzero result if any error is encountered.] */
    TEST_FUNCTION(BUFFER_enlarge_Size_Zero_Fail)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();
        int nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE);
        ASSERT_ARE_EQUAL(int, nResult, 0);

        nResult = BUFFER_enlarge(g_hBuffer, 0);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* BUFFER_content Tests BEGIN */
    /* Tests_SRS_BUFFER_07_019: [BUFFER_content shall return the data contained within the BUFFER_HANDLE.] */
    TEST_FUNCTION(BUFFER_content_Succeed)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();
        int nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE);
        ASSERT_ARE_EQUAL(int, nResult, 0);

        const unsigned char* content = NULL;
        nResult = BUFFER_content(g_hBuffer, &content);

        ///assert
        ASSERT_ARE_EQUAL(int, nResult, 0);
        ASSERT_ARE_EQUAL(int, 0, memcmp(content, BUFFER_TEST_VALUE, ALLOCATION_SIZE));
    }

    /* Tests_SRS_BUFFER_07_020: [If the handle and/or content*is NULL BUFFER_content shall return nonzero.] */
    TEST_FUNCTION(BUFFER_content_HANDLE_NULL_Fail)
    {
        ///arrange

        ///act
        const unsigned char* content = NULL;
        int nResult = BUFFER_content(NULL, &content);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
        ASSERT_IS_NULL(content);

    }

    /* Tests_SRS_BUFFER_07_020: [If the handle and/or content*is NULL BUFFER_content shall return nonzero.] */
    TEST_FUNCTION(BUFFER_content_Char_NULL_Fail)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();
        int nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE);
        ASSERT_ARE_EQUAL(int, nResult, 0);

        nResult = BUFFER_content(g_hBuffer, NULL);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* BUFFER_size Tests BEGIN */
    /* Tests_SRS_BUFFER_07_021: [BUFFER_size shall place the size of the associated buffer in the size variable and return zero on success.] */
    TEST_FUNCTION(BUFFER_size_Succeed)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();
        int nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE);
        ASSERT_ARE_EQUAL(int, nResult, 0);

        size_t size = 0;
        nResult = BUFFER_size(g_hBuffer, &size);

        ///assert
        ASSERT_ARE_EQUAL(int, nResult, 0);
        ASSERT_ARE_EQUAL(int, size, ALLOCATION_SIZE);
    }

    /* Tests_SRS_BUFFER_07_022: [BUFFER_size shall return a nonzero value for any error that is encountered.] */
    TEST_FUNCTION(BUFFER_size_HANDLE_NULL_Fail)
    {
        ///arrange

        ///act
        size_t size = 0;
        int nResult = BUFFER_size(NULL, &size);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* Tests_SRS_BUFFER_07_022: [BUFFER_size shall return a nonzero value for any error that is encountered.] */
    TEST_FUNCTION(BUFFER_size_Size_t_NULL_Fail)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();
        int nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE);
        ASSERT_ARE_EQUAL(int, nResult, 0);

        nResult = BUFFER_size(g_hBuffer, NULL);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* BUFFER_append Tests BEGIN */
    /* Tests_SRS_BUFFER_07_024: [BUFFER_append concatenates b2 onto b1 without modifying b2 and shall return zero on success.] */
    TEST_FUNCTION(BUFFER_append_Succeed)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();
        int nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE);
        ASSERT_ARE_EQUAL(int, nResult, 0);

        BUFFER_HANDLE hAppend = BUFFER_new();
        nResult = BUFFER_build(hAppend, ADDITIONAL_BUFFER, ALLOCATION_SIZE);
        ASSERT_ARE_EQUAL(int, nResult, 0);

        nResult = BUFFER_append(g_hBuffer, hAppend);

        ///assert
        ASSERT_ARE_EQUAL(int, nResult, 0);
        ASSERT_ARE_EQUAL(int, 0, memcmp(BUFFER_u_char(g_hBuffer), TOTAL_BUFFER, TOTAL_ALLOCATION_SIZE));
        ASSERT_ARE_EQUAL(int, 0, memcmp(BUFFER_u_char(hAppend), ADDITIONAL_BUFFER, ALLOCATION_SIZE));

        BUFFER_delete(hAppend);
    }

    /* Tests_SRS_BUFFER_07_023: [BUFFER_append shall return a nonzero upon any error that is encountered.] */
    TEST_FUNCTION(BUFFER_append_HANDLE_NULL_Fail)
    {
        ///arrange

        ///act
        BUFFER_HANDLE hAppend = BUFFER_new();
        int nResult = BUFFER_build(hAppend, ADDITIONAL_BUFFER, ALLOCATION_SIZE);
        ASSERT_ARE_EQUAL(int, nResult, 0);

        nResult = BUFFER_append(NULL, hAppend);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);

        BUFFER_delete(hAppend);
    }

    /* Tests_SRS_BUFFER_07_023: [BUFFER_append shall return a nonzero upon any error that is encountered.] */
    TEST_FUNCTION(BUFFER_append_APPEND_HANDLE_NULL_Fail)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();
        int nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE);
        ASSERT_ARE_EQUAL(int, nResult, 0);

        nResult = BUFFER_append(g_hBuffer, NULL);

        ///assert
        ASSERT_ARE_NOT_EQUAL(int, nResult, 0);
    }

    /* BUFFER_u_char Tests BEGIN */
    /* Tests_SRS_BUFFER_07_025: [BUFFER_u_char shall return a pointer to the underlying unsigned char*.] */
    TEST_FUNCTION(BUFFER_U_CHAR_Succeed)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();
        int nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE);

        ///assert
        ASSERT_ARE_EQUAL(int, nResult, 0);
        ASSERT_ARE_EQUAL(int, 0, memcmp(BUFFER_u_char(g_hBuffer), BUFFER_TEST_VALUE, ALLOCATION_SIZE) );
    }

    /* Tests_SRS_BUFFER_07_026: [BUFFER_u_char shall return NULL for any error that is encountered.] */
    TEST_FUNCTION(BUFFER_U_CHAR_HANDLE_NULL_Fail)
    {
        ///arrange

        ///act
        ASSERT_IS_NULL(BUFFER_u_char(NULL) );
    }

    /* BUFFER_length Tests BEGIN */
    /* Tests_SRS_BUFFER_07_027: [BUFFER_length shall return the size of the underlying buffer.] */
    TEST_FUNCTION(BUFFER_length_Succeed)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();
        int nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE);
        ASSERT_ARE_EQUAL(int, nResult, 0);

        ///assert
        ASSERT_ARE_EQUAL(int, BUFFER_length(g_hBuffer), ALLOCATION_SIZE);
    }

    /* Tests_SRS_BUFFER_07_028: [BUFFER_length shall return zero for any error that is encountered.] */
    TEST_FUNCTION(BUFFER_length_HANDLE_NULL_Succeed)
    {
        ///arrange

        ///act

        ///assert
        ASSERT_ARE_EQUAL(int, BUFFER_length(NULL), 0);
    }

    TEST_FUNCTION(BUFFER_Clone_Succeed)
    {
        ///arrange

        ///act
        g_hBuffer = BUFFER_new();
        int nResult = BUFFER_build(g_hBuffer, BUFFER_TEST_VALUE, ALLOCATION_SIZE);
        ASSERT_ARE_EQUAL(int, nResult, 0);

        BUFFER_HANDLE hclone = BUFFER_clone(g_hBuffer);

        ///assert
        ASSERT_ARE_EQUAL(int, 0, memcmp(BUFFER_u_char(hclone), BUFFER_TEST_VALUE, ALLOCATION_SIZE) );

        /// clean up
        BUFFER_delete(hclone);
    }

    TEST_FUNCTION(BUFFER_Clone_HANDLE_NULL_Fail)
    {
        ///arrange

        ///act

        ///assert
        ASSERT_IS_NULL(BUFFER_clone(NULL) );
    }

    /* BUFFER_Tests END */
END_TEST_SUITE(Buffer_UnitTests)