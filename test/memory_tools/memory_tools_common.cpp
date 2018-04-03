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

#include <inttypes.h>

#include <atomic>
#include <cstdlib>
#include <csignal>

#if defined(__APPLE__)
#include <malloc/malloc.h>
#define MALLOC_PRINTF malloc_printf
#else  // defined(__APPLE__)
#define MALLOC_PRINTF printf
#endif  // defined(__APPLE__)

#include "./memory_tools.hpp"
#include "./scope_exit.hpp"
#include "./stacktrace.h"

namespace memory_tools
{

// Global storage and thread-local storage.

/// Global state for whether or not memory operations should be monitored.
static std::atomic<bool> g_monitoring_enabled(false);
/// Thread-local state for whether or not memory operations should be ignored.
/** Specifically as part of memory monitoring activity. */
static __thread bool tls_in_memory_tools_function = false;

/// Thread-local state to indicate if a malloc() was expected or not.
/** Used by assert_no_malloc_{begin,end}(). */
static __thread bool tls_malloc_expected = true;
/// Global storage for on_malloc() callbacks.
static AnyMemoryToolsCallback g_on_malloc_callback = nullptr;
/// Thread-local storage for on_malloc_thread_local() callbacks.
static __thread MemoryToolsCallback * tls_on_malloc_callback = nullptr;


static __thread bool malloc_expected = true;
static __thread UnexpectedCallbackType * unexpected_malloc_callback = nullptr;
static __thread UnexpectedCallbackType2 * unexpected_malloc_callback2 = nullptr;
static __thread bool realloc_expected = true;
static __thread UnexpectedCallbackType * unexpected_realloc_callback = nullptr;
static __thread UnexpectedCallbackType2 * unexpected_realloc_callback2 = nullptr;
static __thread bool free_expected = true;
static __thread UnexpectedCallbackType * unexpected_free_callback = nullptr;
static __thread UnexpectedCallbackType2 * unexpected_free_callback2 = nullptr;

void
memory_checking_thread_init()
{
  // explicitly access all thread-local storage to make sure allocations have been made

}

// start_memory_monitoring() and stop_memory_monitoring() are implemented in a
// platform specific way and in a different file, e.g. memory_tools.cpp.

void *
custom_malloc(size_t size)
{
  if (!enabled.load() || in_custom_callback) {return malloc(size);}
  in_custom_callback = true;
  auto foo = SCOPE_EXIT(in_custom_callback = false);
  bool should_print_stacktrace = false;
  if (!malloc_expected) {
    if (unexpected_malloc_callback) {
      printf("here\n");
      (*unexpected_malloc_callback)();
    }
    if (unexpected_malloc_callback2) {
      printf("here2\n");
      // if the malloc callback returns true, print stacktrace
      should_print_stacktrace = (*unexpected_malloc_callback2)();
    }
  } else {
    printf("there\n");
  }
  void * memory = malloc(size);
  if (!malloc_expected) {
    uint64_t fw_size = size;
    MALLOC_PRINTF(
      " malloc (%s) %p %" PRIu64 "\n",
      malloc_expected ? "    expected" : "not expected", memory, fw_size);
    if (should_print_stacktrace) {
      print_stacktrace();
    }
  }
  return memory;
}

void *
custom_realloc(void * memory_in, size_t size)
{
  if (!enabled.load() || in_custom_callback) {return realloc(memory_in, size);}
  in_custom_callback = true;
  auto foo = SCOPE_EXIT(in_custom_callback = false);
  bool should_print_stacktrace = false;
  if (!realloc_expected) {
    if (unexpected_realloc_callback) {
      (*unexpected_realloc_callback)();
    }
    if (unexpected_realloc_callback) {
      should_print_stacktrace = (*unexpected_realloc_callback2)();
    }
  }
  void * memory = realloc(memory_in, size);
  if (!realloc_expected) {
    uint64_t fw_size = size;
    MALLOC_PRINTF(
      "realloc (%s) %p %p %" PRIu64 "\n",
      realloc_expected ? "    expected" : "not expected", memory_in, memory, fw_size);
    if (should_print_stacktrace) {
      print_stacktrace();
    }
  }
  return memory;
}

void
custom_free(void * memory)
{
  if (!enabled.load() || in_custom_callback) {return free(memory);}
  in_custom_callback = true;
  auto foo = SCOPE_EXIT(in_custom_callback = false);
  bool should_print_stacktrace = false;
  if (!free_expected) {
    if (unexpected_free_callback) {
      (*unexpected_free_callback)();
    }
    if (unexpected_free_callback2) {
      should_print_stacktrace = (*unexpected_free_callback2)();
    }
  }
  if (!free_expected) {
    MALLOC_PRINTF(
      "   free (%s) %p\n", free_expected ? "    expected" : "not expected", memory);
    if (should_print_stacktrace) {
      print_stacktrace();
    }
  }
  free(memory);
}

void assert_no_malloc_begin() {malloc_expected = false;}
void assert_no_malloc_end() {malloc_expected = true;}
void assert_no_realloc_begin() {realloc_expected = false;}
void assert_no_realloc_end() {realloc_expected = true;}
void assert_no_free_begin() {free_expected = false;}
void assert_no_free_end() {free_expected = true;}

}  // namespace memory_tools
