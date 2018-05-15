PROG:=label
ANOTHER:=second_pass
TYPE_LIST:=u8 u16

# Platform dependents
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	OS = Linux
else ifeq ($(UNAME_S),Darwin)
	OS = Mac
else
	$(error Unknown platform)
endif

HALIDE_ROOT?=/usr/local/
HALIDE_BUILD?=${HALIDE_ROOT}

HALIDE_TOOLS_DIR=${HALIDE_ROOT}/tools/
HALIDE_LIB_CMAKE:=${HALIDE_BUILD}/lib
HALIDE_LIB_MAKE:=${HALIDE_BUILD}/bin
ifeq ($(OS), Linux)
	HALIDE_LIB:=libHalide.so
else
	HALIDE_LIB:=libHalide.dylib
endif
BUILD_BY_CMAKE:=$(shell ls ${HALIDE_LIB_CMAKE} | grep ${HALIDE_LIB})
BUILD_BY_MAKE:=$(shell ls ${HALIDE_LIB_MAKE} | grep ${HALIDE_LIB})

VIVADO_HLS_ROOT?=/opt/Xilinx/Vivado_HLS/2017.2/
DRIVER_ROOT=./${PROG}.hls/${PROG}_zynq.sdk/design_1_wrapper_hw_platform_0/drivers/${PROG}_hp_wrapper_v1_0/src/
TARGET_SRC=${PROG}_run.c ${DRIVER_ROOT}/x${PROG}_hp_wrapper.c ${DRIVER_ROOT}/x${PROG}_hp_wrapper_linux.c
TARGET_LIB=-lm
CFLAGS=-std=c99 -D_GNU_SOURCE -O2 -mcpu=cortex-a9 -I${DRIVER_ROOT} -I../../include

ifeq (${BUILD_BY_CMAKE}, ${HALIDE_LIB})
	HALIDE_LIB_DIR=${HALIDE_LIB_CMAKE}
else ifeq (${BUILD_BY_MAKE}, ${HALIDE_LIB})
	HALIDE_LIB_DIR=${HALIDE_LIB_MAKE}
endif

CXXFLAGS:=-O2 -g -std=c++11 -I${HALIDE_BUILD}/include -I${HALIDE_ROOT}/tools -L${HALIDE_LIB_DIR} -I../../include
CSIM_CXXFLAGS:=-O2 -g -std=c++11 -I${HALIDE_BUILD}/include -I${HALIDE_ROOT}/tools -L${HALIDE_LIB_DIR} -I../../include
LIBS:=-ldl -lpthread -lz

.PHONY: clean

all: ${PROG}_test

${PROG}_gen: ${PROG}_generator.cc
	g++ -fno-rtti ${CXXFLAGS} $< ${HALIDE_TOOLS_DIR}/GenGen.cpp -o ${PROG}_gen ${LIBS} -lHalide;

${ANOTHER}_gen: ${ANOTHER}_generator.cc
	g++ -fno-rtti ${CXXFLAGS} $< ${HALIDE_TOOLS_DIR}/GenGen.cpp -o ${ANOTHER}_gen ${LIBS} -lHalide

${PROG}_gen.exec: ${PROG}_gen
	LD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./${PROG}_gen -o . \
	-g label_u8 -e h,static_library target=x86-64-no_asserts;
	LD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./${PROG}_gen -o . \
	-g label_u16 -e h,static_library target=x86-64-no_asserts;

${ANOTHER}_gen.exec: ${ANOTHER}_gen
	LD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./${ANOTHER}_gen -o . \
	-g second_pass -e h,static_library target=x86-64-no_asserts;

$(foreach type,${TYPE_LIST},${PROG}_${type}.a): ${PROG}_gen.exec
$(foreach type,${TYPE_LIST},${PROG}_${type}.h): ${PROG}_gen.exec
${ANOTHER}.a: ${ANOTHER}_gen.exec
${ANOTHER}.h: ${ANOTHER}_gen.exec

${PROG}_test: ${PROG}_test.cc ${ANOTHER}.h ${ANOTHER}.a $(foreach type,${TYPE_LIST},${PROG}_${type}.h ${PROG}_${type}.a)
	g++ $(foreach type,${TYPE_LIST},-DTYPE_${type}) \
	-I . ${CXXFLAGS} $< -o $@ $(foreach type,${TYPE_LIST},${PROG}_${type}.a) \
	${ANOTHER}.a -ldl -lpthread

test: ${PROG}_test
	./${PROG}_test

${PROG}_gen.hls: ${PROG}_generator.cc ${ANOTHER}_generator.cc
	g++ -D HALIDE_FOR_FPGA -fno-rtti ${CXXFLAGS} $< ${HALIDE_TOOLS_DIR}/GenGen.cpp -o ${PROG}_gen.hls ${LIBS} -lHalide
	g++ -D HALIDE_FOR_FPGA -fno-rtti ${CXXFLAGS} $< ${HALIDE_TOOLS_DIR}/GenGen.cpp -o ${PROG}_gen.hls ${ANOTHER} -lHalide

test_csim:
	@echo $(shell echo 'The command is not supported'; exit 1)
	@exit $(.SHELLSTATUS)

run: ${PROG}_run.c ${PROG}.hls.exec
	arm-linux-gnueabihf-gcc ${CFLAGS} ${TARGET_SRC} -o $@ ${TARGET_LIB}

clean:
	rm -rf ${PROG}_gen ${PROG}_test ${PROG}_*test_csim ${PROG}_run ${PROG}*.h ${PROG}*.a *.o *.hls *.exec *.dSYM *.ppm *.pgm *.dat \
${ANOTHER}_gen ${ANOTHER}_test ${ANOTHER}_*test_csim ${ANOTHER}_run ${ANOTHER}*.h ${ANOTHER}*.a
