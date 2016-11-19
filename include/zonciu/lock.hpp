/*
* Copyright(c) 2016 Zonciu Liang.All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*
* Author:  Zonciu Liang
* Contract: zonciu@zonciu.com
*/
#ifndef ZONCIU_SPIN_LOCK_HPP
#define ZONCIU_SPIN_LOCK_HPP
#include <atomic>
#include <mutex>
#include <thread>
namespace zonciu
{
//Use atomic_flag, yield after 5 spins.
class SpinLock
{
public:
    SpinLock() = default;
    void Lock()
    {
        if (!_lock.test_and_set(std::memory_order_acquire))
        {
            _owner = std::this_thread::get_id();
            ++count_;
            return;
        }
        else
        {
            if (_owner != std::this_thread::get_id())
            {
                int loop_try = 5;
                while (_lock.test_and_set(std::memory_order_acquire))
                {
                    if (!loop_try--)
                    {
                        loop_try = 5;
                        std::this_thread::yield();
                    }
                }
                _owner = std::this_thread::get_id();
                ++count_;
                return;
            }
            else
            {
                ++count_;
                return;
            }
        }
    }
    void Unlock()
    {
        if (_owner == std::this_thread::get_id())
        {
            --count_;
            if (count_ == 0)
            {
                _lock.clear(std::memory_order_release);
                _owner = std::thread::id();
            }
        }
    }
private:
    SpinLock(const SpinLock&) = delete;
    const SpinLock& operator=(const SpinLock&) = delete;
    std::atomic_flag _lock = ATOMIC_FLAG_INIT;
    int count_ = 0;
    std::thread::id _owner;
};
typedef std::lock_guard<SpinLock> SpinGuard;
}
#endif
