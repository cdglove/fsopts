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

class ValueUpdateBase {
 public:
  ValueUpdateBase() = default;
  virtual ~ValueUpdateBase(){};

  ValueUpdateBase(ValueUpdateBase const&) = delete;
  ValueUpdateBase& operator=(ValueUpdateBase const&) = delete;

  void update() {
    handle_update();
  }

 private:
  virtual void handle_update() = 0;
};

template <typename ExistsHandler, typename ResetHandler>
class UpdateHandler
    : public ValueUpdateBase
    , private ExistsHandler
    , private ResetHandler {
 public:
  template <typename T>
  UpdateHandler(std::string path, T&& default_value)
      : ResetHandler(std::forward<T>(default_value))
      , path_(std::move(path)) {
  }

 private:
  void handle_update() override {
    handle_reset();
    std::string const& p = path_;
    if(file_accessible(p)) {
      handle_exists(p);
      remove_file(p);
    }
  }

  void handle_reset() {
    ResetHandler& reset = *this;
    reset();
  }

  void handle_exists(std::string const& path) {
    ExistsHandler& exists = *this;
    exists(path);
  }

  std::string path_;
};

template <typename T>
class ExistsHandlerReadValue {
 public:
  void operator()(std::string const& path) {
    std::ifstream in(path);
    in >> std::boolalpha;
    in >> value();
  }

 private:
  virtual T& value() = 0;
};

class ExistsHandlerSetTrue {
 public:
  void operator()(std::string const&) {
    value() = true;
  }

 private:
  virtual bool& value() = 0;
};

class ResetHandlerNop {
 public:
  template <typename U>
  ResetHandlerNop(U&&) {
  }

  void operator()() {
  }
};

template <typename T>
class ResetHandlerAuto {
 public:
  ResetHandlerAuto(T default_value)
      : default_value_(std::move(default_value)) {
  }

  void operator()() {
    value() = default_value_;
  }

 private:
  virtual T& value() = 0;

  T default_value_;
};

template <typename T, typename ExistsHandler, typename ResetHandler>
class ValueHandler : public UpdateHandler<ExistsHandler, ResetHandler> {
 public:
  ValueHandler(std::string path, T default_value)
      : UpdateHandler<ExistsHandler, ResetHandler>(
            std::move(path), std::move(default_value))
      , value_(default_value) {
  }

  T* ptr() {
    return &value_;
  }

 private:
  T& value() override {
    return value_;
  }

  T value_;
};

template <typename T, typename ExistsHandler, typename ResetHandler>
class RefHandler : public UpdateHandler<ExistsHandler, ResetHandler> {
 public:
  RefHandler(std::string path, T default_value, T* ref)
      : UpdateHandler<ExistsHandler, ResetHandler>(
            std::move(path), std::move(default_value))
      , ref_(ref) {
  }

 private:
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

class Trigger {
 public:
  Trigger() {
    v_.auto_reset(true);
  }
  explicit Trigger(bool* ref)
      : v_(ref) {
    v_.auto_reset(true);
  }

  Trigger& default_value(bool v) {
    v_.default_value(v);
    return *this;
  }

  Trigger& remove_existing(bool remove) {
    v_.remove_existing(remove);
    return *this;
  }

  Value<bool> const& to_value() const {
    return v_;
  }

 private:
  Value<bool> v_;
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
    return add_reset<detail::ExistsHandlerReadValue<T>>(file, value);
  };

  Handle<bool> add(char const* file, Trigger const& trigger) {
    return add_reset<detail::ExistsHandlerSetTrue>(file, trigger.to_value());
  }

  void update() {
    for(auto&& handler : *handlers_) {
      handler->update();
    }
  }

 private:
  template <typename ExistsHandler, typename T>
  Handle<T> add_reset(char const* file, Value<T> const& value) {
    if(value.auto_reset_) {
      return add_updater<ExistsHandler, detail::ResetHandlerAuto<T>>(
          file, value);
    }
    else {
      return add_updater<ExistsHandler, detail::ResetHandlerNop>(file, value);
    }
  }

  template <typename ExistsHandler, typename ResetHandler, typename T>
  Handle<T> add_updater(char const* file, Value<T> const& value) {
    if(value.ref_) {
      return add_ref<ExistsHandler, ResetHandler>(file, value);
    }
    else {
      return add_value<ExistsHandler, ResetHandler>(file, value);
    }
  }

  template <typename ExistsHandler, typename ResetHandler, typename T>
  Handle<T> add_value(char const* file, Value<T> const& value) {
    auto handler =
        std::make_unique<detail::ValueHandler<T, ExistsHandler, ResetHandler>>(
            base_ + file, value.default_val_);
    T* ptr = handler->ptr();
    append_handler(std::move(handler));
    return Handle<T>({handlers_, ptr});
  };

  template <typename ExistsHandler, typename ResetHandler, typename T>
  Handle<T> add_ref(char const* file, Value<T> const& value) {
    auto handler =
        std::make_unique<detail::RefHandler<T, ExistsHandler, ResetHandler>>(
            base_ + file, value.default_val_, value.ref_);
    append_handler(std::move(handler));
    return Handle<T>({handlers_, value.ref_});
  };

  void append_handler(std::unique_ptr<detail::ValueUpdateBase> handler) {
    handlers_->push_back(std::move(handler));
  }

  using HandlerArray = std::vector<std::unique_ptr<detail::ValueUpdateBase>>;
  std::string base_;
  std::shared_ptr<HandlerArray> handlers_ = std::make_shared<HandlerArray>();
};

} // namespace fsopts

#endif // FSOPTS_FSOPTS_HPP_