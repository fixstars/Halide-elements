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

all: erode_rect_test

erode_rect_gen: erode_rect_generator.cc
	g++ -fno-rtti ${CXXFLAGS} $< ${HALIDE_ROOT}/tools/GenGen.cpp -o erode_rect_gen ${LIBS} -lHalide

erode_rect.h: erode_rect_gen
	LD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./erode_rect_gen -o . -e h,static_library target=host

erode_rect_test: erode_rect_test.cc erode_rect.h erode_rect.a
	g++ -I . ${CXXFLAGS} $< -o $@ erode_rect.a -ldl -lpthread

clean:
	rm -rf erode_rect_test erode_rect_gen erode_rect.h erode_rect.a
