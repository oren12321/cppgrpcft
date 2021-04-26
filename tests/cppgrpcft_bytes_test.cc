#define _CRT_SECURE_NO_WARNINGS

#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <random>
#include <algorithm>
#include <vector>
#include <iterator>
#include <functional>
#include <iterator>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <google/protobuf/any.pb.h>

#include "../src/io_client.h"
#include "../src/io_server.h"
#include "../src/io_interfaces.h"
#include "../src/file_interfaces.h"

#include "../proto/file.grpc.pb.h"

#include "utils.h"

class CppGrpcFT_Bytes : public ::testing::Test {
protected:
    void SetUp() override {
        _from = std::string{};
        _to = std::string{};

        char buffer[1024];
        if(std::tmpnam(buffer)) {
            _from = buffer;
        }

        if(std::tmpnam(buffer)) {
            _to = buffer;
        }
    }

    void TearDown() override {
        std::remove(_from.data());
        std::remove(_to.data());
    }

    std::string _from{};
    std::string _to{};
};

TEST_F(CppGrpcFT_Bytes, BytesDownload) {

    // Make tmp file to download.

    ASSERT_FALSE(_from.empty()) << "failed to generate tmp file name";
    ASSERT_FALSE(_to.empty()) << "failed to generate tmp file name";
    
    std::ofstream ofs(_from, std::ios::out | std::ios::binary);
    ASSERT_TRUE(ofs.good()) << "failed to create tmp file " << _from;

    int size = 2048 * 1024 + 1024;
    std::vector<char> buffer = generateRandomBuffer(size);
    ofs.write(buffer.data(), buffer.size());
    ofs.close();

    // Initialize receiver

    FileReceiver receiver;

    // Start the server

    ::grpc::EnableDefaultHealthCheckService(true);
    ::grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    ::grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", ::grpc::InsecureServerCredentials());
    BytesTransfer service;
    builder.RegisterService(&service);
    
    std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());

    // Initialize streamer

    FileStreamer streamer;

    service.setStreamer(&streamer);

    // Run the client

    BytesTransferClient client(::grpc::CreateChannel("127.0.0.1:50051", ::grpc::InsecureChannelCredentials()));

    google::protobuf::Any fromAny;
    ::Interfaces::File fromMsg;
    fromMsg.set_path(_from);
    fromAny.PackFrom(fromMsg);

    google::protobuf::Any toAny;
    ::Interfaces::File toMsg;
    fromMsg.set_path(_to);
    fromAny.PackFrom(toMsg);

    try {
        ::grpc::ClientContext context;
        client.Receive(fromAny, toAny, &receiver, &context);
    } catch (const std::exception& ex) {
        FAIL() << "failed to download file: " << ex.what();
    } catch (...) {
        FAIL() << "failed to download file: unknown reason";
    }

    // Compare downloaded bytes to remote bytes
    
    ASSERT_TRUE(filesEqual(_from, _to));
}

TEST_F(CppGrpcFT_Bytes, BytesUpload) {

    // Make tmp file to download.

    ASSERT_FALSE(_from.empty()) << "failed to generate tmp file name";
    ASSERT_FALSE(_to.empty()) << "failed to generate tmp file name";
    
    std::ofstream ofs(_from, std::ios::out | std::ios::binary);
    ASSERT_TRUE(ofs.good()) << "failed to create tmp file " << _from;

    int size = 2048 * 1024 + 1024;
    std::vector<char> buffer = generateRandomBuffer(size);
    ofs.write(buffer.data(), buffer.size());
    ofs.close();

    // Initialize streamer

    FileStreamer streamer;

    // Start the server

    ::grpc::EnableDefaultHealthCheckService(true);
    ::grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    ::grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", ::grpc::InsecureServerCredentials());
    BytesTransfer service;
    builder.RegisterService(&service);
    
    std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());

    // Initialize receiver

    FileReceiver receiver;

    service.setReceiver(&receiver);

    // Run the client

    BytesTransferClient client(::grpc::CreateChannel("127.0.0.1:50051", ::grpc::InsecureChannelCredentials()));
    
    google::protobuf::Any fromAny;
    ::Interfaces::File fromMsg;
    fromMsg.set_path(_from);
    fromAny.PackFrom(fromMsg);

    google::protobuf::Any toAny;
    ::Interfaces::File toMsg;
    fromMsg.set_path(_to);
    fromAny.PackFrom(toMsg);

    try {
        ::grpc::ClientContext context;
        client.Send(fromAny, toAny, &streamer, &context);
    } catch (const std::exception& ex) {
        FAIL() << "failed to download file: " << ex.what();
    } catch (...) {
        FAIL() << "failed to download file: unknown reason";
    }

    // Compare downloaded file to remote file
    
    ASSERT_TRUE(filesEqual(_from, _to));
}

