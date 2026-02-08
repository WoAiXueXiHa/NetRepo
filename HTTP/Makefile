# ===========================
# 极简 Makefile (V0.1)
# ===========================

# 1. 编译器设置
CXX = g++
# -std=c++11: 使用 C++11 标准
# -Wall: 打开所有警告 (帮你看代码有没有 bug)
# -I./include: 告诉编译器头文件在 include 目录里
CXXFLAGS = -std=c++11 -Wall -I./include

# 2. 文件路径
# 源代码文件
SRC = src/test_socket.cc
# 输出目录
BIN_DIR = bin
# 最终生成的可执行文件路径
TARGET = $(BIN_DIR)/test_socket

# 3. 伪目标 (防止目录下有同名文件导致冲突)
.PHONY: all clean

# 4. 默认目标 (直接敲 make 就会执行这个)
all: $(TARGET)

# 5. 编译规则
# $@ 代表目标文件 (bin/test_socket)
# $< 代表第一个依赖文件 (src/test_socket.cc)
$(TARGET): $(SRC)
	@mkdir -p $(BIN_DIR) 
	$(CXX) $(CXXFLAGS) $< -o $@
	@echo "✅ Build success: $@"

# 6. 清理规则 (敲 make clean 执行)
clean:
	rm -rf $(BIN_DIR)
	@echo "🧹 Cleaned up."