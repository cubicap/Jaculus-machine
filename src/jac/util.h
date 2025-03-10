#pragma once

#include <utility>

namespace jac {


template<typename T>
struct Defer {
    T func;

    Defer(T&& func): func(std::move(func)) {}
    ~Defer() { func(); }
};


} // namespace jac
