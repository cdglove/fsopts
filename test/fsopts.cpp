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

void test_int_float_string() {
  fsopts::Description ops("./");
  auto s = ops.add("testing_fsopts_string", fsopts::Value<std::string>());
  auto i = ops.add("testing_fsopts_int", fsopts::Value<int>());
  auto f = ops.add("testing_fsopts_float", fsopts::Value<float>());
  ops.update();
  ASSERT_EQUAL(*s, std::string{});
  ASSERT_EQUAL(*i, int{});
  ASSERT_EQUAL(*f, float{});
  std::ofstream fout("testing_fsopts_string");
  fout << "fsopts_string";
  fout.close();

  fout.open("testing_fsopts_int");
  fout << 2;
  fout.close();

  fout.open("testing_fsopts_float");
  fout << 0.01f;
  fout.close();

  ops.update();

  ASSERT_EQUAL(*s, "fsopts_string");
  ASSERT_EQUAL(*i, 2);
  ASSERT_EQUAL(*f, 0.01f);

  std::ifstream fin("testing_fsopts_string");
  ASSERT_EQUAL(fin.good(), false);
  fin.close();

  fin.open("testing_fsopts_int");
  ASSERT_EQUAL(fin.good(), false);
  fin.close();

  fin.open("testing_fsopts_float");
  ASSERT_EQUAL(fin.good(), false);
  fin.close();
}

void test_auto_reset() {
  fsopts::Description ops("./");
  auto h = ops.add("testing_fsopts_bool", fsopts::Value<bool>().auto_reset(true));
  ops.update();
  ASSERT_EQUAL(*h, false);
  std::ofstream fout("testing_fsopts_bool");
  fout << "true";
  fout.close();
  ops.update();
  ASSERT_EQUAL(*h, true);
  std::ifstream fin("testing_fsopts_bool");
  ASSERT_EQUAL(fin.good(), false);
  ops.update();
  ASSERT_EQUAL(*h, false);
}

void test_trigger() {
  fsopts::Description ops("./");
  auto h = ops.add("testing_fsopts_trigger", fsopts::Trigger());
  ops.update();
  ASSERT_EQUAL(*h, false);
  std::ofstream fout("testing_fsopts_trigger");
  fout << " ";
  fout.close();
  ops.update();
  ASSERT_EQUAL(*h, true);
  std::ifstream fin("testing_fsopts_trigger");
  ASSERT_EQUAL(fin.good(), false);
  ops.update();
  ASSERT_EQUAL(*h, false);
}

void test_remove_existing() {
}

int main() {
  test_int_float_string();
  test_auto_reset();
  test_trigger();
  test_remove_existing();
  //test_defaults();
  return 0;
}