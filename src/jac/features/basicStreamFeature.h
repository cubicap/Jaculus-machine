#pragma once

#include <jac/machine/machine.h>
#include <jac/machine/values.h>
#include <jac/machine/class.h>
#include <jac/machine/functionFactory.h>
#include <vector>
#include <string>


namespace jac {


template<class Next>
class BasicStreamFeature : public Next {
public:

    class Writable {
    public:
        virtual void write(std::string data) = 0;

        virtual ~Writable() = default;
    };

    struct WritableProtoBuilder : public ProtoBuilder::Opaque<Writable>, public ProtoBuilder::Properties {
        static void addProperties(ContextRef ctx, Object proto) {
            addMethodMember<void(Writable::*)(std::string), &Writable::write>(ctx, proto, "write");

        }
    };

    class Readable {
    public:
        virtual bool get(std::function<void(char)> callback) = 0;
        virtual bool read(std::function<void(std::string)> callback) = 0;

        virtual ~Readable() = default;
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

    class WritableRef : public Writable {
    private:
        Writable* _ptr;
    public:
        WritableRef(Writable* ptr): _ptr(ptr) {}

        void write(std::string data) override {
            _ptr->write(std::move(data));
        }
    };

    class ReadableRef : public Readable {
    private:
        Readable* _ptr;
    public:
        ReadableRef(Readable* ptr): _ptr(ptr) {}

        bool read(std::function<void(std::string)> callback) override {
            return _ptr->read(std::move(callback));
        }

        bool get(std::function<void(char)> callback) override {
            return _ptr->get(std::move(callback));
        }
    };

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
