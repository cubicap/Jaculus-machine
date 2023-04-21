#pragma once

#include <jac/machine/machine.h>
#include <jac/machine/class.h>
#include <jac/machine/functionFactory.h>
#include <noal_func.h>

#include <filesystem>
#include <fstream>
#include <noal_func.h>


template<class Next>
class FilesystemFeature : public Next {
private:
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
                throw std::runtime_error("Invalid file flags");
            }

            this->_file = std::fstream(path, openMode);
            if (!_file.is_open()) {
                throw std::runtime_error("Could not open file: " + path);
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

    struct FileProtoBuilder : public jac::ProtoBuilder::Opaque<File>, public jac::ProtoBuilder::Properties {
        static void addProperties(jac::ContextRef ctx, jac::Object proto) {
            jac::FunctionFactory ff(ctx);

            addPropMember<std::string, &File::path_>(ctx, proto, "path", jac::PropFlags::Enumerable);
            addMethodMember<bool(File::*)(), &File::isOpen>(ctx, proto, "isOpen", jac::PropFlags::Enumerable);
            addMethodMember<void(File::*)(), &File::close>(ctx, proto, "close", jac::PropFlags::Enumerable);
            addMethodMember<std::string(File::*)(int), &File::read>(ctx, proto, "read", jac::PropFlags::Enumerable);
            addMethodMember<void(File::*)(std::string), &File::write>(ctx, proto, "write", jac::PropFlags::Enumerable);
        }
    };

    std::filesystem::path _codeDir = ".";
    std::filesystem::path _workingDir = ".";

public:
    using FileClass = jac::Class<FileProtoBuilder>;

    void setCodeDir(std::string path_) {
        this->_codeDir = std::filesystem::path(path_).lexically_normal();
    }

    void setWorkingDir(std::string path_) {
        this->_workingDir = std::filesystem::path(path_).lexically_normal();
    }

    class Path {
        FilesystemFeature& _feature;
    public:
        Path(FilesystemFeature& feature): _feature(feature) {}

        std::string normalize(std::string path) {
            return std::filesystem::path(path).lexically_normal().string();
        }

        std::string dirname(std::string path) {
            auto res = std::filesystem::path(path).parent_path().string();
            return res.empty() ? "." : res;
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


        File open(std::string path_, std::string flags) {
            return File(_feature._workingDir / path_, flags);
        }

        bool exists(std::string path_) {
            return std::filesystem::exists(_feature._workingDir / path_);
        }

        bool isFile(std::string path_) {
            return std::filesystem::is_regular_file(_feature._workingDir / path_);
        }

        bool isDirectory(std::string path_) {
            return std::filesystem::is_directory(_feature._workingDir / path_);
        }

        void mkdir(std::string path_) {
            std::filesystem::create_directories(_feature._workingDir / path_);
        }

        std::vector<std::string> readdir(std::string path_) {
            std::vector<std::string> res;
            for (auto& p : std::filesystem::directory_iterator(_feature._workingDir / path_)) {
                res.push_back(p.path().filename().string());
            }
            return res;
        }

        void rm(std::string path_) {
            std::filesystem::remove(_feature._workingDir / path_);
        }

        void rmdir(std::string path_) {
            std::filesystem::remove_all(_feature._workingDir / path_);
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

        FileClass::initContext(this->context());

        jac::FunctionFactory ff(this->context());

        jac::Module& pathMod = this->newModule("path");
        pathMod.addExport("normalize", ff.newFunction(noal::function(&Path::normalize, &(this->path))));
        pathMod.addExport("dirname", ff.newFunction(noal::function(&Path::dirname, &(this->path))));
        pathMod.addExport("basename", ff.newFunction(noal::function(&Path::basename, &(this->path))));
        pathMod.addExport("join", ff.newFunctionVariadic([this](std::vector<jac::ValueWeak> paths) {
            std::vector<std::string> paths_;
            for (auto& p : paths) {
                paths_.push_back(p.to<std::string>());
            }
            return this->path.join(paths_);
        }));

        jac::Module& fsMod = this->newModule("fs");
        fsMod.addExport("open", ff.newFunction([this](std::string path_, std::string flags) {
            return FileClass::createInstance(this->context(), new File(this->fs.open(path_, flags)));
        }));
        fsMod.addExport("exists", ff.newFunction(noal::function(&Fs::exists, &(this->fs))));
        fsMod.addExport("isFile", ff.newFunction(noal::function(&Fs::isFile, &(this->fs))));
        fsMod.addExport("isDirectory", ff.newFunction(noal::function(&Fs::isDirectory, &(this->fs))));
        fsMod.addExport("mkdir", ff.newFunction(noal::function(&Fs::mkdir, &(this->fs))));
        fsMod.addExport("rm", ff.newFunction(noal::function(&Fs::rm, &(this->fs))));
        fsMod.addExport("rmdir", ff.newFunction(noal::function(&Fs::rmdir, &(this->fs))));
        fsMod.addExport("readdir", ff.newFunction(noal::function(&Fs::readdir, &(this->fs))));
    }
};
