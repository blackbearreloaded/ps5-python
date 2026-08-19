#include <errno.h>
#include <stdint.h>
#include <time.h>

/* The PS5 libc exposes clock_nanosleep, but its generic syscall wrapper
 * returns ENOSYS. CPython's time.sleep() uses this absolute-clock variant. */
extern int sceKernelUsleep(unsigned int microseconds);

int __wrap_clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *request,
                           struct timespec *remainder)
{
    struct timespec now;
    int64_t nanoseconds;
    uint64_t microseconds;

    (void)remainder;
    if (request == NULL || clock_id != CLOCK_MONOTONIC || flags != TIMER_ABSTIME)
    {
        return EINVAL;
    }
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    {
        return errno;
    }

    nanoseconds = ((int64_t)request->tv_sec - (int64_t)now.tv_sec) * 1000000000LL;
    nanoseconds += (int64_t)request->tv_nsec - (int64_t)now.tv_nsec;
    if (nanoseconds <= 0)
    {
        return 0;
    }
    microseconds = (uint64_t)(nanoseconds + 999) / 1000;
    if (microseconds > UINT32_MAX)
    {
        microseconds = UINT32_MAX;
    }
    return sceKernelUsleep((unsigned int)microseconds) == 0 ? 0 : errno;
}
