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
#include "../src/io_interfaces.h"

#include "utils.h"

class SimpleBufferStreamer : public BytesStreamer {
public:
    SimpleBufferStreamer(std::vector<char> buffer) {
        _buffer = buffer;
    }

    void init(std::string msg) override {
        _index = 0;
    }

    bool hasNext() override {
        return _index < _buffer.size();
    }

    std::string getNext() {
        if (_index + 1234 - 1 < _buffer.size()) {
            std::string data(_buffer.data() + _index, 1234);
            _index += 1234;
            return data;
        }
        int reminder = _buffer.size() - _index;
        std::string data(_buffer.data() + _index, reminder);
        _index += reminder;
        return data;
    }

    void finalize() override { }

private:
    std::vector<char> _buffer;
    int _index;
};

class SimpleBufferReceiver : public BytesReceiver {
public:
    void init(std::string msg) override {
        _buffer.clear();
    }

    void push(std::string data) override {
        std::vector<char> tail(data.begin(), data.end());
        _buffer.insert(_buffer.end(), tail.begin(), tail.end());
    }

    void finalize() override { }

    std::vector<char> buffer() { return _buffer; }

private:
    std::vector<char> _buffer;
};

TEST(CppGrpcFT_Bytes, BytesDownload) {

    // Initialize receiver

    SimpleBufferReceiver receiver;

    // Start the server

    ::grpc::EnableDefaultHealthCheckService(true);
    ::grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    ::grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", ::grpc::InsecureServerCredentials());
    BytesTransferImpl service;
    builder.RegisterService(&service);
    
    std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());

    // Initialize streamer

    int size = 2048 * 1024 + 1024;
    std::vector<char> buffer = generateRandomBuffer(size);

    SimpleBufferStreamer streamer(buffer);

    service.setStreamer(&streamer);

    // Run the client

    BytesTransferClient client(::grpc::CreateChannel("127.0.0.1:50051", ::grpc::InsecureChannelCredentials()));
    
    try {
        ::grpc::ClientContext context;
        client.Download("", "", &receiver, &context);
    } catch (const std::exception& ex) {
        FAIL() << "failed to download file: " << ex.what();
    } catch (...) {
        FAIL() << "failed to download file: unknown reason";
    }

    // Compare downloaded bytes to remote bytes
    
    ASSERT_TRUE(buffer == receiver.buffer());

}

TEST(CppGrpcFT_Bytes, BytesUpload) {

    // Initialize streamer

    int size = 2048 * 1024 + 1024;
    std::vector<char> buffer = generateRandomBuffer(size);

    SimpleBufferStreamer streamer(buffer);

    // Start the server

    ::grpc::EnableDefaultHealthCheckService(true);
    ::grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    ::grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", ::grpc::InsecureServerCredentials());
    BytesTransferImpl service;
    builder.RegisterService(&service);
    
    std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());

    // Initialize receiver

    SimpleBufferReceiver receiver;

    service.setReceiver(&receiver);

    // Run the client

    BytesTransferClient client(::grpc::CreateChannel("127.0.0.1:50051", ::grpc::InsecureChannelCredentials()));
    
    try {
        ::grpc::ClientContext context;
        client.Upload("", "", &streamer, &context);
    } catch (const std::exception& ex) {
        FAIL() << "failed to download file: " << ex.what();
    } catch (...) {
        FAIL() << "failed to download file: unknown reason";
    }

    // Compare downloaded file to remote file
    
    ASSERT_TRUE(buffer == receiver.buffer());
}

