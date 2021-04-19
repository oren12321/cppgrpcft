#include "io_server.h"

#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "../proto/io.grpc.pb.h"

::grpc::Status FilesTransferImpl::Download(::grpc::ServerContext* context, const ::Io::FileInfo* request, ::grpc::ServerWriter< ::Io::Chunk>* writer) {
    
    std::string from = request->path();

    std::ifstream ifs(from, std::ios::in | std::ios::binary);
    if (!ifs.is_open()) {
        std::stringstream ss;
        ss << "failed to open downloaded file: '" << from << "'";
        return ::grpc::Status(::grpc::FAILED_PRECONDITION, ss.str());
    }

    ifs.seekg(0, std::ios::end);
    std::streampos fsize = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    if (fsize == 0) {
        return ::grpc::Status(::grpc::FAILED_PRECONDITION, "requested file is empty");
    }

    std::vector<char> buffer(2048);

    while (!ifs.eof()) {
        ifs.read(buffer.data(), buffer.size());
        std::streamsize n = ifs.gcount();

        ::Io::Chunk chunk;
        chunk.set_data(buffer.data(), n);
        writer->Write(chunk);
    }

    ifs.close();

    return ::grpc::Status::OK;
}

::grpc::Status FilesTransferImpl::Upload(::grpc::ServerContext* context, ::grpc::ServerReader< ::Io::Packet>* reader, ::Io::Status* response) {
    return ::grpc::Status::OK;
}

