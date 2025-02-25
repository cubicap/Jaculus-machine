#pragma once


#include <list>
#include <map>

#include "cfg.h"


namespace jac::cfg {


using Replacements = std::map<BasicBlockPtr, std::list<BasicBlockPtr>>;
using Mapping = std::map<BasicBlockPtr, BasicBlockPtr>;


// FIXME: use some more standard and efficient algorithm

namespace detail {

    inline void removeTransitive(Replacements& replacements) {
        for (auto group = replacements.begin(); group != replacements.end(); ++group) {
            auto& [target, sources] = *group;
            for (const auto& source : sources) {
                auto it = replacements.find(source);
                if (it != replacements.end()) {
                    sources.splice(sources.end(), std::move(it->second));
                    replacements.erase(it);
                }
            }
        }
    }

}  // namespace detail


inline void removeEmptyBlocks(FunctionEmitter& emitter) {
    Replacements replacements;

    for (auto& block : emitter.blocks) {
        if (!block->statements.empty()) {
            continue;
        }
        if (block->jump.type == Terminal::Jump) {
            replacements[block->jump.target].push_back(block.get());
        }
        else if (block->jump.type == Terminal::Branch) {
            if (block->jump.target == block->jump.other) {
                replacements[block->jump.target].push_back(block.get());
            }
        }
    }

    detail::removeTransitive(replacements);

    std::map<BasicBlockPtr, BasicBlockPtr> remap;
    for (const auto& [target, sources] : replacements) {
        for (const auto& source : sources) {
            remap[source] = target;
        }
    }

    auto replace = [&] (BasicBlockPtr& target) {
        if (target && remap.contains(target)) {
            target = remap.at(target);
        }
    };
    for (auto& block : emitter.blocks) {
        replace(block->jump.target);
        replace(block->jump.other);
    }

    if (remap.contains(emitter.getEntry())) {
        emitter.setEntry(remap.at(emitter.getEntry()));
    }

    for (auto it = emitter.blocks.begin(); it != emitter.blocks.end(); ++it) {
        if (remap.contains(it->get())) {
            it = emitter.blocks.erase(it);
        }
    }
}


}  // namespace jac::cfg
