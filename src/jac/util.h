#pragma once

#include <utility>

namespace jac {


template<typename T>
struct Defer {
    T _func;

    Defer(T&& func): _func(std::move(func)) {}
    ~Defer() { _func(); }
};


} // namespace jac
