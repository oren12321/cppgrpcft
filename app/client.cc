#include <iostream>
#include <stdexcept>

#include <grpcpp/grpcpp.h>

#include "../src/io_client.h"

int main(int argc, char** argv) {

    try {
        FilesTransferClient client(::grpc::CreateChannel("127.0.0.1:50051", ::grpc::InsecureChannelCredentials()));
        client.Download("/tmp/remote/info.txt", "/tmp/downloads/info.txt");
        //client.Upload("", "");
    } catch (const std::exception& ex) {
        std::cout << "client failed: " << ex.what() << '\n';
        return 1;
    } catch (...) {
        std::cout << "client failed: unkwnown exception\n";
        return 1;
    }

    return 0;
}

