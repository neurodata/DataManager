FROM ubuntu:16.04

RUN apt-get update
RUN apt-get upgrade -y

# Misc utils
RUN apt-get install -y build-essential
RUN apt-get install -y git \
    cmake \
    wget \
    vim

# basic libraries
RUN apt-get install -y libgoogle-glog-dev \
    libgflags-dev

# libtiff
RUN apt-get install -y libtiff5 \
    libtiff5-dev

# Boost
RUN apt-get install -y libboost-all-dev

# Folly
RUN apt-get install -y \
    g++ \
    automake \
    autoconf \
    autoconf-archive \
    libtool \
    libboost-all-dev \
    libevent-dev \
    libdouble-conversion-dev \
    libgoogle-glog-dev \
    libgflags-dev \
    liblz4-dev \
    liblzma-dev \
    libsnappy-dev \
    make \
    zlib1g-dev \
    binutils-dev \
    libjemalloc-dev \
    libssl-dev \
    pkg-config \
    libiberty-dev

# Folly advanced debugging
RUN apt-get install -y \
    libunwind8-dev \
    libelf-dev \
    libdwarf-dev

# Install folly
RUN mkdir -p /usr/local/src/folly/follylib
WORKDIR /usr/local/src/folly 
RUN wget --continue https://github.com/facebook/folly/archive/v2017.09.04.00.tar.gz
RUN tar xvf v2017.09.04.00.tar.gz
WORKDIR /usr/local/src/folly/folly-2017.09.04.00/folly
RUN /usr/bin/autoreconf -ivf
RUN ./configure --prefix=/usr/local/src/folly/follylib
RUN make -j $(nproc)
RUN make install

# Tests (not reqd)
RUN apt-get install -y google-mock \
    libgtest-dev

# Note that libgtest-dev only includes source files. We still need to build the library.
WORKDIR /usr/src/gtest
RUN cmake .
RUN make

WORKDIR /usr/local/src