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
        virtual void write(std::span<const uint8_t> buffer) = 0;
        virtual void write(jac::ArrayBuffer buffer) {
            write(buffer.typedView<uint8_t>());
        }
        virtual void print(std::string str) {
            write(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(str.data()), str.size()));
        }

        virtual ~Writable() = default;
    };

    struct WritableProtoBuilder : public jac::ProtoBuilder::Opaque<Writable>, public jac::ProtoBuilder::Properties {
        static void addProperties(jac::ContextRef ctx, jac::Object proto) {
            addMethodMember<void(Writable::*)(std::string), &Writable::print>(ctx, proto, "print");
            addMethodMember<void(Writable::*)(jac::ArrayBuffer), &Writable::write>(ctx, proto, "write");

        }
    };

    class Readable {
    public:
        virtual bool read(std::function<void(std::span<const uint8_t>)> callback) = 0;

        virtual ~Readable() = default;
    };

    struct ReadableProtoBuilder : public jac::ProtoBuilder::Opaque<Readable>, public jac::ProtoBuilder::Properties {
        static void addProperties(jac::ContextRef ctx, jac::Object proto) {
            jac::FunctionFactory ff(ctx);

            addProp(ctx, proto, "read", ff.newFunctionThis([](jac::ContextRef ctx_, jac::ValueWeak self) {
                Readable& self_ = *ReadableProtoBuilder::getOpaque(ctx_, self);
                auto [promise, resolve, reject] = jac::Promise::create(ctx_);

                bool res = self_.read([resolve, ctx_](std::span<const uint8_t> buffer) mutable {
                    resolve.call<void>(jac::ArrayBuffer::create(ctx_, buffer));
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

        void write(std::span<const uint8_t> buffer) override {
            _ptr->write(buffer);
        }

        void write(jac::ArrayBuffer buffer) override {
            _ptr->write(buffer);
        }

        void print(std::string str) override {
            _ptr->print(str);
        }
    };

    class ReadableRef : public Readable {
    private:
        Readable* _ptr;
    public:
        ReadableRef(Readable* ptr): _ptr(ptr) {}

        bool read(std::function<void(std::span<const uint8_t>)> callback) override {
            return _ptr->read(std::move(callback));
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
