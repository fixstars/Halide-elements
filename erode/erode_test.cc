#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "erode.h"

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
	const int window_width = 3;
	const int window_height = 3;
	const int iteration = 2;
        Buffer<uint8_t> input(width, height);
        Buffer<uint8_t> output(width, height);
	Buffer<uint8_t> structure(window_width * window_height);
	uint8_t expect[width][height], workbuf[width][height];
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
		input(x, y) = (uint8_t)(x + y);
		workbuf[x][y] = input(x, y);
                output(x, y) = UCHAR_MAX;
            }
        }

	for (int i=0; i<window_width*window_height; ++i) {
	    structure(i) = (uint8_t)i;
	}

	for (int k=0; k<iteration; ++k) {
	    for (int y=0; y<height; ++y) {
		for (int x=0; x<width; ++x) {
		    uint8_t min = UCHAR_MAX;
		    for (int j = -(window_height/2); j < -(window_height/2) + window_height; j++) {
			int yy = y + j >= 0 ? y + j: 0;
			yy = yy < height ? yy : height - 1;
			for (int i = -(window_width/2); i < -(window_width/2) + window_width; i++) {
			    if (structure(window_width/2+i + window_width * (window_height/2+j))) {
				int xx = x + i >= 0 ? x + i: 0;
				xx = xx < width ? xx : width - 1;
				if (min > workbuf[xx][yy]) {
				    min = workbuf[xx][yy];
				}
			    }
			}
		    }
		    expect[x][y] = min;
		}
	    }
	    memcpy(workbuf, expect, sizeof(workbuf));
	}

        erode(input, structure, window_width, window_height, output);

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                uint8_t actual = output(x, y);
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
