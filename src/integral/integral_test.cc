#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "integral_u8_f32.h"
#include "integral_u16_f32.h"
#include "integral_u32_f32.h"
#include "integral_u8_f64.h"
#include "integral_u16_f64.h"
#include "integral_u32_f64.h"

#include "test_common.h"

template<typename T, typename D>
int test(int (*func)(struct halide_buffer_t *_src_buffer, struct halide_buffer_t *_dst_buffer))
{
    try {
        int ret = 0;

        //
        // Run
        //
        const int width = 1024;
        const int height = 768;
        const std::vector<int32_t> extents{width, height};
        auto input = mk_rand_buffer<T>(extents);
        auto output = mk_null_buffer<D>(extents);

        D expect[height * width];
        
        // Reference impl.
        std::vector<uint64_t> line_buffer(width);
        for (int y = 0; y < height; ++y) {
            uint64_t sum = 0;
            for (int x = 0; x < width; ++x) {
                sum += input(x, y);
                line_buffer[x] += sum;
                expect[y * width + x] = static_cast<D>(line_buffer[x]);
            }
        }

        func(input, output);

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                D actual = output(x, y);
                if (expect[y * width + x] != actual) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %f, actual(%d, %d) = %f", x, y, expect[x + y * width], x, y, actual).c_str());
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
#ifdef TYPE_u8_f32
    test<uint8_t, float>(integral_u8_f32);
#endif
#ifdef TYPE_u16_f32
    test<uint16_t, float>(integral_u16_f32);
#endif
#ifdef TYPE_u32_f32
    test<uint32_t, float>(integral_u32_f32);
#endif
#ifdef TYPE_u8_f64
    test<uint8_t, double>(integral_u8_f64);
#endif
#ifdef TYPE_u16_f64
    test<uint16_t, double>(integral_u16_f64);
#endif
#ifdef TYPE_u32_f64
    test<uint32_t, double>(integral_u32_f64);
#endif
}
