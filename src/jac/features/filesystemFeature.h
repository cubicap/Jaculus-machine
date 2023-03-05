#pragma once

#include <jac/machine/machine.h>
#include <noal_func.h>
#include <fstream>


template<class Next>
class FilesystemFeature : public Next {
    std::string _codeDir;
    std::string _dataDir;

    std::string loadFile(std::string filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + filename);
        }
        std::string buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        return buffer;
    }
public:
    std::string loadCode(std::string filename) {
        return this->loadFile(this->_codeDir + filename);
    }

    std::string loadData(std::string filename) {
        return this->loadFile(this->_dataDir + filename);
    }

    void setCodeDir(std::string path) {
        if (path.back() != '/') {
            path += '/';
        }
        this->_codeDir = path;
    }

    void setDataDir(std::string path) {
        if (path.back() != '/') {
            path += '/';
        }
        this->_dataDir = path;
    }

    std::string getParentDir(std::string path) {
        if (path.back() == '/') {
            path.pop_back();
        }
        return path.substr(0, path.find_last_of('/') + 1);
    }

    std::string getFilename(std::string path) {
        if (path.back() == '/') {
            path.pop_back();
        }
        return path.substr(path.find_last_of('/') + 1);
    }
};
