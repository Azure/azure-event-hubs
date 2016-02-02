// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "connection_string_parser.h"
#include "map.h"
#include "string.h"
#include "string_tokenizer.h"
#include "stdbool.h"
#include "iot_logging.h"

MAP_HANDLE connectionstringparser_parse(STRING_HANDLE connection_string)
{
    MAP_HANDLE result;

    STRING_TOKENIZER_HANDLE tokenizer = STRING_TOKENIZER_create(connection_string);
    if (tokenizer == NULL)
    {
        result = NULL;
    }
    else
    {
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
            }
        }

        STRING_TOKENIZER_destroy(tokenizer);
    }

    return result;
}