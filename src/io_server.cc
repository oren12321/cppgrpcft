#include "io_server.h"

#include <grpcpp/grpcpp.h>

#include "../proto/io.grpc.pb.h"

::grpc::Status FilesTransferImpl::Download(::grpc::ServerContext* context, const ::Io::FileInfo* request, ::grpc::ServerWriter< ::Io::Chunk>* writer) {
    return ::grpc::Status::OK;
}

::grpc::Status FilesTransferImpl::Upload(::grpc::ServerContext* context, ::grpc::ServerReader< ::Io::Packet>* reader, ::Io::Status* response) {
    return ::grpc::Status::OK;
}

