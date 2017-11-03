#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "div_scalar_u8.h"
#include "div_scalar_u16.h"
#include "div_scalar_u32.h"

#include "test_common.h"

template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer, double _value, struct halide_buffer_t *_dst_buffer))
{
    try {
        int ret = 0;

        //
        // Run
        //
        const int width = 1024;
        const int height = 768;
        const double value = 2.0;
        const std::vector<int32_t> extents{width, height};
        auto input = mk_rand_buffer<T>(extents);
        auto output = mk_null_buffer<T>(extents);

        func(input, value, output);

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                T expect = input(x, y);
                T actual = output(x, y);
                double f = expect / value;
                f = std::min(static_cast<double>(std::numeric_limits<T>::max()), f);
                f = std::max(static_cast<double>(0.0f), f);
                expect = round_to_nearest_even<T>(f);

                // HLS backend の C-simulation と LLVM backend で丸めの方法が異なるため、1以内の誤差を許している
                // (C-simulation は round half away from zero だが、LLVM 版は round half to even)
                if (abs(expect - actual) > 1) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d (f = %f)", x, y, expect, x, y, actual, f).c_str());
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
    test<uint8_t>(div_scalar_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(div_scalar_u16);
#endif
#ifdef TYPE_u32
    test<uint32_t>(div_scalar_u32);
#endif
}
