#include "cfg.h"


namespace jac::cfg {


TmpId newTmpId() {
    static TmpId id = 1;
    if (id == 0) {
        id = 1;
    }
    return id++;
}


}  // namespace jac::cfg
