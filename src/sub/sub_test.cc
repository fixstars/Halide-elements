#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "sub_u8.h"
#include "sub_u16.h"
#include "sub_u32.h"

#include "test_common.h"

template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer0, struct halide_buffer_t *_src_buffer1, struct halide_buffer_t *_dst_buffer))
{
    try {
        const int width = 1024;
        const int height = 768;
        const std::vector<int32_t> extents{width, height};
        auto input0 = mk_rand_buffer<T>(extents);
        auto input1 = mk_rand_buffer<T>(extents);
        auto output = mk_null_buffer<T>(extents);

        func(input0, input1, output);
        //for each x and y
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                T actual = output(x, y);
                T expect;
                T src0 = input0(x, y);
                T src1 = input1(x, y);
                if (src0 > src1){
                    expect = src0 - src1;
                }else{
                    expect = 0;
                }

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
    test<uint8_t>(sub_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(sub_u16);
#endif
#ifdef TYPE_u32
    test<uint32_t>(sub_u32);
#endif
}
