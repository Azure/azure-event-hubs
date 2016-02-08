// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "connection_string_parser.h"
#include "map.h"
#include "string.h"
#include "string_tokenizer.h"
#include "stdbool.h"
#include "iot_logging.h"

/* Codes_SRS_CONNECTIONSTRINGPARSER_01_001: [connectionstringparser_parse shall parse all key value pairs from the connection_string passed in as argument and return a new map that holds the key/value pairs.] */
MAP_HANDLE connectionstringparser_parse(STRING_HANDLE connection_string)
{
    MAP_HANDLE result;

    /* Codes_SRS_CONNECTIONSTRINGPARSER_01_003: [connectionstringparser_parse shall create a STRING tokenizer to be used for parsing the connection string, by calling STRING_TOKENIZER_create.] */
    /* Codes_SRS_CONNECTIONSTRINGPARSER_01_004: [connectionstringparser_parse shall start scanning at the beginning of the connection string.] */
    STRING_TOKENIZER_HANDLE tokenizer = STRING_TOKENIZER_create(connection_string);
    if (tokenizer == NULL)
    {
        result = NULL;
    }
    else
    {
        /* Codes_SRS_CONNECTIONSTRINGPARSER_01_016: [2 STRINGs shall be allocated in order to hold the to be parsed key and value tokens.] */
        STRING_HANDLE token_key_string = STRING_new();
        if (token_key_string == NULL)
        {
            LogError("Error creating STRING\r\n");
        }
        else
        {
            STRING_HANDLE token_value_string = STRING_new();
            if (token_value_string == NULL)
            {
                LogError("Error creating STRING\r\n");
                STRING_delete(token_key_string);
            }
            else
            {
                result = Map_Create(NULL);
                if (result == NULL)
                {
                    LogError("Error creating Map\r\n");
                    STRING_delete(token_key_string);
                    STRING_delete(token_value_string);
                }
                else
                {
                    /* Codes_SRS_CONNECTIONSTRINGPARSER_01_005: [The following actions shall be repeated until parsing is complete:] */
                    /* Codes_SRS_CONNECTIONSTRINGPARSER_01_006: [connectionstringparser_parse shall find a token (the key of the key/value pair) delimited by the “=” character, by calling STRING_TOKENIZER_get_next_token.] */
                    /* Codes_SRS_CONNECTIONSTRINGPARSER_01_007: [If STRING_TOKENIZER_get_next_token fails, parsing shall be considered complete.] */
                    while (STRING_TOKENIZER_get_next_token(tokenizer, token_key_string, "=") == 0)
                    {
                        bool is_error = false;

                        const char* token = STRING_c_str(token_key_string);
                        if (strlen(token) == 0)
                        {
                            is_error = true;
                        }
                        else
                        {
                            if (STRING_TOKENIZER_get_next_token(tokenizer, token_value_string, ";") != 0)
                            {
                                is_error = true;
                            }
                            else
                            {
                                if (Map_Add(result, STRING_c_str(token_key_string), STRING_c_str(token_value_string)) != 0)
                                {
                                    is_error = true;
                                }
                            }
                        }

                        if (is_error)
                        {
                            LogError("Error parsing connection string\r\n");
                            STRING_delete(token_key_string);
                            STRING_delete(token_value_string);
                            Map_Destroy(result);
                            result = NULL;
                            break;
                        }
                    }
                }

                STRING_delete(token_value_string);
            }

            STRING_delete(token_key_string);
        }

        /* Codes_SRS_CONNECTIONSTRINGPARSER_01_014: [After the parsing is complete the previously allocated STRINGs and STRING tokenizer shall be freed by calling STRING_TOKENIZER_destroy.] */
        STRING_TOKENIZER_destroy(tokenizer);
    }

    return result;
}