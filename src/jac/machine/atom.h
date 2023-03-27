#pragma once

#include <quickjs.h>

#include "internal/declarations.h"
#include "context.h"


namespace jac {


template<typename T=int>
constexpr bool static_false() {
    return false;
};


template<bool managed>
class AtomWrapper {
protected:
    ContextRef _ctx;
    JSAtom _atom;
public:
    AtomWrapper(ContextRef ctx, JSAtom atom) : _ctx(ctx), _atom(atom) {}
    AtomWrapper(const AtomWrapper &other):
        _ctx(other._ctx),
        _atom(managed ? JS_DupAtom(_ctx, other._atom) : other._atom)
    {}
    AtomWrapper(AtomWrapper &&other) : _ctx(other._ctx), _atom(other._atom) {
        other._atom = JS_ATOM_NULL;
        other._ctx = nullptr;
    }

    AtomWrapper& operator=(const AtomWrapper &other) {
        if (managed) {
            if (_ctx) {
                JS_FreeAtom(_ctx, _atom);
            }
            _atom = JS_DupAtom(_ctx, other._atom);
        }
        else {
            _atom = other._atom;
        }
        _ctx = other._ctx;
        return *this;
    }

    AtomWrapper& operator=(AtomWrapper &&other) {
        if (managed && _ctx) {
            JS_FreeAtom(_ctx, _atom);
        }
        _atom = other._atom;
        _ctx = other._ctx;
        other._atom = JS_ATOM_NULL;
        other._ctx = nullptr;
        return *this;
    }

    operator AtomWrapper<false>() const {
        return AtomWrapper<false>(_ctx, _atom);
    }

    ~AtomWrapper() {
        if (managed && _ctx) {
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

    static AtomWrapper<true> create(ContextRef ctx, uint32_t value) {
        return AtomWrapper<true>(ctx, JS_NewAtomUInt32(ctx, value));
    }

    static AtomWrapper<true> create(ContextRef ctx, const char* value) {
        return AtomWrapper<true>(ctx, JS_NewAtom(ctx, value));
    }

    static AtomWrapper<true> create(ContextRef ctx, std::string value) {
        return create(ctx, value.c_str());
    }

    friend std::ostream& operator<<(std::ostream& os, AtomWrapper& val) {
        os << val.toString();
        return os;
    }
};


using Atom = AtomWrapper<true>;
using AtomConst = AtomWrapper<false>;


}  // namespace jac
