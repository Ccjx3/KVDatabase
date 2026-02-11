CC=g++  
CXXFLAGS = -std=c++17 -Wno-vla-cxx-extension
CFLAGS=-I

# 编译原版跳表
skiplist: main.o 
	$(CC) -o ./bin/main main.o --std=c++11 -pthread 
	rm -f ./*.o

# 编译优化版测试程序
test_optimized: test_optimized.o
	$(CC) -o ./bin/test_optimized test_optimized.o --std=c++17 -pthread
	rm -f ./*.o

# 编译所有
all: skiplist test_optimized

clean: 
	rm -f ./*.o
	rm -f ./bin/main ./bin/test_optimized
