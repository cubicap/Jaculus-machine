#pragma once

#include <quickjs.h>

#include "internal/declarations.h"


namespace jac {


/**
 * @brief A wrapper around JSContext* providing some related functionality
 * @note ContextRef must be freed manually using free()
 */
class ContextRef {
    JSContext* _ctx;

public:
    ContextRef(JSContext* ctx) : _ctx(ctx) {}

    /**
     * @brief Get the underlying JSContext*
     *
     * @return The JSContext*
     */
    JSContext* get() { return _ctx; }

    /**
     * @brief Get pending exception thrown in this context
     * @note If there is no pending exception, new Exception will be thrown
     *
     * @return The Exception
     */
    Exception getException();

    // XXX: remove implicit conversions?
    operator JSContext*() { return _ctx; }
    operator JSContext*() const { return _ctx; }
    operator bool() { return _ctx != nullptr; }
    ContextRef operator=(JSContext* ctx) { _ctx = ctx; return *this; }

    /**
     * @brief Free the context
     */
    void free() {
        JS_FreeContext(_ctx);
        _ctx = nullptr;
    }
};


}  // namespace jac
