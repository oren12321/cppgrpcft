#include "io_client.h"

#include <string>
#include <stdexcept>
#include <memory>
#include <sstream>
#include <fstream>
#include <cstdio>

#include <grpcpp/grpcpp.h>

#include <google/protobuf/any.pb.h>
#include <google/protobuf/empty.pb.h>

#include "../proto/io.grpc.pb.h"

#include "io_interfaces.h"
#include "io_defer.h"

void BytesTransferClient::Receive(const google::protobuf::Any& streamerMsg, const google::protobuf::Any& receiverMsg, BytesReceiver* receiver, ::grpc::ClientContext* context) {

    if (receiver == nullptr) {
        throw std::runtime_error("receiver is null");
    }

    try {
        receiver->init(receiverMsg);
    }
    catch(const std::exception& ex) {
        std::stringstream ss;
        ss << "failed to init receiver: " << ex.what();
        throw std::runtime_error(ss.str());
    }

    Defer defer([&receiver]() noexcept {
        receiver->finalize();
    });

    ::Io::Info info;
    info.mutable_msg()->CopyFrom(streamerMsg);
    std::unique_ptr<::grpc::ClientReader<::Io::Packet>> reader(stub_->Receive(context, info));

    ::Io::Packet packet;
    while (reader->Read(&packet)) {
        std::string data = packet.chunk().data();
        try {
            receiver->push(data, packet.info().msg());
        }
        catch(const std::exception& ex) {
            std::stringstream ss;
            ss << "failed to push data to receiver: " << ex.what();
            throw std::runtime_error(ss.str());
        }
    }

    ::grpc::Status status = reader->Finish();
    if (!status.ok()) {
        std::stringstream ss;
        ss << "client failed - code: " << status.error_code() << ", message: " << status.error_message();
        throw std::runtime_error(ss.str());
    }
}

void BytesTransferClient::Send(const google::protobuf::Any& streamerMsg, const google::protobuf::Any& receiverMsg, BytesStreamer* streamer, ::grpc::ClientContext* context) {

    if (streamer == nullptr) {
        throw std::runtime_error("streamer is null");
    }

    try {
        streamer->init(streamerMsg);
    }
    catch(const std::exception& ex) {
        std::stringstream ss;
        ss << "failed to init streamer: " << ex.what();
        throw std::runtime_error(ss.str());
    }

    Defer defer([&streamer]() noexcept {
        streamer->finalize();
    });

    google::protobuf::Empty empty;
    std::unique_ptr<::grpc::ClientWriter<::Io::Packet>> writer(stub_->Send(context, &empty));

    ::Io::Packet header;
    header.mutable_info()->mutable_msg()->CopyFrom(receiverMsg);
    if(!writer->Write(header))
    {
        writer->WritesDone();

        ::grpc::Status status = writer->Finish();
        if (!status.ok()) {
            std::stringstream ss;
            ss << "client failed - code: " << status.error_code() << ", message: " << status.error_message();
            throw std::runtime_error(ss.str());
        }
    }

    while (streamer->hasNext()) {
        try {
            std::pair<std::string, google::protobuf::Any> next = streamer->getNext();
            ::Io::Packet payload;
            payload.mutable_info()->mutable_msg()->CopyFrom(next.second);
            payload.mutable_chunk()->set_data(next.first);

            if(!writer->Write(payload)) {
                break;
            }
        }
        catch(const std::exception& ex) {
            std::stringstream ss;
            ss << "failed to read from streamer: " << ex.what();
        }
    }

    writer->WritesDone();

    ::grpc::Status status = writer->Finish();
    if (!status.ok()) {
        std::stringstream ss;
        ss << "client failed - code: " << status.error_code() << ", message: " << status.error_message();
        throw std::runtime_error(ss.str());
    }
}

