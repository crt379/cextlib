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

all: run

build: chashmap

run: build run_chashmap
	
chashmap:
	$(CC) $(CFLAGS) -o benchmark_chashmap$(TARGET_SUFFIX) benchmark_chashmap.c ../chashmap.c

run_chashmap: chashmap
	./benchmark_chashmap$(TARGET_SUFFIX)

perf_chashmap: chashmap
	perf record -g ./benchmark_chashmap$(TARGET_SUFFIX) -o perf.data
	perf script -i perf.data &> perf.unfold
	stackcollapse-perf.pl perf.unfold &> perf.folded
	flamegraph.pl perf.folded > perf.svg

valgrind_chashmap: chashmap
	#  valgrind --tool=memcheck --leak-check=full ./benchmark_chashmap$(TARGET_SUFFIX)
	valgrind --tool=massif --massif-out-file=massif.out.0 ./benchmark_chashmap$(TARGET_SUFFIX)
	ms_print massif.out.0
	
clean:
	$(RM) *.exe *.o *.ilk *.pdb

clean_perf:
	$(RM) *.data *.unfold *.folded *.svg *.old
	
clean_valgrind:
	$(RM) massif.out.0