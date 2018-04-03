// Copyright 2018 Open Source Robotics Foundation, Inc.
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

#if __cplusplus
extern "C"
{
#endif

#include "rcutils/error_handling.h"
#include "rcutils/logging_macros.h"
#include "rcutils/macros.h"
#include "rcutils/time.h"

// These are forward declrations which will be implemented in a platform
// specific way further below.

static rcutils_ret_t
__rcutils_time_common_get_last_steady_timestamp_thread_local(int64_t * last_timestamp);

static rcutils_ret_t
__rcutils_time_common_set_last_steady_timestamp_thread_local(int64_t new_last_steady_timestamp);

static rcutils_ret_t
__rcutils_time_common_ensure_last_steady_timestamp(rcutils_allocator_t allocator);

static rcutils_ret_t
__rcutils_time_common_destroy_last_steady_timestamp();

// This section has implementations of public functions from `rctuils/time.h`.

rcutils_ret_t
rcutils_steady_time_now_thread_specific_init(rcutils_allocator_t custom_allocator)
{
  return __rcutils_time_common_ensure_last_steady_timestamp(custom_allocator);
}

rcutils_ret_t
rcutils_steady_time_now_thread_specific_fini()
{
  return __rcutils_time_common_destroy_last_steady_timestamp();
}

// This section has platform specific implemenations of private functions from `time_common.h`:

rcutils_ret_t
__rcutils_check_steady_time_monotonicity_thread_local(int64_t current_steady_timestamp)
{
  // Get last steady sample.
  int64_t last_steady_sample;
  rcutils_ret_t ret =
    __rcutils_time_common_get_last_steady_timestamp_thread_local(&last_steady_sample);
  if (RCUTILS_RET_OK != ret) {
    // error message already set
    return ret;
  }

  // Check for monotonicity.
  if (last_steady_sample > current_steady_timestamp) {
    RCUTILS_SET_ERROR_MSG("non-monotonic steady time", rcutils_get_default_allocator());
    return RCUTILS_NON_MONOTONIC_STEADY_TIME;
  }

  // Store the current timestamp as "last"
  ret = __rcutils_time_common_set_last_steady_timestamp_thread_local(current_steady_timestamp);
  if (RCUTILS_RET_OK != ret) {
    // error already set
    return ret;
  }

  return RCUTILS_RET_OK;
}

// This section has platform specific implemenations of private functions, forward declared above:
//   - __rcutils_time_common_get_last_steady_timestamp_thread_local
//   - __rcutils_time_common_set_last_steady_timestamp_thread_local
//   - __rcutils_time_common_ensure_last_steady_timestamp
//   - __rcutils_time_common_destroy_last_steady_timestamp
// For three different cases:
//   - sanity checks disabled (i.e. no need for thread-local storage)
//   - sanity checks enabled, with _Thread_local
//   - sanity checks enabled, with pthread (i.e. platform has no _Thread_local)

#if RCUTILS_DISABLE_TIME_SANITY_CHECKS
// Here starts the case "sanity checks disabled (i.e. no need for thread-local storage)"

static rcutils_ret_t
__rcutils_time_common_get_last_steady_timestamp_thread_local(int64_t * last_timestamp)
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(
    last_timestamp, RCUTILS_RET_INVALID_ARGUMENT, rcutils_get_default_allocator()) *
  last_timestamp = INT64_MIN;
  return RCUTILS_RET_OK;
}

static rcutils_ret_t
__rcutils_time_common_set_last_steady_timestamp_thread_local(int64_t new_last_steady_timestamp)
{
  (void)new_last_steady_timestamp;
  return RCUTILS_RET_OK;
}

static rcutils_ret_t
__rcutils_time_common_ensure_last_steady_timestamp(rcutils_allocator_t allocator)
{
  return RCUTILS_RET_OK;
}

static rcutils_ret_t
__rcutils_time_common_destroy_last_steady_timestamp()
{
  return RCUTILS_RET_OK;
}

#elif !defined(RCUTILS_THREAD_LOCAL_PTHREAD) || !RCUTILS_THREAD_LOCAL_PTHREAD
// Here starts the case "sanity checks enabled, with _Thread_local"

static RCUTILS_THREAD_LOCAL int64_t __last_steady_timestamp = INT64_MIN;

