HALIDE_ROOT?=/usr/local/
CXXFLAGS=-std=c++11 -I${HALIDE_ROOT}/include -L${HALIDE_ROOT}/lib
LIBS=-lHalide -ldl -lpthread

.PHONY: clean

all: copy_test

copy_gen: copy_generator.cc
	g++ -fno-rtti ${CXXFLAGS} $< ${HALIDE_ROOT}/tools/GenGen.cpp -o copy_gen ${LIBS}

copy.h: copy_gen
	LD_LIBRARY_PATH=${HALIDE_ROOT}/lib /tmp/gen -o . -e h,static_library target=host 

copy_test: copy_test.cc copy.h copy.a
	g++ ${CXXFLAGS} $^ -o $@ -ldl -lpthread

clean:
	rm -rf copy_test copy_gen copy.h copy.a
