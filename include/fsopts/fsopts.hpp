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
#ifndef FSOPTS_FSOPTS_HPP
#define FSOPTS_FSOPTS_HPP
#pragma once

#include <string>
#include <vector>

namespace fsopts {

namespace detail {


class TypeIdBase {
 public:
  static std::size_t next_index() {
    static std::size_t x = 0;
    return x++;
  }
};

template <typename T>
class TypeId : TypeIdBase {
 public:
  static id() {
    static auto type = next_index();
    return type;
  }
};
} // namespace detail

template<typename T>
class Handle;

template<typename T>
class Value;

class Description;
class Map;

template <typename T>
class Handle {
  Handle() = default;

  T const& operator*() const {
  }

  T const& get() const {
  }

 private:
  friend class Description;
  explicit Handle(std::size_t idx)
      : idx_(idx) {
  }

  std::size_t idx_;
};

class Description {
 public:
  Description(char const* base_directory)
      : base_(base_directory) {
  }

  template <typename T>
  Handle<T> add(char const* file, Value<T> const& value){};

  void update(Map& storage) {
  }

 private:
  std::string base_;
};

template <typename T>
class Value {};

class Map {};

} // namespace fsopts

#endif // FSOPTS_FSOPTS_HPP_