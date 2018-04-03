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

#include <gtest/gtest.h>

#include <inttypes.h>

#include <chrono>
#include <thread>

#include "rcutils/error_handling.h"
#include "../src/time_common.h"

#include "./memory_tools/memory_tools.hpp"

class TestTimeCommonFixture : public ::testing::Test
{
public:
  void SetUp()
  {
    set_on_unexpected_malloc_callback([]() {ASSERT_FALSE(true) << "UNEXPECTED MALLOC";});
    set_on_unexpected_realloc_callback([]() {ASSERT_FALSE(true) << "UNEXPECTED REALLOC";});
    set_on_unexpected_free_callback([]() {ASSERT_FALSE(true) << "UNEXPECTED FREE";});
    start_memory_checking();
  }

  void TearDown()
  {
    assert_no_malloc_end();
    assert_no_realloc_end();
    assert_no_free_end();
    stop_memory_checking();
    set_on_unexpected_malloc_callback(nullptr);
    set_on_unexpected_realloc_callback(nullptr);
    set_on_unexpected_free_callback(nullptr);
  }
};

// Tests the __rcutils_check_steady_time_monotonicity_thread_local() function.
TEST_F(TestTimeCommonFixture, test___rcutils_check_steady_time_monotonicity_thread_local) {
  // The first call is allowed to allocate memory, but not later ones.
  size_t malloc_count = 0;
  // Test the first call.
  int64_t t = 0;
  // Setup to assert that the first call allocates memory.
  set_on_unexpected_malloc_callback2([&malloc_count]() {malloc_count++; return true;});
  set_on_unexpected_realloc_callback2([&malloc_count]() {malloc_count++; return true;});
  assert_no_realloc_begin();
  assert_no_malloc_begin();
  assert_no_free_begin();
  rcutils_ret_t ret = __rcutils_check_steady_time_monotonicity_thread_local(t++);
  assert_no_malloc_end();
  assert_no_realloc_end();
  assert_no_free_end();
  EXPECT_EQ(RCUTILS_RET_OK, ret);
  // Make sure it allocated thread-local storage.
  ASSERT_EQ(1, malloc_count);
  // Setup the malloc detector to fail again when called, as future calls are not allowed to malloc.
  set_on_unexpected_malloc_callback([]() {ASSERT_FALSE(true) << "UNEXPECTED MALLOC";});
  set_on_unexpected_realloc_callback([]() {ASSERT_FALSE(true) << "UNEXPECTED REALLOC";});
  assert_no_realloc_begin();
  assert_no_malloc_begin();
  assert_no_free_begin();
  // Test a few more calls.
  ret = __rcutils_check_steady_time_monotonicity_thread_local(t++);
  EXPECT_EQ(RCUTILS_RET_OK, ret);
  ret = __rcutils_check_steady_time_monotonicity_thread_local(t++);
  EXPECT_EQ(RCUTILS_RET_OK, ret);
  ret = __rcutils_check_steady_time_monotonicity_thread_local(t++);
  EXPECT_EQ(RCUTILS_RET_OK, ret);
  // Disable malloc assertions because we currently allow allocation in error state creation.
  assert_no_malloc_end();
  assert_no_realloc_end();
  assert_no_free_end();
  // Test some failure cases.
  ret = __rcutils_check_steady_time_monotonicity_thread_local(t);
  EXPECT_EQ(RCUTILS_NON_MONOTONIC_STEADY_TIME, ret);  // Duplicate timestamp.
  ret = __rcutils_check_steady_time_monotonicity_thread_local(t--);
  EXPECT_EQ(RCUTILS_NON_MONOTONIC_STEADY_TIME, ret);  // One step older timestamp.
  ret = __rcutils_check_steady_time_monotonicity_thread_local(0);
  EXPECT_EQ(RCUTILS_NON_MONOTONIC_STEADY_TIME, ret);  // Much older timestamp.
  ret = __rcutils_check_steady_time_monotonicity_thread_local(t++);
  EXPECT_EQ(RCUTILS_RET_OK, ret);  // Continued normal operation.
}
