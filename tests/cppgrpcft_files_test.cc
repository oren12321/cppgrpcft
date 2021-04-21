#define _CRT_SECURE_NO_WARNINGS

#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <random>
#include <algorithm>
#include <vector>
#include <iterator>
#include <functional>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "../src/io_client.h"
#include "../src/io_server.h"

#include "utils.h"

class CppGrpcFT_File : public ::testing::Test {
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


TEST_F(CppGrpcFT_File, FileDownload) {

    // Make tmp file to download.

    ASSERT_FALSE(_from.empty()) << "failed to generate tmp file name";
    ASSERT_FALSE(_to.empty()) << "failed to generate tmp file name";
    
    std::ofstream ofs(_from, std::ios::out | std::ios::binary);
    ASSERT_TRUE(ofs.good()) << "failed to create tmp file " << _from;

    int size = 2048 * 1024 + 1024;
    std::vector<char> buffer = generateRandomBuffer(size);
    ofs.write(buffer.data(), buffer.size());
    ofs.close();

    // Start the server

    ::grpc::EnableDefaultHealthCheckService(true);
    ::grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    ::grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", ::grpc::InsecureServerCredentials());
    FilesTransferImpl service;
    builder.RegisterService(&service);
    
    std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());

    // Run the client

    FilesTransferClient client(::grpc::CreateChannel("127.0.0.1:50051", ::grpc::InsecureChannelCredentials()));
    
    try {
        ::grpc::ClientContext context;
        client.Download(_from, _to, &context);
    } catch (const std::exception& ex) {
        FAIL() << "failed to download file: " << ex.what();
    } catch (...) {
        FAIL() << "failed to download file: unknown reason";
    }

    // Compare downloaded file to remote file
    
    ASSERT_TRUE(filesEqual(_from, _to));

}

TEST_F(CppGrpcFT_File, FileUpload) {

    // Make tmp file to download.

    ASSERT_FALSE(_from.empty()) << "failed to generate tmp file name";
    ASSERT_FALSE(_to.empty()) << "failed to generate tmp file name";
    
    std::ofstream ofs(_from, std::ios::out | std::ios::binary);
    ASSERT_TRUE(ofs.good()) << "failed to create tmp file " << _from;

    int size = 2048 * 1024 + 1024;
    std::vector<char> buffer = generateRandomBuffer(size);
    ofs.write(buffer.data(), buffer.size());
    ofs.close();

    // Start the server

    ::grpc::EnableDefaultHealthCheckService(true);
    ::grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    ::grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", ::grpc::InsecureServerCredentials());
    FilesTransferImpl service;
    builder.RegisterService(&service);
    
    std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());

    // Run the client

    FilesTransferClient client(::grpc::CreateChannel("127.0.0.1:50051", ::grpc::InsecureChannelCredentials()));
    
    try {
        ::grpc::ClientContext context;
        client.Upload(_from, _to, &context);
    } catch (const std::exception& ex) {
        FAIL() << "failed to download file: " << ex.what();
    } catch (...) {
        FAIL() << "failed to download file: unknown reason";
    }

    // Compare downloaded file to remote file
    
    ASSERT_TRUE(filesEqual(_from, _to));
}

