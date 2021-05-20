cmake_minimum_required(VERSION 3.19)

project(FilesTransfer CXX)

find_package(Threads REQUIRED)

find_package(Protobuf REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")
find_package(gRPC REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(hw_proto_srcs_dir ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(protos
    ${hw_proto_srcs_dir}/io.proto
    ${hw_proto_srcs_dir}/file.proto)

add_custom_command(
    OUTPUT ${hw_proto_srcs_dir}/io.pb.cc
           ${hw_proto_srcs_dir}/io.pb.h
           ${hw_proto_srcs_dir}/file.pb.cc
           ${hw_proto_srcs_dir}/file.pb.h
    COMMAND protoc
    ARGS -I ${hw_proto_srcs_dir} --cpp_out=${hw_proto_srcs_dir} ${protos}
    WORKING_DIRECTORY ${hw_proto_srcs_dir}
    COMMENT "Generating C++ protobuf sources")

add_custom_command(
    OUTPUT ${hw_proto_srcs_dir}/io.grpc.pb.cc
           ${hw_proto_srcs_dir}/io.grpc.pb.h
           ${hw_proto_srcs_dir}/file.grpc.pb.cc
           ${hw_proto_srcs_dir}/file.grpc.pb.h
    COMMAND protoc
    ARGS -I ${hw_proto_srcs_dir} --grpc_out=${hw_proto_srcs_dir} --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ${protos}
    WORKING_DIRECTORY ${hw_proto_srcs_dir}
    COMMENT "Generating C++ gRPC sources")

set(hw_proto_srcs
    ${hw_proto_srcs_dir}/io.pb.cc
    ${hw_proto_srcs_dir}/file.pb.cc)
set(hw_grpc_srcs
    ${hw_proto_srcs_dir}/io.grpc.pb.cc
    ${hw_proto_srcs_dir}/file.grpc.pb.cc)

add_library(cppgrpcft)
target_sources(cppgrpcft
    PRIVATE
        src/io_server.cc
        src/io_client.cc
        ${hw_proto_srcs}
        ${hw_grpc_srcs}
    PUBLIC
        src/io_server.h
        src/io_client.h
        src/io_interfaces.h
        src/file_interfaces.h)
target_link_libraries(cppgrpcft
    gRPC::grpc++)

install(TARGETS cppgrpcft DESTINATION lib)
install(
    FILES 
        src/io_server.h
        src/io_client.h
        src/io_interfaces.h
        src/file_interfaces.h
    DESTINATION include/cppgrpcft)

add_executable(cppgrpcft-client
    app/client.cc)
target_link_libraries(cppgrpcft-client
    gRPC::grpc++
    ${CMAKE_BINARY_DIR}/libcppgrpcft.a)

add_executable(cppgrpcft-server
    app/server.cc)
target_link_libraries(cppgrpcft-server
    gRPC::grpc++
    gRPC::grpc++_reflection
    ${CMAKE_BINARY_DIR}/libcppgrpcft.a)

install(
    TARGETS
        cppgrpcft-server
        cppgrpcft-client
    DESTINATION
        bin)

find_package(GTest REQUIRED)

add_executable(cppgrpcft-tests
    tests/main.cc
    tests/cppgrpcft_bytes_test.cc
    tests/cppgrpcft_metadata_test.cc
    tests/utils.cc)
target_link_libraries(cppgrpcft-tests
    gRPC::grpc++
    gRPC::grpc++_reflection
    GTest::gtest
    GTest::gtest_main
    ${CMAKE_BINARY_DIR}/libcppgrpcft.a)

install(TARGETS cppgrpcft-tests DESTINATION bin)

add_dependencies(cppgrpcft-client cppgrpcft)
add_dependencies(cppgrpcft-server cppgrpcft)
add_dependencies(cppgrpcft-tests cppgrpcft)

enable_testing()
add_test(NAME CppGrpcFtTest COMMAND cppgrpcft-tests)

