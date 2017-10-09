# NeuroDataManager

A collection of programs for spatial access to large data volumes.

## Documentation

Both a reference guide and example usage are available in the `docs` directory. Additional documentation is also available for specific features or concepts (e.g. `Coordinates`) -- check the `docs` directory after each release for the latest documentation.

## Building

The following dependencies are required:
* boost
* folly
* google-glog
* gflags
For specific tips/instructions for obtaining dependencies on different platforms, scroll past the build instructions below.

1. Create a `build` directory in the repository root.
2. Enter the `build` directory and use `cmake` to configure the build. Most users will want to run something like:
```
cmake -DCMAKE_BUILD_TYPE=release .. 
```
MacOS users may want to run something like:
```
cmake -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCMAKE_BUILD_TYPE=release .. 
```
Developers will want to enable debugging:
```
cmake -DCMAKE_BUILD_TYPE=debug .. 
```
3. Run `make` to build the binaries.
4. Assuming `make` ran successfully, binaries will be available in the `bin` directory.

#### Arch Linux

The `folly` package in AUR is typically a bit out of date. So, we recommend installing from source. The following instructions should get you started (and will also install many of the dependencies required above).

1. Install the following packages:
```
pacman -Syu \
  boost \
  gflags \
  google-glog \
  double-conversion \
  libevent \
  snappy \
  jemalloc
```
2. Download folly v2017.09.04.00 sources from [github](https://github.com/facebook/folly/archive/v2017.09.04.00.tar.gz).
3. Unpack and enter the folly build dir (`cd folly-2017.09.04.00/folly`)
4. `autoreconf -ivf`
5. `./configure LDFLAGS=-lunwind CC=gcc CXX=g++`
6. `make -j $(nproc)`
7. `sudo make install`
