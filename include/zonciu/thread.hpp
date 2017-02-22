#ifndef ZONCIU_THREAD_HPP
#define ZONCIU_THREAD_HPP
#include "zonciu/lock.hpp"
#include "zonciu/assert.hpp"
#include <thread>
#include <list>
namespace zonciu
{
class ThreadGroup
{
public:
    ThreadGroup() {}
    ~ThreadGroup()
    {
        for (auto it = _group.begin(); it != _group.end(); ++it)
        {
            delete *it;
        }
    }
    template<typename _Func>
    std::thread* Create(_Func func_)
    {
        zonciu::RecursiveSpinGuard sg(_lock);
        std::thread* thread = new std::thread(func_);
        _group.push_back(thread);
        return thread;
    }

    void Add(std::thread* thread_)
    {
        if (thread_)
        {
            ZONCIU_ASSERT(!IsContainThread(thread_), "This thread is in this group");
            zonciu::RecursiveSpinGuard sg(_lock);
            _group.push_back(thread_);
        }
    }
    void Remove(std::thread* thread_)
    {
        if (thread_)
        {
            std::thread::id id = thread_->get_id();
            zonciu::RecursiveSpinGuard sg(_lock);
            for (auto it = _group.begin(); it != _group.end(); ++it)
            {
                if ((*it)->get_id() == id)
                {
                    delete *it;
                    _group.erase(it);
                    return;
                }
            }
        }
    }
    bool IsContainThread(std::thread* thread_)
    {
        if (thread_)
        {
            std::thread::id id = thread_->get_id();
            zonciu::RecursiveSpinGuard sg(_lock);
            for (auto&it : _group)
            {
                if (it->get_id() == id)
                    return true;
            }
        }
        return false;
    }

    bool IsContainThisThread()
    {
        std::thread::id id = std::this_thread::get_id();
        zonciu::RecursiveSpinGuard sg(_lock);
        for (auto&it : _group)
        {
            if (it->get_id() == id)
                return true;
        }
        return false;
    }

    void JoinAll()
    {
        zonciu::RecursiveSpinGuard sg(_lock);
        for (auto it = _group.begin(); it != _group.end();)
        {
            (*it)->join();
            delete *it;
            it = _group.erase(it);
        }
    }

    bool Join(std::thread* thread_)
    {
        if (thread_)
        {
            std::thread::id id = thread_->get_id();
            zonciu::RecursiveSpinGuard sg(_lock);
            for (auto it = _group.begin(); it != _group.end();)
            {
                if ((*it)->get_id() == id)
                {
                    (*it)->join();
                    delete *it;
                    it = _group.erase(it);
                    return true;
                }
            }
        }
        return false;
    }

    size_t Size()
    {
        zonciu::RecursiveSpinGuard sg(_lock);
        return _group.size();
    }
private:
    ThreadGroup(const ThreadGroup&) = delete;
    const ThreadGroup& operator=(const ThreadGroup&) = delete;
    std::list<std::thread*> _group;
    mutable zonciu::RecursiveSpinLock _lock;
};
}
#endif
