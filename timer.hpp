/*!
 * Timer
 * \author Zonciu
 * Contact: zonciu@zonciu.com
 *
 * \brief
 * priority_queue implement
 * \note
*/
#ifndef ZONCIU_TIMER_HPP
#define ZONCIU_TIMER_HPP

#include "3rd/concurrentqueue/blockingconcurrentqueue.h"
#include "zonciu/spinlock.hpp"
#include <atomic>
#include <functional>
#include <map>
#include <queue>
#include <vector>
namespace zonciu
{
namespace timer
{
typedef std::function<void()> JobHandle;
namespace detail
{
enum class Flag
{
    stop,
    once,
    forever
};
inline std::uint64_t _now()
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count());
}
}
class ThreadTimer
{
private:

};
/*
 * Multi Timer.
 * example: return - timer id
 * | SetInterval
 * | SetIntervalNow
 * | SetTimes
 * | SetTimesNow
 * | SetOnce - equal to [SetTimes(interval,1,handle)]
*/
class MinHeapTimer
{
private:
    struct Job
    {
        Job(std::uint64_t _id, detail::Flag _flag,
            std::uint64_t _interval, JobHandle _job)
            :
            id(_id), interval(_interval), handle(_job), flag(_flag),
            next_time(detail::_now() + _interval)
        {}
        struct Comp
        {
            bool operator()(const Job* const _left, const Job* const _right)
            {
                return (_left->next_time > _right->next_time);
            }
        };
        const std::uint64_t id;
        const std::uint64_t interval;
        const JobHandle handle;
        std::uint64_t next_time;
        detail::Flag flag;
    };
public:
    //default precision = 100 ms
    MinHeapTimer(std::uint64_t _precision_milli = 10) :
        jobs_id_count_(0),
        destruct_(false),
        running_(true),
        precision_(_precision_milli)
    {
        observer_ = std::thread(&MinHeapTimer::Observe, this);
        worker_ = std::thread(&MinHeapTimer::Work, this);
    }
    ~MinHeapTimer()
    {
        running_ = false;
        destruct_ = true;
        observer_.join();
        queue_.enqueue([]() {});
        worker_.join();
        int i = 0;
        Job* tmp = nullptr;
        while (!jobs_.empty())
        {
            tmp = jobs_.top();
            jobs_.pop();
            delete tmp;
            ++i;
        }
        printf("Delete %d timer", i);
    }
    //Wait and run
    auto SetInterval(std::uint64_t _interval_ms, JobHandle _job)
    {
        auto ret_id = jobs_id_count_++;
        auto* tmp = new Job(ret_id, detail::Flag::forever, _interval_ms, _job);
        id_lock_.Lock();
        jobs_id_.insert(std::make_pair(ret_id, tmp));
        id_lock_.UnLock();
        job_lock_.Lock();
        jobs_.push(tmp);
        job_lock_.UnLock();
        return ret_id;
    }
    //Run it immediately and wait
    auto SetIntervalNow(std::uint64_t _interval_ms, JobHandle _job)
    {
        auto ret_id = jobs_id_count_++;
        auto* tmp = new Job(ret_id, detail::Flag::forever, _interval_ms, _job);
        tmp->next_time = 0;
        id_lock_.Lock();
        jobs_id_.insert(std::make_pair(ret_id, tmp));
        id_lock_.UnLock();
        job_lock_.Lock();
        jobs_.push(tmp);
        job_lock_.UnLock();
        return ret_id;
    }
    //wait and run it once
    auto SetTimeout(std::uint64_t _interval_ms, JobHandle _job)
    {
        auto ret_id = jobs_id_count_++;
        auto* tmp = new Job(ret_id, detail::Flag::once, _interval_ms, _job);
        id_lock_.Lock();
        jobs_id_.insert(std::make_pair(ret_id, tmp));
        id_lock_.UnLock();
        job_lock_.Lock();
        jobs_.push(tmp);
        job_lock_.UnLock();
        return ret_id;
    }
    void StopTimer(std::uint64_t _job_id)
    {
        id_lock_.Lock();
        auto it = jobs_id_.find(_job_id);
        if (it != jobs_id_.end())
            it->second->flag = detail::Flag::stop;
        id_lock_.UnLock();
    }
private:
    MinHeapTimer(const MinHeapTimer&) = delete;
    MinHeapTimer(MinHeapTimer&&) = delete;
    void Observe()
    {
        using namespace std::chrono;
        std::uint64_t _time_begin;
        Job* top = nullptr;
        while (!destruct_)
        {
            while (running_)
            {
                _time_begin = detail::_now();
                job_lock_.Lock();
                while (!jobs_.empty())
                {
                    if (jobs_.top()->next_time <= detail::_now())
                    {
                        top = jobs_.top();
                        jobs_.pop();
                        switch (top->flag)
                        {
                            case detail::Flag::forever:
                            {
                                top->next_time = detail::_now() + top->interval;
                                queue_.enqueue(top->handle);
                                jobs_.push(top);
                                break;
                            }
                            case detail::Flag::once:
                            {
                                queue_.enqueue(top->handle);
                                id_lock_.Lock();
                                jobs_id_.erase(top->id);
                                id_lock_.UnLock();
                                delete top;
                                break;
                            }
                            case detail::Flag::stop:
                            {
                                id_lock_.Lock();
                                jobs_id_.erase(top->id);
                                id_lock_.UnLock();
                                delete top;
                                break;
                            }
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                job_lock_.UnLock();
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(static_cast<std::int64_t>(
                    precision_ - (detail::_now() - _time_begin))
                )
                );
            }
            std::this_thread::yield();
        }
    }
    void Work()
    {
        JobHandle handle;
        while (!destruct_)
        {
            queue_.wait_dequeue(handle);
            handle();
        }
    }
    zonciu::SpinLock id_lock_;
    zonciu::SpinLock job_lock_;
    std::atomic_uint64_t jobs_id_count_;
    std::uint64_t precision_;
    std::atomic_bool destruct_;
    std::atomic_bool running_;
    std::thread observer_;
    std::thread worker_;
    moodycamel::BlockingConcurrentQueue<JobHandle> queue_;
    std::map<std::uint64_t, Job*> jobs_id_;
    std::priority_queue<Job*, std::vector<Job*>, Job::Comp> jobs_;
};
}
}
#endif
