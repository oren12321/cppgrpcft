#define _CRT_SECURE_NO_WARNINGS

#include <gtest/gtest.h>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <google/protobuf/any.pb.h>
#include <google/protobuf/empty.pb.h>
#include <google/protobuf/wrappers.pb.h>

#include "../src/io_client.h"
#include "../src/io_server.h"
#include "../src/io_interfaces.h"

class DummyStreamer : public BytesStreamer {
public:
    void init(const google::protobuf::Any& any) override {
    }

    bool hasNext() override {
        static bool flag = true;
        if (flag) {
            flag = false;
            return true;
        }
        return false;
    }

    std::pair<std::string, google::protobuf::Any> getNext() override {
        google::protobuf::StringValue sv;
        sv.set_value("info about dummy data");
        google::protobuf::Any any;
        any.PackFrom(sv);

        return std::make_pair(std::string("dummy data"), any);
    }

    void finalize() noexcept override {
    }
};

class DummyReceiver : public BytesReceiver {
public:
    void init(const google::protobuf::Any& any) override {
    }

    void push(std::string data, const google::protobuf::Any& any) override {
        google::protobuf::StringValue sv;
        if (!any.UnpackTo(&sv)) {
            throw std::runtime_error("failed to unpack 'Any' to 'StringValue'");
        }

        if (data != "dummy data") {
            throw std::runtime_error("received wrong data");
        }

        if (sv.value() != "info about dummy data") {
            throw std::runtime_error("received wrong data info");
        }
    }

    void finalize() noexcept override {
    }
};

TEST(CppGrpcFT_BytesWithInfo, BytesDownload) {

    // Initialize receiver

    DummyReceiver receiver;

    // Start the server

    ::grpc::EnableDefaultHealthCheckService(true);
    ::grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    ::grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", ::grpc::InsecureServerCredentials());
    BytesTransfer service;
    builder.RegisterService(&service);
    
    std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());

    // Initialize streamer

    DummyStreamer streamer;

    service.setStreamer(&streamer);

    // Run the client

    BytesTransferClient client(::grpc::CreateChannel("127.0.0.1:50051", ::grpc::InsecureChannelCredentials()));

    google::protobuf::Any fromAny;
    google::protobuf::Empty fromMsg;
    fromAny.PackFrom(fromMsg);

    google::protobuf::Any toAny;
    google::protobuf::Empty toMsg;
    toAny.PackFrom(toMsg);

    try {
        ::grpc::ClientContext context;
        client.Receive(fromAny, toAny, &receiver, &context);
    } catch (const std::exception& ex) {
        FAIL() << "failed to download: " << ex.what();
    } catch (...) {
        FAIL() << "failed to download: unknown reason";
    }
}

TEST(CppGrpcFT_BytesWithInfo, BytesUpload) {

    // Initialize streamer

    DummyStreamer streamer;

    // Start the server

    ::grpc::EnableDefaultHealthCheckService(true);
    ::grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    ::grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", ::grpc::InsecureServerCredentials());
    BytesTransfer service;
    builder.RegisterService(&service);
    
    std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());

    // Initialize receiver

    DummyReceiver receiver;

    service.setReceiver(&receiver);

    // Run the client

    BytesTransferClient client(::grpc::CreateChannel("127.0.0.1:50051", ::grpc::InsecureChannelCredentials()));
    
    google::protobuf::Any fromAny;
    google::protobuf::Empty fromMsg;
    fromAny.PackFrom(fromMsg);

    google::protobuf::Any toAny;
    google::protobuf::Empty toMsg;
    toAny.PackFrom(toMsg);

    try {
        ::grpc::ClientContext context;
        client.Send(fromAny, toAny, &streamer, &context);
    } catch (const std::exception& ex) {
        FAIL() << "failed to upload: " << ex.what();
    } catch (...) {
        FAIL() << "failed to upload: unknown reason";
    }
}

