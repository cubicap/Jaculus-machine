#pragma once

#include <jac/machine/machine.h>
#include <jac/machine/class.h>
#include <jac/machine/functionFactory.h>
#include <vector>


template<class Next>
class BasicStreamFeature : public Next {
public:
    class Writable {
    public:
        virtual void print(std::vector<jac::ValueConst> args) = 0;
        virtual void println(std::vector<jac::ValueConst> args) = 0;

        virtual void print(std::string str) = 0;
        virtual void println(std::string str) = 0;

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
