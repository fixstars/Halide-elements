#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include "HalideRuntime.h"
#include "HalideBuffer.h"
#include "test_common.h"

#include "warp_map_bilinear_u8.h"
#include "warp_map_bilinear_u16.h"

#define BORDER_INTERPOLATE(x, l) (x < 0 ? 0 : (x >= l ? l - 1 : x))

template<typename T>
T interpolateBL(const Halide::Runtime::Buffer<T>& data, const int width, const int height,
                float x, float y, T border_value, const int border_type)
{
    if(x != x){x=0;}
    if(y != y){y=0;}

    x -= 0.5f;
    y -= 0.5f;

    int yf = static_cast<int>(y);
    yf = yf- (yf > y);
    int xf = static_cast<int>(x);
    xf = xf - (xf > x);

    T d[4];
    if (xf >= 0 && yf >= 0 && xf < width-1 && yf < height -1){
        d[0] = data(xf, yf);
        d[1] = data(xf+1, yf);
        d[2] = data(xf, yf+1);
        d[3] = data(xf+1, yf+1);
    }else if (border_type==1){
        int xf0 = BORDER_INTERPOLATE(xf, width);
        int xf1 = BORDER_INTERPOLATE(xf+1, width);
        int yf0 = BORDER_INTERPOLATE(yf, height);
        int yf1 = BORDER_INTERPOLATE(yf+1, height);
        d[0] = data(xf0, yf0);
        d[1] = data(xf1, yf0);
        d[2] = data(xf0, yf1);
        d[3] = data(xf1, yf1);
    }else{
        assert(border_type==0);
        int cmpxf0 = xf >= 0 && xf < width;
        int cmpxf1 = xf >= -1 && xf < width -1;
        int cmpyf0 = yf >= 0 && yf < height;
        int cmpyf1 = yf >= -1 && yf < height -1;

        d[0] = cmpxf0 && cmpyf0 ? data(xf, yf) : border_value;
        d[1] = cmpxf1 && cmpyf0 ? data(xf+1, yf) : border_value;
        d[2] = cmpxf0 && cmpyf1 ? data(xf, yf+1) : border_value;
        d[3] = cmpxf1 && cmpyf1 ? data(xf+1, yf+1) : border_value;
    }

    float dx = (std::min)((std::max)(0.0f, x - xf), 1.0f);
    float dy = (std::min)((std::max)(0.0f, y - yf), 1.0f);
    float value = (d[0]*(1-dx) * (1-dy) + d[1] * dx * (1-dy)) +
                  (d[2] * (1-dx) * dy + d[3] * dx * dy);

    return static_cast<T>(value+0.5f);

}

template<typename T>
Halide::Runtime::Buffer<T>& warp_map_bilinear_ref(Halide::Runtime::Buffer<T>& dst,
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

            dst(j, i) = interpolateBL(src, width, height, src_x, src_y, border_value, border_type);
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
        expect = warp_map_bilinear_ref(expect, input0, input1, input2, border_value, border_type, width, height);
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
    test<uint8_t>(warp_map_bilinear_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(warp_map_bilinear_u16);
#endif
}
