#ifndef __IOINTERFACES_H__
#define __IOINTERFACES_H__

#include <string>

#include <google/protobuf/any.pb.h>

class BytesStreamer {
public:
    virtual void init(const google::protobuf::Any& any) = 0;
    virtual bool hasNext() = 0;
    virtual std::pair<std::string, google::protobuf::Any> getNext() = 0;
    virtual void finalize() noexcept = 0;
};

class BytesReceiver {
public:
    virtual void init(const google::protobuf::Any& any) = 0;
    virtual void push(std::string data, const google::protobuf::Any& any) = 0;
    virtual void finalize() noexcept = 0;
};

#endif

