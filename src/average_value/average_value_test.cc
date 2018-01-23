#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include "average_value_u8_f32.h"
#include "average_value_u16_f32.h"
#include "average_value_u8_f64.h"
#include "average_value_u16_f64.h"

#include "HalideRuntime.h"
#include "HalideBuffer.h"
#include "test_common.h"

using std::string;
using std::vector;

template <typename S, typename D>
int test(int (*func)(struct halide_buffer_t *_src_buffer,
                    struct halide_buffer_t *_roi_buffer,
                    struct halide_buffer_t *_dst_buffer))
{
    try{
        const int32_t width = 1024;
        const int32_t height = 768;
        const std::vector<int32_t> extents{width, height};
        auto input = mk_rand_buffer<S>(extents);
        auto roi = mk_rand_buffer<uint8_t>(extents);
        auto output = mk_null_buffer<D>({1});

        func(input, roi, output);
        //reference
        D expect;
        double sum = 0;
        int count = 0;
        int els = 0;
        for (int j = 0; j < width; j++) {
            for (int i = 0; i < height; i++) {
                if (roi(j, i) != 0) {
                    sum += input(j, i);
                    count++;
                }
            }
        }
        expect = static_cast<D>(sum/count);

        D actual = output(0);
        if (expect != actual){
            throw std::runtime_error(format("Error: expect = %f, actual = %f\n",
                                            expect, actual));
        }
    }catch (const std::exception& e){
        std::cerr << e.what() << std::endl;
        return 1;
    }

    printf("Success!\n");
    return 0;
}

int main(){
    #ifdef TYPE_u8_f32
        test<uint8_t, float>(average_value_u8_f32);
    #endif
    #ifdef TYPE_u16_f32
        test<uint16_t, float>(average_value_u16_f32);
    #endif
    #ifdef TYPE_u8_f64
        test<uint8_t, double>(average_value_u8_f64);
    #endif
    #ifdef TYPE_u16_f64
        test<uint16_t, double>(average_value_u16_f64);
    #endif
}
