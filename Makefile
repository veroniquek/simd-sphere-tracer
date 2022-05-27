UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
    CC=g++-10
else
   CC=g++
endif

CFLAGS=-O3 -lm -flto -march=skylake -Wall
DEBUGFLAGS=-g -lm -march=skylake
AVXFLAGS=-march=skylake	# todo not sure what to put here

OPTIM_SRCS = src/geometry.c src/obj_vec_spheretrace.c src/ray_vec_spheretrace.c  src/spheretrace.c
REF_SRCS = reference/spheretrace.cpp

all: ref test spheretrace

# Reference implementation
ref: $(REF_SRCS)
	$(CC) $(CFLAGS) $? -o $@

# Validation
test: $(OPTIM_SRCS) auxiliary/load_scene.cpp run_test.cpp src/benchmark.c
	$(CC) $(CFLAGS) $? -o $@

test-debug: $(OPTIM_SRCS) auxiliary/load_scene.cpp run_test.cpp src/benchmark.c
	$(CC) $(DEBUGFLAGS) $? -o $@

benchmark: benchmark-executable spheretrace spheretrace-only-scene-load

# run the benchmark
benchmark-executable: $(OPTIM_SRCS) auxiliary/load_scene.cpp src/benchmark.c run_benchmark.cpp
	$(CC) $(CFLAGS) $? -o benchmark

spheretrace: $(OPTIM_SRCS)  auxiliary/load_scene.cpp main.cpp src/benchmark.c
	$(CC) $(CFLAGS) $? -o $@

spheretrace-stats: $(OPTIM_SRCS)  auxiliary/load_scene.cpp main.cpp src/benchmark.c
	$(CC) $(CFLAGS) -D COLLECT_STATS $? -o $@

spheretrace-only-scene-load: $(OPTIM_SRCS)  auxiliary/load_scene.cpp main.cpp
	$(CC) $(CFLAGS) -D ONLY_SCENE_LOAD $? -o $@


vec-test:  $(OPTIM_SRCS) test_vectorization.c
	$(CC) $(DEBUGFLAGS) $(AVXFLAGS) $? -o $@

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ test test-debug ref spheretrace spheretrace-only-scene-load benchmark vec-test *.ppm *.csv
