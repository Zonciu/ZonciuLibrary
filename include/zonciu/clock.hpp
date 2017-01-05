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
    void Begin() { begin_ = std::chrono::high_resolution_clock::now(); }
    void End() { time_ = std::chrono::high_resolution_clock::now() - begin_; }
    //\default second
    //\std::milli -> millisecond
    //\std::micro -> microsecond
    //\std::nano  -> nanosecond
    template<typename T = std::ratio<1, 1>>
    double Get() {
        return std::chrono::duration<double, T>(time_).count();
    }
private:
    std::chrono::high_resolution_clock::time_point begin_;
    std::chrono::nanoseconds time_;
};
}
#endif
