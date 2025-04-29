# C++ 聊天测试工具

## 项目介绍
这是一个基于C++的聊天测试工具，用于测试语音识别和处理能力。该工具会处理`cases`目录中的音频文件，并将结果输出到`output`目录。

## 环境要求
- Docker（用于构建和运行）
- Bash shell
- 有效的API密钥

## 使用步骤

### 1. 编译程序
```bash
sh build_chat_test.sh
```
此脚本将：
- 创建必要的目录结构（cases、output、bin）
- 构建Docker镜像用于编译C++应用
- 提取编译产物到本地bin目录

### 2. 运行测试
```bash
API_KEY=<your_api_key> sh run_chat_test.sh
```
如果不提供API_KEY环境变量，脚本会提示手动输入。

此脚本将：
- 检查编译产物是否存在
- 验证API密钥是否已设置
- 检查音频测试文件
- 构建运行时Docker容器
- 在Docker容器中运行测试程序
- 结果将保存在output目录中

### 3. 测试音频文件
请确保在`cases`目录中放入所需的测试音频文件，特别是`case_6_xiaoyanxiaoyan.pcm`。

### 4. 查看结果
测试完成后，可以在`output`目录中查看输出结果。  