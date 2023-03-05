#pragma once

#include <quickjs.h>

#include "context.h"


namespace jac {


class StringView : public std::basic_string_view<char> {
    using basic_string_view = std::basic_string_view<char>;

    ContextRef _ctx;

    StringView(const basic_string_view &other) = delete;
    StringView(const StringView &other) = delete;
    StringView &operator=(const StringView &other) = delete;
public:
    StringView(StringView &&other) : basic_string_view(other), _ctx(other._ctx) {
        other._ctx = nullptr;
    }

    StringView &operator=(StringView &&other) {
        if (_ctx) {
            JS_FreeCString(_ctx, data());
        }
        basic_string_view::operator=(other);
        _ctx = other._ctx;
        other._ctx = nullptr;
        return *this;
    }

    const char* c_str() const {
        return data();
    }

    StringView() = default;
    StringView(ContextRef ctx, const char* str) : basic_string_view(str), _ctx(ctx) {}
    StringView(ContextRef ctx, const char* str, size_t len) : basic_string_view(str, len), _ctx(ctx) {}

    ~StringView() {
        if (_ctx) {
            JS_FreeCString(_ctx, data());
        }
    }
};


}  // namespace jac
