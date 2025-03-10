#include "mirUtil.h"


namespace jac::cfg::mir_emit {


int getId() {
    static int counter = 0;
    return ++counter;
}


}  // namespace jac::cfg::mir_emit
