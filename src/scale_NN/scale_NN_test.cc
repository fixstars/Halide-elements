#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "scale_NN_u8.h"
#include "scale_NN_u16.h"
#include "scale_NN_i16.h"

#include "test_common.h"

template<typename T>
Halide::Runtime::Buffer<T>& ref_NN(Halide::Runtime::Buffer<T>& dst, const Halide::Runtime::Buffer<T>& src,
                const int src_width, const int src_height,
                const int dst_width, const int dst_height)
{
    float scale_w = static_cast<float>(src_width) / static_cast<float>(dst_width);
    float scale_h = static_cast<float>(src_height) / static_cast<float>(dst_height);

    for (int i = 0; i < dst_height; ++i) {
        for (int j = 0; j < dst_width; ++j) {
            float src_x = (static_cast<float>(j) + 0.5f) * scale_w;
            float src_y = (static_cast<float>(i) + 0.5f) * scale_h;
            float copy = src_x;

            int src_i = static_cast<int>(src_y);
            int src_j = static_cast<int>(src_x);
            src_j = src_j < src_width ? src_j : src_width - 1;
            src_i = src_i < src_height ? src_i : src_height - 1;
            dst(j, i) = src(src_j, src_i);
        }
    }
    return dst;
}

template <typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer, struct halide_buffer_t *_dst_buffer))
{
    try {
        const int32_t in_width = 1024;
        const int32_t in_height = 768;
        const std::vector<int32_t> in_extents{in_width, in_height};

        const int32_t out_width = 500;
        const int32_t out_height = 500;
        const std::vector<int32_t> out_extents{out_width, out_height};
        auto input = mk_rand_buffer<T>(in_extents);
        auto output = mk_null_buffer<T>(out_extents);

        func(input, output); //onl NN yet
        auto expect = mk_null_buffer<T>(out_extents);

        expect = ref_NN(expect, input, in_width, in_height, out_width, out_height);

        for (int y=0; y<out_height; ++y) {
            for (int x=0; x<out_width; ++x) {
                T actual = output(x, y);
                if (abs(expect(x,y) - actual) > 0) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d",
                                                    x, y, expect(x, y), x, y, actual).c_str());
                }
            }
        }

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    printf("Success!\n");
    return 0;
}


int main()
{
#ifdef TYPE_u8
    test<uint8_t>(scale_NN_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(scale_NN_u16);
#endif
#ifdef TYPE_i16
    test<int16_t>(scale_NN_i16);
#endif
}
