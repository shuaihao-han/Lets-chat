#!/bin/bash

set -e  # 遇到错误立即退出
set -x  # 打印执行过程

# 定义基础目录
BASE_DIR=$(pwd)

# 清理 build 和 bin
rm -rf "${BASE_DIR}/build"/*
rm -rf "${BASE_DIR}/bin"/*

# 创建 build 目录
mkdir -p "${BASE_DIR}/build"

# 编译主程序
cd "${BASE_DIR}/build" &&
cmake .. &&
make