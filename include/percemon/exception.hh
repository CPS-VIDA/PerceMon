#pragma once

#ifndef __PERCEMON_EXCEPTION_HH__
#define __PERCEMON_EXCEPTION_HH__

#include <exception>
#include <string>

namespace percemon {

struct not_implemented_error : public std::exception {
 private:
  const std::string what_arg;

 public:
  not_implemented_error(const std::string& what_arg) : what_arg{what_arg} {}
  not_implemented_error(const char* what_arg) : what_arg{what_arg} {}

  not_implemented_error(const not_implemented_error& other) noexcept = default;

  virtual const char* what() const noexcept {
    return this->what_arg.c_str();
  }
};

} // namespace percemon

#endif /* end of include guard: __PERCEMON_EXCEPTION_HH__ */
