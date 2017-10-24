PROG:=affine
include ../../common.mk

${PROG}_test2: ${PROG}_test2.cc ${PROG}.h ${PROG}.a
	g++ -I . ${CXXFLAGS} $< -o $@ ${PROG}.a -ldl -lpthread -ljpeg -lpng

clean2:
	rm -rf ${PROG}_test2
