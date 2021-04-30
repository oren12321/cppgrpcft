#ifndef __FILEINTERFACES_H__
#define __FILEINTERFACES_H__

#include <string>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <sstream>

#include <google/protobuf/any.pb.h>

#include "io_interfaces.h"
#include "../proto/file.grpc.pb.h"

class FileStreamer : public BytesStreamer {
public:
    void init(const google::protobuf::Any& any) override {
        ::Interfaces::File file;
        if (!any.UnpackTo(&file)) {
            throw std::runtime_error("failed to unpack 'Any' to 'File'");
        }

        std::string path = file.path();
        _buf.resize(2048);
        _empty = false;

        _ifs.open(path, std::ios::in | std::ios::binary);
        if (!_ifs.is_open()) {
            std::stringstream ss;
            ss << "failed to open '" << path << "'";
            throw std::runtime_error(ss.str());
        }
    }

    bool hasNext() override {
        return !_empty;
    }

    std::string getNext() override {
        if (_ifs.eof()) {
            _empty = true;
            return std::string{};
        }
        _ifs.read(_buf.data(), _buf.size());
        std::streamsize n = _ifs.gcount();
        return std::string(_buf.data(), n);
    }

    void finalize() noexcept override {
        _ifs.close();
    }

private:
    std::ifstream _ifs;
    std::vector<char> _buf;
    bool _empty;
};

class FileReceiver : public BytesReceiver {
public:
    void init(const google::protobuf::Any& any) override {
        ::Interfaces::File file;
        if (!any.UnpackTo(&file)) {
            throw std::runtime_error("failed to unpack 'Any' to 'File'");
        }

        std::string path = file.path();

        _ofs.open(path, std::ios::out | std::ios::binary);
        if (!_ofs.is_open()) {
            std::stringstream ss;
            ss << "failed to create '" << path << "'";
            throw std::runtime_error(ss.str());
        }
    }

    void push(std::string data) override {
        _ofs.write(data.data(), data.size());
    }

    void finalize() noexcept override {
        _ofs.close();
    }

private:
    std::ofstream _ofs;
};

#endif

