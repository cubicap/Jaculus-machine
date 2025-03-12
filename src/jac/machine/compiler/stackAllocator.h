#pragma once

#include "cfg.h"
#include "opcode.h"

#include <map>


namespace jac::cfg {


inline std::map<TmpId, int> allocateStackSlots(jac::cfg::Function& cfg) {
    std::map<TmpId, int> stackSlots;
    int stackOffset = 0;

    for (auto& block : cfg.blocks) {
        for (auto& stmt : block->statements) {
            Temp res = stmt.res();
            if (res.type == ValueType::Any) {
                stackSlots.emplace(res.id, stackOffset);
                stackOffset++;
            }
        }
    }

    return stackSlots;
}


};  // namespace jac::cfg
