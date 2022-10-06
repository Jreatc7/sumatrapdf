# Expat v143

This package contains the Expat C++ SDK, with DLLs, import libraries and header files.
It was built with Visual Studio 2022 (v143).

## Source Code

The source code used to build this package is available at https://github.com/zeroc-ice/libexpat.

## Build Instructions

```
git clone https://github.com/zeroc-ice/libexpat.git
git checkout <libexpat release tag>-nuget
cd libexpat
MSBuild expat\msbuild\expat.proj /t:NugetPack
```
