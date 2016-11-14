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
    Clock() { begin(); }
    void begin() { begin_ = std::chrono::high_resolution_clock::now(); }
    void end() { end_ = std::chrono::high_resolution_clock::now() - begin_; }
    //\default second
    //\std::milli -> millisecond
    //\std::micro -> microsecond
    //\std::nano  -> nanosecond
    template<typename T = std::ratio<1, 1>>
    auto get()
    {
        return std::chrono::duration<double, T>(end_).count();
    }
private:
    mutable std::chrono::high_resolution_clock::time_point begin_;
    mutable std::chrono::nanoseconds end_;
};
}
#endif
