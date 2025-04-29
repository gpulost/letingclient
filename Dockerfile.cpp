FROM ubuntu:22.04

# 避免交互式提示
ENV DEBIAN_FRONTEND=noninteractive

# 配置清华镜像源
RUN sed -i 's/archive.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list && \
    sed -i 's/security.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list

# 安装基本工具和编译环境
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    libssl-dev \
    libjsoncpp-dev \
    wget \
    libasio-dev \
    && rm -rf /var/lib/apt/lists/*

# 安装websocketpp (从源码编译)
RUN cd /tmp && \
    git clone https://github.com/zaphoyd/websocketpp.git && \
    cd websocketpp && \
    mkdir build && cd build && \
    cmake .. && \
    make install

# 设置工作目录
WORKDIR /app

# 复制修复版本的源代码
COPY chat_test.cpp chat_test.cpp

# 创建输出目录
RUN mkdir -p output

# 编译应用程序 - 添加所有必要的头文件路径和库
RUN g++ -std=c++17 chat_test.cpp -o chat_test \
    -I/usr/include/jsoncpp \
    -I/usr/local/include \
    -lssl -lcrypto -lboost_system -pthread -ljsoncpp

# 设置入口点
ENTRYPOINT ["./chat_test"] 