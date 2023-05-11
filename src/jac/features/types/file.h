#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <stdexcept>


namespace jac {


class File {
    std::fstream _file;
public:
    std::string path_;

    File(std::string path, std::string flags): path_(path) {
        std::ios::openmode openMode = static_cast<std::ios::openmode>(0);
        if (flags.find('r') != std::string::npos) {
            openMode |= std::ios::in;
        }
        if (flags.find('w') != std::string::npos) {
            openMode |= std::ios::out;
        }
        if (flags.find('a') != std::string::npos) {
            openMode |= std::ios::app;
        }
        if (flags.find('b') != std::string::npos) {
            openMode |= std::ios::binary;
        }
        if (flags.find('t') != std::string::npos) {
            openMode |= std::ios::trunc;
        }

        if (openMode == static_cast<std::ios::openmode>(0)) {
            throw jac::Exception::create(jac::Exception::Type::Error, "Invalid file flags");
        }

        this->_file = std::fstream(path, openMode);
        if (!_file.is_open()) {
            throw jac::Exception::create(jac::Exception::Type::Error, "Could not open file: " + path);
        }
    }
    File(std::filesystem::path path, std::string flags): File(path.string(), flags) {}

    std::string read(int length = 1024) {
        std::string buffer;
        buffer.resize(length);
        this->_file.readsome(buffer.data(), length);
        buffer.resize(this->_file.gcount());
        return buffer;
    }

    void write(std::string data) {
        this->_file.write(data.data(), data.size());
    }

    bool isOpen() {
        return this->_file.is_open();
    }

    void close() {
        this->_file.close();
    }

    ~File() {
        this->close();
    }
};


} // namespace jac
