#ifndef __IOSERVER_H__
#define __IOSERVER_H__

#include <grpcpp/grpcpp.h>

#include "../proto/io.grpc.pb.h"

#include "io_interfaces.h"

class BytesTransfer final : public ::Io::Transfer::Service {

    ::grpc::Status Receive(::grpc::ServerContext* context, const ::Io::Info* request, ::grpc::ServerWriter< ::Io::Packet>* writer) override;

    ::grpc::Status Send(::grpc::ServerContext* context, ::grpc::ServerReader< ::Io::Packet>* reader, ::Io::Status* response) override;

    BytesStreamer* _streamer = nullptr;
    BytesReceiver* _receiver = nullptr;

public:

    void setStreamer(BytesStreamer* streamer) { _streamer = streamer; }
    void setReceiver(BytesReceiver* receiver) { _receiver = receiver; }

};

#endif

