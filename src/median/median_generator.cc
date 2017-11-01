#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Median : public Halide::Generator<Median<T>> {
    Var x, y;
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{type_of<T>(), 2, "src"};
    GeneratorParam<int32_t> window_width{"window_width", 3};
    GeneratorParam<int32_t> window_height{"window_height", 3};

    Func bitonic_sort(Func input, int size) {
        input = BoundaryConditions::constant_exterior(input, input.value().type().max(),
            {{0, size}, {0, cast<int>(width)}, {0, cast<int>(height)}});
        size = pow(2, ceil(log2((double)size)));
    
        Func next, prev = input;

        Var xo{"xo"}, xi{"xi"};
        Var i{"i"};
        
        for (int pass_size = 1; pass_size < size; pass_size <<= 1) {
            for (int chunk_size = pass_size; chunk_size > 0; chunk_size >>= 1) {
                next = Func("bitonic_pass_" + std::to_string(pass_size) + "_" + std::to_string(chunk_size));
                Expr chunk_start = (i/(2*chunk_size))*(2*chunk_size);
                Expr chunk_end = (i/(2*chunk_size) + 1)*(2*chunk_size);
                Expr chunk_middle = chunk_start + chunk_size;
                Expr chunk_index = i - chunk_start;
                if (pass_size == chunk_size && pass_size > 1) {
                    // Flipped pass
                    Expr partner = 2*chunk_middle - i - 1;
                    // We need a clamp here to help out bounds inference
                    partner = clamp(partner, chunk_start, chunk_end-1);
                    next(i, x, y) = select(i < chunk_middle,
                                           min(prev(i, x, y), prev(partner, x, y)),
                                           max(prev(i, x, y), prev(partner, x, y)));


                } else {
                    // Regular pass
                    Expr partner = chunk_start + (chunk_index + chunk_size) % (chunk_size*2);
                    next(i, x, y) = select(i < chunk_middle,
                                           min(prev(i, x, y), prev(partner, x, y)),
                                           max(prev(i, x, y), prev(partner, x, y)));


                }

                schedule(next, {size, width, height});
                prev = next;
            }
        }

        return next;
    }

    Func build() {
        Expr offset_x = window_width / 2, offset_y = window_height / 2;
        int window_size = static_cast<int>(window_width) * static_cast<int>(window_height);
        
        Func clamped = BoundaryConditions::repeat_edge(src);
        RDom r(-offset_x, window_width, -offset_y, window_height);

        Func dst("dst");
        Func window("window");

        Var i{"i"};
        window(i, x, y) = cast<T>(0);
        window((r.x + offset_x) + (r.y + offset_y) * window_width, x, y) =
            clamped(x + r.x, y + r.y);
        Func sorted = bitonic_sort(window, window_size);

        dst(x,y) = sorted(window_size / 2, x, y);
        
        schedule(src, {width, height});
        schedule(window, {window_size, width, height});
        schedule(dst, {width, height});
        
        return dst;
    }
};

RegisterGenerator<Median<uint8_t>> median_u8{"median_u8"};
RegisterGenerator<Median<uint16_t>> median_u16{"median_u16"};
