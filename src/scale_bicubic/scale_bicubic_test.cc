#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "scale_bicubic_u8.h"
#include "scale_bicubic_u16.h"
#include "scale_bicubic_i16.h"

#include "test_common.h"

#define BORDER_INTERPOLATE(x, l) (x < 0 ? 0 : (x >= l ? l - 1 : x))

float weight(float input){
    float alpha = -1;
    float x = (input < 0)? -input : input;
    float x2 = x * x;
    float x3 = x * x * x;

    if(x <= 1){
        return (alpha + 2) * x3 - (alpha + 3) * x2 + 1;
    }else if(x < 2){
        return alpha * x3 - 5 * alpha * x2 + 8 * alpha * x - 4 * alpha;
    }else{
        return 0x0;
    }
}

template<typename T>
Halide::Runtime::Buffer<T>& ref_bicubic(Halide::Runtime::Buffer<T>& dst,
                                        const Halide::Runtime::Buffer<T>& src,
                                        const int32_t src_width, const int32_t src_height,
                                        const uint32_t dst_width, const uint32_t dst_height)
{
    double min_value = static_cast<double>(std::numeric_limits<T>::min());
    double max_value = static_cast<double>(std::numeric_limits<T>::max());
    for(int dh = 0; dh < dst_height; dh++){
        for(int dw = 0; dw < dst_width; dw++){
            double value = 0;
            float totalWeight = 0;

            float x = ((static_cast<float>(dw)+ 0.5f)
                        *static_cast<float>(src_width)) / static_cast<float>(dst_width);
            x -= 0.5f;
            float y = (static_cast<float>(dh)+ 0.5f)
                        *static_cast<float>(src_height) / static_cast<float>(dst_height);
            y -= 0.5f;
            float dx = x - static_cast<float>(floor(x));
            float dy = y - static_cast<float>(floor(y));

            for(int i = -1; i < 3; i++){
                for(int j = -1; j < 3; j++){

                    float wx = weight(j - dx);
                    float wy = weight(i - dy);
                    float w = wx * wy;

                    int sw = BORDER_INTERPOLATE((int)(x + j), src_width);
                    int sh = BORDER_INTERPOLATE((int)(y + i), src_height);
                    T s = src(sw, sh);

                    value += w*s;
                    totalWeight += w;
                }

            }
            if(fabs(totalWeight)>0){
                value /= fabs(totalWeight);
            }else{
                value= 0;
            }
            value += 0.5;
            value = (value < min_value) ? min_value : value;
            value = (value > max_value) ? max_value : value;
            dst(dw, dh) = static_cast<T>(value);
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

        func(input, output);
        auto expect = mk_null_buffer<T>(out_extents);
        expect = ref_bicubic(expect, input, in_width, in_height, out_width, out_height);

        for (int y=0; y<out_height; ++y) {
            for (int x=0; x<out_width; ++x) {
                T actual = output(x, y);

                if (abs(expect(x,y) - actual) > 9) {
                    // Temporarily the largest error sets 8 where 1024x768->500x500
                    // Check the comment in the function scale_bicubic in ImageProcessing.h
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
    test<uint8_t>(scale_bicubic_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(scale_bicubic_u16);
#endif
#ifdef TYPE_i16
    test<int16_t>(scale_bicubic_i16);
#endif
}
