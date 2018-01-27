#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "sub_scalar_u8.h"
#include "sub_scalar_u16.h"
#include "sub_scalar_u32.h"

#include "test_common.h"


template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer, double _value, struct halide_buffer_t *_dst_buffer))
{
    try {
        const int width = 1024;
        const int height = 768;
        const double value = mk_rand_scalar<double>();
        const std::vector<int32_t> extents{width, height};
        auto input = mk_rand_buffer<T>(extents);
        auto output = mk_null_buffer<T>(extents);

        func(input, value, output);
        const double max_value = static_cast<double>(std::numeric_limits<T>::max());

        //for each x and y
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                T actual = output(x, y);
                double diff = std::max(static_cast<double>(0.0f), static_cast<double>(input(x, y)) - value);
                T expect = round_to_nearest_even<T>(std::min(diff, max_value));

                if (expect != actual) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d",
                                                    x, y, expect, x, y, actual).c_str());
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
    test<uint8_t>(sub_scalar_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(sub_scalar_u16);
#endif
#ifdef TYPE_u32
    test<uint32_t>(sub_scalar_u32);
#endif
}
