#include "io_client.h"

#include <string>
#include <stdexcept>
#include <memory>
#include <sstream>
#include <fstream>
#include <cstdio>

#include <grpcpp/grpcpp.h>

#include "../proto/io.grpc.pb.h"

#include "io_interfaces.h"

void FilesTransferClient::Download(std::string from, std::string to, ::grpc::ClientContext* context) {

    std::ofstream ofs(to, std::ios::out | std::ios::binary);
    if (!ofs.is_open()) {
        std::stringstream ss;
        ss << "failed to create destination file: '" << to << "'";
        throw std::runtime_error(ss.str());
    }

    ::Io::FileInfo fileInfo;
    fileInfo.set_path(from);
    std::unique_ptr<::grpc::ClientReader<::Io::Chunk>> reader(stub_->Download(context, fileInfo));

    ::Io::Chunk chunk;
    while (reader->Read(&chunk)) {
        std::string data = chunk.data();
        ofs.write(data.data(), data.size());
    }

    ofs.close();

    ::grpc::Status status = reader->Finish();
    if (!status.ok()) {
        std::remove(to.c_str());

        std::stringstream ss;
        ss << "client failed - code: " << status.error_code() << ", message: " << status.error_message();
        throw std::runtime_error(ss.str());
    }
}

void FilesTransferClient::Upload(std::string from, std::string to, ::grpc::ClientContext* context) {

    std::ifstream ifs(from, std::ios::in | std::ios::binary);
    if (!ifs.is_open()) {
        std::stringstream ss;
        ss << "failed to open downloaded file: '" << from << "'";
        throw std::runtime_error(ss.str());
    }

    ifs.seekg(0, std::ios::end);
    std::streampos fsize = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    if (fsize == 0) {
        throw std::runtime_error("file for upload is empty");
    }

    ::Io::Status ioStatus;
    std::unique_ptr<::grpc::ClientWriter<::Io::Packet>> writer(stub_->Upload(context, &ioStatus));

    ::Io::Packet header;
    header.mutable_fileinfo()->set_path(to);
    if(!writer->Write(header))
    {
        writer->WritesDone();

        ::grpc::Status status = writer->Finish();
        if (!status.ok()) {
            std::stringstream ss;
            ss << "client failed - code: " << status.error_code() << ", message: " << status.error_message() << ", io status: (success: " << ioStatus.success() << ", msg: " << ioStatus.msg() << ')';
            throw std::runtime_error(ss.str());
        }
    }

    std::vector<char> buffer(2048);

    while (!ifs.eof()) {
        ifs.read(buffer.data(), buffer.size());
        std::streamsize n = ifs.gcount();

        ::Io::Packet payload;
        payload.mutable_chunk()->set_data(buffer.data(), n);
        if(!writer->Write(payload)) {
            break;
        }
    }

    ifs.close();

    writer->WritesDone();

    ::grpc::Status status = writer->Finish();
    if (!status.ok()) {
        std::stringstream ss;
        ss << "client failed - code: " << status.error_code() << ", message: " << status.error_message() << ", io status: (success: " << ioStatus.success() << ", msg: " << ioStatus.msg() << ')';
        throw std::runtime_error(ss.str());
    }
}

void BytesTransferClient::Download(std::string from, std::string to, BytesReceiver* receiver, ::grpc::ClientContext* context) {

    if (receiver == nullptr) {
        throw std::runtime_error( "uninitialized bytes receiver");
    }

    try {
        receiver->init(to);
    }
    catch(const std::exception& ex) {
        std::stringstream ss;
        ss << "bytes receiver initialization failed: " << ex.what();
        throw std::runtime_error(ss.str());
    }

    ::Io::FileInfo fileInfo;
    fileInfo.set_path(from);
    std::unique_ptr<::grpc::ClientReader<::Io::Chunk>> reader(stub_->Download(context, fileInfo));

    ::Io::Chunk chunk;
    while (reader->Read(&chunk)) {
        std::string data = chunk.data();
        try {
            receiver->push(data);
        }
        catch(const std::exception& ex) {
            std::stringstream ss;
            ss << "failed to push bytes: " << ex.what();
            throw std::runtime_error(ss.str());
        }
    }

    try {
        receiver->finalize();
    }
    catch(const std::exception& ex) {
        std::stringstream ss;
        ss << "bytes receiver finalize failed: " << ex.what();
        throw std::runtime_error(ss.str());
    }

    ::grpc::Status status = reader->Finish();
    if (!status.ok()) {
        std::stringstream ss;
        ss << "client failed - code: " << status.error_code() << ", message: " << status.error_message();
        throw std::runtime_error(ss.str());
    }
}

void BytesTransferClient::Upload(std::string from, std::string to, BytesStreamer* streamer, ::grpc::ClientContext* context) {

    if (streamer == nullptr) {
        throw std::runtime_error("uninitialized bytes streamer");
    }

    try {
        streamer->init(from);
    }
    catch(const std::exception& ex) {
        std::stringstream ss;
        ss << "bytes streamer initialization failed: " << ex.what();
        throw std::runtime_error(ss.str());
    }

    ::Io::Status ioStatus;
    std::unique_ptr<::grpc::ClientWriter<::Io::Packet>> writer(stub_->Upload(context, &ioStatus));

    ::Io::Packet header;
    header.mutable_fileinfo()->set_path(to);
    if(!writer->Write(header))
    {
        writer->WritesDone();

        ::grpc::Status status = writer->Finish();
        if (!status.ok()) {
            std::stringstream ss;
            ss << "client failed - code: " << status.error_code() << ", message: " << status.error_message() << ", io status: (success: " << ioStatus.success() << ", msg: " << ioStatus.msg() << ')';
            throw std::runtime_error(ss.str());
        }
    }

    while (streamer->hasNext()) {
        try {
            ::Io::Packet payload;
            payload.mutable_chunk()->set_data(streamer->getNext());
            if(!writer->Write(payload)) {
                break;
            }
        }
        catch(const std::exception& ex) {
            std::stringstream ss;
            ss << "failed to get next streamer bytes: " << ex.what();
        }
    }

    try {
        streamer->finalize();
    }
    catch(const std::exception& ex) {
        std::stringstream ss;
        ss << "bytes streamer finalize failed: " << ex.what();
        throw std::runtime_error(ss.str());
    }

    writer->WritesDone();

    ::grpc::Status status = writer->Finish();
    if (!status.ok()) {
        std::stringstream ss;
        ss << "client failed - code: " << status.error_code() << ", message: " << status.error_message() << ", io status: (success: " << ioStatus.success() << ", msg: " << ioStatus.msg() << ')';
        throw std::runtime_error(ss.str());
    }
}

