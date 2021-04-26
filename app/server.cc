#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <memory>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "../src/io_server.h"

#include "../src/io_interfaces.h"
#include "../src/file_interfaces.h"

#include "args_parser.h"

int main(int argc, char** argv) {

    std::unordered_map<std::string, std::string> mapped_args = parseArgs(argc, argv);

    std::string server_address("0.0.0.0:50051");
    if (mapped_args.find("--address") != mapped_args.end()) {
        server_address = mapped_args["--address"];
    }

    BytesTransfer service;
    std::unique_ptr<BytesStreamer> streamer(new FileStreamer);
    std::unique_ptr<BytesReceiver> receiver(new FileReceiver);
    service.setStreamer(streamer.get());
    service.setReceiver(receiver.get());

    ::grpc::EnableDefaultHealthCheckService(true);
    ::grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    ::grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());

    std::cout << "Server listening on " << server_address << '\n';
    server->Wait();

    return 0;
}

