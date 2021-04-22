#ifndef __IOCLIENT_H__
#define __IOCLIENT_H__

#include <string>
#include <memory>

#include <grpcpp/grpcpp.h>

#include "../proto/io.grpc.pb.h"

#include "io_interfaces.h"

class FilesTransferClient {
public:
    FilesTransferClient(std::shared_ptr<::grpc::Channel> channel)
        : stub_(::Io::Transfer::NewStub(channel)) {}

    void Receive(std::string from, std::string to, ::grpc::ClientContext* context);

    void Send(std::string from, std::string to, ::grpc::ClientContext* context);

private:
    std::unique_ptr<Io::Transfer::Stub> stub_;
};

class BytesTransferClient {
public:
    BytesTransferClient(std::shared_ptr<::grpc::Channel> channel)
        : stub_(::Io::Transfer::NewStub(channel)) {}

    void Receive(std::string streamerMsg, std::string receiverMsg, BytesReceiver* receiver, ::grpc::ClientContext* context);

    void Send(std::string streamerMsg, std::string receiverMsg, BytesStreamer* streamer, ::grpc::ClientContext* context);

private:
    std::unique_ptr<Io::Transfer::Stub> stub_;
};

#endif

