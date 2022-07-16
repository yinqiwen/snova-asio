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

echo "${BUILD_VERSION}" "${machine}" "${arch}" 

if [ "$machine" = "linux" ]
then
    # shellcheck disable=SC2086
    bazel --bazelrc=bazelrc.linux build  ${gcc_opts} --stripopt=--strip-all --cxxopt=-DSNOVA_VERSION="${BUILD_VERSION}" --platforms=//toolchains:"${platform}" //snova/app:snova.stripped
    mv ./bazel-bin/snova/app/snova.stripped ./bazel-bin/snova/app/snova
elif [ "$machine" = "windows" ]
then
    vcpkg install boringssl:x64-windows-static
    export MSYS2_ARG_CONV_EXCL="*"
    bazel.exe --bazelrc=bazelrc.windows build --cxxopt=/DSNOVA_VERSION="${BUILD_VERSION}"  //...
else
    if [ "$(uname -m)" = "arm64" ]; then
        bazel --bazelrc=bazelrc.macos build --cxxopt=-DSNOVA_VERSION="${BUILD_VERSION}" //...
    else
        bazel --bazelrc=bazelrc.macos build --features=fully_static_link  --cxxopt=-DSNOVA_VERSION="${BUILD_VERSION}" //...
    fi
fi

retval=$?
if [ $retval -ne 0 ]; then
  echo "Build failed!" >&2
  exit 1
fi

tar -cjf snova_asio_"${BUILD_VERSION}"_"${machine}"_"${arch}".tar.bz2 -C ./bazel-bin/snova/app/ snova
# ls -l ./bazel-bin/snova/app/snova
# ls -l snova_asio_"${BUILD_VERSION}"_"${machine}"_"${arch}".tar.bz2





