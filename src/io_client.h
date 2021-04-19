#ifndef __IOCLIENT_H__
#define __IOCLIENT_H__

#include <string>
#include <memory>

#include <grpcpp/grpcpp.h>

#include "../proto/io.grpc.pb.h"

class FilesTransferClient {
public:
    FilesTransferClient(std::shared_ptr<::grpc::Channel> channel)
        : stub_(::Io::FilesTransfer::NewStub(channel)) {}

    void Download(std::string from, std::string to);

    void Upload(std::string from, std::string to);

private:
    std::unique_ptr<Io::FilesTransfer::Stub> stub_;
};

#endif

