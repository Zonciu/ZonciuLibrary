/*
* Copyright(c) 2016 Zonciu Liang.All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*
* Author:  Zonciu Liang
* Contract: zonciu@zonciu.com
* Description: lock library
*/
#ifndef ZONCIU_LOCK_HPP
#define ZONCIU_LOCK_HPP
#include <atomic>
#include <thread>
#include <mutex>
namespace zonciu
{
class SpinLock
{
public:
    SpinLock() = default;
    void lock()
    {
        int loop_try = 5;
        while (lock_.test_and_set()) {
            if (!loop_try--) {
                loop_try = 5;
                std::this_thread::yield();
            }
        }
    }
    void unlock() { lock_.clear(); }
private:
    SpinLock(const SpinLock&) = delete;
    const SpinLock& operator=(const SpinLock&) = delete;
    std::atomic_flag lock_ = ATOMIC_FLAG_INIT;
};
class RecursiveSpinLock
{
public:
    RecursiveSpinLock() = default;
    void lock()
    {
        if (!lock_.test_and_set(std::memory_order_acquire)) {
            owner_ = std::this_thread::get_id();
            ++count_;
            return;
        }
        else if (owner_ != std::this_thread::get_id()) {
            int loop_try = 5;
            while (lock_.test_and_set(std::memory_order_acquire)) {
                if (!loop_try--) {
                    loop_try = 5;
                    std::this_thread::yield();
                }
            }
            owner_ = std::this_thread::get_id();
        }
        ++count_;
        return;
    }
    void unlock()
    {
        if (owner_ == std::this_thread::get_id()) {
            if (!--count_)// clear while count_ == 0
                lock_.clear(std::memory_order_release);
        }
    }
private:
    RecursiveSpinLock(const RecursiveSpinLock&) = delete;
    const RecursiveSpinLock& operator=(const RecursiveSpinLock&) = delete;
    std::atomic_flag lock_ = ATOMIC_FLAG_INIT;
    int count_ = 0;
    std::thread::id owner_;
};

typedef std::lock_guard<SpinLock> SpinGuard;
typedef std::lock_guard<RecursiveSpinLock> RecSpinGuard;
}
#endif
