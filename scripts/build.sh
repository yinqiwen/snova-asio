#!/bin/sh

unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     machine=Linux;;
    Darwin*)    machine=Mac;;
    CYGWIN*)    machine=Cygwin;;
    MINGW*)     machine=MinGw;;
    *)          machine="UNKNOWN:${unameOut}"
esac
echo ${machine}


if [ "$machine" = "Linux" ]; then
    bazel build --define local_musl=true --features=fully_static_link --cxxopt=-std=c++20 --incompatible_enable_cc_toolchain_resolution --platforms=//toolchains:linux_musl_x64 //...
else
    if [[ $(uname -m) == 'arm64' ]]; then
        bazel build --cxxopt=-std=c++20  //...
    else
        bazel build --features=fully_static_link --cxxopt=-std=c++20  //...
    fi
fi





