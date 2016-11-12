/*
* Copyright(c) 2016 Zonciu Liang.All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*
* Author:  Zonciu Liang
* Contract: zonciu@zonciu.com
*/
#ifndef ZONCIU_UTIL_H
#define ZONCIU_UTIL_H

#include <string>
#include <chrono>
#include <thread>

#define COUNTOF(Array) (sizeof(Array) / sizeof(Array[0]))

namespace zonciu
{
namespace util
{
inline void Sleep(unsigned int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
inline void uSleep(unsigned int us)
{
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::microseconds(us);
    do
    {
        std::this_thread::yield();
    } while (std::chrono::high_resolution_clock::now() < end);
}

//T = duration types
template<typename T = std::chrono::seconds>
inline auto GetTimeStamp()
{
    return std::chrono::duration_cast<T>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

//Convert to Uppercase hex string
template<class T>
inline std::string ToHex(const T* const _ptr, size_t _size)
{
    const unsigned char* _tp = reinterpret_cast<const unsigned char*>(_ptr);
    static char base[] = {
        '0','1','2','3',
        '4','5','6','7',
        '8','9','A','B',
        'C','D','E','F' };
    size_t _pos = 0;
    std::string _hexbuf;
    while (_pos < _size)
    {
        if (_pos && !(_pos & 15))
            _hexbuf.push_back('\n');
        if (static_cast<unsigned char>(*_tp) < 0x10)
            _hexbuf.push_back('0');
        else
            _hexbuf += base[((static_cast<int>(*_tp) >> 4) & 15)];
        _hexbuf += base[(static_cast<int>(*_tp) & 15)];
        _hexbuf += " ";
        ++_pos;
        ++_tp;
    }
    return _hexbuf;
}
} // namespace util
} // namespace zonciu
#endif
