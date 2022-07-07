#!/bin/sh

unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     machine=linux;;
    Darwin*)    machine=darwin;;
    CYGWIN*)    machine=windows;;
    MINGW*)     machine=windows;;
    *)          machine="UNKNOWN:${unameOut}"
esac

platform="linux_musl_x64"
if [ "$#" -ge 1 ]; then
    arch=$1
    case "${arch}" in
        amd64)       platform="linux_musl_x64";;
        arm_eabi)    platform="linux_musl_arm_eabi";;
        arm_eabihf)  platform="linux_musl_arm_eabihf";;
        aarch64)     platform="linux_musl_aarch64";;
        *)           platform="linux_musl_x64";; 
esac
fi

echo "${BUILD_VERSION}" "${machine}" "${arch}" 

if [ "$machine" = "linux" ]
then
    bazel build  --features=fully_static_link  --cxxopt=-std=c++20 --cxxopt=-DSNOVA_VERSION="${BUILD_VERSION}" --incompatible_enable_cc_toolchain_resolution --platforms=//toolchains:"${platform}" //...
elif [ "$machine" = "windows" ]
then
    export MSYS2_ARG_CONV_EXCL="*"
    bazel.exe build --features=fully_static_link --cxxopt=/O2 --enable_runfiles --cxxopt=/std:c++20  --cxxopt=/GL --linkopt=/LTCG --cxxopt=/DSNOVA_VERSION="${BUILD_VERSION}" //...
else
    if [ "$(uname -m)" = "arm64" ]; then
        bazel build --copt=-flto --linkopt=-flto --cxxopt=-std=c++20  //...
    else
        bazel build --features=fully_static_link --copt=-flto --linkopt=-flto --cxxopt=-std=c++20 --cxxopt=-DSNOVA_VERSION="${BUILD_VERSION}" //...
    fi
fi

retval=$?
if [ $retval -ne 0 ]; then
  echo "Build failed!" >&2
  exit 1
fi

tar cjf snova_asio_"${BUILD_VERSION}"_"${machine}"_"${arch}".tar.bz2 -C ./bazel-bin/snova/app/ snova





