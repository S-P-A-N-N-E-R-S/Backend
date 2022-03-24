FROM ubuntu:20.04

# Silence interactive choices for region selection
ENV DEBIAN_FRONTEND=noninteractive

# Install all dependencies
RUN apt-get -yq update && \
    apt-get -y install g++ make cmake libboost-all-dev libasio-dev libpq-dev clang-format autoconf automake libtool wget postgresql-client libssl-dev

# Install pqxx manually
WORKDIR /tmp
RUN wget https://github.com/jtv/libpqxx/archive/refs/tags/7.5.2.tar.gz
RUN tar xf 7.5.2.tar.gz
WORKDIR libpqxx-7.5.2
RUN ./configure --enable-shared
RUN make -j$(nproc)
RUN make install
WORKDIR /tmp
RUN rm -rf libpqxx-7.5.2

# Install protobuf 3.16 manually
WORKDIR /tmp
RUN wget https://github.com/protocolbuffers/protobuf/releases/download/v3.16.0/protobuf-cpp-3.16.0.tar.gz
RUN tar xf protobuf-cpp-3.16.0.tar.gz
WORKDIR protobuf-3.16.0
RUN ./autogen.sh
RUN ./configure
RUN make -j$(nproc)
RUN make install
RUN ldconfig
WORKDIR /tmp
RUN rm -rf protobuf-3.16.0

# Copy backend
COPY . /opt/backend
WORKDIR /opt/backend/build

# Configure server
ENV SPANNERS_SERVER_PORT=4711

# Compile and run server with TLS encryption at container start up
EXPOSE 4711
CMD cmake .. && make -j$(nproc) && ./apps/server