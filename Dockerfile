FROM ndm:deps

RUN mkdir -p /usr/local/src/DataManager
WORKDIR /usr/local/src/DataManager
COPY . /usr/local/src/DataManager/
RUN mkdir -p build

WORKDIR /usr/local/src/DataManager/build
# Note that tests do not yet build statically
RUN LD_LIBRARY_PATH=/usr/local/src/folly/follylib/lib:$LD_LIBRARY_PATH cmake \
   -DCMAKE_BUILD_TYPE=release \
   -DENABLE_TESTS=off \
   -DGTEST_ROOT=/usr/src/gtest \ 
   -DUSE_STATIC_LIBS=on ..  
RUN make -j $(nproc)


FROM ubuntu:16.04 

RUN apt-get update
RUN apt-get upgrade -y

# Even compiled statically, NDM still needs a few libraries
RUN apt-get install -y \ 
    libjbig-dev \
    libjpeg-dev 

RUN mkdir -p /opt/DataManager
WORKDIR /opt/DataManager
COPY --from=0 /usr/local/src/DataManager/build/bin/ndm .

ENTRYPOINT ["/opt/DataManager/ndm"]
CMD ["-help"]
