#pragma once

#include <string>
#include <functional>


namespace jac {


class Writable {
public:
    virtual void write(std::string data) = 0;

    virtual ~Writable() = default;
};


class Readable {
public:
    virtual bool get(std::function<void(char)> callback) = 0;
    virtual bool read(std::function<void(std::string)> callback) = 0;

    virtual ~Readable() = default;
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


} // namespace jac
