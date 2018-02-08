#include <iostream>
#include <limits>
#include "Halide.h"

#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Scale : public Halide::Generator<Scale<T>> {
    ImageParam src{type_of<T>(), 2, "src"};
    //Param<uint8_t> interpolation{"interpolation", 1};

    GeneratorParam<int32_t> in_width{"in_width", 1024};
    GeneratorParam<int32_t> in_height{"in_height", 768};

    GeneratorParam<int32_t> out_width{"out_width", 7};
    GeneratorParam<int32_t> out_height{"out_height", 7};

public:
    Func build() {
        Func dst{"dst"};
        dst = Element::scale_bicubic<T>(src, in_width, in_height,
                                    out_width, out_height
                                    );
        // dst = Element::scale_NN<T>(src, in_width, in_height,
        //                             out_width, out_height
        //                             );

        schedule(src, {in_width, in_height});
        schedule(dst, {out_width, out_height});
        return dst;
    }
};

RegisterGenerator<Scale<uint8_t>> scale_u8{"scale_u8"};
RegisterGenerator<Scale<uint16_t>> scale_u16{"scale_u16"};
RegisterGenerator<Scale<int16_t>> scale_i16{"scale_i16"};
