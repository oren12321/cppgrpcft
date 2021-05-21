#include "io_server.h"

#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "../proto/io.grpc.pb.h"

::grpc::Status FilesTransferImpl::Download(::grpc::ServerContext* context, const ::Io::FileInfo* request, ::grpc::ServerWriter< ::Io::Chunk>* writer) {
    
    if (context->IsCancelled()) {
        return ::grpc::Status(::grpc::CANCELLED, "deadline exceeded or client cancelled, abandoning");
    }

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

    if (context->IsCancelled()) {
        return ::grpc::Status(::grpc::CANCELLED, "deadline exceeded or client cancelled, abandoning");
    }

    ::Io::Packet header;
    reader->Read(&header);
    if (!header.has_fileinfo()) {
        //response->set_success(false);
        //response->set_msg("first packet is not file info");
        return ::grpc::Status(::grpc::FAILED_PRECONDITION, "first packet is not file info");
    }

    std::string to = header.fileinfo().path();
    std::ofstream ofs(to, std::ios::out | std::ios::binary);
    if (!ofs.is_open()) {
        std::stringstream ss;
        ss << "failed to create destination file: '" << to << "'";
        //response->set_success(false);
        //response->set_msg(ss.str());
        return ::grpc::Status(::grpc::FAILED_PRECONDITION, ss.str());
    }

    ::Io::Packet payload;
    while (reader->Read(&payload)) {
        if (!payload.has_chunk()) {
            ofs.close();
            std::remove(to.c_str());

            //response->set_success(false);
            //response->set_msg("packet is not chunk");
            return ::grpc::Status(::grpc::FAILED_PRECONDITION, "packet is not chunk");
        }

        std::string data = payload.chunk().data();
        ofs.write(data.data(), data.size());
    }

    return ::grpc::Status::OK;
}

