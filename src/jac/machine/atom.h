#pragma once

#include <quickjs.h>

#include "internal/declarations.h"
#include "context.h"


namespace jac {


template<typename T=int>
constexpr bool static_false() {
    return false;
};


class Atom {
protected:
    ContextRef _ctx;
    JSAtom _atom;
public:
    Atom(ContextRef ctx, JSAtom atom) : _ctx(ctx), _atom(atom) {}
    Atom(const Atom &other):
        _ctx(other._ctx),
        _atom(JS_DupAtom(_ctx, other._atom))
    {}
    Atom(Atom &&other) : _ctx(other._ctx), _atom(other._atom) {
        other._atom = JS_ATOM_NULL;
        other._ctx = nullptr;
    }

    Atom& operator=(const Atom &other) {
        if (_ctx) {
            JS_FreeAtom(_ctx, _atom);
        }
        _atom = JS_DupAtom(_ctx, other._atom);
        _ctx = other._ctx;

        return *this;
    }

    Atom& operator=(Atom &&other) {
        if (_ctx) {
            JS_FreeAtom(_ctx, _atom);
        }
        _atom = other._atom;
        _ctx = other._ctx;
        other._atom = JS_ATOM_NULL;
        other._ctx = nullptr;
        return *this;
    }

    ~Atom() {
        if (_ctx) {
            JS_FreeAtom(_ctx, _atom);
        }
    }

    StringView toString() const {
        return StringView(_ctx, JS_AtomToCString(_ctx, _atom));
    }

    std::pair<ContextRef, JSAtom> loot() {
        JSAtom atom_ = _atom;
        ContextRef ctx_ = this->_ctx;
        _ctx = nullptr;
        _atom = JS_ATOM_NULL;
        return {ctx_, atom_};
    }

    JSAtom get() {
        return _atom;
    }

    JSAtom* getPtr() {
        return &_atom;
    }

    static Atom create(ContextRef ctx, uint32_t value) {
        return Atom(ctx, JS_NewAtomUInt32(ctx, value));
    }

    static Atom create(ContextRef ctx, const char* value) {
        return Atom(ctx, JS_NewAtom(ctx, value));
    }

    static Atom create(ContextRef ctx, std::string value) {
        return create(ctx, value.c_str());
    }

    friend std::ostream& operator<<(std::ostream& os, Atom& val) {
        os << val.toString();
        return os;
    }
};


}  // namespace jac
