#include <string>
#include <vector>

using std::string;
using std::vector;

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

template<typename T>
T round_to_nearest_even(float v)
{
    float i;
    float f = modff(v, &i);
    if (0.5 == f || -0.5 == f) {
        if (0 == static_cast<int64_t>(i) % 2) {
            return static_cast<T>(i);
        } else {
            if (v < 0) {
                return static_cast<T>(i - 1);
            } else {
                return static_cast<T>(i + 1);
            }
        }
    } else {
        return static_cast<T>(round(v));
    }   
}

}

