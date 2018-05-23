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
	g++ -fno-rtti ${CXXFLAGS} $< ${HALIDE_TOOLS_DIR}/GenGen.cpp -o ${PROG}_gen ${LIBS} -lHalide

${PROG}_gen.exec: ${PROG}_gen
ifdef TYPE_LIST
ifeq ($(OS), Linux)
	$(foreach type,${TYPE_LIST},LD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./$< -o . -g ${PROG}_${type} -e h,static_library target=x86-64-no_asserts;)
else
	$(foreach type,${TYPE_LIST},DYLD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./$< -o . -g ${PROG}_${type} -e h,static_library target=x86-64-no_asserts;)
endif
else
ifeq ($(OS), Linux)
	LD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./$< -o . -g ${PROG} -e h,static_library target=host-no_asserts
else
	DYLD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./$< -o .  -g ${PROG} -e h,static_library target=host-no_asserts
endif
endif
	@touch ${PROG}_gen.exec

ifdef TYPE_LIST
$(foreach type,${TYPE_LIST},${PROG}_${type}.a): ${PROG}_gen.exec

$(foreach type,${TYPE_LIST},${PROG}_${type}.h): ${PROG}_gen.exec

${PROG}_test: ${PROG}_test.cc $(foreach type,${TYPE_LIST},${PROG}_${type}.h ${PROG}_${type}.a)
	g++ $(foreach type,${TYPE_LIST},-DTYPE_${type}) -I . ${CXXFLAGS} $< -o $@ $(foreach type,${TYPE_LIST},${PROG}_${type}.a) -ldl -lpthread
else
${PROG}.a: ${PROG}_gen.exec

${PROG}.h: ${PROG}_gen.exec

${PROG}_test: ${PROG}_test.cc ${PROG}.h ${PROG}.a
	g++ -I . ${CXXFLAGS} $< -o $@ ${PROG}.a -ldl -lpthread
endif

test: ${PROG}_test
	./${PROG}_test

${PROG}_gen.hls: ${PROG}_generator.cc
	g++ -D HALIDE_FOR_FPGA -fno-rtti ${CXXFLAGS} $< ${HALIDE_TOOLS_DIR}/GenGen.cpp -o ${PROG}_gen.hls ${LIBS} -lHalide

ifdef TYPE_LIST
$(foreach type,${TYPE_LIST},${PROG}_${type}.hls): ${PROG}_gen.hls
ifeq ($(OS), Linux)
	$(foreach type,${TYPE_LIST},LD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./$< -o . -g ${PROG}_${type} -e hls target=fpga-64-vivado_hls;)
else
	$(foreach type,${TYPE_LIST},DYLD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./$< -o . -g ${PROG}_${type} -e hls target=fpga-64-vivado_hls;)
endif
else
${PROG}.hls: ${PROG}_gen.hls
ifeq ($(OS), Linux)
	LD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./$< -o .  -g ${PROG} -e hls target=fpga-64-vivado_hls
else
	DYLD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./$< -o .  -g ${PROG} -e hls target=fpga-64-vivado_hls
endif
endif

ifdef TYPE_LIST
define hls_template
${PROG}_$(1).hls.exec: ${PROG}_$(1).hls
	cd ${PROG}_$(1).hls; make
	@touch ${PROG}_$(1).hls.exec

${PROG}_$(1)_csim.o: ${PROG}_$(1).hls
	g++ -I . -I ${VIVADO_HLS_ROOT}/include ${CSIM_CXXFLAGS} -std=c++03 ${PROG}_$(1).hls/${PROG}_$(1).cc -c -o ${PROG}_$(1)_csim.o
${PROG}_$(1)_test_csim: ${PROG}_test.cc ${PROG}_$(1)_csim.o ${PROG}_$(1).h
	g++ -DTYPE_$(1) -I . -I ${VIVADO_HLS_ROOT}/include ${CSIM_CXXFLAGS} $$< ${PROG}_$(1)_csim.o -o $$@ -ldl -lpthread
endef
$(foreach type,${TYPE_LIST},$(eval $(call hls_template,${type})))

test_csim: $(foreach type,${TYPE_LIST},${PROG}_${type}_test_csim)
	$(foreach type,${TYPE_LIST},./${PROG}_${type}_test_csim;)
else
${PROG}.hls.exec: ${PROG}.hls
	cd ${PROG}.hls; make
	@touch ${PROG}.hls.exec

${PROG}_csim.o: ${PROG}.hls
	g++ -I . -I ${VIVADO_HLS_ROOT}/include ${CSIM_CXXFLAGS} -std=c++03 ${PROG}.hls/${PROG}.cc -c -o $@

${PROG}_test_csim: ${PROG}_test.cc ${PROG}_csim.o ${PROG}.h
	g++ -I . -I ${VIVADO_HLS_ROOT}/include ${CSIM_CXXFLAGS} $< ${PROG}_csim.o -o $@ -ldl -lpthread

test_csim: ${PROG}_test_csim
	./${PROG}_test_csim
endif

run: ${PROG}_run.c ${PROG}.hls.exec
	arm-linux-gnueabihf-gcc ${CFLAGS} ${TARGET_SRC} -o $@ ${TARGET_LIB}

clean:
	rm -rf ${PROG}_gen ${PROG}_test ${PROG}_*test_csim ${PROG}_run ${PROG}*.h ${PROG}*.a *.o *.hls *.exec *.dSYM *.ppm *.pgm *.dat
