#include "values.h"


namespace jac {


Exception ContextRef::getException() {
    try {
        return Exception(_ctx, JS_GetException(_ctx));
    }
    catch (std::exception& e) {
        throw Exception::create(_ctx, Exception::Type::InternalError, "No exception");
    }
}

Object ContextRef::getGlobalObject() {
    return Object(_ctx, JS_GetGlobalObject(_ctx));
}


} // namespace jac
