#pragma once

#include <jac/machine/machine.h>
#include <jac/machine/values.h>
#include <jac/machine/class.h>
#include <jac/machine/functionFactory.h>
#include <vector>
#include <string>


template<class Next>
class BasicStreamFeature : public Next {
public:
    class Writable {
    public:
        virtual void print(std::string str) = 0;

        virtual void println(std::string str) {
            print(str + "\n");
        }

        virtual void print(std::vector<jac::ValueConst> args) {
            for (auto val : args) {
                try {
                    print(val.to<std::string>() + " ");
                }
                catch (...) {
                    print(std::string("[Cannot convert]") + " ");
                }
            }
        }

        virtual void println(std::vector<jac::ValueConst> args) {
            print(args);
            print("\n");
        }

        virtual ~Writable() = default;
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

    struct ReadableProtoBuilder : public jac::ProtoBuilder::Opaque<Readable>, public jac::ProtoBuilder::Properties {
        static void addProperties(jac::ContextRef ctx, jac::Object proto) {
            addMethodMember<std::string(Readable::*)(), &Readable::readLine>(ctx, proto, "readLine");
        }
    };

    class WritableRef : public Writable {
    private:
        Writable* _ptr;
    public:
        WritableRef(Writable* ptr): _ptr(ptr) {}

        void print(std::string str) override {
            _ptr->print(str);
        }

        void println(std::string str) override {
            _ptr->println(str);
        }

        void print(std::vector<jac::ValueConst> args) override {
            for (auto val : args) {
                try {
                    _ptr->print(val.to<std::string>() + " ");
                }
                catch (...) {
                    _ptr->print(std::string("[Cannot convert]") + " ");
                }
            }
        }

        void println(std::vector<jac::ValueConst> args) override {
            _ptr->print(args);
            _ptr->print("\n");
        }

    };

    class ReadableRef : public Readable {
    private:
        Readable* _ptr;
    public:
        ReadableRef(Readable* ptr): _ptr(ptr) {}

        std::string readLine() override {
            return _ptr->readLine();
        }
    };

    using WritableClass = jac::Class<WritableProtoBuilder>;
    using ReadableClass = jac::Class<ReadableProtoBuilder>;

    BasicStreamFeature() {
        WritableClass::init("Writable");
        ReadableClass::init("Readable");
    }

    void initialize() {
        Next::initialize();

        WritableClass::initContext(this->_context);
        ReadableClass::initContext(this->_context);
    }
};
