FROM amd64/ubuntu:18.04

WORKDIR /tmp/
RUN apt-get update && apt-get install -y wget \
 && rm -rf /var/lib/apt/lists/* \
 && wget -q -O cmake-linux.sh https://github.com/Kitware/CMake/releases/download/v3.19.6/cmake-3.19.6-Linux-x86_64.sh \
 && sh cmake-linux.sh -- --skip-license --prefix=/usr/local \
 && rm -rf /tmp/*

WORKDIR /tmp/
RUN apt-get update && apt-get install -y \
    git \
    build-essential \
    autoconf \
    libtool \
    pkg-config \
 && rm -rf /var/lib/apt/lists/* \
 && git clone --recurse-submodules -b v1.35.0 https://github.com/grpc/grpc \
 && cd grpc \
 && mkdir -p cmake/build \
 && cd cmake/build \
 && cmake -DgRPC_INSTALL=ON \
          -DgRPC_BUILD_TESTS=OFF \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          ../.. \
 && make -j$(nproc) \
 && make install \
 && cd - && rm -rf /tmp/*
