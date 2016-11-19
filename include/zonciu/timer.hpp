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
#include "zonciu/lock.hpp"
#include "zonciu/semaphor.hpp"
#include <stdint.h>
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
    struct Job
    {
        Job(TimerId _id, Flag _flag, IntervalType _interval, TimerHandle _handle)
            :
            id(_id), interval(_interval), handle(_handle), flag(_flag)
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
        _jobs_id_count(0),
        _destruct(false)
    {
        _observer = std::thread(&MinHeapTimer::_Observe, this);
        _worker = std::thread(&MinHeapTimer::_Work, this);
    }
    ~MinHeapTimer()
    {
        _destruct = true;
        _observer.join();
        _queue.enqueue([]() {});
        _worker.join();
        Clear();
    }
    //Wait and run
    //Return timer_id
    TimerId SetInterval(std::uint32_t interval_milli, TimerHandle func)
    {
        using namespace std::chrono;
        TimerId ret_id = _jobs_id_count++;
        auto* tmp = new Job(ret_id, Flag::forever, IntervalType(milliseconds(interval_milli)), func);
        tmp->next_time = time_point_cast<microseconds>(high_resolution_clock::now() + tmp->interval);
        zonciu::SpinGuard idlck(_id_lock);
        _jobs_id.insert(std::make_pair(ret_id, tmp));
        zonciu::SpinGuard joblck(_jobs_lock);
        _jobs.push(tmp);
        _waiter.signal();
        return ret_id;
    }
    template<class _Rep, class _Period>
    TimerId SetInterval(std::chrono::duration<_Rep, _Period> interval, TimerHandle func)
    {
        using namespace std::chrono;
        TimerId ret_id = _jobs_id_count++;
        auto* tmp = new Job(ret_id, Flag::forever, duration_cast<microseconds>(interval), func);
        tmp->next_time = time_point_cast<microseconds>(high_resolution_clock::now() + tmp->interval);
        zonciu::SpinGuard idlck(_id_lock);
        _jobs_id.insert(std::make_pair(ret_id, tmp));
        zonciu::SpinGuard joblck(_jobs_lock);
        _jobs.push(tmp);
        _waiter.signal();
        return ret_id;
    }
    //Wait and run once
    //Return timer_id
    TimerId SetTimeout(std::uint32_t interval_milli, TimerHandle func)
    {
        using namespace std::chrono;
        TimerId ret_id = _jobs_id_count++;
        auto* tmp = new Job(ret_id, Flag::once, IntervalType(milliseconds(interval_milli)), func);
        tmp->next_time = tmp->next_time = time_point_cast<microseconds>(high_resolution_clock::now() + tmp->interval);
        zonciu::SpinGuard idlck(_id_lock);
        _jobs_id.insert(std::make_pair(ret_id, tmp));
        zonciu::SpinGuard joblck(_jobs_lock);
        _jobs.push(tmp);
        _waiter.signal();
        return ret_id;
    }
    template<class _Rep, class _Period>
    TimerId SetTimeout(std::chrono::duration<_Rep, _Period> interval, TimerHandle func)
    {
        using namespace std::chrono;
        TimerId ret_id = _jobs_id_count++;
        auto* tmp = new Job(ret_id, Flag::once, duration_cast<microseconds>(interval), func);
        tmp->next_time = time_point_cast<microseconds>(high_resolution_clock::now() + tmp->interval);
        zonciu::SpinGuard idlck(_id_lock);
        _jobs_id.insert(std::make_pair(ret_id, tmp));
        zonciu::SpinGuard joblck(_jobs_lock);
        _jobs.push(tmp);
        _waiter.signal();
        return ret_id;
    }
    //Return false if timer not found
    bool Remove(TimerId timer_id)
    {
        zonciu::SpinGuard idlck(_id_lock);
        auto it = _jobs_id.find(timer_id);
        if (it != _jobs_id.end())
        {
            it->second->flag = Flag::stop;
            return true;
        }
        else
            return false;
    }
    void Clear()
    {
        zonciu::SpinGuard idlck(_id_lock);
        zonciu::SpinGuard joblck(_jobs_lock);
        Job* tmp = nullptr;
        while (!_jobs.empty())
        {
            tmp = _jobs.top();
            _jobs.pop();
            delete tmp;
        }
        _jobs_id.clear();
        _waiter.signal();
    }
private:
    void _Observe()
    {
        using namespace std::chrono;
        Job* top = nullptr;
        while (!_destruct)
        {
            _jobs_lock.lock();
            if (!_jobs.empty())
            {
                for (top = _jobs.top();top->next_time <= high_resolution_clock::now();top = _jobs.top())
                {
                    _jobs.pop();
                    switch (top->flag)
                    {
                        case Flag::forever:
                        {
                            top->next_time += top->interval;
                            _queue.enqueue(top->handle);
                            _jobs.push(top);
                            break;
                        }
                        case Flag::once:
                        {
                            _queue.enqueue(top->handle);
                            zonciu::SpinGuard idlck(_id_lock);
                            _jobs_id.erase(top->id);
                            delete top;
                            break;
                        }
                        default:
                        {
                            zonciu::SpinGuard idlck(_id_lock);
                            _jobs_id.erase(top->id);
                            delete top;
                            break;
                        }
                    }
                }
                _jobs_lock.unlock();
                if (_jobs.top()->next_time > high_resolution_clock::now())
                {
                    auto wait_time = duration_cast<microseconds>(_jobs.top()->next_time - high_resolution_clock::now());
                    _waiter.timed_wait((wait_time.count() > 0) ? wait_time.count() : 0);
                }
            }
            else
            {
                _jobs_lock.unlock();
                _waiter.wait();
            }
        }
    }
    void _Work()
    {
        TimerHandle handle;
        while (!_destruct)
        {
            _queue.wait_dequeue(handle);
            handle();
        }
    }
    zonciu::Semaphore _waiter;
    zonciu::SpinLock _id_lock;
    zonciu::SpinLock _jobs_lock;
    std::atomic<TimerId> _jobs_id_count;
    bool _destruct;
    std::thread _observer;
    std::thread _worker;
    moodycamel::BlockingConcurrentQueue<TimerHandle> _queue;
    std::map<unsigned int, Job*> _jobs_id;
    std::priority_queue<Job*, std::vector<Job*>, Job::Comp> _jobs;
}; // class MinHeapTimer

} // namespace timer
} // namespace zonciu

#endif // ZONCIU_TIMER_HPP
