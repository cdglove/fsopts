// Copyright (c) 2020 Google Inc.
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
#include "fsopts/fsopts.hpp"
#include <sstream>

template <typename A, typename B>
std::stringstream handle_fail(
    char const* file,
    int line,
    char const* function,
    char const* op,
    char const* as,
    A&& a,
    char const* bs,
    B&& b) {
  std::stringstream str;
  str << std::boolalpha;
  str << file << ':' << line << " in " << function << ": " << a << op << b
      << " [" << as << op << bs << "]";
  throw std::runtime_error(str.str());
}

#ifndef __FUNCTION_NAME__
#  ifdef _MSC_VER
#    define __FUNCTION_NAME__ __FUNCTION__
#  else
#    define __FUNCTION_NAME__ __func__
#  endif
#endif

#define ASSERT_EQUAL(x, y)                                                     \
  {                                                                            \
    if((x) != (y)) {                                                           \
      handle_fail(                                                             \
          __FILE__, __LINE__, __FUNCTION_NAME__, " != ", #x, (x), #y, (y));    \
    }                                                                          \
  }

void test_existance() {
fsopts::OptionsDescription opts("/tmp");
opts.add_option("am_i_set");
opts.update();

}

int main() {
  test_existance();
  return 0;
}