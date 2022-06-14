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
    bazel build --cxxopt=-std=c++20 --incompatible_enable_cc_toolchain_resolution --platforms=//toolchains:linux_musl_x64 //...
else
    bazel build --cxxopt=-std=c++20  //...
fi





