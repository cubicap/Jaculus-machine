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
        virtual void write(std::string data) = 0;

        virtual ~Writable() = default;
    };

    struct WritableProtoBuilder : public jac::ProtoBuilder::Opaque<Writable>, public jac::ProtoBuilder::Properties {
        static void addProperties(jac::ContextRef ctx, jac::Object proto) {
            addMethodMember<void(Writable::*)(std::string), &Writable::write>(ctx, proto, "write");

        }
    };

    class Readable {
    public:
        virtual bool get(std::function<void(char)> callback) = 0;
        virtual bool read(std::function<void(std::string)> callback) = 0;

        virtual ~Readable() = default;
    };

    struct ReadableProtoBuilder : public jac::ProtoBuilder::Opaque<Readable>, public jac::ProtoBuilder::Properties {
        static void addProperties(jac::ContextRef ctx, jac::Object proto) {
            jac::FunctionFactory ff(ctx);

            proto.defineProperty("get", ff.newFunctionThis([](jac::ContextRef ctx_, jac::ValueWeak self) {
                Readable& self_ = *ReadableProtoBuilder::getOpaque(ctx_, self);
                auto [promise, resolve, reject] = jac::Promise::create(ctx_);

                bool res = self_.get([resolve, ctx_](char data) mutable {
                    resolve.call<void>(std::string{static_cast<char>(data)});
                });

                if (!res) {
                    reject.call<void>(jac::Exception::create(ctx_, jac::Exception::Type::Error, "Stream is not readable"));
                }

                return promise;
            }));

            proto.defineProperty("read", ff.newFunctionThis([](jac::ContextRef ctx_, jac::ValueWeak self) {
                Readable& self_ = *ReadableProtoBuilder::getOpaque(ctx_, self);
                auto [promise, resolve, reject] = jac::Promise::create(ctx_);

                bool res = self_.read([resolve, ctx_](std::string data) mutable {
                    resolve.call<void>(data);
                });

                if (!res) {
                    reject.call<void>(jac::Exception::create(ctx_, jac::Exception::Type::Error, "Stream is not readable"));
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
