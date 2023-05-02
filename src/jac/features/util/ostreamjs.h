#pragma once

#include <vector>
#include <memory>
#include <iostream>

#include "../types/streams.h"

namespace jac {


class OsWritable : public Writable {
    std::ostream& _stream;
public:
    OsWritable(std::ostream& stream): _stream(stream) {}

    void write(std::string data) override {
        _stream.write(data.data(), data.size());
    }
};


template<class Machine>
static inline void initializeIo(Machine& machine) {
    if (!machine.stdio.out) {
        machine.stdio.out = std::make_unique<OsWritable>(std::cout);
    }
    if (!machine.stdio.err) {
        machine.stdio.err = std::make_unique<OsWritable>(std::cerr);
    }
}


} // namespace jac
