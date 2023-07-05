#pragma once

#include <jac/machine/machine.h>
#include <jac/machine/values.h>
#include <jac/machine/class.h>
#include <jac/machine/functionFactory.h>
#include <vector>
#include <string>

#include "types/streams.h"

namespace jac {


struct WritableProtoBuilder : public ProtoBuilder::Opaque<Writable>, public ProtoBuilder::Properties {
    static void addProperties(ContextRef ctx, Object proto) {
        addMethodMember<void(Writable::*)(std::string), &Writable::write>(ctx, proto, "write");
    }
};


struct ReadableProtoBuilder : public ProtoBuilder::Opaque<Readable>, public ProtoBuilder::Properties {
    static void addProperties(ContextRef ctx, Object proto) {
        FunctionFactory ff(ctx);

        proto.defineProperty("get", ff.newFunctionThis([](ContextRef ctx_, ValueWeak self) {
            Readable& self_ = *ReadableProtoBuilder::getOpaque(ctx_, self);
            auto [promise, resolve, reject] = Promise::create(ctx_);

            bool res = self_.get([resolve, ctx_](char data) mutable {
                resolve.call<void>(std::string{static_cast<char>(data)});
            });

            if (!res) {
                reject.call<void>(Exception::create(Exception::Type::Error, "Stream is not readable"));
            }

            return promise;
        }));

        proto.defineProperty("read", ff.newFunctionThis([](ContextRef ctx_, ValueWeak self) {
            Readable& self_ = *ReadableProtoBuilder::getOpaque(ctx_, self);
            auto [promise, resolve, reject] = Promise::create(ctx_);

            bool res = self_.read([resolve, ctx_](std::string data) mutable {
                resolve.call<void>(data);
            });

            if (!res) {
                reject.call<void>(Exception::create(Exception::Type::Error, "Stream is not readable"));
            }

            return promise;
        }));
    }
};


template<class Next>
class BasicStreamFeature : public Next {
public:

    using WritableClass = Class<WritableProtoBuilder>;
    using ReadableClass = Class<ReadableProtoBuilder>;

    BasicStreamFeature() {
        WritableClass::init("Writable");
        ReadableClass::init("Readable");
    }

    void initialize() {
        Next::initialize();

        WritableClass::initContext(this->context());
        ReadableClass::initContext(this->context());
    }
};


} // namespace jac
