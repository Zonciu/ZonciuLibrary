/*
* Copyright(c) 2016 Zonciu Liang.All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*
* Author:  Zonciu Liang
* Contract: zonciu@zonciu.com
*/

#ifndef ZONCIU_TIMER_HPP
#define ZONCIU_TIMER_HPP

#include "zonciu/3rd/concurrentqueue/blockingconcurrentqueue.h"
#include "zonciu/spinlock.hpp"
#include "zonciu/semaphor.hpp"
#include <functional>
#include <atomic>
#include <map>
#include <queue>
#include <vector>
namespace zonciu
{
namespace timer
{
/*
 * Min heap timer.
 * api:
 * | SetInterval - return (unsigned int)timer_id
 * | SetTimeOut  - return (unsigned int)timer_id
 * | StopTimer
 * | StopAll
*/
class MinHeapTimer
{
public:
    typedef std::function<void()> TimerHandle;
    typedef int TimerId;
    typedef std::chrono::duration<std::uint64_t, std::micro> IntervalType;
private:
    enum class Flag
    {
        stop,
        once,
        forever
    };
    class Job
    {
    public:
        Job(TimerId _id, Flag _flag,
            IntervalType _interval, TimerHandle _job)
            :
            id(_id), interval(_interval), handle(_job), flag(_flag)
        {}
        struct Comp
        {
            bool operator()(const Job* const _left, const Job* const _right)
            {
                return (_left->next_time > _right->next_time);
            }
        };
        const TimerId id;
        const IntervalType interval;
        const TimerHandle handle;
        std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::microseconds> next_time;
        Flag flag;
    };
public:
    MinHeapTimer() :
        jobs_id_count_(0),
        destruct_(false)
    {
        observer_ = std::thread(&MinHeapTimer::_observe, this);
        worker_ = std::thread(&MinHeapTimer::_work, this);
    }
    ~MinHeapTimer()
    {
        destruct_ = true;
        observer_.join();
        queue_.enqueue([]() {});
        worker_.join();
        remove_all();
    }
    //Wait and run
    //Return timer_id
    TimerId set_interval(std::uint32_t interval_milli, TimerHandle func)
    {
        using namespace std::chrono;
        TimerId ret_id = jobs_id_count_++;
        auto* tmp = new Job(ret_id, Flag::forever, IntervalType(milliseconds(interval_milli)), func);
        tmp->next_time = time_point_cast<microseconds>(high_resolution_clock::now() + tmp->interval);
        zonciu::SpinGuard idlck(id_lock_);
        jobs_id_.insert(std::make_pair(ret_id, tmp));
        zonciu::SpinGuard joblck(jobs_lock_);
        jobs_.push(tmp);
        waiter_.signal();
        return ret_id;
    }
    template<class _Rep, class _Period>
    TimerId set_interval(std::chrono::duration<_Rep, _Period> interval, TimerHandle func)
    {
        using namespace std::chrono;
        TimerId ret_id = jobs_id_count_++;
        auto* tmp = new Job(ret_id, Flag::forever, duration_cast<microseconds>(interval), func);
        tmp->next_time = time_point_cast<microseconds>(high_resolution_clock::now() + tmp->interval);
        zonciu::SpinGuard idlck(id_lock_);
        jobs_id_.insert(std::make_pair(ret_id, tmp));
        zonciu::SpinGuard joblck(jobs_lock_);
        jobs_.push(tmp);
        waiter_.signal();
        return ret_id;
    }
    //Wait and run once
    //Return timer_id
    TimerId set_timeout(std::uint32_t interval_milli, TimerHandle func)
    {
        using namespace std::chrono;
        TimerId ret_id = jobs_id_count_++;
        auto* tmp = new Job(ret_id, Flag::once, IntervalType(milliseconds(interval_milli)), func);
        tmp->next_time = tmp->next_time = time_point_cast<microseconds>(high_resolution_clock::now() + tmp->interval);
        zonciu::SpinGuard idlck(id_lock_);
        jobs_id_.insert(std::make_pair(ret_id, tmp));
        zonciu::SpinGuard joblck(jobs_lock_);
        jobs_.push(tmp);
        waiter_.signal();
        return ret_id;
    }
    template<class _Rep, class _Period>
    TimerId set_timeout(std::chrono::duration<_Rep, _Period> interval, TimerHandle func)
    {
        using namespace std::chrono;
        TimerId ret_id = jobs_id_count_++;
        auto* tmp = new Job(ret_id, Flag::once, duration_cast<microseconds>(interval), func);
        tmp->next_time = time_point_cast<microseconds>(high_resolution_clock::now() + tmp->interval);
        zonciu::SpinGuard idlck(id_lock_);
        jobs_id_.insert(std::make_pair(ret_id, tmp));
        zonciu::SpinGuard joblck(jobs_lock_);
        jobs_.push(tmp);
        waiter_.signal();
        return ret_id;
    }
    //Return false if timer not found
    bool remove_one(TimerId timer_id)
    {
        zonciu::SpinGuard idlck(id_lock_);
        auto it = jobs_id_.find(timer_id);
        if (it != jobs_id_.end())
        {
            it->second->flag = Flag::stop;
            return true;
        }
        else
            return false;
    }
    void remove_all()
    {
        zonciu::SpinGuard idlck(id_lock_);
        zonciu::SpinGuard joblck(jobs_lock_);
        Job* tmp = nullptr;
        while (!jobs_.empty())
        {
            tmp = jobs_.top();
            jobs_.pop();
            delete tmp;
        }
        jobs_id_.clear();
        waiter_.signal();
    }
private:
    void _observe()
    {
        using namespace std::chrono;
        Job* top = nullptr;
        while (!destruct_)
        {
            jobs_lock_.lock();
            if (!jobs_.empty())
            {
                for (top = jobs_.top();top->next_time <= high_resolution_clock::now();top = jobs_.top())
                {
                    jobs_.pop();
                    switch (top->flag)
                    {
                        case Flag::forever:
                        {
                            top->next_time += top->interval;
                            queue_.enqueue(top->handle);
                            jobs_.push(top);
                            break;
                        }
                        case Flag::once:
                        {
                            queue_.enqueue(top->handle);
                            zonciu::SpinGuard idlck(id_lock_);
                            jobs_id_.erase(top->id);
                            delete top;
                            break;
                        }
                        default:
                        {
                            zonciu::SpinGuard idlck(id_lock_);
                            jobs_id_.erase(top->id);
                            delete top;
                            break;
                        }
                    }
                }
                jobs_lock_.unlock();
                if (jobs_.top()->next_time > high_resolution_clock::now())
                {
                    auto wait_time = duration_cast<microseconds>(jobs_.top()->next_time - high_resolution_clock::now());
                    waiter_.timed_wait((wait_time.count() > 0) ? wait_time.count() : 0);
                }
            }
            else
            {
                jobs_lock_.unlock();
                waiter_.wait();
            }
        }
    }
    void _work()
    {
        TimerHandle handle;
        while (!destruct_)
        {
            queue_.wait_dequeue(handle);
            handle();
        }
    }
    zonciu::Semaphore waiter_;
    zonciu::SpinLock id_lock_;
    zonciu::SpinLock jobs_lock_;
    std::atomic<TimerId> jobs_id_count_;
    bool destruct_;
    std::thread observer_;
    std::thread worker_;
    moodycamel::BlockingConcurrentQueue<TimerHandle> queue_;
    std::map<unsigned int, Job*> jobs_id_;
    std::priority_queue<Job*, std::vector<Job*>, Job::Comp> jobs_;
}; // class MinHeapTimer

} // namespace timer
} // namespace zonciu

#endif // ZONCIU_TIMER_HPP
