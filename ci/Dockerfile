FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -y &&\
    apt-get install -y ninja-build git lsb-release software-properties-common python3-pip wget python3 build-essential &&\
    add-apt-repository -y ppa:ubuntu-toolchain-r/test &&\
    apt-get install -y gcc-11 g++-11 &&\
    wget https://github.com/Kitware/CMake/releases/download/v3.21.4/cmake-3.21.4-linux-x86_64.tar.gz &&\
    tar -xvf cmake-3.21.4-linux-x86_64.tar.gz &&\
    cp -a cmake-3.21.4-linux-x86_64/* /usr &&\
    cmake --version &&\
    bash -c "$(wget -O - https://apt.llvm.org/llvm.sh)" &&\
    ln -s /usr/bin/clang-13 /usr/bin/clang &&\
    ln -s /usr/bin/clang++-13 /usr/bin/clang++ &&\
    ln -s /usr/bin/lld-13 /usr/bin/lld &&\
    pip3 install conan &&\
    rm /usr/bin/ld &&\
    ln -s /usr/bin/ld.lld-13 /usr/bin/ld
  
