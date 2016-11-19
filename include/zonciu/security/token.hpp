/*
* Copyright(c) 2016 Zonciu Liang.All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*
* Author:  Zonciu Liang
* Contract: zonciu@zonciu.com
* Description: token
*/
#ifndef ZONCIU_SECURITY_TOKEN_HPP
#define ZONCIU_SECURITY_TOKEN_HPP

#include "zonciu/typedef.hpp"
#include "zonciu/random.hpp"
#include <string>
#include <vector>
namespace zonciu
{
namespace security
{
//Token length must be less than 65535, use default dictionary [a-zA-Z0-9]
class Token
{
    std::string Make(u16 length)
    {
        static const byte dictionary[62] = {
            '0','1','2','3','4','5','6','7','8','9',
            'A','B','C','D','E','F','G','H','I','J',
            'K','L','M','N','O','P','Q','R','S','T',
            'U','V','W','X','Y','Z',
            'a','b','c','d','e','f','g','h','i','j',
            'k','l','m','n','o','p','q','r','s','t',
            'u','v','w','x','y','z'
        };
        std::string _token;
        _token.resize(length);
        for (auto&&val : _token)
        {
            val = dictionary[zonciu::Random::Make<u16>(0, 61)];
        }
        return _token;
    }
    //Token length must be less than 65535, you can use your own dictionary.
    std::string Make(u16 length, const std::vector<uint8_t>& dictionary)
    {
        if (dictionary.empty())
            throw std::runtime_error("Dictionary is empty");
        std::string _token;
        size_t dict_end = dictionary.size() - 1;
        _token.resize(length);
        for (auto&&val : _token)
        {
            val = dictionary.at(zonciu::Random::Make<size_t>(0, dict_end));
        }
        return std::move(_token);
    }
};
}
}
#endif
