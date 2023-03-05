#include "values.h"


namespace jac {


Exception ContextRef::getException(ValueConst val) {
    try {
        return Exception(_ctx, JS_GetException(_ctx));
    }
    catch (std::exception& e) {
        throw std::runtime_error("no exception");
    }
}


} // namespace jac
