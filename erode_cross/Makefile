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

all: erode_cross_test

erode_cross_gen: erode_cross_generator.cc
	g++ -fno-rtti ${CXXFLAGS} $< ${HALIDE_ROOT}/tools/GenGen.cpp -o erode_cross_gen ${LIBS} -lHalide

erode_cross.h: erode_cross_gen
	LD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./erode_cross_gen -o . -e h,static_library target=host

erode_cross_test: erode_cross_test.cc erode_cross.h erode_cross.a
	g++ -I . ${CXXFLAGS} $< -o $@ erode_cross.a -ldl -lpthread

clean:
	rm -rf erode_cross_test erode_cross_gen erode_cross.h erode_cross.a
