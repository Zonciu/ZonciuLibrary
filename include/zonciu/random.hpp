/*
* Copyright(c) 2016 Zonciu Liang.All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*
* Author:  Zonciu Liang
* Contract: zonciu@zonciu.com
* Description: static class Random
* Api: Num(T min,T max)
*      Num(T max)
*      Bool(double p_true)
*/
#ifndef ZONCIU_RANDOM_HPP
#define ZONCIU_RANDOM_HPP
#include <random>
#include <chrono>
namespace zonciu
{
//\use std::default_random_engine
//\! no exception
//\! make sure the min/max is correct.
class Random
{
public:
    // 0 <= p_true <= 1, [p_true] chance return true, [1-p_true] chance return false
    static bool Bool(double p_true)
    {
        return std::bernoulli_distribution(p_true)(_GetEngine());
    }

    // min <= result <= max
    template<class T = int>
    static T Num(T min, T max)
    {
        return std::uniform_int_distribution<T>(min, max)(_GetEngine());
    }

    // 0 <= result <= max
    template<class T = int>
    static T Num(T max)
    {
        return std::uniform_int_distribution<T>(0, max)(_GetEngine());
    }
private:
    static std::default_random_engine& _GetEngine()
    {
        static std::default_random_engine rng(static_cast<int>(
            std::chrono::system_clock::now().time_since_epoch().count()));
        return rng;
    }
};
}
#endif
