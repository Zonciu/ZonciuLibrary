/*!
 *
 * author Zonciu
 * Contact: zonciu@zonciu.com
 * BSD license.
 * \brief
 * Jeff Preshing's semaphore implementation (under the terms of its separate
 * zlib license, embedded below).
 * Code from moodycamel::BlockingConcurrentQueue
 * \note
*/
#ifndef ZONCIU_SEMAPHORE_H
#define ZONCIU_SEMAPHORE_H
#include <assert.h>
#include <stdint.h>
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
    void* m_hSema;

    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

public:
    Semaphore(int initialCount = 0)
    {
        assert(initialCount >= 0);
        const long maxLong = 0x7fffffff;
        m_hSema = CreateSemaphoreW(nullptr, initialCount, maxLong, nullptr);
    }

    ~Semaphore()
    {
        CloseHandle(m_hSema);
    }

    void wait()
    {
        const unsigned long infinite = 0xffffffff;
        WaitForSingleObject(m_hSema, infinite);
    }

    bool try_wait()
    {
        const unsigned long RC_WAIT_TIMEOUT = 0x00000102;
        return WaitForSingleObject(m_hSema, 0) != RC_WAIT_TIMEOUT;
    }

    bool timed_wait(unsigned long long usecs)
    {
        const unsigned long RC_WAIT_TIMEOUT = 0x00000102;
        return WaitForSingleObject(m_hSema, (unsigned long)(usecs / 1000)) != RC_WAIT_TIMEOUT;
    }

    void signal(int count = 1)
    {
        ReleaseSemaphore(m_hSema, count, nullptr);
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
    semaphore_t m_sema;

    Semaphore(const Semaphore& other) = delete;
    Semaphore& operator=(const Semaphore& other) = delete;

public:
    Semaphore(int initialCount = 0)
    {
        assert(initialCount >= 0);
        semaphore_create(mach_task_self(), &m_sema, SYNC_POLICY_FIFO, initialCount);
    }

    ~Semaphore()
    {
        semaphore_destroy(mach_task_self(), m_sema);
    }

    void wait()
    {
        semaphore_wait(m_sema);
    }

    bool try_wait()
    {
        return timed_wait(0);
    }

    bool timed_wait(unsigned long long timeout_usecs)
    {
        mach_timespec_t ts;
        ts.tv_sec = timeout_usecs / 1000000;
        ts.tv_nsec = (timeout_usecs % 1000000) * 1000;

        // added in OSX 10.10: https://developer.apple.com/library/prerelease/mac/documentation/General/Reference/APIDiffsMacOSX10_10SeedDiff/modules/Darwin.html
        kern_return_t rc = semaphore_timedwait(m_sema, ts);

        return rc != KERN_OPERATION_TIMED_OUT;
    }

    void signal()
    {
        semaphore_signal(m_sema);
    }

    void signal(int count)
    {
        while (count-- > 0)
        {
            semaphore_signal(m_sema);
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
    sem_t m_sema;

    Semaphore(const Semaphore& other) = delete;
    Semaphore& operator=(const Semaphore& other) = delete;

public:
    Semaphore(int initialCount = 0)
    {
        assert(initialCount >= 0);
        sem_init(&m_sema, 0, initialCount);
    }

    ~Semaphore()
    {
        sem_destroy(&m_sema);
    }

    void wait()
    {
        // http://stackoverflow.com/questions/2013181/gdb-causes-sem-wait-to-fail-with-eintr-error
        int rc;
        do
        {
            rc = sem_wait(&m_sema);
        } while (rc == -1 && errno == EINTR);
    }

    bool try_wait()
    {
        int rc;
        do
        {
            rc = sem_trywait(&m_sema);
        } while (rc == -1 && errno == EINTR);
        return !(rc == -1 && errno == EAGAIN);
    }

    bool timed_wait(unsigned long long usecs)
    {
        struct timespec ts;
        const int usecs_in_1_sec = 1000000;
        const int nsecs_in_1_sec = 1000000000;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += usecs / usecs_in_1_sec;
        ts.tv_nsec += (usecs % usecs_in_1_sec) * 1000;
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
            rc = sem_timedwait(&m_sema, &ts);
        } while (rc == -1 && errno == EINTR);
        return !(rc == -1 && errno == ETIMEDOUT);
    }

    void signal()
    {
        sem_post(&m_sema);
    }

    void signal(int count)
    {
        while (count-- > 0)
        {
            sem_post(&m_sema);
        }
    }
};
#else
#error Unsupported platform! (No semaphore wrapper available)
#endif
} // namespace zonciu
#endif
