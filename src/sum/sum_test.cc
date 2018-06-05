#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"
#include "Element/Util.h"

#include "sum_u8_f32.h"
#include "sum_u16_f32.h"
#include "sum_u32_f32.h"
#include "sum_u8_f64.h"
#include "sum_u16_f64.h"
#include "sum_u32_f64.h"
#include "sum_f32_f32.h"
#include "sum_f64_f64.h"

#include "test_common.h"

using Halide::Element::SumType;

template<typename D, typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer0,  struct halide_buffer_t *_dst_buffer))
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
        auto output = mk_null_buffer<D>({1, 1});
        D actual_total;
        typename SumType<T>::type sum(0);
        D expect_total = 0.0;
	
        func(input, output);
        actual_total = output(0, 0);

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                sum += static_cast<typename SumType<T>::type>(input(x, y));
            }
        }
        expect_total = static_cast<D>(sum);
        if (expect_total != actual_total) {
            throw std::runtime_error(format("Error: expect_total = %f, actual_total = %f", expect_total, actual_total).c_str());
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
    test<float, uint8_t>(sum_u8_f32);
#endif
#ifdef TYPE_u16_f32
    test<float, uint16_t>(sum_u16_f32);
#endif
#ifdef TYPE_u32_f32
    test<float, uint32_t>(sum_u32_f32);
#endif
#ifdef TYPE_u8_f64
    test<double, uint8_t>(sum_u8_f64);
#endif
#ifdef TYPE_u16_f64
    test<double, uint16_t>(sum_u16_f64);
#endif
#ifdef TYPE_u32_f64
    test<double, uint32_t>(sum_u32_f64);
#endif
#ifdef TYPE_f32_f32
    test<float, float>(sum_f32_f32);
#endif
#ifdef TYPE_f64_f64
    test<double, double>(sum_f64_f64);
#endif
}
