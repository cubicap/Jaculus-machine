#pragma once

#include <jac/machine/machine.h>
#include <jac/machine/class.h>
#include <jac/machine/functionFactory.h>
#include <vector>
#include <memory>
#include <iostream>


template<class Next>
class StdioFeature : public Next {
private:
    class Writable {
    public:
        virtual void print(std::vector<jac::ValueConst> args) = 0;
        virtual void println(std::vector<jac::ValueConst> args) = 0;

        virtual void print(std::string str) = 0;
        virtual void println(std::string str) = 0;

        virtual ~Writable() = default;
    };

    class OsWritable : public Writable {
        std::ostream& _stream;
    public:
        OsWritable(std::ostream& stream): _stream(stream) {}
        void print(std::vector<jac::ValueConst> args) override {
            for (auto val : args) {
                try {
                    _stream << val.toString() << " ";
                }
                catch (...) {
                    _stream << "[Cannot convert]" << " ";
                }
            }
        }

        void println(std::vector<jac::ValueConst> args) override {
            print(args);
            _stream << std::endl;
        }

        void print(std::string str) override {
            _stream << str << std::flush;
        }

        void println(std::string str) override {
            _stream << str << std::endl;
        }
    };

    struct WritableProtoBuilder : public jac::ProtoBuilder::Opaque<Writable>, public jac::ProtoBuilder::Properties {
        static void addProperties(jac::ContextRef ctx, jac::Object proto) {
            addMethodMemberVariadic<void(Writable::*)(std::vector<jac::ValueConst>), &Writable::print>(ctx, proto, "print");
            addMethodMemberVariadic<void(Writable::*)(std::vector<jac::ValueConst>), &Writable::println>(ctx, proto, "println");
        }
    };

    class Readable {
    public:
        virtual std::string readLine() = 0;

        virtual ~Readable() = default;
    };

    class IsReadable : public Readable {
        std::istream& _stream;
    public:
        IsReadable(std::istream& stream): _stream(stream) {}
        std::string readLine() override {
            std::string line;
            std::getline(_stream, line);
            return line;
        }
    };

    struct ReadableProtoBuilder : public jac::ProtoBuilder::Opaque<Readable>, public jac::ProtoBuilder::Properties {
        static void addProperties(jac::ContextRef ctx, jac::Object proto) {
            addMethodMember<std::string(Readable::*)(), &Readable::readLine>(ctx, proto, "readLine");
        }
    };

    class Stdio {
    public:
        std::unique_ptr<Writable> out = std::make_unique<OsWritable>(std::cout);
        std::unique_ptr<Writable> err = std::make_unique<OsWritable>(std::cerr);
        std::unique_ptr<Readable> in = std::make_unique<IsReadable>(std::cin);
    };

    using WritableClass = jac::Class<WritableProtoBuilder>;
    using ReadableClass = jac::Class<ReadableProtoBuilder>;
public:
    Stdio stdio;

    StdioFeature() {
        WritableClass::init("Writable");
        ReadableClass::init("Readable");
    }

    void initialize() {
        Next::initialize();

        WritableClass::initContext(this->_context);

        jac::FunctionFactory ff(this->_context);

        this->registerGlobal("print", ff.newFunctionVariadic([this](std::vector<jac::ValueConst> args) {
            this->stdio.out->println(args);
        }));

        // TODO: reference the same io objects as cpp interface
        auto& module = this->newModule("stdio");
        module.addExport("stdout", WritableClass::createInstance(this->_context, new OsWritable(std::cout)));
        module.addExport("stderr", WritableClass::createInstance(this->_context, new OsWritable(std::cerr)));
        module.addExport("stdin", ReadableClass::createInstance(this->_context, new IsReadable(std::cin)));
    }
};
