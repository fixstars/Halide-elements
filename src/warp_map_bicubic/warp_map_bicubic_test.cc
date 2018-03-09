#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include "HalideRuntime.h"
#include "HalideBuffer.h"
#include "test_common.h"

#include "warp_map_bicubic_u8.h"
#include "warp_map_bicubic_u16.h"

#define BORDER_INTERPOLATE(x, l) (x < 0 ? 0 : (x >= l ? l - 1 : x))

void getCubicKernel(float n, float w[4]){
    static const float a = -0.75f;
    w[0] = ((a*(n+1.0f)-5.0f*a)*(n+1.0f)+8.0f*a)*(n+1.0f)-4.0f*a;
    w[1] = ((a+2.0f)*n-(a+3.0f))*n*n+1.0f;
    w[2] = ((a+2.0f)*(1.0f-n)-(a+3.0f))*(1.0f-n)*(1.0f-n)+1.0f;
    w[3] = 1.0f-w[2]-w[1]-w[0];
}

template<typename T>
T interpolateBC(const Halide::Runtime::Buffer<T>& data, const int width, const int height,
                float x, float y, T border_value, const int border_type)
{
    if(x != x){x=0;}
    if(y != y){y=0;}
    x -= 0.5f;
    y -= 0.5f;

    int xf = static_cast<int>(x - 1.0f);
    int yf = static_cast<int>(y - 1.0f);

    xf = xf - (xf > x - 1);
    yf = yf - (yf > y - 1);

    float d[4][4];
    if (xf >= 0 && yf >= 0 && xf < width - 3 && yf < height - 3) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                d[i][j] = data(xf+j, yf+i);
            }
        }
    }else{
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (xf >= -j && yf >= -i && xf < width-j && yf < height-i) {
                    d[i][j] = data(xf+j, yf+i);
                } else if (border_type == 1) {
                    int xfj = BORDER_INTERPOLATE(xf + j, width);
                    int yfi = BORDER_INTERPOLATE(yf + i, height);
                    d[i][j] = data(xfj, yfi);
                } else {
                    assert(border_type == 0);
                    d[i][j] = border_value;
                }
            }
        }
    }

    float dx = (std::min)((std::max)(0.0f, x - xf - 1.0f), 1.0f);
    float dy = (std::min)((std::max)(0.0f, y - yf - 1.0f), 1.0f);

    float w[4];
    getCubicKernel(dx, w);

    float col[4];
    for (int i = 0; i < 4; i++) {
        col[i] = (d[i][0] * w[0] + d[i][1] * w[1])
                    + (d[i][2] * w[2] + d[i][3] * w[3]);
    }

    getCubicKernel(dy, w);
    float value = (col[0] * w[0] + col[1] * w[1])
                    + (col[2] * w[2] + col[3] * w[3]);

    T min = (std::numeric_limits<T>::min)();
    T max = (std::numeric_limits<T>::max)();
    return static_cast<T>(value < min ? min : value > max ? max: value + 0.5f);


}

template<typename T>
Halide::Runtime::Buffer<T>& warp_map_bicubic_ref(Halide::Runtime::Buffer<T>& dst,
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

            dst(j, i) = interpolateBC(src, width, height, src_x, src_y, border_value, border_type);
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
        expect = warp_map_bicubic_ref(expect, input0, input1, input2, border_value, border_type, width, height);
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
    test<uint8_t>(warp_map_bicubic_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(warp_map_bicubic_u16);
#endif
}
