#include "values.h"


namespace jac {


Exception ContextRef::getException() {
    try {
        return { _ctx, JS_GetException(_ctx) };
    }
    catch (std::exception& e) {
        throw Exception::create(Exception::Type::InternalError, "No exception");
    }
}

Object ContextRef::getGlobalObject() {
    return { _ctx, JS_GetGlobalObject(_ctx) };
}


} // namespace jac
