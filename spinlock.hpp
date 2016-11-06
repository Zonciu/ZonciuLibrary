/*!
 * 
 * \author Zonciu
 * Contact: zonciu@zonciu.com
 *
 * \brief
 * SpinLock
 * \note
*/
#ifndef ZONCIU_SPINLOCK_HPP
#define ZONCIU_SPINLOCK_HPP
#include <atomic>
namespace zonciu
{
//Use atomic_flag, yield after 5 spins.
class SpinLock
{
public:
    SpinLock() { lock_.clear(); }
    ~SpinLock() { lock_.clear(); }
    inline void Lock()
    {
        int loop_try = 5;
        while (lock_.test_and_set())
        {
            if (!loop_try--)
            {
                loop_try = 5;
                std::this_thread::yield();
            }
        }
    }
    inline void UnLock() { lock_.clear(); }
private:
    std::atomic_flag lock_;
};
}
#endif
