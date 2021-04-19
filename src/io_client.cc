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
    ::Io::FileInfo fileInfo;
    fileInfo.set_path(from);

    std::ofstream ofs(to, std::ios::out | std::ios::binary);
    if (!ofs.is_open()) {
        std::stringstream ss;
        ss << "failed to create destination file: '" << to << "'";
        throw std::runtime_error(ss.str());
    }

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
    throw std::runtime_error("unimplemented");
}

