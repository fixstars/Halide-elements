#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "test_common.h"

#include "cifar10.h"

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

double accuracy(const Buffer<float>& probs, const Buffer<int64_t>& labels)
{
    assert(probs.dimensions() == 2 && labels.dimensions() == 1);

    int label_num = probs.extent(0);
    int data_num = probs.extent(1);
    assert(labels.extent(0) == data_num);

    int correct_num = 0;

    for (int i = 0; i < data_num; i++) {
        std::vector<std::pair<float, int> > probs_vec(label_num);
        int label_val = static_cast<int>(labels(i));
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

int main(int argc, char **argv) {
    try {
        Buffer<int32_t> in = load_data<int32_t>("data/test_data.bin");
        Buffer<int32_t> c11w = load_data<int32_t>("data/conv1_1_weight.bin");
        Buffer<int32_t> c11b = load_data<int32_t>("data/conv1_1_bias.bin");
        Buffer<int32_t> bn11m = load_data<int32_t>("data/bn1_1_mean.bin");
        Buffer<int32_t> bn11v = load_data<int32_t>("data/bn1_1_variance.bin");
        Buffer<int32_t> bn12m = load_data<int32_t>("data/bn1_2_mean.bin");
        Buffer<int32_t> bn12v = load_data<int32_t>("data/bn1_2_variance.bin");
        Buffer<int32_t> bn12w = load_data<int32_t>("data/bn1_2_weight.bin");
        Buffer<int32_t> bn12b = load_data<int32_t>("data/bn1_2_bias.bin");
        Buffer<int32_t> c12w = load_data<bool>("data/conv1_2_weight.bin");
        Buffer<int32_t> c12a = load_data<int32_t>("data/conv1_2_alpha.bin");
        Buffer<int32_t> c12b = load_data<int32_t>("data/conv1_2_bias.bin");
        Buffer<int32_t> bn13m = load_data<int32_t>("data/bn1_3_mean.bin");
        Buffer<int32_t> bn13v = load_data<int32_t>("data/bn1_3_variance.bin");
        Buffer<int32_t> bn13w = load_data<int32_t>("data/bn1_3_weight.bin");
        Buffer<int32_t> bn13b = load_data<int32_t>("data/bn1_3_bias.bin");
        Buffer<int32_t> c13w = load_data<bool>("data/conv1_3_weight.bin");
        Buffer<int32_t> c13a = load_data<int32_t>("data/conv1_3_alpha.bin");
        Buffer<int32_t> c13b = load_data<int32_t>("data/conv1_3_bias.bin");
        Buffer<int32_t> bn21m = load_data<int32_t>("data/bn2_1_mean.bin");
        Buffer<int32_t> bn21v = load_data<int32_t>("data/bn2_1_variance.bin");
        Buffer<int32_t> bn21w = load_data<int32_t>("data/bn2_1_weight.bin");
        Buffer<int32_t> bn21b = load_data<int32_t>("data/bn2_1_bias.bin");
        Buffer<int32_t> c21w = load_data<bool>("data/conv2_1_weight.bin");
        Buffer<int32_t> c21a = load_data<int32_t>("data/conv2_1_alpha.bin");
        Buffer<int32_t> c21b = load_data<int32_t>("data/conv2_1_bias.bin");
        Buffer<int32_t> bn22m = load_data<int32_t>("data/bn2_2_mean.bin");
        Buffer<int32_t> bn22v = load_data<int32_t>("data/bn2_2_variance.bin");
        Buffer<int32_t> bn22w = load_data<int32_t>("data/bn2_2_weight.bin");
        Buffer<int32_t> bn22b = load_data<int32_t>("data/bn2_2_bias.bin");
        Buffer<int32_t> c22w = load_data<bool>("data/conv2_2_weight.bin");
        Buffer<int32_t> c22a = load_data<int32_t>("data/conv2_2_alpha.bin");
        Buffer<int32_t> c22b = load_data<int32_t>("data/conv2_2_bias.bin");
        Buffer<int32_t> bn23m = load_data<int32_t>("data/bn2_3_mean.bin");
        Buffer<int32_t> bn23v = load_data<int32_t>("data/bn2_3_variance.bin");
        Buffer<int32_t> bn23w = load_data<int32_t>("data/bn2_3_weight.bin");
        Buffer<int32_t> bn23b = load_data<int32_t>("data/bn2_3_bias.bin");
        Buffer<int32_t> c23w = load_data<bool>("data/conv2_3_weight.bin");
        Buffer<int32_t> c23a = load_data<int32_t>("data/conv2_3_alpha.bin");
        Buffer<int32_t> c23b = load_data<int32_t>("data/conv2_3_bias.bin");
        Buffer<int32_t> bn31m = load_data<int32_t>("data/bn3_1_mean.bin");
        Buffer<int32_t> bn31v = load_data<int32_t>("data/bn3_1_variance.bin");
        Buffer<int32_t> bn31w = load_data<int32_t>("data/bn3_1_weight.bin");
        Buffer<int32_t> bn31b = load_data<int32_t>("data/bn3_1_bias.bin");
        Buffer<int32_t> c31w = load_data<bool>("data/conv3_1_weight.bin");
        Buffer<int32_t> c31a = load_data<int32_t>("data/conv3_1_alpha.bin");
        Buffer<int32_t> c31b = load_data<int32_t>("data/conv3_1_bias.bin");
        Buffer<int32_t> bn32m = load_data<int32_t>("data/bn3_2_mean.bin");
        Buffer<int32_t> bn32v = load_data<int32_t>("data/bn3_2_variance.bin");
        Buffer<int32_t> bn32w = load_data<int32_t>("data/bn3_2_weight.bin");
        Buffer<int32_t> bn32b = load_data<int32_t>("data/bn3_2_bias.bin");
        Buffer<int32_t> c32w = load_data<bool>("data/conv3_2_weight.bin");
        Buffer<int32_t> c32a = load_data<int32_t>("data/conv3_2_alpha.bin");
        Buffer<int32_t> c32b = load_data<int32_t>("data/conv3_2_bias.bin");
        Buffer<int32_t> bn33m = load_data<int32_t>("data/bn3_3_mean.bin");
        Buffer<int32_t> bn33v = load_data<int32_t>("data/bn3_3_variance.bin");
        Buffer<int32_t> c33w = load_data<int32_t>("data/conv3_3_weight.bin");
        Buffer<int32_t> c33b = load_data<int32_t>("data/conv3_3_bias.bin");

        const int classes = 10;
        const int batch_size = in.extent(3);

        Buffer<float> out(classes, batch_size);

        cifar10(in,
                c11w, c11b, bn11m, bn11v,
                bn12m, bn12v, bn12w, bn12b, c12w, c12a, c12b,
                bn13m, bn13v, bn13w, bn13b, c13w, c13a, c13b,
                bn21m, bn21v, bn21w, bn21b, c21w, c21a, c21b,
                bn22m, bn22v, bn22w, bn22b, c22w, c22a, c22b,
                bn23m, bn23v, bn23w, bn23b, c23w, c23a, c23b,
                bn31m, bn31v, bn31w, bn31b, c31w, c31a, c31b,
                bn32m, bn32v, bn32w, bn32b, c32w, c32a, c32b,
                bn33m, bn33v, c33w, c33b,
                out);

        Buffer<int64_t> labels = load_data<int64_t>("data/test_label.bin");

        double acc = accuracy(out, labels);
        std::cout << "Accurary: " << 100*acc << "%" << std::endl;;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    printf("Success!\n");
    return 0;
}
