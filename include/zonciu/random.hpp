/*
* Copyright(c) 2016 Zonciu Liang.All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*
* Author:  Zonciu Liang
* Contract: zonciu@zonciu.com
* Description: static class Random
* Api: make(T min,T max)
*      make(T max)
*      make_bool(double p_true)
*/
#ifndef ZONCIU_RANDOM_HPP
#define ZONCIU_RANDOM_HPP
#include <random>
#include <chrono>
#include <zonciu/typedef.hpp>
namespace zonciu
{
//\use std::default_random_engine
//\! no exception
//\! make sure the min/max is correct.
class Random
{
public:
    // 0 <= p_true <= 1, [p_true] chance return true, [1-p_true] chance return false
    static bool make_bool(const double p_true)
    {
        return std::bernoulli_distribution(p_true)(_get_engine());
    }

    // min <= result <= max
    template<class T = i32>
    static T make(const T min, const T max)
    {
        return std::uniform_int_distribution<T>(min, max)(_get_engine());
    }

    // 0 <= result <= max
    template<class T = i32>
    static T make(const T max)
    {
        return std::uniform_int_distribution<T>(0, max)(_get_engine());
    }
private:
    static std::default_random_engine& _get_engine()
    {
        static std::default_random_engine rng_(static_cast<int>(
            std::chrono::system_clock::now().time_since_epoch().count()));
        return rng_;
    }
};
}
#endif
