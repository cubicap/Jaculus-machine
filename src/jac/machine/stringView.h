#pragma once

#include <quickjs.h>

#include "context.h"


namespace jac {


/**
 * @brief A wrapper around QuickJS C-string with automatic memory management
 */
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

    /**
     * @brief Get the C string
     *
     * @return const char*
     */
    const char* c_str() const {
        return data();
    }

    StringView() = default;

    /**
     * @brief Wrap a QuickJS allocated string. The string will be freed when the StringView is freed.
     * @note The string must be allocated using QuickJS functions - JS_NewString, JS_ToCString, etc.
     *
     * @param ctx context to work in
     * @param str string to wrap
     */
    StringView(ContextRef ctx, const char* str) : basic_string_view(str), _ctx(ctx) {}

    ~StringView() {
        if (_ctx) {
            JS_FreeCString(_ctx, data());
        }
    }
};


}  // namespace jac
