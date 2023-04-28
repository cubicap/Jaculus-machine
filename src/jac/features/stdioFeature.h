#pragma once

#include <jac/machine/machine.h>
#include <jac/machine/values.h>
#include <jac/machine/functionFactory.h>
#include <vector>
#include <string>
#include <memory>


namespace jac {


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

        FunctionFactory ff(this->context());

        if (!this->stdio.out) {
            throw std::runtime_error("StdioFeature: stdio.out is not set");
        }
        if (!this->stdio.err) {
            throw std::runtime_error("StdioFeature: stdio.err is not set");
        }

        Object console = Object::create(this->context());
        console.set("debug", ff.newFunction([this](std::string str) {
            this->stdio.out->write(str + "\n");
        }));
        console.set("log", ff.newFunction([this](std::string str) {
            this->stdio.out->write(str + "\n");
        }));
        console.set("info", ff.newFunction([this](std::string str) {
            this->stdio.out->write(str + "\n");
        }));
        console.set("warn", ff.newFunction([this](std::string str) {
            this->stdio.err->write(str + "\n");
        }));
        console.set("error", ff.newFunction([this](std::string str) {
            this->stdio.err->write(str + "\n");
        }));
        Object global = this->context().getGlobalObject();
        global.defineProperty("console", console);

        auto& mdl = this->newModule("stdio");
        mdl.addExport("stdout", Next::WritableClass::createInstance(this->context(), new Next::WritableRef(stdio.out.get())));
        mdl.addExport("stderr", Next::WritableClass::createInstance(this->context(), new Next::WritableRef(stdio.err.get())));
        if (stdio.in) {
            mdl.addExport("stdin", Next::ReadableClass::createInstance(this->context(), new Next::ReadableRef(stdio.in.get())));
        }
    }
};


} // namespace jac