static rcutils_ret_t
__rcutils_time_common_get_last_steady_timestamp_thread_local(int64_t * last_timestamp)
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(
    last_timestamp, RCUTILS_RET_INVALID_ARGUMENT, rcutils_get_default_allocator()) *
  last_timestamp = __last_steady_timestamp;
  return RCUTILS_RET_OK;
}

static rcutils_ret_t
__rcutils_time_common_set_last_steady_timestamp_thread_local(int64_t new_last_steady_timestamp)
{
  __last_steady_timestamp = new_last_steady_timestamp;
  return RCUTILS_RET_OK;
}

static rcutils_ret_t
__rcutils_time_common_ensure_last_steady_timestamp(rcutils_allocator_t allocator)
{
  (void)allocator;
  // By forcing the thread-local storage to be accessed, it ensure its storage
  // has also been allocated.
  int64_t force_access_throw_away = __last_steady_timestamp;
  force_access_throw_away = RCUTILS_RET_OK;
  return force_access_throw_away;
}

static rcutils_ret_t
__rcutils_time_common_destroy_last_steady_timestamp()
{
  // no-op, thread-local storage is cleaned up when the thread exits
  return RCUTILS_RET_OK;
}

#else
// Here starts the case "sanity checks enabled, with pthread (i.e. platform has no _Thread_local)"

#include <errno.h>
#include <pthread.h>

typedef struct
{
  int64_t timestamp;
  rcutils_allocator_t self_allocator;
} __last_steady_timestamp_storage_t;

static pthread_once_t __key_init_once = PTHREAD_ONCE_INIT;
static pthread_key_t __last_steady_timestamp_storage_key;
static rcutils_ret_t __make_pthread_keys_ret = RCUTILS_RET_OK;

static void
__last_steady_timestamp_storage_t_destructor(void * data)
{
  if (data) {
    rcutils_allocator_t allocator;
    {
      __last_steady_timestamp_storage_t * temp = (__last_steady_timestamp_storage_t *)data;
      allocator = temp->self_allocator;
    }
    allocator.deallocate(data, allocator.state);
  }
}

static void
__make_pthread_keys()
{
  int ret = pthread_key_create(
    &__last_steady_timestamp_storage_key,
    __last_steady_timestamp_storage_t_destructor);
  switch (ret) {
    case 0:
      break;  // everything ok, continue...
    case EAGAIN:
      RCUTILS_SET_ERROR_MSG(
        "The system lacked the necessary resources to create another thread-specific data key, "
        "or the system-imposed limit on the total number of keys per process {PTHREAD_KEYS_MAX} "
        "has been exceeded.", rcutils_get_default_allocator())
      __make_pthread_keys_ret = RCUTILS_RET_ERROR;
      break;
    case ENOMEM:
      RCUTILS_SET_ERROR_MSG(
        "Insufficient memory exists to create the key.", rcutils_get_default_allocator())
      __make_pthread_keys_ret = RCUTILS_RET_BAD_ALLOC;
      break;
    default:
      RCUTILS_SET_ERROR_MSG(
        "Unexpected error with pthread_key_create", rcutils_get_default_allocator())
      __make_pthread_keys_ret = RCUTILS_RET_ERROR;
  }
}

static rcutils_ret_t
__set_last_steady_timestamp(__last_steady_timestamp_storage_t * last_timestamp)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  int pthread_ret = pthread_setspecific(__last_steady_timestamp_storage_key, last_timestamp);
  switch (pthread_ret) {
    case 0:  // everything ok with pthread_setspecific()
      break;
    case ENOMEM:
      RCUTILS_SET_ERROR_MSG(
        "pthread_setspecific: Insufficient memory exists to associate the value with the key.",
        allocator)
      return RCUTILS_RET_BAD_ALLOC;
    case EINVAL:
      RCUTILS_SET_ERROR_MSG("pthread_setspecific: The key value is invalid.", allocator)
      return RCUTILS_RET_ERROR;
  }
  return RCUTILS_RET_OK;
}

