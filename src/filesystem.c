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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include <direct.h>
#endif  // _WIN32
#include "rcutils/concat.h"
#include "rcutils/filesystem.h"

bool
rcutils_get_cwd(char * buffer, size_t max_length)
{
  if (NULL == buffer) {
    return false;
  }
#ifdef _WIN32
  if (NULL == _getcwd(buffer, (int)max_length)) {
    return false;
  }
#else
  if (NULL == getcwd(buffer, max_length)) {
    return false;
  }
#endif  // _WIN32
  return true;
}

bool
rcutils_is_directory(const char * abs_path)
{
  struct stat buf;
  if (stat(abs_path, &buf) < 0) {
    return false;
  }
#ifdef _WIN32
  return (buf.st_mode & S_IFDIR) == S_IFDIR;
#else
  return S_ISDIR(buf.st_mode);
#endif  // _WIN32
}

bool
rcutils_is_file(const char * abs_path)
{
  struct stat buf;
  if (stat(abs_path, &buf) < 0) {
    return false;
  }
#ifdef _WIN32
  return (buf.st_mode & S_IFREG) == S_IFREG;
#else
  return S_ISREG(buf.st_mode);
#endif  // _WIN32
}

bool
rcutils_exists(const char * abs_path)
{
  struct stat buf;
  if (stat(abs_path, &buf) < 0) {
    return false;
  }
  return true;
}

bool
rcutils_is_readable(const char * abs_path)
{
  struct stat buf;
  if (!rcutils_exists(abs_path)) {
    return false;
  }
  stat(abs_path, &buf);
#ifdef _WIN32
  if (!(buf.st_mode & _S_IREAD)) {
#else
  if (!(buf.st_mode & S_IRUSR)) {
#endif  // _WIN32
    return false;
  }
  return true;
}

bool
rcutils_is_writable(const char * abs_path)
{
  struct stat buf;
  if (!rcutils_exists(abs_path)) {
    return false;
  }
  stat(abs_path, &buf);
#ifdef _WIN32
  if (!(buf.st_mode & _S_IWRITE)) {
#else
  if (!(buf.st_mode & S_IWUSR)) {
#endif  // _WIN32
    return false;
  }
  return true;
}

bool
rcutils_is_readable_and_writable(const char * abs_path)
{
  struct stat buf;
  if (!rcutils_exists(abs_path)) {
    return false;
  }
  stat(abs_path, &buf);
#ifdef _WIN32
  // NOTE(marguedas) on windows all writable files are readable
  // hence the following check is equivalent to "& _S_IWRITE"
  if (!((buf.st_mode & _S_IWRITE) && (buf.st_mode & _S_IREAD))) {
#else
  if (!((buf.st_mode & S_IWUSR) && (buf.st_mode & S_IRUSR))) {
#endif  // _WIN32
    return false;
  }
  return true;
}

char *
rcutils_join_path(const char * left_hand_path, const char * right_hand_path)
{
  if (NULL == left_hand_path) {
    return NULL;
  }
  if (NULL == right_hand_path) {
    return NULL;
  }

#ifdef  _WIN32
  const char * delimiter = "\\";
#else
  const char * delimiter = "/";
#endif  // _WIN32

  return rcutils_concat(left_hand_path, right_hand_path, delimiter);
}

#if __cplusplus
}
#endif
