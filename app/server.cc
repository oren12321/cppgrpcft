#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "../src/io_server.h"

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    FilesTransferImpl service;

    ::grpc::EnableDefaultHealthCheckService(true);
    ::grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    ::grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());

    std::cout << "Server listening on " << server_address << '\n';
    server->Wait();
}

int main(int argc, char** argv) {
    RunServer();

    return 0;
}

