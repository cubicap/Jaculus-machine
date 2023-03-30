#pragma once

#include <jac/machine/machine.h>
#include <jac/machine/values.h>
#include <jac/machine/functionFactory.h>
#include <vector>
#include <string>
#include <memory>


template<class Next>
class StdioFeature : public Next {
private:
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

        jac::FunctionFactory ff(this->_context);

        assert(this->stdio.out);
        assert(this->stdio.err);

        jac::Object console = jac::Object::create(this->_context);
        console.set("debug", ff.newFunction([this](std::string str) {
            this->stdio.out->print(str + "\n");
        }));
        console.set("log", ff.newFunction([this](std::string str) {
            this->stdio.out->print(str + "\n");
        }));
        console.set("info", ff.newFunction([this](std::string str) {
            this->stdio.out->print(str + "\n");
        }));
        console.set("warn", ff.newFunction([this](std::string str) {
            this->stdio.err->print(str + "\n");
        }));
        console.set("error", ff.newFunction([this](std::string str) {
            this->stdio.err->print(str + "\n");
        }));
        this->registerGlobal("console", console);

        auto& module = this->newModule("stdio");
        module.addExport("stdout", Next::WritableClass::createInstance(this->_context, new Next::WritableRef(stdio.out.get())));
        module.addExport("stderr", Next::WritableClass::createInstance(this->_context, new Next::WritableRef(stdio.err.get())));
        if (stdio.in) {
            module.addExport("stdin", Next::ReadableClass::createInstance(this->_context, new Next::ReadableRef(stdio.in.get())));
        }
    }
};
