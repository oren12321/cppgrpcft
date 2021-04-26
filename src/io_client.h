#ifndef __IOCLIENT_H__
#define __IOCLIENT_H__

#include <string>
#include <memory>

#include <grpcpp/grpcpp.h>

#include <google/protobuf/any.pb.h>

#include "../proto/io.grpc.pb.h"

#include "io_interfaces.h"

class BytesTransferClient {
public:
    BytesTransferClient(std::shared_ptr<::grpc::Channel> channel)
        : stub_(::Io::Transfer::NewStub(channel)) {}

    void Receive(const google::protobuf::Any& streamerMsg, const google::protobuf::Any& receiverMsg, BytesReceiver* receiver, ::grpc::ClientContext* context);

    void Send(const google::protobuf::Any& streamerMsg, const google::protobuf::Any& receiverMsg, BytesStreamer* streamer, ::grpc::ClientContext* context);

private:
    std::unique_ptr<Io::Transfer::Stub> stub_;
};

#endif

