#ifndef __IOSERVER_H__
#define __IOSERVER_H__

#include <grpcpp/grpcpp.h>

#include "../proto/io.grpc.pb.h"

#include "io_interfaces.h"

class FilesTransferImpl final : public ::Io::FilesTransfer::Service {

    ::grpc::Status Download(::grpc::ServerContext* context, const ::Io::FileInfo* request, ::grpc::ServerWriter< ::Io::Chunk>* writer) override;

    ::grpc::Status Upload(::grpc::ServerContext* context, ::grpc::ServerReader< ::Io::Packet>* reader, ::Io::Status* response) override;

};

class BytesTransferImpl final : public ::Io::FilesTransfer::Service {

    ::grpc::Status Download(::grpc::ServerContext* context, const ::Io::FileInfo* request, ::grpc::ServerWriter< ::Io::Chunk>* writer) override;

    ::grpc::Status Upload(::grpc::ServerContext* context, ::grpc::ServerReader< ::Io::Packet>* reader, ::Io::Status* response) override;

    BytesStreamer* _streamer = nullptr;
    BytesReceiver* _receiver = nullptr;

public:

    void setStreamer(BytesStreamer* streamer) { _streamer = streamer; }
    void setReceiver(BytesReceiver* receiver) { _receiver = receiver; }

};

#endif

