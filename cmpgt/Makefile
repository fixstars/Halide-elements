HALIDE_ROOT?=/usr/local/
HALIDE_BUILD?=${HALIDE_ROOT}

HALIDE_LIB_CMAKE:=${HALIDE_BUILD}/lib
HALIDE_LIB_MAKE:=${HALIDE_BUILD}/bin
HALIDE_LIB:=libHalide.so
BUILD_BY_CMAKE:=$(shell ls ${HALIDE_LIB_CMAKE} | grep ${HALIDE_LIB})
BUILD_BY_MAKE:=$(shell ls ${HALIDE_LIB_MAKE} | grep ${HALIDE_LIB})

ifeq (${BUILD_BY_CMAKE}, ${HALIDE_LIB})
	HALIDE_LIB_DIR=${HALIDE_LIB_CMAKE}
else ifeq (${BUILD_BY_MAKE}, ${HALIDE_LIB})
	HALIDE_LIB_DIR=${HALIDE_LIB_MAKE}
endif

CXXFLAGS:=-std=c++11 -I${HALIDE_BUILD}/include -I${HALIDE_ROOT}/tools -L${HALIDE_LIB_DIR}
LIBS:=-ldl -lpthread -lz

.PHONY: clean

all: cmpgt_test

cmpgt_gen: cmpgt_generator.cc
	g++ -fno-rtti ${CXXFLAGS} $< ${HALIDE_ROOT}/tools/GenGen.cpp -o cmpgt_gen ${LIBS} -lHalide

cmpgt.h: cmpgt_gen
	LD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./cmpgt_gen -o . -e h,static_library target=host

cmpgt_test: cmpgt_test.cc cmpgt.h cmpgt.a
	g++ -I . ${CXXFLAGS} $< -o $@ cmpgt.a -ldl -lpthread

clean:
	rm -rf cmpgt_test cmpgt_gen cmpgt.h cmpgt.a
