#pragma once

#include <jac/machine/machine.h>
#include <jac/machine/functionFactory.h>
#include <vector>
#include <memory>
#include <iostream>


template<class Next>
class StdioFeature : public Next {
private:
    class OsWritable : public Next::Writable {
        std::ostream& _stream;
    public:
        OsWritable(std::ostream& stream): _stream(stream) {}

        void print(std::string str) override {
            _stream << str << std::flush;
        }
    };

    class IsReadable : public Next::Readable {
        std::istream& _stream;
    public:
        IsReadable(std::istream& stream): _stream(stream) {}
        std::string readLine() override {
            std::string line;
            std::getline(_stream, line);
            return line;
        }
    };

    class Stdio {
    public:
        std::unique_ptr<typename Next::Writable> out;
        std::unique_ptr<typename Next::Writable> err;
        std::unique_ptr<typename Next::Readable> in;
    };
public:
    Stdio stdio;

    void initialize() {
        Next::initialize();

        if (!stdio.out) {
            stdio.out = std::make_unique<OsWritable>(std::cout);
        }
        if (!stdio.err) {
            stdio.err = std::make_unique<OsWritable>(std::cerr);
        }
        if (!stdio.in) {
            stdio.in = std::make_unique<IsReadable>(std::cin);
        }

        jac::FunctionFactory ff(this->_context);

        this->registerGlobal("print", ff.newFunctionVariadic([this](std::vector<jac::ValueConst> args) {
            this->stdio.out->println(args);
        }));

        auto& module = this->newModule("stdio");
        module.addExport("stdout", Next::WritableClass::createInstance(this->_context, new Next::WritableRef(stdio.out.get())));
        module.addExport("stderr", Next::WritableClass::createInstance(this->_context, new Next::WritableRef(stdio.err.get())));
        module.addExport("stdin", Next::ReadableClass::createInstance(this->_context, new Next::ReadableRef(stdio.in.get())));
    }
};
