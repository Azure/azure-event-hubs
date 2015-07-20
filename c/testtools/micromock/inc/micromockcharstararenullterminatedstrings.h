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

#ifndef MicroMockCharStarArenullTerminatedStrings_H
#define MicroMockCharStarArenullTerminatedStrings_H

#ifndef MICROMOCK_H
#error the file MicroMockCharStarArenullTerminatedStrings can only be #included after #include "micromock.h"
#endif

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

/*for strcpy()*/
#include "string.h" 

/*for malloc()*/
#include <stdlib.h>

/*for everything else*/
#include "stdafx.h"

template<>
class CMockValue <char*> :
    public CMockValueBase
{
protected:
    char* m_Value;
    char* m_OriginalValue;
public:
    CMockValue(_In_ char* value)
    {
        if(value==NULL)
        {
            m_Value=NULL;
        }
        else
        {
            size_t s= strlen(value);
            m_OriginalValue = value;
            m_Value = (char*)malloc(s + 1);
            strcpy(m_Value, value);
        }
    }

    virtual ~CMockValue()
    {
        if(m_Value!=NULL)
        {
            free(m_Value);
            m_Value = NULL;
        }
    }

    virtual _Check_return_
    std::tstring ToString() const
    {
        std::tostringstream strStream;
        if (NULL == m_Value)
        {
            strStream << _T("NULL");
        }
        else
        {
            strStream << m_Value;
        }
        return strStream.str();
    }

    virtual _Must_inspect_result_
    bool EqualTo(_In_ const CMockValueBase* right)
    {
        return (*this == *(reinterpret_cast<const CMockValue<char*>*>(right)));
        
    }

    void SetValue(_In_ char* value)
    {
        if(value == NULL)
        {
            if(m_Value !=NULL)
            {
                free(m_Value);
                m_Value = NULL;
            }
            else
            {
                
            }
        }
        else
        {
            size_t s = strlen(value);
            if(m_Value !=NULL)
            {
                free(m_Value);
                m_Value=NULL;
            }
            m_Value = (char*)malloc(s+1);
            m_OriginalValue = value;
            strcpy(m_Value, value);
        };
    }

    _Must_inspect_result_ char* GetValue() const
    {
        return m_Value;
    }
};

template<>
class CMockValue <const char*> :
    public CMockValueBase
{
protected:
    char* m_Value;
    const char* m_OriginalValue;
public:
    CMockValue(_In_ const char* value)
    {
        if(value==NULL)
        {
            m_Value=NULL;
        }
        else
        {
            size_t s= strlen(value);
            m_Value = (char*)malloc(s+1);
            m_OriginalValue = value;
            strcpy(m_Value, value);
        }
    }

    virtual ~CMockValue()
    {
        if(m_Value!=NULL)
        {
            free(m_Value);
            m_Value = NULL;
        }
    }

    virtual _Check_return_
    std::tstring ToString() const
    {
        std::tostringstream strStream;
        if (NULL == m_Value)
        {
            strStream << _T("NULL");
        }
        else
        {
            strStream << m_Value;
        }
        return strStream.str();
    }

    virtual _Must_inspect_result_
    bool EqualTo(_In_ const CMockValueBase* right)
    {
        return (*this == *(reinterpret_cast<const CMockValue<const char*>*>(right)));
    }

    void SetValue(_In_ const char* value)
    {
        if(value == NULL)
        {
            if(m_Value !=NULL)
            {
                free(m_Value);
                m_Value = NULL;
            }
            else
            {
                
            }
        }
        else
        {
            size_t s = strlen(value);
            if(m_Value !=NULL)
            {
                free(m_Value);
                m_Value=NULL;
            }
            m_Value = (char*)malloc(s+1);
            m_OriginalValue = value;
            strcpy(m_Value, value);
        };
    }

    _Must_inspect_result_ const char* GetValue() const
    {
        return m_Value;
    }
};

bool operator==(_In_ const CMockValue<char*>& lhs, _In_ const CMockValue<char*>& rhs); 
bool operator==(_In_ const CMockValue<const char*>& lhs, _In_ const CMockValue<const char*>& rhs);

#endif
