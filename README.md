# DataManager

A collection of programs for spatial access to large data volumes.

## Building

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