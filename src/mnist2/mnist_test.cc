#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "test_common.h"

#include "mnist.h"

using namespace Halide::Runtime;

template<typename Type>
Halide::Runtime::Buffer<Type> load_data(const std::string& fname)
{
    std::ifstream ifs(fname.c_str(), std::ios_base::binary);
    if (!ifs.is_open()) {
        throw std::runtime_error(format("File not found : %s", fname.c_str()).c_str());
    }

    uint32_t dim;
    ifs.read(reinterpret_cast<char*>(&dim), sizeof(dim));

    std::vector<int> extents(dim);

    for (size_t i=0; i<dim; i++) {
        uint32_t e;
        ifs.read(reinterpret_cast<char*>(&e), sizeof(e));
        extents[i] = static_cast<int>(e);
    }

    Halide::Runtime::Buffer<Type> buffer(extents);

    // remaing size
    ifs.seekg(0, std::ifstream::end);
    std::ifstream::pos_type end = ifs.tellg();

    ifs.seekg((1+dim)*sizeof(uint32_t), std::ifstream::beg);
    std::ifstream::pos_type beg = ifs.tellg();

    ifs.read(reinterpret_cast<char*>(buffer.data()), end-beg);

    return buffer;
}

double accuracy(const Buffer<float>& probs, const Buffer<int>& labels)
{
    assert(probs.dimensions() == 2 && labels.dimensions() == 1);

    int label_num = probs.extent(0);
    int data_num = probs.extent(1);
    assert(labels.extent(0) == data_num);

    int correct_num = 0;

    for (int i = 0; i < data_num; i++) {
        std::vector<std::pair<float, int> > probs_vec(label_num);
        int label_val = labels(i);
        assert(label_val >= 0 && label_val < label_num);

        for (int j = 0; j < label_num; j++) {
            probs_vec[j] = std::make_pair(probs(j, i), j);
        }

        std::sort(probs_vec.begin(), probs_vec.end(), std::greater<std::pair<float, int>>());
        int top = probs_vec[0].second;

        if (top == label_val) {
            correct_num++;
        }
    }

    double accuracy = correct_num / static_cast<double>(data_num);
    return accuracy;
}

template<typename T>
void Verify(const Buffer<T>& actuals, const Buffer<T>& expects, T tolerance = 10e-3)
{
    std::cerr << actuals.number_of_elements() << " : " << expects.number_of_elements() << std::endl;
    assert(actuals.number_of_elements() == expects.number_of_elements());

    for (size_t i = 0; i < actuals.number_of_elements(); i++) {
        T actual = actuals.data()[i];
        T expect = expects.data()[i];
        T error = std::fabs((actual - expect) / actual);
        if (error > tolerance) {
            throw std::runtime_error(format("Error: expect(%d) = %f, actual(%d) = %f",
                                            i, expect, i, actual).c_str());
        }
    }
}

int main(int argc, char **argv) {
    try {
        //Buffer<int32_t> in = load_data<int32_t>("data/test_data_b1.bin");
        Buffer<float> in = load_data<float>("data/mnist_input.bin");

        const int classes = 10;
        const int batch_size = in.extent(3);

        Buffer<float> out(classes, batch_size);
        // Buffer<float> out(20, 24, 24, batch_size);

        mnist(in, out);

        Buffer<int> labels = load_data<int>("data/mnist_label.bin");

        // Buffer<float> expects = load_data<float>("data/conv1_out.bin");
        // Verify(out, expects);

        double acc = accuracy(out, labels);
        std::cout << "Accurary: " << acc << std::endl;;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    printf("Success!\n");
    return 0;
}
