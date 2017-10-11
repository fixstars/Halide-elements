#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "histogram2d.h"

using std::string;
using std::vector;
using Halide::Runtime::Buffer;

namespace {

template<typename... Rest>
string format(const char *fmt, const Rest&... rest)
{
    int length = snprintf(NULL, 0, fmt, rest...) + 1; // Explicit place for null termination
    vector<char> buf(length, 0);
    snprintf(&buf[0], length, fmt, rest...);
    string s(buf.begin(), std::find(buf.begin(), buf.end(), '\0'));
    return s;
}

}

int main()
{
    try {
        int ret = 0;
        
        //
        // Run
        //
        const int width = 1024;
	const int height = 768;
	const int hist_width = UCHAR_MAX + 1;
        Buffer<uint8_t> input0(width, height), input1(width, height);
        Buffer<uint32_t> output(hist_width, hist_width);
	uint32_t expect[hist_width][hist_width];

	memset(expect, 0, sizeof(expect));
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                input0(x, y) = (uint8_t)(x + y);
                input1(x, y) = (uint8_t)(x - y);		
		expect[input0(x, y) * (hist_width - 1) / UCHAR_MAX][input1(x, y) * (hist_width - 1) / UCHAR_MAX]++;
            }
        }

	for (int y=0; y<hist_width; ++y) {
	    for (int x=0; x<hist_width; ++x) {
		output(x, y) = 0;
	    }
	}

        histogram2d(input0, input1, output);

	for (int y=0; y<hist_width; ++y) {
	    for (int x=0; x<hist_width; ++x) {
		uint32_t actual = output(x, y);
		if (expect[x][y] != actual) {
		    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d", x, y, expect[x][y], x, y, actual).c_str());
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
