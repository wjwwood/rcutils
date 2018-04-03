// Copyright 2017 Open Source Robotics Foundation, Inc.
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

#include "rcutils/strdup.h"

#include <stddef.h>
#include <string.h>

char *
rcutils_strdup(const char * str, rcutils_allocator_t allocator)
{
  if (!str) {
    return NULL;
  }
  return rcutils_strndup(str, strlen(str), allocator);
}

char *
rcutils_strndup(const char * str, size_t string_length, rcutils_allocator_t allocator)
{
  if (!str) {
    return NULL;
  }
  char * new_string = allocator.allocate(string_length + 1, allocator.state);
  if (!new_string) {
    return NULL;
  }
  memcpy(new_string, str, string_length + 1);
  new_string[string_length] = '\0';
  return new_string;
}

#if __cplusplus
}
#endif
