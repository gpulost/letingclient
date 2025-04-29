#!/bin/bash

# 退出时如果任何命令失败
set -e

# 准备音频测试文件目录
if [ ! -d "cases" ]; then
  mkdir -p cases
  echo "已创建cases目录，请确保放入测试音频文件"
fi

# 创建输出目录
if [ ! -d "output" ]; then
  mkdir -p output
fi

# 创建bin目录用于存放编译产物
if [ ! -d "bin" ]; then
  mkdir -p bin
fi

# 构建Docker镜像 - 仅用于编译
echo "正在构建C++应用的Docker镜像..."
docker build -t chat_test_cpp_builder -f Dockerfile.cpp .

# 创建临时容器用于提取编译产物
echo "正在提取编译产物..."
CONTAINER_ID=$(docker create chat_test_cpp_builder)
docker cp $CONTAINER_ID:/app/chat_test ./bin/
docker rm $CONTAINER_ID

# 设置执行权限
chmod +x ./bin/chat_test

echo "编译完成，可执行文件位于 ./bin/chat_test"
echo "使用 ./run_chat_test.sh 运行程序，它会提示您输入API密钥"