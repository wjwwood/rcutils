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

// Code to do replacing of malloc/free/etc... inspired by:
//   https://dxr.mozilla.org/mozilla-central/rev/
//    cc9c6cd756cb744596ba039dcc5ad3065a7cc3ea/memory/build/replace_malloc.c

#ifndef MEMORY_TOOLS__MEMORY_TOOLS_HPP_
#define MEMORY_TOOLS__MEMORY_TOOLS_HPP_

#include <stddef.h>

#include <functional>

// This logic was borrowed (then namespaced) from the examples on the gcc wiki:
//     https://gcc.gnu.org/wiki/Visibility

#if defined _WIN32 || defined __CYGWIN__
  #ifdef __GNUC__
    #define RCUTILS_MEMORY_TOOLS_EXPORT __attribute__ ((dllexport))
    #define RCUTILS_MEMORY_TOOLS_IMPORT __attribute__ ((dllimport))
  #else
    #define RCUTILS_MEMORY_TOOLS_EXPORT __declspec(dllexport)
    #define RCUTILS_MEMORY_TOOLS_IMPORT __declspec(dllimport)
  #endif
  #ifdef RCUTILS_MEMORY_TOOLS_BUILDING_DLL
    #define RCUTILS_MEMORY_TOOLS_PUBLIC RCUTILS_MEMORY_TOOLS_EXPORT
  #else
    #define RCUTILS_MEMORY_TOOLS_PUBLIC RCUTILS_MEMORY_TOOLS_IMPORT
  #endif
  #define RCUTILS_MEMORY_TOOLS_PUBLIC_TYPE RCUTILS_MEMORY_TOOLS_PUBLIC
  #define RCUTILS_MEMORY_TOOLS_LOCAL
#else
  #define RCUTILS_MEMORY_TOOLS_EXPORT __attribute__ ((visibility("default")))
  #define RCUTILS_MEMORY_TOOLS_IMPORT
  #if __GNUC__ >= 4
    #define RCUTILS_MEMORY_TOOLS_PUBLIC __attribute__ ((visibility("default")))
    #define RCUTILS_MEMORY_TOOLS_LOCAL  __attribute__ ((visibility("hidden")))
  #else
    #define RCUTILS_MEMORY_TOOLS_PUBLIC
    #define RCUTILS_MEMORY_TOOLS_LOCAL
  #endif
  #define RCUTILS_MEMORY_TOOLS_PUBLIC_TYPE
#endif

namespace memory_tools {

/// Does any thread-local initialization in a thread that might use any memory operations.
/**
 * Some thread-local storage implementations (e.g. ones based on pthread keys)
 * can require memory allocation the first time used in a new thread.
 * These memory allocations are hard to automatically discriminate from user
 * code, so you can call this function once in each thread you're using to
 * ensure (to the best of our ability) that no additional memory allocation
 * is required during runtime.
 *
 * This function is not required on most systems which do memory allocation
 * at thread creation time.
 *
 * Note this only initialized thread-local memory used by memory_tools, and
 * any thread-local initialization based memory allocations due to user
 * software using thread-local storage can still be caught by memory_tools.
 */
RCUTILS_MEMORY_TOOLS_PUBLIC
void
memory_checking_thread_init();

/// Begin monitoring supported memory allocation operations globally.
RCUTILS_MEMORY_TOOLS_PUBLIC
void
start_memory_monitoring();

/// Stop monitoring supported memory allocation operations globally.
RCUTILS_MEMORY_TOOLS_PUBLIC
void
stop_memory_monitoring();

/// Forward declaration of private MemoryToolsService factory class.
class MemoryToolsServiceFactory;

/// Service injected in to user callbacks which allow them to control behavior.
/**
 * This is a Service (in the terminology of the dependency injection pattern)
 * which is given to user callbacks which allows the user to control the
 * behavior of the memory tools test suite, like whether or not to log the
 * occurrence, print a stacktrace, or make a gtest assert.
 *
 * Creation of the MemoryToolsService is prevent publicly, so to enforce the
 * functions are only called from within a memory tools callback.
 * Similarly, the instance given by the callback parameter is only valid until
 * the callback returns, therefore it should not be stored globally and called
 * later.
 *
 * The default behavior is that a single line message about the event is
 * printed to stderr and a gtest assertion fails if between the
 * `assert_no_*_{begin,end}()` function calls for a given memory operation.
 * However, you can avoid both behaviors by calling ignore(), or just avoid
 * the gtest failure by calling ignore_but_log().
 *
 * You can have it include a stacktrace to see where the memory-related call is
 * originating from by calling print_stacktrace() before returning.
 *
 * The user's callback, if set, will occur before memory operations.
 * The gtest failure, if not ignored, will also occur before memory operations.
 * Any logging activity occurs after the memory operations.
 */
struct MemoryToolsService
{
  /// If called, no gtest failure occurs and no log message is displayed.
  /** Repeated calls do nothing. */
  RCUTILS_MEMORY_TOOLS_PUBLIC
  static void
  ignore();

  /// If called, no gtest failure occurs but a log message is printed.
  /**
   * Repeated calls do nothing and will not restore the log message if
   * ignore() was called.
   */
  RCUTILS_MEMORY_TOOLS_PUBLIC
  static void
  ignore_but_log();

  /// Adds a stacktrace to the log message.
  /** Repeated calls do nothing, and only prints if a log is also printed. */
  RCUTILS_MEMORY_TOOLS_PUBLIC
  static void
  print_stacktrace();

