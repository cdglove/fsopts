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

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#  include <io.h>
#else
#  include <unistd.h>
#endif

namespace fsopts {

namespace detail {

template <typename T>
inline void default_exists_handler(std::string const& path, T& value) {
  std::ifstream in(path);
  in >> value;
}

inline void default_exists_handler(std::string const& path, bool& value) {
  value = true;
}

inline bool file_accessible(std::string const& path) {
#ifdef _WIN32
  return access(path.c_str(), 0) == 0;
#else
  return access(path.c_str(), F_OK) == 0;
#endif
}

inline void remove_file(std::string path) {
  remove(path.c_str());
}

class BasicHandler {
 public:
  BasicHandler() = default;
  virtual ~BasicHandler(){};

  BasicHandler(BasicHandler const&) = delete;
  BasicHandler& operator=(BasicHandler const&) = delete;

  void update() {
    handle_reset();

    if(file_accessible(path())) {
      handle_exists();
      remove_file(path());
    }
  }

  virtual std::string const& path() const = 0;

 private:
  virtual void handle_reset()  = 0;
  virtual void handle_exists() = 0;
};

template <typename T>
class ValueProvider {
 private:
  virtual T& value() = 0;
};

class PathProvider : private virtual BasicHandler {
 public:
  PathProvider(std::string path)
      : path_(std::move(path)) {
  }

 protected:
  std::string const& path() const override {
    return path_;
  }

  std::string path_;
};

class NullResetHandler : public virtual BasicHandler {
 public:
  template <typename T>
  NullResetHandler(T&&) {
  }

 private:
  void handle_reset() override {
  }
};

template <typename T>
class AutoResetHandler
    : public virtual BasicHandler
    , private virtual ValueProvider<T> {
 public:
  AutoResetHandler(T default_value)
      : default_value_(std::move(default_value)) {
  }

 private:
  void handle_reset() override {
    value() = default_value_;
  }

  virtual T& value() = 0;

  T default_value_;
};

template <typename T, typename ResetHandler>
class ValueHandler
    : public virtual BasicHandler
    , private virtual ValueProvider<T>
    , private PathProvider
    , private ResetHandler {
 public:
  ValueHandler(std::string path, T default_value)
      : PathProvider(std::move(path))
      , ResetHandler(std::move(default_value))
      , value_(default_value) {
  }

  T* ptr() {
    return &value_;
  }

 private:
  void handle_exists() override {
    default_exists_handler(PathProvider::path(), value_);
  }

  T& value() override {
    return value_;
  }

  T value_;
};

template <typename T, typename ResetHandler>
class RefHandler
    : public virtual BasicHandler
    , private virtual ValueProvider<T>
    , private PathProvider
    , private ResetHandler {
 public:
  RefHandler(std::string path, T default_value, T* ref)
      : PathProvider(std::move(path))
      , ResetHandler(std::move(default_value))
      , ref_(ref) {
  }

 private:
  void handle_exists() override {
    default_exists_handler(PathProvider::path(), *ref_);
  }

  T& value() override {
    return *ref_;
  }

  T* ref_;
};

} // namespace detail

class Description;

template <typename T>
class Handle {
 public:
  Handle() = default;

  T const& operator*() const {
    return get();
  }

  T const& get() const {
    return *value_;
  }

 private:
  friend class Description;
  explicit Handle(std::shared_ptr<T> ptr)
      : value_(std::move(ptr)) {
  }

  std::shared_ptr<T> value_;
};

template <typename T>
class Value {
 public:
  Value() = default;
  explicit Value(T* ref)
      : ref_(ref) {
  }

  Value& default_value(T const& v) {
    default_val_ = v;
    return *this;
  }

  Value& auto_reset(bool auto_reset) {
    auto_reset_ = auto_reset;
    return *this;
  }

  Value& remove_existing(bool remove) {
    remove_existing_ = remove;
    return *this;
  }

 private:
  friend class Description;
  T* ref_               = nullptr;
  T default_val_        = {};
  bool remove_existing_ = true;
  bool auto_reset_      = false;
};

class Description {
 public:
  Description(char const* base_directory)
      : base_(base_directory) {
    if(base_.back() != '/') {
      base_.push_back('/');
    }
  }

  template <typename T>
  Handle<T> add(char const* file, Value<T> const& value) {
    std::string full_path = base_ + file;
    T* handle_ptr         = nullptr;
    if(value.ref_) {
      handle_ptr = value.ref_;
      if(value.auto_reset_) {
        auto handler = std::make_unique<
            detail::RefHandler<T, detail::AutoResetHandler<T>>>(
            full_path, value.default_val_, handle_ptr);
        handlers_->push_back(std::move(handler));
      }
      else {
        auto handler =
            std::make_unique<detail::RefHandler<T, detail::NullResetHandler>>(
                full_path, value.default_val_, handle_ptr);
        handlers_->push_back(std::move(handler));
      }
    }
    else {
      if(value.auto_reset_) {
        auto handler = std::make_unique<
            detail::ValueHandler<T, detail::AutoResetHandler<T>>>(
            full_path, value.default_val_);
        handle_ptr = handler->ptr();
        handlers_->push_back(std::move(handler));
      }
      else {
        auto handler =
            std::make_unique<detail::ValueHandler<T, detail::NullResetHandler>>(
                full_path, value.default_val_);
        handle_ptr = handler->ptr();
        handlers_->push_back(std::move(handler));
      }
    }

    if(value.remove_existing_) {
      if(access(full_path.c_str(), F_OK) == 0) {
        remove(full_path.c_str());
      }
    }

    return Handle<T>({handlers_, handle_ptr});
  };

  void update() {
    for(auto&& handler : *handlers_) {
      handler->update();
    }
  }

 private:
  using HandlerArray = std::vector<std::unique_ptr<detail::BasicHandler>>;
  std::string base_;
  std::shared_ptr<HandlerArray> handlers_ = std::make_shared<HandlerArray>();
};

} // namespace fsopts

#endif // FSOPTS_FSOPTS_HPP_