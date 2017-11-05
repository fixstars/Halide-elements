#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Cmpge : public Halide::Generator<Cmpge<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src0{type_of<T>(), 2, "src0"}, src1{type_of<T>(), 2, "src1"};

    Var x, y;

    Func build() {

        Func dst("dst");
        Expr srcval0 = src0(x, y), srcval1 = src1(x, y);
        Expr dstval = cast<T>(select(srcval0 >= srcval1, type_of<T>().max(), 0));;
        dst(x, y) = dstval;

        schedule(src0, {width, height});
        schedule(src1, {width, height});
        schedule(dst, {width, height});
        
        return dst;
    }
};

RegisterGenerator<Cmpge<uint8_t>> cmpge_u8{"cmpge_u8"};
RegisterGenerator<Cmpge<uint16_t>> cmpge_u16{"cmpge_u16"};
RegisterGenerator<Cmpge<uint32_t>> cmpge_u32{"cmpge_u32"};
