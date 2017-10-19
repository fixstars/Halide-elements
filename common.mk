HALIDE_ROOT?=/usr/local/
HALIDE_BUILD?=${HALIDE_ROOT}

HALIDE_LIB_CMAKE:=${HALIDE_BUILD}/lib
HALIDE_LIB_MAKE:=${HALIDE_BUILD}/bin
HALIDE_LIB:=libHalide.so
BUILD_BY_CMAKE:=$(shell ls ${HALIDE_LIB_CMAKE} | grep ${HALIDE_LIB})
BUILD_BY_MAKE:=$(shell ls ${HALIDE_LIB_MAKE} | grep ${HALIDE_LIB})

DRIVER_ROOT=./${PROG}.hls/${PROG}_zynq.sdk/design_1_wrapper_hw_platform_0/drivers/${PROG}_hp_wrapper_v1_0/src/
TARGET_SRC=${PROG}_run.c ${DRIVER_ROOT}/x${PROG}_hp_wrapper.c ${DRIVER_ROOT}/x${PROG}_hp_wrapper_linux.c
TARGET_LIB=-lm
CFLAGS=-std=c99 -D_GNU_SOURCE -O2 -mcpu=cortex-a9 -I${DRIVER_ROOT} -I../../include

ifeq (${BUILD_BY_CMAKE}, ${HALIDE_LIB})
	HALIDE_LIB_DIR=${HALIDE_LIB_CMAKE}
	HALIDE_TOOLS_DIR=${HALIDE_ROOT}/../tools/
else ifeq (${BUILD_BY_MAKE}, ${HALIDE_LIB})
	HALIDE_LIB_DIR=${HALIDE_LIB_MAKE}
	HALIDE_TOOLS_DIR=${HALIDE_ROOT}/tools/
endif

CXXFLAGS:=-O0 -g -std=c++11 -I${HALIDE_BUILD}/include -I${HALIDE_ROOT}/tools -L${HALIDE_LIB_DIR} -I../../include
LIBS:=-ldl -lpthread -lz

.PHONY: clean

all: ${PROG}_test

${PROG}_gen: ${PROG}_generator.cc
	g++ -fno-rtti ${CXXFLAGS} $< ${HALIDE_TOOLS_DIR}/GenGen.cpp -o ${PROG}_gen ${LIBS} -lHalide

${PROG}_gen.exec: ${PROG}_gen
ifdef TYPE_LIST
	$(foreach type,${TYPE_LIST},LD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./$< -o . -g ${PROG}_${type} -e h,static_library target=host;)
else
	LD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./$< -o . -e h,static_library target=host
endif
	@touch ${PROG}_gen.exec

ifdef TYPE_LIST
$(foreach type,${TYPE_LIST},${PROG}_${type}.a): ${PROG}_gen.exec

$(foreach type,${TYPE_LIST},${PROG}_${type}.h): ${PROG}_gen.exec

${PROG}_test: ${PROG}_test.cc $(foreach type,${TYPE_LIST},${PROG}_${type}.h ${PROG}_${type}.a)
	g++ -I . ${CXXFLAGS} $< -o $@ $(foreach type,${TYPE_LIST},${PROG}_${type}.a) -ldl -lpthread
else
${PROG}.a: ${PROG}_gen.exec

${PROG}.h: ${PROG}_gen.exec

${PROG}_test: ${PROG}_test.cc ${PROG}.h ${PROG}.a
	g++ -I . ${CXXFLAGS} $< -o $@ ${PROG}.a -ldl -lpthread
endif

${PROG}_gen.hls: ${PROG}_generator.cc
	g++ -D HALIDE_FOR_FPGA -fno-rtti ${CXXFLAGS} $< ${HALIDE_TOOLS_DIR}/GenGen.cpp -o ${PROG}_gen.hls ${LIBS} -lHalide

${PROG}.hls: ${PROG}_gen.hls
	LD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./$< -o . -e hls target=fpga-64-vivado_hls

${PROG}.hls.exec: ${PROG}.hls
	cd ${PROG}.hls; make 
	@touch ${PROG}.hls.exec

${PROG}_run: ${PROG}_run.c ${PROG}.hls.exec
	arm-linux-gnueabihf-gcc ${CFLAGS} ${TARGET_SRC} -o $@ ${TARGET_LIB}

clean:
	rm -rf ${PROG}_gen ${PROG}_test ${PROG}_run ${PROG}.h ${PROG}.a *.hls *.exec
