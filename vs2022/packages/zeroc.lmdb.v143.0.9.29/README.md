# LMDB.V143

This package contains debug and release builds of the lmdb 0.9.29 static library. It was built with Visual Studio 2022 (V143).

## Source

The source code used to build this package is available at https://github.com/zeroc-ice/lmdb/tree/msvc.

## Build Instructions
```
git clone git@github.com:zeroc-ice/lmdb.git -b msvc
cd lmdb
MSBuild libraries\liblmdb\msbuild\lmdb.proj /t:NugetPack
```
