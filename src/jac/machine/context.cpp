#include "values.h"


namespace jac {


Exception ContextRef::getException(ValueConst val) {
    try {
        return Exception(_ctx, JS_GetException(_ctx));
    }
    catch (std::exception& e) {
        throw Exception::create(_ctx, Exception::Type::InternalError, "No exception");
    }
}


} // namespace jac
