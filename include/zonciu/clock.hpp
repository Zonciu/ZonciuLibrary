/*!
 *
 * \author Zonciu
 * Contact: zonciu@zonciu.com
 *
 * \brief
 * Random
 * \note
*/
#ifndef ZONCIU_CLOCK_HPP
#define ZONCIU_CLOCK_HPP
#include <chrono>
namespace zonciu
{
//calculate time passed, use std::chrono
class Clock
{
public:
    Clock() { Begin(); }
    void Begin() { _begin = std::chrono::high_resolution_clock::now(); }
    void End() { _time = std::chrono::high_resolution_clock::now() - _begin; }
    //\default second
    //\std::milli -> millisecond
    //\std::micro -> microsecond
    //\std::nano  -> nanosecond
    template<typename T = std::ratio<1, 1>>
    double Get()
    {
        return std::chrono::duration<double, T>(_time).count();
    }
private:
    std::chrono::high_resolution_clock::time_point _begin;
    std::chrono::nanoseconds _time;
};
}
#endif
