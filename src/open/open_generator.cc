#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Open : public Halide::Generator<Open<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};
    
    ImageParam src{type_of<T>(), 2, "src"};
    ImageParam structure{UInt(8), 2, "structure"};

    Func build() {
        Func erode = conv_with_structure(src, [](RDom r, Expr e){return minimum_unroll(r, e);}, type_of<T>().max(),
            structure, width, height, window_width, window_height, iteration);
        Func dilate = conv_with_structure(erode, [](RDom r, Expr e){return maximum_unroll(r, e);}, type_of<T>().min(),
            structure, width, height, window_width, window_height, iteration);

        schedule(src, {width, height});
        schedule(structure, {window_width, window_height});
        
        return dilate;
    }
};

RegisterGenerator<Open<uint8_t>> open_u8{"open_u8"};
RegisterGenerator<Open<uint16_t>> open_u16{"open_u16"};
