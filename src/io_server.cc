#include "io_server.h"

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>

#include <grpcpp/grpcpp.h>

#include "../proto/io.grpc.pb.h"

#include "io_interfaces.h"

::grpc::Status FilesTransfer::Receive(::grpc::ServerContext* context, const ::Io::Info* request, ::grpc::ServerWriter< ::Io::Chunk>* writer) {
    
    if (context->IsCancelled()) {
        return ::grpc::Status(::grpc::CANCELLED, "deadline exceeded or client cancelled, abandoning");
    }

    std::string from = request->msg();

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

::grpc::Status FilesTransfer::Send(::grpc::ServerContext* context, ::grpc::ServerReader< ::Io::Packet>* reader, ::Io::Status* response) {

    if (context->IsCancelled()) {
        return ::grpc::Status(::grpc::CANCELLED, "deadline exceeded or client cancelled, abandoning");
    }

    ::Io::Packet header;
    reader->Read(&header);
    if (!header.has_info()) {
        response->set_success(false);
        response->set_desc("first packet is not file info");
        return ::grpc::Status(::grpc::FAILED_PRECONDITION, "first packet is not file info");
    }

    std::string to = header.info().msg();
    std::ofstream ofs(to, std::ios::out | std::ios::binary);
    if (!ofs.is_open()) {
        std::stringstream ss;
        ss << "failed to create destination file: '" << to << "'";
        response->set_success(false);
        response->set_desc(ss.str());
        return ::grpc::Status(::grpc::FAILED_PRECONDITION, ss.str());
    }

    ::Io::Packet payload;
    while (reader->Read(&payload)) {
        if (!payload.has_chunk()) {
            ofs.close();
            std::remove(to.c_str());

            response->set_success(false);
            response->set_desc("packet is not chunk");
            return ::grpc::Status(::grpc::FAILED_PRECONDITION, "packet is not chunk");
        }

        std::string data = payload.chunk().data();
        ofs.write(data.data(), data.size());
    }

    return ::grpc::Status::OK;
}

::grpc::Status BytesTransfer::Receive(::grpc::ServerContext* context, const ::Io::Info* request, ::grpc::ServerWriter< ::Io::Chunk>* writer) {
    
    if (context->IsCancelled()) {
        return ::grpc::Status(::grpc::CANCELLED, "deadline exceeded or client cancelled, abandoning");
    }

    if (_streamer == nullptr) {
        return ::grpc::Status(::grpc::FAILED_PRECONDITION, "uninitialized bytes streamer");
    }

    std::string msg = request->msg();
    try {
        _streamer->init(msg);
    }
    catch(const std::exception& ex) {
        std::stringstream ss;
        ss << "bytes streamer initialization failed: " << ex.what();
        return ::grpc::Status(::grpc::FAILED_PRECONDITION, ss.str());
    }

    while (_streamer->hasNext()) {
        try {
            ::Io::Chunk chunk;
            chunk.set_data(_streamer->getNext());
            writer->Write(chunk);
        }
        catch(const std::exception& ex) {
            std::stringstream ss;
            ss << "failed to get next streamer bytes: " << ex.what();
            return ::grpc::Status(::grpc::FAILED_PRECONDITION, ss.str());
        }
    }

    try {
        _streamer->finalize();
    }
    catch(const std::exception& ex) {
        std::stringstream ss;
        ss << "bytes streamer finalize failed: " << ex.what();
        return ::grpc::Status(::grpc::FAILED_PRECONDITION, ss.str());
    }

    return ::grpc::Status::OK;
}

::grpc::Status BytesTransfer::Send(::grpc::ServerContext* context, ::grpc::ServerReader< ::Io::Packet>* reader, ::Io::Status* response) {

    if (context->IsCancelled()) {
        return ::grpc::Status(::grpc::CANCELLED, "deadline exceeded or client cancelled, abandoning");
    }

    if (_receiver == nullptr) {
        return ::grpc::Status(::grpc::FAILED_PRECONDITION, "uninitialized bytes receiver");
    }

    ::Io::Packet header;
    reader->Read(&header);
    if (!header.has_info()) {
        response->set_success(false);
        response->set_desc("first packet is not file info");
        return ::grpc::Status(::grpc::FAILED_PRECONDITION, "first packet is not file info");
    }

    std::string msg = header.info().msg();
    try {
        _receiver->init(msg);
    }
    catch(const std::exception& ex) {
        std::stringstream ss;
        ss << "bytes receiver initialization failed: " << ex.what();
        response->set_success(false);
        response->set_desc(ss.str());
        return ::grpc::Status(::grpc::FAILED_PRECONDITION, ss.str());
    }

    ::Io::Packet payload;
    while (reader->Read(&payload)) {
        if (!payload.has_chunk()) {

            response->set_success(false);
            response->set_desc("packet is not chunk");
            return ::grpc::Status(::grpc::FAILED_PRECONDITION, "packet is not chunk");
        }

        std::string data = payload.chunk().data();
        try {
            _receiver->push(data);
        }
        catch(const std::exception& ex) {
            std::stringstream ss;
            ss << "failed to push bytes: " << ex.what();
            response->set_success(false);
            response->set_desc(ss.str());
            return ::grpc::Status(::grpc::FAILED_PRECONDITION, ss.str());
        }
    }

    try {
        _receiver->finalize();
    }
    catch(const std::exception& ex) {
        std::stringstream ss;
        ss << "bytes receiver finalize failed: " << ex.what();
        response->set_success(false);
        response->set_desc(ss.str());
        return ::grpc::Status(::grpc::FAILED_PRECONDITION, ss.str());
    }

    return ::grpc::Status::OK;
}

