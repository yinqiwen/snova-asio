#!/bin/sh

unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     machine=Linux;;
    Darwin*)    machine=Mac;;
    CYGWIN*)    machine=Cygwin;;
    MINGW*)     machine=MinGw;;
    *)          machine="UNKNOWN:${unameOut}"
esac

platform="linux_musl_x64"
if [ "$#" -ge 1 ]; then
    arch=$1
    case "${arch}" in
        x64)         platform="linux_musl_x64";;
        arm_eabi)    platform="linux_musl_arm_eabi";;
        arm_eabihf)  platform="linux_musl_arm_eabihf";;
        *)           platform="linux_musl_x64";; 
esac
fi

echo ${machine} ${platform}

if [ "$machine" = "Linux" ]; then
    bazel build --define local_musl=true --features=fully_static_link --cxxopt=-std=c++20 --incompatible_enable_cc_toolchain_resolution --platforms=//toolchains:"${platform}" //...
else
    if [[ $(uname -m) == 'arm64' ]]; then
        bazel build --cxxopt=-std=c++20  //...
    else
        bazel build --features=fully_static_link --cxxopt=-std=c++20  //...
    fi
fi





