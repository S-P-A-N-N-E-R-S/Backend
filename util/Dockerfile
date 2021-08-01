FROM ubuntu:20.04

# Silence interactive choices for region selection
ENV DEBIAN_FRONTEND=noninteractive

# Install all dependencies
RUN apt-get -yq update && \
    apt-get -y install g++ make cmake libboost-all-dev libasio-dev libpq-dev clang-format autoconf automake libtool wget

# Install pqxx manually
RUN wget https://github.com/jtv/libpqxx/archive/refs/tags/7.5.2.tar.gz
RUN tar xf 7.5.2.tar.gz
WORKDIR libpqxx-7.5.2
RUN ./configure --enable-shared
RUN make -j8
RUN make install
WORKDIR /app
RUN rm -rf libpqxx-7.5.2

# Install protobuf 3.16 manually
WORKDIR /app
RUN wget https://github.com/protocolbuffers/protobuf/releases/download/v3.16.0/protobuf-cpp-3.16.0.tar.gz
RUN tar xf protobuf-cpp-3.16.0.tar.gz
WORKDIR protobuf-3.16.0
RUN ./autogen.sh
RUN ./configure
RUN make -j8
RUN make install
RUN ldconfig
WORKDIR /app
RUN rm -rf protobuf-3.16.0
