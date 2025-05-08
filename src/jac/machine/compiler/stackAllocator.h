#pragma once

#include "cfg.h"
#include "opcode.h"

#include <map>


namespace jac::cfg {


inline std::map<TmpId, int> allocateStackSlots(jac::cfg::Function& cfg) {
    std::map<TmpId, int> stackSlots;
    int stackOffset = 0;

    std::set<TmpId> args;
    for (auto& arg : cfg.args) {
        if (arg.type == ValueType::Any) {
            args.insert(arg.id);
        }
    }

    for (auto& block : cfg.blocks) {
        for (auto& stmt : block->statements) {
            Temp res = stmt.res();
            if (res.type == ValueType::Any && !args.contains(res.id)) {
                stackSlots.emplace(res.id, stackOffset);
                stackOffset++;
            }
        }
    }

    return stackSlots;
}


};  // namespace jac::cfg