  explicit operator bool() const;
private:
  MemoryToolsService();

  friend MemoryToolsServiceFactory;
};

/// Callback provided by the user, to be called when a memory operation occurs.
/**
 * Which memory allocation is determined by which function the callback was
 * given to, see: on_{malloc,realloc,free}() and
 * on_{malloc,realloc,free}_thread_local() functions.
 *
 * Memory allocations which occur in the user's callback will not be monitored.
 */
typedef std::function<void (MemoryToolsService)> MemoryToolsCallback;
/// Simpler callback signature, see MemoryToolsCallback for more documentation.
typedef std::function<void ()> MemoryToolsSimpleCallback;

struct AnyMemoryToolsCallback
{
  explicit AnyMemoryToolsCallback(MemoryToolsCallback callback)
  : AnyMemoryToolsCallback(nullptr)
  {
    memory_tools_callback = callback;
  }
  explicit AnyMemoryToolsCallback(MemoryToolsSimpleCallback callback)
  : AnyMemoryToolsCallback(nullptr)
  {
    memory_tools_simple_callback = callback;
  }
  AnyMemoryToolsCallback(nullptr_t)
  : memory_tools_callback(nullptr),
    memory_tools_simple_callback(nullptr)
  {}

  MemoryToolsCallback memory_tools_callback;
  MemoryToolsSimpleCallback memory_tools_simple_callback;

private:
  AnyMemoryToolsCallback() = delete;
};

//
// malloc
//

/// Begin gtest-asserting that malloc() is not called.
/**
 * Does nothing if start_memory_monitoring() has not been called.
 */
RCUTILS_MEMORY_TOOLS_PUBLIC
void
assert_no_malloc_begin();

/// End gtest-asserting that malloc() is not called.
RCUTILS_MEMORY_TOOLS_PUBLIC
void
assert_no_malloc_end();

/// Register a callback to be called if malloc() is called in any thread.
/** Only calls callback if start_memory_monitoring() has been called. */
RCUTILS_MEMORY_TOOLS_PUBLIC
void
on_malloc(AnyMemoryToolsCallback callback);

/// Register a callback to be called if malloc() is called in this thread.
/** Only calls callback if start_memory_monitoring() has been called. */
RCUTILS_MEMORY_TOOLS_PUBLIC
void
on_malloc_thread_local(AnyMemoryToolsCallback callback);

/// Wraps the statements in assert_no_malloc_{begin,end}() calls.
#define ASSERT_NO_MALLOC(statements) \
  assert_no_malloc_begin(); statements; assert_no_malloc_end();

//
// realloc
//

/// Begin gtest-asserting that realloc() is not called.
/**
 * Does nothing if start_memory_monitoring() has not been called.
 */
RCUTILS_MEMORY_TOOLS_PUBLIC
void
assert_no_realloc_begin();

/// End gtest-asserting that realloc() is not called.
RCUTILS_MEMORY_TOOLS_PUBLIC
void
assert_no_realloc_end();

/// Register a callback to be called if realloc() is called in any thread.
/** Only calls callback if start_memory_monitoring() has been called. */
RCUTILS_MEMORY_TOOLS_PUBLIC
void
on_realloc(AnyMemoryToolsCallback callback);

/// Register a callback to be called if realloc() is called in this thread.
/** Only calls callback if start_memory_monitoring() has been called. */
RCUTILS_MEMORY_TOOLS_PUBLIC
void
on_realloc_thread_local(AnyMemoryToolsCallback callback);

/// Wraps the statements in assert_no_realloc_{begin,end}() calls.
#define ASSERT_NO_REALLOC(statements) \
  assert_no_realloc_begin(); statements; assert_no_realloc_end();

//
// free
//

/// Begin gtest-asserting that free() is not called.
/**
 * Does nothing if start_memory_monitoring() has not been called.
 */
RCUTILS_MEMORY_TOOLS_PUBLIC
void
assert_no_free_begin();

/// End gtest-asserting that free() is not called.
RCUTILS_MEMORY_TOOLS_PUBLIC
void
assert_no_free_end();
RCUTILS_MEMORY_TOOLS_PUBLIC

/// Register a callback to be called if free() is called in any thread.
/** Only calls callback if start_memory_monitoring() has been called. */
RCUTILS_MEMORY_TOOLS_PUBLIC
void
on_free(AnyMemoryToolsCallback callback);

/// Register a callback to be called if free() is called in this thread.
/** Only calls callback if start_memory_monitoring() has been called. */
RCUTILS_MEMORY_TOOLS_PUBLIC
void
on_free_thread_local(AnyMemoryToolsCallback callback);

/// Wraps the statements in assert_no_free_{begin,end}() calls.
#define ASSERT_NO_FREE(statements) \
  assert_no_free_begin(); statements; assert_no_free_end();

//
// all memory operations
//

/// Wraps statements in all assert_no_*_{begin,end}() calls.
#define ASSERT_NO_MEMORY_OPERATIONS(statements) \
  assert_no_malloc_begin(); assert_no_realloc_begin(); assert_no_free_begin(); \
  statements; \
  assert_no_malloc_end(); assert_no_realloc_end(); assert_no_free_end();

}  // namespace memory_tools

#endif  // MEMORY_TOOLS__MEMORY_TOOLS_HPP_
