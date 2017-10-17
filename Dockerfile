FROM ndm:deps

RUN mkdir -p /usr/local/src/DataManager
WORKDIR /usr/local/src/DataManager
COPY . /usr/local/src/DataManager/
RUN mkdir -p build

WORKDIR /usr/local/src/DataManager/build
RUN LD_LIBRARY_PATH=/usr/local/src/folly/follylib/lib:$LD_LIBRARY_PATH cmake \
   -DCMAKE_BUILD_TYPE=release \
   -DENABLE_TESTS=off \
   -DGTEST_ROOT=/usr/src/gtest \
   -DUSE_STATIC_LIBS=on ..  
RUN make -j $(nproc)

RUN mkdir -p /opt/DataManager
RUN cp -R bin/* /opt/DataManager/

WORKDIR /opt/DataManager

# Remove dependencies and build
RUN rm -r /usr/local/src/DataManager

RUN apt-get remove -y libboost-all-dev
RUN apt-get remove -y libgoogle-glog-dev
RUN apt-get remove -y libgflags-dev

RUN rm -r /usr/local/src/folly
RUN apt-get remove -y libunwind8-dev

CMD ["/opt/DataManager/ndm", "-help"]
