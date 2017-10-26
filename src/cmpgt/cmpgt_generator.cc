#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Cmpgt : public Halide::Generator<Cmpgt<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src0{type_of<T>(), 2, "src0"}, src1{type_of<T>(), 2, "src1"};

    Var x, y;

    Func build() {

        Func dst("dst");
        Expr srcval0 = src0(x, y), srcval1 = src1(x, y);
        Expr dstval = cast<T>(select(srcval0 > srcval1, type_of<T>().max(), 0));
        dst(x, y) = dstval;

        schedule(src0, {width, height});
        schedule(src1, {width, height});
        schedule(dst, {width, height});
        
        return dst;
    }
};

RegisterGenerator<Cmpgt<uint8_t>> cmpgt_u8{"cmpgt_u8"};
RegisterGenerator<Cmpgt<uint16_t>> cmpgt_u16{"cmpgt_u16"};
RegisterGenerator<Cmpgt<uint32_t>> cmpgt_u32{"cmpgt_u32"};
