#pragma once

#include <jac/machine/machine.h>
#include <noal_func.h>
#include <fstream>
#include <jac/machine/class.h>
#include <jac/machine/functionFactory.h>
#include <iostream>
#include <filesystem>
#include <noal_func.h>


template<class Next>
class FilesystemFeature : public Next {
private:
    class File {
        std::fstream _file;
    public:
        std::string path_;

        File(std::string path, std::string mode): path_(path) {
            std::ios::openmode openMode = static_cast<std::ios::openmode>(0);
            if (mode.find('r') != std::string::npos) {
                openMode |= std::ios::in;
            }
            if (mode.find('w') != std::string::npos) {
                openMode |= std::ios::out;
            }
            if (mode.find('a') != std::string::npos) {
                openMode |= std::ios::app;
            }
            if (mode.find('b') != std::string::npos) {
                openMode |= std::ios::binary;
            }
            if (mode.find('t') != std::string::npos) {
                openMode |= std::ios::trunc;
            }

            if (openMode == static_cast<std::ios::openmode>(0)) {
                throw std::runtime_error("Invalid file mode");
            }

            this->_file = std::fstream(path, openMode);
            if (!_file.is_open()) {
                throw std::runtime_error("Could not open file: " + path);
            }
        }

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

    struct FileProtoBuilder : public jac::ProtoBuilder::Opaque<File>, public jac::ProtoBuilder::Properties {
        static void addProperties(jac::ContextRef ctx, jac::Object proto) {
            jac::FunctionFactory ff(ctx);

            addPropMember<std::string, &File::path_>(ctx, proto, "path", jac::PropFlags::ReadOnly);
            addMethodMember<bool(File::*)(), &File::isOpen>(ctx, proto, "isOpen", jac::PropFlags::ReadOnly);
            addMethodMember<void(File::*)(), &File::close>(ctx, proto, "close", jac::PropFlags::ReadOnly);
            addMethodMember<std::string(File::*)(int), &File::read>(ctx, proto, "read", jac::PropFlags::ReadOnly);
            addMethodMember<void(File::*)(std::string), &File::write>(ctx, proto, "write", jac::PropFlags::ReadOnly);
        }
    };

    std::filesystem::path _codeDir;
    std::filesystem::path _dataDir;

public:
    using FileClass = jac::Class<FileProtoBuilder>;

    void setCodeDir(std::string path_) {
        std::cout << "Setting code dir to " << path_ << std::endl;
        this->_codeDir = std::filesystem::path(path_).lexically_normal();
    }

    void setDataDir(std::string path_) {
        std::cout << "Setting data dir to " << path_ << std::endl;
        this->_dataDir = std::filesystem::path(path_).lexically_normal();
    }

    class Path {
        FilesystemFeature& _feature;
    public:
        Path(FilesystemFeature& feature): _feature(feature) {}

        std::string normalize(std::string path) {
            return std::filesystem::path(path).lexically_normal().string();
        }

        std::string dirname(std::string path) {
            return std::filesystem::path(path).parent_path().string();
        }

        std::string basename(std::string path) {
            return std::filesystem::path(path).filename().string();
        }

        std::string join(std::vector<std::string> paths) {
            std::filesystem::path path;
            for (auto& p : paths) {
                path /= p;
            }
            return path.string();
        }
    };

private:
    class Fs {
        FilesystemFeature& _feature;
    public:
        Fs(FilesystemFeature& feature) : _feature(feature) {}

        std::string loadCode(std::string filename) {
            std::string buffer;
            File file(_feature._codeDir / filename, "r");
            std::string read = file.read();
            while (read.size() > 0) {
                buffer += read;
                read = file.read();
            }
            return buffer;
        }


        File openData(std::string path_, std::string mode) {
            return File(_feature._dataDir / path_, mode);
        }

        bool exists(std::string path_) {
            return std::filesystem::exists(_feature._dataDir / path_);
        }

        bool isFile(std::string path_) {
            return std::filesystem::is_regular_file(_feature._dataDir / path_);
        }

        bool isDir(std::string path_) {
            return std::filesystem::is_directory(_feature._dataDir / path_);
        }

        void mkdir(std::string path_) {
            std::filesystem::create_directories(_feature._dataDir / path_);
        }

        void rm(std::string path_) {
            std::filesystem::remove(_feature._dataDir / path_);
        }

        void rmdir(std::string path_) {
            std::filesystem::remove_all(_feature._dataDir / path_);
        }
    };

public:
    Path path;
    Fs fs;

    FilesystemFeature() : path(*this), fs(*this) {
        FileClass::init("File");
    }

    void initialize() {
        Next::initialize();

        FileClass::initContext(this->_context);

        jac::FunctionFactory ff(this->_context);

        jac::JSModule& pathMod = this->newModule("path");
        pathMod.addExport("normalize", ff.newFunction(noal::function(&Path::normalize, &(this->path))));
        pathMod.addExport("dirname", ff.newFunction(noal::function(&Path::dirname, &(this->path))));
        pathMod.addExport("basename", ff.newFunction(noal::function(&Path::basename, &(this->path))));
        pathMod.addExport("join", ff.newFunction(noal::function(&Path::join, &(this->path))));

        jac::JSModule& fsMod = this->newModule("fs");
        fsMod.addExport("open", ff.newFunction([this](std::string path_, std::string mode) {
            return FileClass::createInstance(this->_context, new File(this->fs.openData(path_, mode)));
        }));
        fsMod.addExport("exists", ff.newFunction(noal::function(&Fs::exists, &(this->fs))));
        fsMod.addExport("isFile", ff.newFunction(noal::function(&Fs::isFile, &(this->fs))));
        fsMod.addExport("isDir", ff.newFunction(noal::function(&Fs::isDir, &(this->fs))));
        fsMod.addExport("mkdir", ff.newFunction(noal::function(&Fs::mkdir, &(this->fs))));
        fsMod.addExport("rm", ff.newFunction(noal::function(&Fs::rm, &(this->fs))));
        fsMod.addExport("rmdir", ff.newFunction(noal::function(&Fs::rmdir, &(this->fs))));
    }
};