static rcutils_ret_t
__ensure_last_steady_timestamp_storage(rcutils_allocator_t allocator)
{
  // Create the pthread key, exactly once.
  int pthread_ret = pthread_once(&__key_init_once, __make_pthread_keys);
  switch (pthread_ret) {
    case 0:
      // Everything ok with pthread_once, check the result of __make_pthread_keys()
      switch (__make_pthread_keys_ret) {
        case RCUTILS_RET_OK:
          break;  // everything ok, continue
        default:
          // ret not ok and error message already set
          return __make_pthread_keys_ret;
      }
      break;
    case EINVAL:
      RCUTILS_SET_ERROR_MSG(
        "pthread_once: either once_control or init_routine is invalid.", allocator)
      return RCUTILS_RET_ERROR;
    default:
      RCUTILS_SET_ERROR_MSG("pthread_once: unknown error", allocator)
      return RCUTILS_RET_ERROR;
  }

  // Check if the storage exists, if not allocator memory for it.
  __last_steady_timestamp_storage_t * last_steady_timestamp =
    (__last_steady_timestamp_storage_t *)pthread_getspecific(__last_steady_timestamp_storage_key);
  if (!last_steady_timestamp) {
    // Not initialized, allocate space for it.
    last_steady_timestamp =
      allocator.allocate(sizeof(__last_steady_timestamp_storage_t), allocator.state);
    if (!last_steady_timestamp) {
      RCUTILS_SET_ERROR_MSG(
        "failed to allocate thread-local storage for last steady timestamp", allocator)
      return RCUTILS_RET_BAD_ALLOC;
    }
    // initialize to int64 min
    last_steady_timestamp->timestamp = INT64_MIN;
    // store the allocator for use in the key destructor
    last_steady_timestamp->self_allocator = allocator;
    // storage pointer in pthread key
    return __set_last_steady_timestamp(last_steady_timestamp);
  }
  return RCUTILS_RET_OK;
}

static rcutils_ret_t
__destroy_last_steady_timestamp_storage()
{
  __last_steady_timestamp_storage_t * last_steady_timestamp =
    (__last_steady_timestamp_storage_t *)pthread_getspecific(__last_steady_timestamp_storage_key);
  if (last_steady_timestamp) {
    rcutils_allocator_t allocator = last_steady_timestamp->self_allocator;
    allocator.deallocate(last_steady_timestamp, allocator.state);
    // store null pointer in pthread key
    return __set_last_steady_timestamp(NULL);
  }
  return RCUTILS_RET_OK;
}

static rcutils_ret_t
__rcutils_time_common_get_last_steady_timestamp_thread_local(int64_t * last_timestamp)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(
    last_timestamp, RCUTILS_RET_INVALID_ARGUMENT, allocator)
  rcutils_ret_t ret = __ensure_last_steady_timestamp_storage(allocator);
  if (RCUTILS_RET_OK != ret) {
    // error messge already set
    return ret;
  }
  __last_steady_timestamp_storage_t * last_steady_timestamp =
    (__last_steady_timestamp_storage_t *)pthread_getspecific(__last_steady_timestamp_storage_key);
  if (!last_steady_timestamp) {
    RCUTILS_SET_ERROR_MSG("pthread_getspecific: unexpectedly returned NULL", allocator)
    return RCUTILS_RET_ERROR;
  }
  *last_timestamp = last_steady_timestamp->timestamp;
  return RCUTILS_RET_OK;
}

static rcutils_ret_t
__rcutils_time_common_set_last_steady_timestamp_thread_local(int64_t new_last_steady_timestamp)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  rcutils_ret_t ret = __ensure_last_steady_timestamp_storage(allocator);
  if (RCUTILS_RET_OK != ret) {
    // error messge already set
    return ret;
  }
  __last_steady_timestamp_storage_t * last_steady_timestamp =
    (__last_steady_timestamp_storage_t *)pthread_getspecific(__last_steady_timestamp_storage_key);
  if (!last_steady_timestamp) {
    RCUTILS_SET_ERROR_MSG("pthread_getspecific: unexpectedly returned NULL", allocator)
    return RCUTILS_RET_ERROR;
  }
  last_steady_timestamp->timestamp = new_last_steady_timestamp;
  return __set_last_steady_timestamp(last_steady_timestamp);
}

static rcutils_ret_t
__rcutils_time_common_ensure_last_steady_timestamp(rcutils_allocator_t allocator)
{
  return __ensure_last_steady_timestamp_storage(allocator);
}

static rcutils_ret_t
__rcutils_time_common_destroy_last_steady_timestamp()
{
  return __destroy_last_steady_timestamp_storage();
}

#endif

#if __cplusplus
}
#endif
