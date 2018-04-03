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

#ifndef RCUTILS__TIME_H_
#define RCUTILS__TIME_H_

#if __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#include "rcutils/macros.h"
#include "rcutils/types.h"
#include "rcutils/visibility_control.h"

/// Convenience macro to convert seconds to nanoseconds.
#define RCUTILS_S_TO_NS(seconds) (seconds * (1000 * 1000 * 1000))
/// Convenience macro to convert milliseconds to nanoseconds.
#define RCUTILS_MS_TO_NS(milliseconds) (milliseconds * (1000 * 1000))
/// Convenience macro to convert microseconds to nanoseconds.
#define RCUTILS_US_TO_NS(microseconds) (microseconds * 1000)

/// Convenience macro to convert nanoseconds to seconds.
#define RCUTILS_NS_TO_S(nanoseconds) (nanoseconds / (1000 * 1000 * 1000))
/// Convenience macro to convert nanoseconds to milliseconds.
#define RCUTILS_NS_TO_MS(nanoseconds) (nanoseconds / (1000 * 1000))
/// Convenience macro to convert nanoseconds to microseconds.
#define RCUTILS_NS_TO_US(nanoseconds) (nanoseconds / 1000)

/// A single point in time, measured in nanoseconds since the Unix epoch.
typedef int64_t rcutils_time_point_value_t;
/// A duration of time, measured in nanoseconds.
typedef int64_t rcutils_duration_value_t;

/**
 * This function returns the time from a system clock.
 * The closest equivalent would be to std::chrono::system_clock::now();
 *
 * The resolution (e.g. nanoseconds vs microseconds) is not guaranteed.
 *
 * The now argument must point to an allocated rcutils_time_point_value_t object,
 * as the result is copied into this variable.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | Yes
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[out] now a datafield in which the current time is stored
 * \return `RCUTILS_RET_OK` if the current time was successfully obtained, or
 * \return `RCUTILS_RET_INVALID_ARGUMENT` if any arguments are invalid, or
 * \return `RCUTILS_RET_ERROR` an unspecified error occur.
 */
RCUTILS_PUBLIC
RCUTILS_WARN_UNUSED
rcutils_ret_t
rcutils_system_time_now(rcutils_time_point_value_t * now);

/// Retrieve the current time as a rcutils_time_point_value_t object.
/**
 * This function returns the time from a monotonically increasing clock.
 * The closest equivalent would be to std::chrono::steady_clock::now();
 *
 * The resolution (e.g. nanoseconds vs microseconds) is not guaranteed.
 *
 * The now argument must point to an allocated rcutils_time_point_value_t object,
 * as the result is copied into this variable.
 *
 * This function uses thread-local storage to check for non-monotonic values
 * being returned from the operating system.
 * Thread-local storage implementations may allocate memory the first time this
 * function is called on a new thread.
 * By default it will use the allocator returned by
 * rcutils_get_default_allocator() on the first call to initialize the
 * thread-local storage.
 * However, the rcutils_steady_time_now_thread_specific_init()
 * function can be used to setup new threads to avoiding allocation in this
 * function and as well to optionally control which allocator is used.
 * Not all thread-local storage implementations allow for completely custom
 * allocators to be used.
 *
 * Alternatively, all possibility of memory allocation can be avoided if
 * this library is compiled with the `RCUTILS_DISABLE_TIME_SANITY_CHECKS`
 * compiler definition set to a value which evaluates to true in the
 * C preprocessor.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No [1]
 * Thread-Safe        | Yes
 * Uses Atomics       | No
 * Lock-Free          | Yes
 * <i>[1] if initialized with `rcutils_steady_time_now_thread_specific_init()`</i>
 *
 * \param[out] now a struct in which the current time is stored
 * \return `RCUTILS_RET_OK` if the current time was successfully obtained, or
 * \return `RCUTILS_RET_INVALID_ARGUMENT` if any arguments are invalid, or
 * \return `RCUTILS_RET_BAD_ALLOC` if thread-local memory allocation fails, or
 * \return `RCUTILS_RET_ERROR` an unspecified error occur.
 */
RCUTILS_PUBLIC
RCUTILS_WARN_UNUSED
rcutils_ret_t
rcutils_steady_time_now(rcutils_time_point_value_t * now);


/// Initialize thread local storage used by the rcutils_steady_time_now() function.
/**
 * Initializes thread-local storage used by rcutils_steady_time_now() to prevent
 * rcutils_steady_time_now() from allocating memory when called in that thread.
 * If the `RCUTILS_DISABLE_TIME_SANITY_CHECKS` compiler definition is
 * set to true when compiling this library, then this function does nothing
 * and will always return RCUTILS_RET_OK.
 *
 * If you do not care what allocator is used, pass the result of
 * rcutils_get_default_allocator() for the allocator.
 *
 * Repeated calls to this function will be a no-op and return RCUTILS_RET_OK.
 *
 * Not all thread-local storage implementations allow for completely custom
 * allocation, so use of the provided or default allocator is best effort.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes
 * Thread-Safe        | Yes
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[in] custom_allocator an allocator to be used
 * \return `RCUTILS_RET_OK` if successful, or
 * \return `RCUTILS_RET_BAD_ALLOC` if memory allocation fails.
 */
RCUTILS_PUBLIC
RCUTILS_WARN_UNUSED
rcutils_ret_t
rcutils_steady_time_now_thread_specific_init(rcutils_allocator_t custom_allocator);

/// Finalize thread local storage used by the rcutils_steady_time_now() function.
/**
 * This function cleans up thread-local storage memory allocations made by
 * rcutils_steady_time_now_thread_specific_init().
 *
 * It is not necessary to call this function when terminating a thread, as all
 * thread-local implementations will cleanup resources automatically, but it
 * is provided so you can cleanup thread-local storage for
 * rcutils_steady_time_now() and then call
 * rcutils_steady_time_now_thread_specific_init() with a different allocator.
 *
 * Calls to this function before rcutils_steady_time_now_thread_specific_init()
 * or rcutils_steady_time_now() are called is a no-op and return RCUTILS_RET_OK.
 *
 * Repeated calls to this function will be a no-op and return RCUTILS_RET_OK.
 *
 * Unless there is an unexpected internal state due to a bug, this function
 * will never fail.
 *
 * Using this function will prevent future rcutils_steady_time_now() calls from
 * detecting a violation of monotonic time with previous calls.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes
 * Thread-Safe        | Yes
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \return `RCUTILS_RET_OK` if successful, or
 * \return `RCUTILS_RET_ERROR` an unspecified error occur.
 */
RCUTILS_PUBLIC
RCUTILS_WARN_UNUSED
rcutils_ret_t
rcutils_steady_time_now_thread_specific_fini();

#if __cplusplus
}
#endif

#endif  // RCUTILS__TIME_H_
