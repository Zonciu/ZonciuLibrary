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
class Random
{
public:
    //p = [0,1], p -> true, 1-p -> false
    static bool MakeBool(const double& p)
    {
        return std::bernoulli_distribution(p)(_GetEngine());
    }
    //[min,max]
    template<class T = i32>
    static T Make(const T& min, const T& max)
    {
        return std::uniform_int_distribution<T>(min, max)(_GetEngine());
    }
    //[0,max]
    template<class T = i32>
    static T Make(const T& max)
    {
        return std::uniform_int_distribution<T>(0, max)(_GetEngine());
    }
private:
    static std::default_random_engine& _GetEngine()
    {
        static std::default_random_engine rng_(static_cast<int>(
            std::chrono::system_clock::now().time_since_epoch().count()));
        return rng_;
    }
};
}
#endif
