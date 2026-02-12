CC=g++  
CXXFLAGS = -std=c++17 -Wno-vla-cxx-extension
CFLAGS=-I

# 编译原版跳表
main: main.o 
	$(CC) -o ./bin/main main.o --std=c++11 -pthread 
	rm -f ./*.o

# 编译优化版测试程序
test_optimized: test_optimized.o
	$(CC) -o ./bin/test_optimized test_optimized.o --std=c++17 -pthread
	rm -f ./*.o

# 编译MVCC测试程序
test_mvcc: test_mvcc.o
	$(CC) -o ./bin/test_mvcc test_mvcc.o --std=c++11 -pthread
	rm -f ./*.o

# 编译所有
all: main test_optimized test_mvcc

clean: 
	rm -f ./*.o
	rm -f ./bin/main ./bin/test_optimized ./bin/test_mvcc
