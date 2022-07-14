#!/bin/sh

unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     machine=Linux;;
    Darwin*)    machine=Mac;;
    CYGWIN*)    machine=Cygwin;;
    MINGW*)     machine=Windows;;
    *)          machine="UNKNOWN:${unameOut}"
esac

platform="linux_musl_x64"
gcc_opts="--copt=-O2"
if [ "$#" -ge 1 ]; then
    arch=$1
    case "${arch}" in
        x64)         
          platform="linux_musl_x64"
          ;;
        arm_eabi)    
          platform="linux_musl_arm_eabi"
          gcc_opts="--copt=-Os --copt=-Wno-stringop-overflow --per_file_copt=.*/curve25519.c@-DOPENSSL_NO_ASM"
          ;;
        arm_eabihf)  
          platform="linux_musl_arm_eabihf"
          gcc_opts="--copt=-Os --copt=-Wno-stringop-overflow"
          ;;
        aarch64)     
          platform="linux_musl_aarch64"
          ;;
        *)           
          platform="linux_musl_x64"
          ;; 
esac
fi

echo "${machine}" "${platform}" "${gcc_opts}"

if [ "$machine" = "Linux" ]; then
    # shellcheck disable=SC2086
    bazel --bazelrc=bazelrc.linux build  --define local_musl=true ${gcc_opts} --platforms=//toolchains:"${platform}" //...
    #bazel --bazelrc=bazelrc.linux build --define local_musl=true  --stripopt=--strip-all --platforms=//toolchains:"${platform}" //snova/app:snova.stripped
elif [ "$machine" = "Windows" ]; then
    bazel --bazelrc=bazelrc.windows build  //...
else
    if [ "$(uname -m)" = "arm64" ]; then
        bazel --bazelrc=bazelrc.macos build   //...
    else
        bazel --bazelrc=bazelrc.macos build --features=fully_static_link //...
    fi
fi





