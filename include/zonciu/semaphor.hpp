/*
* Updater: Zonciu Liang
* Contract: zonciu@zonciu.com
* Description:
* - Code from moodycamel::BlockingConcurrentQueue
* - Jeff Preshing's semaphore implementation (under the terms of its separate
* - zlib license, embedded below).
* Update: Unify code style;
* BSD license.
*/
#ifndef ZONCIU_SEMAPHORE_H
#define ZONCIU_SEMAPHORE_H
#include <assert.h>

#if defined(_WIN32)
// Avoid including windows.h in a header; we only need a handful of
// items, so we'll redeclare them here (this is relatively safe since
// the API generally has to remain stable between Windows versions).
// I know this is an ugly hack but it still beats polluting the global
// namespace with thousands of generic names or adding a .cpp for nothing.
extern "C" {
    struct _SECURITY_ATTRIBUTES;
    __declspec(dllimport) void* __stdcall CreateSemaphoreW(_SECURITY_ATTRIBUTES* lpSemaphoreAttributes, long lInitialCount, long lMaximumCount, const wchar_t* lpName);
    __declspec(dllimport) int __stdcall CloseHandle(void* hObject);
    __declspec(dllimport) unsigned long __stdcall WaitForSingleObject(void* hHandle, unsigned long dwMilliseconds);
    __declspec(dllimport) int __stdcall ReleaseSemaphore(void* hSemaphore, long lReleaseCount, long* lpPreviousCount);
}
#elif defined(__MACH__)
#include <mach/mach.h>
#elif defined(__unix__)
#include <semaphore.h>
#endif
namespace zonciu
{
#if defined(_WIN32)
class Semaphore
{
private:
    void* _sema;

    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

public:
    Semaphore(int initialCount = 0)
    {
        assert(initialCount >= 0);
        const long maxLong = 0x7fffffff;
        _sema = CreateSemaphoreW(nullptr, initialCount, maxLong, nullptr);
    }

    ~Semaphore()
    {
        CloseHandle(_sema);
    }

    void Wait()
    {
        const unsigned long infinite = 0xffffffff;
        WaitForSingleObject(_sema, infinite);
    }

    bool TryWait()
    {
        const unsigned long RC_WAIT_TIMEOUT = 0x00000102;
        return WaitForSingleObject(_sema, 0) != RC_WAIT_TIMEOUT;
    }
    //microsecond
    bool WaitFor(unsigned long long us)
    {
        const unsigned long RC_WAIT_TIMEOUT = 0x00000102;
        return WaitForSingleObject(_sema, (unsigned long)(us / 1000)) != RC_WAIT_TIMEOUT;
    }

    void Signal(int count = 1)
    {
        ReleaseSemaphore(_sema, count, nullptr);
    }
};
#elif defined(__MACH__)
//---------------------------------------------------------
// Semaphore (Apple iOS and OSX)
// Can't use POSIX semaphores due to http://lists.apple.com/archives/darwin-kernel/2009/Apr/msg00010.html
//---------------------------------------------------------
class Semaphore
{
private:
    semaphore_t _sema;

    Semaphore(const Semaphore& other) = delete;
    Semaphore& operator=(const Semaphore& other) = delete;

public:
    Semaphore(int initialCount = 0)
    {
        assert(initialCount >= 0);
        semaphore_create(mach_task_self(), &_sema, SYNC_POLICY_FIFO, initialCount);
    }

    ~Semaphore()
    {
        semaphore_destroy(mach_task_self(), _sema);
    }

    void Wait()
    {
        semaphore_wait(_sema);
    }

    bool TryWait()
    {
        return timed_wait(0);
    }
    //microsecond
    bool WaitFor(unsigned long long us)
    {
        mach_timespec_t ts;
        ts.tv_sec = us / 1000000;
        ts.tv_nsec = (us % 1000000) * 1000;

        // added in OSX 10.10: https://developer.apple.com/library/prerelease/mac/documentation/General/Reference/APIDiffsMacOSX10_10SeedDiff/modules/Darwin.html
        kern_return_t rc = semaphore_timedwait(_sema, ts);

        return rc != KERN_OPERATION_TIMED_OUT;
    }

    void Signal()
    {
        semaphore_signal(_sema);
    }

    void Signal(int count)
    {
        while (count-- > 0)
        {
            semaphore_signal(_sema);
        }
    }
};
#elif defined(__unix__)
//---------------------------------------------------------
// Semaphore (POSIX, Linux)
//---------------------------------------------------------
class Semaphore
{
private:
    sem_t _sema;

    Semaphore(const Semaphore& other) = delete;
    Semaphore& operator=(const Semaphore& other) = delete;

public:
    Semaphore(int initialCount = 0)
    {
        assert(initialCount >= 0);
        sem_init(&_sema, 0, initialCount);
    }

    ~Semaphore()
    {
        sem_destroy(&_sema);
    }

    void Wait()
    {
        // http://stackoverflow.com/questions/2013181/gdb-causes-sem-wait-to-fail-with-eintr-error
        int rc;
        do
        {
            rc = sem_wait(&_sema);
        } while (rc == -1 && errno == EINTR);
    }

    bool TryWait()
    {
        int rc;
        do
        {
            rc = sem_trywait(&_sema);
        } while (rc == -1 && errno == EINTR);
        return !(rc == -1 && errno == EAGAIN);
    }

    bool WaitFor(unsigned long long us)
    {
        struct timespec ts;
        const int usecs_in_1_sec = 1000000;
        const int nsecs_in_1_sec = 1000000000;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += us / usecs_in_1_sec;
        ts.tv_nsec += (us % usecs_in_1_sec) * 1000;
        // sem_timedwait bombs if you have more than 1e9 in tv_nsec
        // so we have to clean things up before passing it in
        if (ts.tv_nsec > nsecs_in_1_sec)
        {
            ts.tv_nsec -= nsecs_in_1_sec;
            ++ts.tv_sec;
        }

        int rc;
        do
        {
            rc = sem_timedwait(&_sema, &ts);
        } while (rc == -1 && errno == EINTR);
        return !(rc == -1 && errno == ETIMEDOUT);
    }

    void Signal()
    {
        sem_post(&_sema);
    }

    void Signal(int count)
    {
        while (count-- > 0)
        {
            sem_post(&_sema);
        }
    }
};
#else
#error Unsupported platform! (No semaphore wrapper available)
#endif
} // namespace zonciu
#endif
