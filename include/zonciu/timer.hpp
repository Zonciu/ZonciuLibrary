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
        Job(unsigned int _id, Flag _flag,
            unsigned long long _interval_us, TimerHandle _job)
            :
            id(_id), interval(_interval_us), handle(_job), flag(_flag)
        {}
        struct Comp
        {
            bool operator()(const Job* const _left,
                const Job* const _right)
            {
                return (_left->next_time > _right->next_time);
            }
        };
        const unsigned int id;
        const unsigned long long interval;
        const TimerHandle handle;
        long long next_time;
        Flag flag;
    };
public:
    MinHeapTimer() :
        jobs_id_count_(0),
        destruct_(false)
    {
        observer_ = std::thread(&MinHeapTimer::_Observe, this);
        worker_ = std::thread(&MinHeapTimer::_Work, this);
    }
    ~MinHeapTimer()
    {
        destruct_ = true;
        observer_.join();
        queue_.enqueue([]() {});
        worker_.join();
        StopAll();
    }
    //Wait and run
    //Return timer_id
    unsigned int SetInterval(unsigned int _interval_ms, TimerHandle _func)
    {
        auto ret_id = jobs_id_count_++;
        auto* tmp = new Job(ret_id, Flag::forever, _interval_ms * 1000, _func);
        tmp->next_time = _Now() + tmp->interval;
        id_lock_.Lock();
        jobs_id_.insert(std::make_pair(ret_id, tmp));
        id_lock_.UnLock();
        jobs_lock_.Lock();
        jobs_.push(tmp);
        jobs_lock_.UnLock();
        waiter_.signal();
        return ret_id;
    }
    //Wait and run once
    //Return timer_id
    unsigned int SetTimeout(unsigned int _interval_ms, TimerHandle _func)
    {
        auto ret_id = jobs_id_count_++;
        auto* tmp = new Job(ret_id, Flag::once, _interval_ms * 1000, _func);
        tmp->next_time = _Now() + tmp->interval;
        id_lock_.Lock();
        jobs_id_.insert(std::make_pair(ret_id, tmp));
        id_lock_.UnLock();
        jobs_lock_.Lock();
        jobs_.push(tmp);
        jobs_lock_.UnLock();
        waiter_.signal();
        return ret_id;
    }
    void StopTimer(unsigned int _job_id)
    {
        id_lock_.Lock();
        auto it = jobs_id_.find(_job_id);
        if (it != jobs_id_.end())
            it->second->flag = Flag::stop;
        id_lock_.UnLock();
        waiter_.signal();
    }
    void StopAll()
    {
        id_lock_.Lock();
        jobs_lock_.Lock();
        Job* tmp = nullptr;
        while (!jobs_.empty())
        {
            tmp = jobs_.top();
            jobs_.pop();
            delete tmp;
        }
        jobs_id_.clear();
        jobs_lock_.UnLock();
        id_lock_.UnLock();
        waiter_.signal();
    }
private:
    MinHeapTimer(const MinHeapTimer&) = delete;
    MinHeapTimer(MinHeapTimer&&) = delete;
    void _Observe()
    {
        /*const std::chrono::duration<int, std::micro> tick(10);*/
        Job* top = nullptr;
        while (!destruct_)
        {
            jobs_lock_.Lock();
            if (!jobs_.empty())
            {
                for (top = jobs_.top();top->next_time <= _Now();top = jobs_.top())
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
                            id_lock_.Lock();
                            jobs_id_.erase(top->id);
                            id_lock_.UnLock();
                            delete top;
                            break;
                        }
                        case Flag::stop:
                        {
                            id_lock_.Lock();
                            jobs_id_.erase(top->id);
                            id_lock_.UnLock();
                            delete top;
                            break;
                        }
                    }
                }
                if (jobs_.top()->next_time > _Now())
                {
                    auto wait_time = jobs_.top()->next_time - _Now();
                    waiter_.timed_wait((wait_time > 0) ? wait_time : 0);
                }
                jobs_lock_.UnLock();
            }
            else
            {
                jobs_lock_.UnLock();
                waiter_.wait();
            }
        }
    }
    void _Work()
    {
        TimerHandle handle;
        while (!destruct_)
        {
            queue_.wait_dequeue(handle);
            handle();
        }
    }
    long long _Now()
    {
        using namespace std::chrono;
        return duration_cast<microseconds>(
            high_resolution_clock::now().time_since_epoch()).count();
    };
    zonciu::semaphore::Semaphore waiter_;
    zonciu::SpinLock id_lock_;
    zonciu::SpinLock jobs_lock_;
    std::atomic<int> jobs_id_count_;
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
