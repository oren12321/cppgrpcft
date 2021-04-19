#include "io_client.h"

#include <string>
#include <stdexcept>
#include <memory>
#include <sstream>
#include <fstream>
#include <cstdio>

#include <grpcpp/grpcpp.h>

#include "../proto/io.grpc.pb.h"

void FilesTransferClient::Download(std::string from, std::string to) {

    std::ofstream ofs(to, std::ios::out | std::ios::binary);
    if (!ofs.is_open()) {
        std::stringstream ss;
        ss << "failed to create destination file: '" << to << "'";
        throw std::runtime_error(ss.str());
    }

    ::Io::FileInfo fileInfo;
    fileInfo.set_path(from);
    ::grpc::ClientContext context;
    std::unique_ptr<::grpc::ClientReader<::Io::Chunk>> reader(stub_->Download(&context, fileInfo));

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

void FilesTransferClient::Upload(std::string from, std::string to) {

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
    ::grpc::ClientContext context;
    std::unique_ptr<::grpc::ClientWriter<::Io::Packet>> writer(stub_->Upload(&context, &ioStatus));

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

