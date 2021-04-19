#ifndef __IOSERVER_H__
#define __IOSERVER_H__

#include <grpcpp/grpcpp.h>

#include "../proto/io.grpc.pb.h"

class FilesTransferImpl final : public ::Io::FilesTransfer::Service {

    ::grpc::Status Download(::grpc::ServerContext* context, const ::Io::FileInfo* request, ::grpc::ServerWriter< ::Io::Chunk>* writer) override;

    ::grpc::Status Upload(::grpc::ServerContext* context, ::grpc::ServerReader< ::Io::Packet>* reader, ::Io::Status* response) override;

};

#endif

