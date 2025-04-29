#!/bin/bash

# 退出时如果任何命令失败
set -e

# 检查编译产物是否存在
if [ ! -f "./bin/chat_test" ]; then
  echo "错误: 找不到可执行文件 ./bin/chat_test"
  echo "请先运行 ./build_chat_test.sh 编译程序"
  exit 1
fi

# 检查API_KEY是否已设置
if [ -z "$API_KEY" ]; then
  echo "请输入API密钥:"
  read -r API_KEY
  
  if [ -z "$API_KEY" ]; then
    echo "错误: API密钥不能为空"
    exit 1
  fi
fi

# 准备音频测试文件目录
if [ ! -d "cases" ]; then
  mkdir -p cases
  echo "已创建cases目录，请确保放入测试音频文件"
fi

# 检查音频文件
if [ ! -f "cases/case_6_xiaoyanxiaoyan.pcm" ]; then
  echo "警告: 音频文件 'cases/case_6_xiaoyanxiaoyan.pcm' 不存在!"
  echo "程序可能无法正常工作"
fi

# 创建输出目录
if [ ! -d "output" ]; then
  mkdir -p output
fi

# 构建运行时容器
echo "构建运行时Docker镜像..."
docker build -t chat_test_runner -f Dockerfile.run .

# 运行程序
echo "在Docker容器中运行chat_test程序..."
docker run --rm -e API_KEY="$API_KEY" -v $(pwd)/cases:/app/cases -v $(pwd)/output:/app/output chat_test_runner

echo "程序运行完成，请检查output目录中的输出文件" 