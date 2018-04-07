PROG:=label
TYPE_LIST:=u8 u16

HALIDE_ROOT?=/usr/local/
HALIDE_BUILD?=${HALIDE_ROOT}
HALIDE_TOOLS_DIR=${HALIDE_ROOT}/tools/
HALIDE_LIB_MAKE:=${HALIDE_BUILD}/bin


# ifeq ($(OS), Linux)
	HALIDE_LIB:=libHalide.so
# else
# 	HALIDE_LIB:=libHalide.dylib
# endif
BUILD_BY_MAKE:=$(shell ls ${HALIDE_LIB_MAKE} | grep ${HALIDE_LIB})
ifeq (${BUILD_BY_MAKE}, ${HALIDE_LIB})
	HALIDE_LIB_DIR=${HALIDE_LIB_MAKE}
endif

all:
	g++ -fno-rtti \
	-O2 -g -std=c++11 -I${HALIDE_BUILD}/include -I${HALIDE_ROOT}/tools \
	-L${HALIDE_LIB_MAKE} -I../../include label_generator.cc \
	${HALIDE_TOOLS_DIR}/GenGen.cpp -o label_gen -ldl -lpthread -lz -lHalide

	g++ -fno-rtti \
	-O2 -g -std=c++11 -I${HALIDE_BUILD}/include -I${HALIDE_ROOT}/tools \
	-L${HALIDE_LIB_MAKE} -I../../include second_pass_generator.cc \
	${HALIDE_TOOLS_DIR}/GenGen.cpp -o second_pass_gen -ldl -lpthread -lz -lHalide

	LD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./label_gen -o . \
	-g label_u8 -e h,static_library target=x86-64-no_asserts;
	LD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./label_gen -o . \
	-g label_u16 -e h,static_library target=x86-64-no_asserts;

	LD_LIBRARY_PATH=${HALIDE_LIB_DIR} ./second_pass_gen -o . \
	-g second_pass -e h,static_library target=x86-64-no_asserts;

	g++ -DTYPE_u8 -DTYPE_u16 -I . -O2 -g -std=c++11 \
	-I${HALIDE_BUILD}/include -I${HALIDE_ROOT}/tools \
	-L${HALIDE_LIB_DIR} -I../../include label_test.cc \
	-o label_test label_u8.a label_u16.a second_pass.a -ldl -lpthread



clean:
	rm -rf label_gen label_test label_*test_csim label_run label*.h label*.a *.o *.hls *.exec *.dSYM *.ppm *.pgm *.dat \
	second_pass_gen second_pass*.h second_pass*.a

# $label_test: $label_test.cc $(foreach type,${TYPE_LIST},$label_${type}.h ${PROG}_${type}.a)
# 	g++ $(foreach type,${TYPE_LIST},-DTYPE_${type}) -I . ${CXXFLAGS} $< -o $@ $(foreach type,${TYPE_LIST},$label_${type}.a) -ldl -lpthread
