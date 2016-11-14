/*!
 *
 * \author Zonciu
 * Contact: zonciu@zonciu.com
 *
 * \brief
 * Random
 * \note
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
namespace random
{
//p = [0,1], p -> true, 1-p -> false
inline static bool make_bool(const double& p)
{
    return std::bernoulli_distribution(p)(_get_engine());
}

//[min,max]
template<class T = i32>
inline static T make(const T& min, const T& max)
{
    return std::uniform_int_distribution<T>(min, max)(_get_engine());
}

//[0,max]
template<class T = i32>
inline static T make(const T& max)
{
    return std::uniform_int_distribution<T>(0, max)(_get_engine());
}

inline static std::default_random_engine& _get_engine()
{
    static std::default_random_engine rng_(static_cast<int>(
        std::chrono::system_clock::now().time_since_epoch().count()));
    return rng_;
}
}
}
#endif
