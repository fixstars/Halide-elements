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

all: histogram2d_test

histogram2d_gen: histogram2d_generator.cc
	g++ -fno-rtti ${CXXFLAGS} $< ${HALIDE_ROOT}/tools/GenGen.cpp -o histogram2d_gen ${LIBS} -lHalide

histogram2d.h: histogram2d_gen
	LD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./histogram2d_gen -o . -e h,static_library target=host

histogram2d_test: histogram2d_test.cc histogram2d.h histogram2d.a
	g++ -I . ${CXXFLAGS} $< -o $@ histogram2d.a -ldl -lpthread

clean:
	rm -rf histogram2d_test histogram2d_gen histogram2d.h histogram2d.a
