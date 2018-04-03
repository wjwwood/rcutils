// Copyright 2015 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#if defined(_WIN32)
# error time_unix.c is not intended to be used with win32 based systems
#endif  // defined(_WIN32)

#if __cplusplus
extern "C"
{
#endif

#include "rcutils/time.h"

#if defined(__MACH__)
#include <mach/clock.h>
#include <mach/mach.h>
#endif  // defined(__MACH__)
#include <math.h>
#include <time.h>
#include <unistd.h>

#include "./time_common.h"
#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"

#if !defined(__MACH__)  // Assume clock_get_time is available on OS X.
// This id an appropriate check for clock_gettime() according to:
//   http://man7.org/linux/man-pages/man2/clock_gettime.2.html
# if !defined(_POSIX_TIMERS) || !_POSIX_TIMERS
#  error no monotonic clock function available
# endif  // !defined(_POSIX_TIMERS) || !_POSIX_TIMERS
#endif  // !defined(__MACH__)

#define __WOULD_BE_NEGATIVE(seconds, subseconds) (seconds < 0 || (subseconds < 0 && seconds == 0))

rcutils_ret_t
rcutils_system_time_now(rcutils_time_point_value_t * now)
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(
    now, RCUTILS_RET_INVALID_ARGUMENT, rcutils_get_default_allocator());
  struct timespec timespec_now;
#if defined(__MACH__)
  // On OS X use clock_get_time.
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  timespec_now.tv_sec = mts.tv_sec;
  timespec_now.tv_nsec = mts.tv_nsec;
#else  // defined(__MACH__)
  // Otherwise use clock_gettime.
  clock_gettime(CLOCK_REALTIME, &timespec_now);
#endif  // defined(__MACH__)
  if (__WOULD_BE_NEGATIVE(timespec_now.tv_sec, timespec_now.tv_nsec)) {
    RCUTILS_SET_ERROR_MSG("unexpected negative time", rcutils_get_default_allocator());
    return RCUTILS_RET_ERROR;
  }

  rcutils_time_point_value_t total_seconds_in_ns =
    RCUTILS_S_TO_NS(timespec_now.tv_sec);

  rcutils_time_point_value_t total_ns =
    total_seconds_in_ns + timespec_now.tv_nsec;

#if RCUTILS_DISABLE_TIME_SANITY_CHECKS
  *now = total_ns;
  return RCUTILS_RET_OK;
#else
  bool overflow_happened = timespec_now.tv_sec > (INT64_MAX / 1000000000LL);
  overflow_happened = overflow_happened ||
    ((timespec_now.tv_nsec > 0LL) &&
    (total_seconds_in_ns > (INT64_MAX - timespec_now.tv_nsec)));

  rcutils_ret_t retval;
  if (overflow_happened) {
    RCUTILS_SET_ERROR_MSG("system time overflow", rcutils_get_default_allocator());
    retval = RCUTILS_RET_ERROR;
  } else {
    *now = total_ns;
    retval = RCUTILS_RET_OK;
  }
  return retval;
#endif  // RCUTILS_DISABLE_TIME_SANITY_CHECKS
}

rcutils_ret_t
rcutils_steady_time_now(rcutils_time_point_value_t * now)
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(
    now, RCUTILS_RET_INVALID_ARGUMENT, rcutils_get_default_allocator());
  // If clock_gettime is available or on OS X, use a timespec.
  struct timespec timespec_now;
#if defined(__MACH__)
  // On OS X use clock_get_time.
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  timespec_now.tv_sec = mts.tv_sec;
  timespec_now.tv_nsec = mts.tv_nsec;
#else  // defined(__MACH__)
  // Otherwise use clock_gettime.
#if defined(CLOCK_MONOTONIC_RAW)
  clock_gettime(CLOCK_MONOTONIC_RAW, &timespec_now);
#else  // defined(CLOCK_MONOTONIC_RAW)
  clock_gettime(CLOCK_MONOTONIC, &timespec_now);
#endif  // defined(CLOCK_MONOTONIC_RAW)
#endif  // defined(__MACH__)
  if (__WOULD_BE_NEGATIVE(timespec_now.tv_sec, timespec_now.tv_nsec)) {
    RCUTILS_SET_ERROR_MSG("unexpected negative time", rcutils_get_default_allocator());
    return RCUTILS_RET_ERROR;
  }

  rcutils_time_point_value_t total_seconds_in_ns =
    RCUTILS_S_TO_NS(timespec_now.tv_sec);

  rcutils_time_point_value_t total_ns =
    total_seconds_in_ns + timespec_now.tv_nsec;

#if !defined(RCUTILS_DISABLE_TIME_SANITY_CHECKS) || !RCUTILS_DISABLE_TIME_SANITY_CHECKS
  // Check for overflow.
  bool overflow_happened = timespec_now.tv_sec > (INT64_MAX / 1000000000LL);
  overflow_happened = overflow_happened ||
    ((timespec_now.tv_nsec > 0LL) &&
    (total_seconds_in_ns > (INT64_MAX - timespec_now.tv_nsec)));
  if (overflow_happened) {
    RCUTILS_SET_ERROR_MSG("steady time overflow", rcutils_get_default_allocator());
    return RCUTILS_CALCULATION_OVERFLOW;
  }

  rcutils_ret_t ret = __rcutils_check_steady_time_monotonicity_thread_local(total_ns);
  if (RCUTILS_RET_OK != ret) {
    // error is already set
    return ret;
  }
#endif  // !defined(RCUTILS_DISABLE_TIME_SANITY_CHECKS) || !RCUTILS_DISABLE_TIME_SANITY_CHECKS

  *now = total_ns;
  return RCUTILS_RET_OK;
}

#if __cplusplus
}
#endif
