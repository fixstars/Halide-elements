HALIDE_ROOT?=/usr/local/
CXXFLAGS=-std=c++11 -I${HALIDE_ROOT}/include -L${HALIDE_ROOT}/lib
LIBS=-ldl -lpthread -lz

.PHONY: clean

all: copy_test

copy_gen: copy_generator.cc
	g++ -fno-rtti ${CXXFLAGS} $< ${HALIDE_ROOT}/tools/GenGen.cpp -o copy_gen ${LIBS} -lHalide

copy.h: copy_gen
	LD_LIBRARY_PATH=${HALIDE_ROOT}/lib ./copy_gen -o . -e h,static_library target=host 

copy_test: copy_test.cc copy.h copy.a
	g++ -I . ${CXXFLAGS} $< -o $@ -ldl -lpthread copy.a

clean:
	rm -rf copy_test copy_gen copy.h copy.a
