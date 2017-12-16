#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "sad_u8.h"
#include "sad_u16.h"
#include "sad_u32.h"

#include "test_common.h"

using namespace std;

template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer1,
                     struct halide_buffer_t *_src_buffer2,
                     struct halide_buffer_t *_dst_buffer))
{
    try {
        int ret = 0;
        constexpr unsigned int N = 2; 
		typedef int64_t S;
		
        //
        // Run
        //
        const int width = 1024;
        const int height = 768;
        //const int width = 10;
        //const int height = 8;
        const std::vector<int32_t> in_extents{width, height};
        const std::vector<int32_t> out_extents{width, height};
        Halide::Runtime::Buffer<T> input[N];
        for (int i = 0; i < N; ++i) {
            input[i] = mk_rand_buffer<T>(in_extents);
        }
        auto output = mk_null_buffer<T>(out_extents);
        
        T expect[height * width];
        
        // Reference impl.
		// first pass
		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				S diff = static_cast<S>(input[0](i,j)) -
					static_cast<S>(input[1](i,j));
				expect[j * width + i] = static_cast<T>(diff < 0 ? -diff : diff);
			}
		}

        func(input[0], input[1], output);

//		cout << "input[0]" << endl;
//        for (int y=0; y<height; ++y) {
//			for (int x=0; x<width; ++x) {
//				cout << input[0](x,y) << " ";
//            }
//			cout << endl;
//        }
//		cout << "input[1]" << endl;
//        for (int y=0; y<height; ++y) {
//			for (int x=0; x<width; ++x) {
//				cout << input[1](x,y) << " ";
//            }
//			cout << endl;
//        }
//
//		cout << "halide" << endl;
//        for (int y=0; y<height; ++y) {
//			for (int x=0; x<width; ++x) {
//				T actual = output(x,y);
//				cout << hex << actual << " ";
//            }
//			cout << endl;
//        }
//
//		cout << "ref" << endl;
//        for (int y=0; y<height; ++y) {
//			for (int x=0; x<width; ++x) {
//				cout << hex << expect[y*width+x] << " ";
//            }
//			cout << endl;
//        }

        for (int y=0; y<height; ++y) {
			for (int x=0; x<width; ++x) {
				T actual = output(x,y);
				if (expect[y * width + x] != actual) {
					throw std::runtime_error(format("Error: expect(%d, %d) = %u, actual(%d, %d) = %u, input0=%d,input1=%d", x, y, expect[y * width + x], x, y, actual,input[0](x,y),input[1](x,y)).c_str());
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
    test<uint8_t>(sad_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(sad_u16);
#endif
#ifdef TYPE_u32
    test<uint32_t>(sad_u32);
#endif
}
