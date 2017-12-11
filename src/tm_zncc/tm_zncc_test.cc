#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "tm_zncc_u8.h"
#include "tm_zncc_u16.h"
#include "tm_zncc_u32.h"

#include "test_common.h"

template<typename T>
int test(int (*func)(struct halide_buffer_t *_src0_buffer, struct halide_buffer_t *_src1_buffer, struct halide_buffer_t *_dst_buffer))
{
    try {
        int ret = 0;

        //
        // Run
        //
        const int img_width = 1024;
        const int img_height = 768;
        const int tmp_width = 16;
        const int tmp_height = 16;
        const int res_width = img_width - tmp_width + 1;
        const int res_height = img_height - tmp_height + 1;
        const std::vector<int32_t> img_extents{img_width, img_height};
        const std::vector<int32_t> tmp_extents{tmp_width, tmp_height};
        const std::vector<int32_t> res_extents{res_width, res_height};
        auto input0 = mk_rand_buffer<T>(img_extents);
        auto input1 = mk_rand_buffer<T>(tmp_extents);
        auto output = mk_null_buffer<double>(res_extents);

        if (typeid(T) == typeid(uint32_t)) {
            for (int y=0; y<img_height; ++y) {
                for (int x=0; x<img_width; ++x) {
                    input0(x, y) = static_cast<T>(input0(x, y) / 10000000);
                }
            }
            for (int y=0; y<tmp_height; ++y) {
                for (int x=0; x<tmp_width; ++x) {
                    input1(x, y) = static_cast<T>(input1(x, y) / 10000000);
                }
            }
        }

        func(input0, input1, output);

        const double tmp_size = static_cast<double>(tmp_width * tmp_height);
        for (int y=0; y<res_height; ++y) {
            for (int x=0; x<res_width; ++x) {
                double avr0 = 0.0;
                double avr1 = 0.0;
                for (int tmp_y=0; tmp_y<tmp_height; ++tmp_y) {
                    for (int tmp_x=0; tmp_x<tmp_width; ++tmp_x) {
                        avr0 += static_cast<double>(input0(x+tmp_x, y+tmp_y));
                        avr1 += static_cast<double>(input1(tmp_x, tmp_y));
                    }
                }
                avr0 = avr0 / tmp_size;
                avr1 = avr1 / tmp_size;

                double sum1 = 0.0;
                double sum2 = 0.0;
                double sum3 = 0.0;
                for (int tmp_y=0; tmp_y<tmp_height; ++tmp_y) {
                    for (int tmp_x=0; tmp_x<tmp_width; ++tmp_x) {
                        sum1 += static_cast<double>(input0(x+tmp_x, y+tmp_y)-avr0) * static_cast<double>(input1(tmp_x, tmp_y)-avr1);
                        sum2 += static_cast<double>(input0(x+tmp_x, y+tmp_y)-avr0) * static_cast<double>(input0(x+tmp_x, y+tmp_y)-avr0);
                        sum3 += static_cast<double>(input1(tmp_x, tmp_y)-avr1) * static_cast<double>(input1(tmp_x, tmp_y)-avr1);
                    }
                }
                double expect = sum1 / sqrt(sum2 * sum3);

                double actual = output(x, y);
                if (expect != actual) {
                    throw std::runtime_error(format("Error0: expect(%d, %d) = %f, actual(%d, %d) = %f", x, y, expect, x, y, actual).c_str());
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
    test<uint8_t>(tm_zncc_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(tm_zncc_u16);
#endif
#ifdef TYPE_u32
    test<uint32_t>(tm_zncc_u32);
#endif
}
