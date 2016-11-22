/*
* Copyright(c) 2016 Zonciu Liang.All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*
* Author:  Zonciu Liang
* Contract: zonciu@zonciu.com
* Description: A wrapper for Rapidjson(https://github.com/miloyip/rapidjson/)
*/
#ifndef ZONCIU_JSON_HPP
#define ZONCIU_JSON_HPP
#define NOMINMAX

#include "3rd/rapidjson/document.h"
#include "3rd/rapidjson/istreamwrapper.h"
#include "3rd/rapidjson/ostreamwrapper.h"
#include "3rd/rapidjson/writer.h"
#include "3rd/rapidjson/stringbuffer.h"
#include <string>
#include <map>
#include <fstream>
namespace zonciu
{
class Json
{
public:
    rapidjson::Document value;

    bool ParseFromFile(const char* _filename)
    {
        std::ifstream _ifs(_filename);
        if (!_ifs.is_open())
            return false;
        rapidjson::IStreamWrapper _warp(_ifs);
        value.ParseStream(_warp);
        _ifs.close();
        return !value.HasParseError();
    }
    bool ParseFromString(const char* _json)
    {
        value.Parse(_json);
        return !value.HasParseError();
    }
    bool WriteFile(const char* _filename)
    {
        std::ofstream _ofs(_filename);
        if (!_ofs.is_open())
            return false;
        rapidjson::OStreamWrapper _warp(_ofs);
        rapidjson::Writer<rapidjson::OStreamWrapper> _writer(_warp);
        value.Accept(_writer);
        _ofs.close();
        return true;
    }
    std::string WriteString()
    {
        rapidjson::StringBuffer _buffer;
        rapidjson::Writer<rapidjson::StringBuffer> _writer(_buffer);
        value.Accept(_writer);
        return std::string(_buffer.GetString());
    }
    const char* GetParseErrorString()
    {
        typedef std::map<rapidjson::ParseErrorCode, const char*> ErrorGroup;
        static ErrorGroup error_group_{
            ErrorGroup::value_type(rapidjson::kParseErrorNone                         ,"No error"),                                            //!< No error.

            ErrorGroup::value_type(rapidjson::kParseErrorDocumentEmpty                ,"The document is empty"),                               //!< The document is empty.
            ErrorGroup::value_type(rapidjson::kParseErrorDocumentRootNotSingular      ,"The document root must not follow by other values"),   //!< The document root must not follow by other values.

            ErrorGroup::value_type(rapidjson::kParseErrorValueInvalid                 ,"Invalid value") ,                                       //!< Invalid value.

            ErrorGroup::value_type(rapidjson::kParseErrorObjectMissName               ,"Missing a name for object member") ,                    //!< Missing a name for object member.
            ErrorGroup::value_type(rapidjson::kParseErrorObjectMissColon              ,"Missing a colon after a name of object member") ,       //!< Missing a colon after a name of object member.
            ErrorGroup::value_type(rapidjson::kParseErrorObjectMissCommaOrCurlyBracket,"Missing a comma or '}' after an object member") ,       //!< Missing a comma or '}' after an object member.

            ErrorGroup::value_type(rapidjson::kParseErrorArrayMissCommaOrSquareBracket,"Missing a comma or ']' after an array element") ,       //!< Missing a comma or ']' after an array element.

            ErrorGroup::value_type(rapidjson::kParseErrorStringUnicodeEscapeInvalidHex,"Incorrect hex digit after \\u escape in string") ,      //!< Incorrect hex digit after \\u escape in string.
            ErrorGroup::value_type(rapidjson::kParseErrorStringUnicodeSurrogateInvalid,"The surrogate pair in string is invalid") ,             //!< The surrogate pair in string is invalid.
            ErrorGroup::value_type(rapidjson::kParseErrorStringEscapeInvalid          ,"Invalid escape character in string") ,                  //!< Invalid escape character in string.
            ErrorGroup::value_type(rapidjson::kParseErrorStringMissQuotationMark      ,"Missing a closing quotation mark in string") ,          //!< Missing a closing quotation mark in string.
            ErrorGroup::value_type(rapidjson::kParseErrorStringInvalidEncoding        ,"Invalid encoding in string") ,                          //!< Invalid encoding in string.

            ErrorGroup::value_type(rapidjson::kParseErrorNumberTooBig                 ,"Number too big to be stored in double") ,               //!< Number too big to be stored in double.
            ErrorGroup::value_type(rapidjson::kParseErrorNumberMissFraction           ,"Miss fraction part in number") ,                        //!< Miss fraction part in number.
            ErrorGroup::value_type(rapidjson::kParseErrorNumberMissExponent           ,"Miss exponent in number") ,                             //!< Miss exponent in number.

            ErrorGroup::value_type(rapidjson::kParseErrorTermination                  ,"Parsing was terminated") ,                              //!< Parsing was terminated.
            ErrorGroup::value_type(rapidjson::kParseErrorUnspecificSyntaxError        ,"Unspecific syntax error")                               //!< Unspecific syntax error.
        };
        return error_group_[value.GetParseError()];
    }
};
}
#endif
