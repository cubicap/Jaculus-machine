#pragma once

#include <quickjs.h>

#include "internal/declarations.h"


namespace jac {


class ContextRef {
    JSContext* _ctx;

public:
    ContextRef(JSContext* ctx) : _ctx(ctx) {}

    JSContext* get() { return _ctx; }

    Exception getException();

    // TODO: remove implicit conversions?
    operator JSContext*() { return _ctx; }
    operator JSContext*() const { return _ctx; }
    operator bool() { return _ctx != nullptr; }
    ContextRef operator=(JSContext* ctx) { _ctx = ctx; return *this; }

    void free() {
        JS_FreeContext(_ctx);
        _ctx = nullptr;
    }
};


}  // namespace jac
