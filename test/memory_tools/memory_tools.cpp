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

#if defined(__linux__)
/******************************************************************************
 * Begin Linux
 *****************************************************************************/

#include <dlfcn.h>
#include <stdio.h>
#include <malloc.h>

#include "./memory_tools_common.cpp"

void *
malloc(size_t size)
{
  void * (* libc_malloc)(size_t) = (void *(*)(size_t))dlsym(RTLD_NEXT, "malloc");
  if (g_monitoring_enabled.load()) {
    return custom_malloc(size);
  }
  return libc_malloc(size);
}

void *
realloc(void * pointer, size_t size)
{
  void * (* libc_realloc)(void *, size_t) = (void *(*)(void *, size_t))dlsym(RTLD_NEXT, "realloc");
  if (g_monitoring_enabled.load()) {
    return custom_realloc(pointer, size);
  }
  return libc_realloc(pointer, size);
}

void
free(void * pointer)
{
  void (* libc_free)(void *) = (void (*)(void *))dlsym(RTLD_NEXT, "free");
  if (g_monitoring_enabled.load()) {
    return custom_free(pointer);
  }
  return libc_free(pointer);
}

void start_memory_monitoring()
{
  if (!g_monitoring_enabled.exchange(true)) {
    printf("starting memory monitoring...\n");
  }
}

void stop_memory_monitoring()
{
  if (g_monitoring_enabled.exchange(false)) {
    printf("stopping memory monitoring...\n");
  }
}

/******************************************************************************
 * End Linux
 *****************************************************************************/
#elif defined(__APPLE__)
/******************************************************************************
 * Begin Apple
 *****************************************************************************/

// The apple implementation is in a separate shared library, loaded with DYLD_INSERT_LIBRARIES.
// Therefore we do not include the common cpp file here.

void osx_start_memory_monitoring();
void osx_stop_memory_monitoring();

void start_memory_monitoring()
{
  return osx_start_memory_monitoring();
}

void stop_memory_monitoring()
{
  return osx_stop_memory_monitoring();
}

/******************************************************************************
 * End Apple
 *****************************************************************************/
// #elif defined(_WIN32)
/******************************************************************************
 * Begin Windows
 *****************************************************************************/

// TODO(wjwwood): install custom malloc (and others) for Windows.

/******************************************************************************
 * End Windows
 *****************************************************************************/
#else
// Default case: do nothing.

#include "./memory_tools.hpp"

#include <stdio.h>

void start_memory_monitoring()
{
  printf("starting memory monitoring... not available\n");
}
void stop_memory_monitoring()
{
  printf("stopping memory monitoring... not available\n");
}

void assert_no_malloc_begin() {}

void assert_no_malloc_end() {}

void set_on_unexpected_malloc_callback(UnexpectedCallbackType callback) {}

void assert_no_realloc_begin() {}

void assert_no_realloc_end() {}

void set_on_unexpected_realloc_callback(UnexpectedCallbackType callback) {}

void assert_no_free_begin() {}

void assert_no_free_end() {}

void set_on_unexpected_free_callback(UnexpectedCallbackType callback) {}

void memory_checking_thread_init() {}

#endif  // if defined(__linux__) elif defined(__APPLE__) elif defined(_WIN32) else ...
