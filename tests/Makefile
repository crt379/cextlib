# 定义编译器
CC = clang

# 编译选项
CFLAGS = -g -Wall

# 编译文件后缀
TARGET_SUFFIX = 

# 清理命令
RM = rm -f

ifeq ($(OS),Windows_NT)
TARGET_SUFFIX = .exe
RM = del
endif

# 默认目标为构建并运行测试
all: run

build: chashmap cstring cvec

run: build run_chashmap run_cstring run_cvec
	
chashmap:
	$(CC) $(CFLAGS) -o test_chashmap$(TARGET_SUFFIX) test_chashmap.c ../chashmap.c

run_chashmap: chashmap
	./test_chashmap$(TARGET_SUFFIX)

cstring:
	$(CC) $(CFLAGS) -o test_cstring$(TARGET_SUFFIX) test_cstring.c ../cstring.c

run_cstring: cstring
	./test_cstring$(TARGET_SUFFIX)

cvec:
	$(CC) $(CFLAGS) -o test_cvec$(TARGET_SUFFIX) test_cvec.c ../cvec.c

run_cvec: cvec
	./test_cvec$(TARGET_SUFFIX)

clean:
	$(RM) *.exe *.o *.ilk *.pdb