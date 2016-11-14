/*
* Copyright(c) 2016 Zonciu Liang.All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*
* Author:  Zonciu Liang
* Contract: zonciu@zonciu.com
*/
#ifndef ZONCIU_SPINLOCK_HPP
#define ZONCIU_SPINLOCK_HPP
#include <atomic>
#include <mutex>

namespace zonciu
{
//Use atomic_flag, yield after 5 spins.
class SpinLock
{
public:
    SpinLock() = default;
    void lock()
    {
        int loop_try = 5;
        while (lock_.test_and_set(std::memory_order_acquire))
        {
            if (!loop_try--)
            {
                loop_try = 5;
                std::this_thread::yield();
            }
        }
    }
    void unlock() { lock_.clear(std::memory_order_release); }
private:
    SpinLock(const SpinLock&) = delete;
    const SpinLock& operator=(const SpinLock&) = delete;
    std::atomic_flag lock_ = ATOMIC_FLAG_INIT;
};

typedef std::lock_guard<SpinLock> SpinGuard;
}
#endif
