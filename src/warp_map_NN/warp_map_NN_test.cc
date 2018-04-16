#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include "HalideRuntime.h"
#include "HalideBuffer.h"
#include "test_common.h"

#include "warp_map_NN_u8.h"
#include "warp_map_NN_u16.h"

#define BORDER_INTERPOLATE(x, l) (x < 0 ? 0 : (x >= l ? l - 1 : x))

template<typename T>
T interpolateNN(const Halide::Runtime::Buffer<T>& data, const int width, const int height,
                float x, float y, T border_value, const int border_type)
{
    if(x != x){x=0;}
    if(y != y){y=0;}

    int i = static_cast<int>(floor(y));
    int j = static_cast<int>(floor(x));

    if (i>=0 && j >=0 && i < height && j < width){
        return data(j, i);
    }else if(border_type == 1){
        i = BORDER_INTERPOLATE(i, height);
        j = BORDER_INTERPOLATE(j, width);
        return data(j, i);
    }else{
        assert(border_type==0);
        return border_value;
    }
}

template<typename T>
Halide::Runtime::Buffer<T>& warp_map_NN_ref(Halide::Runtime::Buffer<T>& dst,
                                const Halide::Runtime::Buffer<T>& src,
                                const Halide::Runtime::Buffer<float>& mapX,
                                const Halide::Runtime::Buffer<float>& mapY,
                                const T border_value, const int32_t border_type,
                                const int32_t width, const int32_t height)
{
    /* avoid overflow from X-1 to X+2 */
    float imin = static_cast<float>((std::numeric_limits<int>::min)() + 1);
    float imax = static_cast<float>((std::numeric_limits<int>::max)() - 2);

    for(int i = 0; i < height; ++i){
        for(int j = 0; j < width; ++j){
            float src_x = mapX(j, i);
            float src_y = mapY(j, i);

            src_x = std::max(imin, std::min(imax, src_x));
            src_y = std::max(imin, std::min(imax, src_y));

            dst(j, i) = interpolateNN(src, width, height, src_x, src_y, border_value, border_type);
        }
    }
    return dst;
}

template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer0,
                     struct halide_buffer_t *_src_buffer1,
                     struct halide_buffer_t *_src_buffer2,
                     T _border_value,
                     struct halide_buffer_t *_dst_buffer))
{
    try {
        const int width = 1024;
        const int height = 768;
        const std::vector<int32_t> extents{width, height};
        const T border_value = mk_rand_scalar<T>();
        const int32_t border_type = 0; // 0 or 1
        auto input0 = mk_rand_buffer<T>(extents);
        auto input1 = mk_rand_buffer<float>(extents);
        auto input2 = mk_rand_buffer<float>(extents);
        auto output = mk_null_buffer<T>(extents);


        func(input0, input1, input2, border_value, output);

        auto expect = mk_null_buffer<T>(extents);
        expect = warp_map_NN_ref(expect, input0, input1, input2, border_value, border_type, width, height);
        // for each x and y
        for (int j=0; j<width; ++j) {
            for (int i=0; i<height; ++i) {
                if (abs(expect(j, i) - output(j, i)) > 1) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d",
                                                j, i, expect(j, i), j, i, output(j, i)));
                }
            }
        }

    } catch (const std::exception& e){
        std::cerr << e.what() << std::endl;
        return 1;
    }

    printf("Success!\n");
    return 0;
}

int main()
{
#ifdef TYPE_u8
    test<uint8_t>(warp_map_NN_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(warp_map_NN_u16);
#endif
}
