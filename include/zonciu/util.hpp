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
template<class T = std::chrono::milliseconds>
inline void Sleep(T time)
{
    std::this_thread::sleep_for(time);
}
inline void Sleep(unsigned int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
inline void SleepUs(unsigned int us)
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
inline long long Timestamp()
{
    return std::chrono::duration_cast<T>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

//Convert to Uppercase hex string
template<class T>
inline std::string ToHex(const T* data, size_t length)
{
    const unsigned char* ptr = reinterpret_cast<const unsigned char*>(data);
    static const char base[] = {
        '0','1','2','3',
        '4','5','6','7',
        '8','9','A','B',
        'C','D','E','F' };
    size_t pos = 0;
    std::string hexbuf;
    while (pos < length)
    {
        if (pos && !(pos % 16))
            hexbuf.push_back('\n');
        if (static_cast<unsigned char>(*ptr) < 0x10)
            hexbuf.push_back('0');
        else
            hexbuf += base[((static_cast<int>(*ptr) / 16) % 16)];
        hexbuf += base[(static_cast<int>(*ptr) % 16)];
        hexbuf += " ";
        ++pos;
        ++ptr;
    }
    return hexbuf;
}
} // namespace util
} // namespace zonciu
#endif
