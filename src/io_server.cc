#include "io_server.h"

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>

#include <grpcpp/grpcpp.h>

#include "../proto/io.grpc.pb.h"

#include "io_interfaces.h"
#include "io_defer.h"

::grpc::Status BytesTransfer::Receive(::grpc::ServerContext* context, const ::Io::Info* request, ::grpc::ServerWriter< ::Io::Chunk>* writer) {
    
    if (context->IsCancelled()) {
        return ::grpc::Status(::grpc::CANCELLED, "deadline exceeded or client cancelled, abandoning");
    }

    if (_streamer == nullptr) {
        return ::grpc::Status(::grpc::FAILED_PRECONDITION, "uninitialized bytes streamer");
    }

    try {
        _streamer->init(request->msg());
    }
    catch(const std::exception& ex) {
        std::stringstream ss;
        ss << "bytes streamer initialization failed: " << ex.what();
        return ::grpc::Status(::grpc::FAILED_PRECONDITION, ss.str());
    }

    Defer defer([this]() noexcept {
        _streamer->finalize();
    });

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

    try {
        _receiver->init(header.info().msg());
    }
    catch(const std::exception& ex) {
        std::stringstream ss;
        ss << "bytes receiver initialization failed: " << ex.what();
        response->set_success(false);
        response->set_desc(ss.str());
        return ::grpc::Status(::grpc::FAILED_PRECONDITION, ss.str());
    }

    Defer defer([this]() noexcept {
        _receiver->finalize();
    });

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

    response->set_success(true);
    response->set_desc("upload succeeded");

    return ::grpc::Status::OK;
}

