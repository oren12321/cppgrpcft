#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <memory>

#include <grpcpp/grpcpp.h>

#include <google/protobuf/any.pb.h>

#include "../src/io_client.h"

#include "../src/io_interfaces.h"
#include "../src/file_interfaces.h"

#include "../proto/file.pb.h"

#include "args_parser.h"

int main(int argc, char** argv) {

    std::unordered_map<std::string, std::string> mapped_args = parseArgs(argc, argv);

    std::string server_address("127.0.0.1:50051");
    if (mapped_args.find("--address") != mapped_args.end()) {
        server_address = mapped_args["--address"];
    }

    std::string from("<unknown>");
    if (mapped_args.find("--from") != mapped_args.end()) {
        from = mapped_args["--from"];
    }

    std::string to("<unknown>");
    if (mapped_args.find("--to") != mapped_args.end()) {
        to = mapped_args["--to"];
    }

    std::string method("<unknown>");
    if (mapped_args.find("--method") != mapped_args.end()) {
        method = mapped_args["--method"];
    }

    try {
        BytesTransferClient client(::grpc::CreateChannel(server_address, ::grpc::InsecureChannelCredentials()));

        if (method == "<unknown>" || from == "<unknown>" || to == "<unknown>") {
            std::cout << "missing command line argument(s)\n";
            std::cout << "usage: cppgrpcft-client --method=<download|upload> --from=<path> --to=<path> [--address=<ip:port>]\n";
            return 1;
        }

        if (method == "download") {
            google::protobuf::Any fromAny;
            ::Interfaces::File fromMsg;
            fromMsg.set_path(from);
            fromAny.PackFrom(fromMsg);

            google::protobuf::Any toAny;
            ::Interfaces::File toMsg;
            toMsg.set_path(to);
            toAny.PackFrom(toMsg);

            std::unique_ptr<BytesReceiver> receiver(new FileReceiver);

            std::cout << "downloading file from " << server_address << ":" << from << " to " << to << '\n';
            ::grpc::ClientContext context;
            client.Receive(fromAny, toAny, receiver.get(), &context);
        }
        else if (method == "upload") {
            google::protobuf::Any fromAny;
            ::Interfaces::File fromMsg;
            fromMsg.set_path(from);
            fromAny.PackFrom(fromMsg);

            google::protobuf::Any toAny;
            ::Interfaces::File toMsg;
            toMsg.set_path(to);
            toAny.PackFrom(toMsg);

            std::unique_ptr<BytesStreamer> streamer(new FileStreamer);

            std::cout << "uploading file from " << from << " to " << server_address << ":" << to << '\n';
            ::grpc::ClientContext context;
            client.Send(fromAny, toAny, streamer.get(), &context);
        }
        else {
            std::cout << "unknown --method\n";
            return 1;
        }
    } catch (const std::exception& ex) {
        std::cout << "client failed: " << ex.what() << '\n';
        return 1;
    } catch (...) {
        std::cout << "client failed: unkwnown exception\n";
        return 1;
    }

    return 0;
}

