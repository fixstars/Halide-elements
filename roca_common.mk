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

CXXFLAGS:=-std=c++11 -I${HALIDE_BUILD}/include -I${HALIDE_ROOT}/tools -L${HALIDE_LIB_DIR} -I../../include
LIBS:=-ldl -lpthread -lz

.PHONY: clean

all: ${PROG}_test

${PROG}_gen: ${PROG}_generator.cc
	g++ -fno-rtti ${CXXFLAGS} $< ${HALIDE_ROOT}/tools/GenGen.cpp -o ${PROG}_gen ${LIBS} -lHalide

${PROG}_gen.exec: ${PROG}_gen
	$(foreach type,${TYPE_LIST},LD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./${PROG}_gen -o . -g ${PROG}_${type} -e h,static_library target=host;)
	@touch ${PROG}_gen.exec

$(foreach type,${TYPE_LIST},${PROG}_${type}.a): ${PROG}_gen.exec

$(foreach type,${TYPE_LIST},${PROG}_${type}.h): ${PROG}_gen.exec

${PROG}_test: ${PROG}_test.cc $(foreach type,${TYPE_LIST},${PROG}_${type}.h ${PROG}_${type}.a)
	g++ -I . ${CXXFLAGS} $< -o $@ $(foreach type,${TYPE_LIST},${PROG}_${type}.a) -ldl -lpthread

clean:
	rm -rf ${PROG}_test ${PROG}_gen ${PROG}_gen.exec $(foreach type,${TYPE_LIST},${PROG}_${type}.h ${PROG}_${type}.a)
