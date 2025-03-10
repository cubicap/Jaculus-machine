#pragma once

#include "opcode.h"
#include "cfg.h"
#include <map>


namespace jac::cfg {


inline std::map<TmpId, int> allocateStackSlots(jac::cfg::Function& cfg) {
    std::map<TmpId, int> stackSlots;
    int stackOffset = 0;

    for (auto& block : cfg.blocks) {
        for (auto& stmt : block->statements) {
            RValue res = stmt.res();
            if (res.type == ValueType::Any) {
                stackSlots.emplace(res.id, stackOffset);
                stackOffset++;
            }
        }
    }

    return stackSlots;
}


};  // namespace jac::cfg
