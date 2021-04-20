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

#include "../src/io_client.h"
#include "../src/io_server.h"

std::vector<char> generateRandomBuffer(int size) {
  std::random_device r;
  std::seed_seq seed{};
  std::mt19937 eng(seed);

  std::uniform_int_distribution<char> dist;
  std::vector<char> v(size);

  std::generate(v.begin(), v.end(), std::bind(dist, eng));
  return v;
}

template <typename InputIter>
bool rangeEqual(InputIter begin1, InputIter end1, InputIter begin2, InputIter end2) {
    while (begin1 != end1 && begin2 != end2) {
        if (*begin1 != *begin2) return false;
        ++begin1;
        ++begin2;
    }
    return (begin1 == end1) && (begin2 == end2);
}

bool filesEqual(const std::string& path1, const std::string& path2) {
    std::ifstream ifs1(path1);
    std::ifstream ifs2(path2);

    std::istreambuf_iterator<char> begin1(ifs1);
    std::istreambuf_iterator<char> begin2(ifs2);

    std::istreambuf_iterator<char> end;

    return rangeEqual(begin1, end, begin2, end);
}

class CppGrpcFT : public ::testing::Test {
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

        if (_server != nullptr) _server->Shutdown();
    }

    std::string _from{};
    std::string _to{};
    std::unique_ptr<::grpc::Server> _server = nullptr;
};


TEST_F(CppGrpcFT, Download) {

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
        client.Download(_from, _to);
    } catch (const std::exception& ex) {
        FAIL() << "failed to download file: " << ex.what();
    } catch (...) {
        FAIL() << "failed to download file: unknown reason";
    }

    // Compare downloaded file to remote file
    
    ASSERT_TRUE(filesEqual(_from, _to));

}

TEST_F(CppGrpcFT, Upload) {

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
        client.Upload(_from, _to);
    } catch (const std::exception& ex) {
        FAIL() << "failed to download file: " << ex.what();
    } catch (...) {
        FAIL() << "failed to download file: unknown reason";
    }

    // Compare downloaded file to remote file
    
    ASSERT_TRUE(filesEqual(_from, _to));
}

