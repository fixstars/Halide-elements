#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "test_common.h"

#include "alexnet_xnor.h"

using namespace Halide::Runtime;

namespace {


template<typename Type>
Halide::Runtime::Buffer<Type> load_data(const std::string& fname)
{
    std::ifstream ifs(fname.c_str(), std::ios_base::binary);
    if (!ifs.is_open()) {
        throw std::runtime_error("File not found");
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

template<typename T>
bool pair_compare(const std::pair<T, size_t>& lhs,
                  const std::pair<T, size_t>& rhs)
{
    return lhs.first > rhs.first;
}

template<typename T>
std::vector<size_t> argmax(const T* v, int class_num, size_t top_num)
{
    std::vector<std::pair<T, size_t>> pairs;
    for (auto i = decltype(class_num)(0); i < class_num; ++i) {
        pairs.push_back(std::make_pair(v[i], i));
    }

    std::partial_sort(pairs.begin(), pairs.begin() + top_num, pairs.end(), pair_compare<T>);

    std::vector<size_t> result;
    for (auto i = decltype(top_num)(0); i < top_num; ++i) {
        result.push_back(pairs[i].second);
    }

    return result;
}

std::vector<std::string> read_labels(const std::string &label_name)
{
    std::ifstream ifs(label_name.c_str());
    std::vector<std::string> labels;
    std::string line;

    while(std::getline(ifs, line)) {
        labels.push_back(line);
    }

    return labels;
}

void classify(const Buffer<float>& probs, int class_num, int batch_size, const std::string &label_file)
{
    int label_indices[] = {
        147, 312, 722, 440, 429, 507, 917, 204, 306, 538, 870, 642,  84, 338, 413, 975, 417, 300, 871, 692,
        859, 901, 992, 944, 635, 556, 447, 407, 724, 549, 160, 360, 988, 520, 425, 337, 989, 414, 972, 292,
        141, 913, 104, 868, 256, 443,  92, 744, 746, 961, 791, 463, 721, 347, 884, 339, 905, 481, 927, 241,
        388, 386, 866, 198, 126, 369, 565, 690, 796, 168, 701, 996, 449, 254, 916,  98,  83, 454, 242, 719,
        833, 541, 415, 942,  72, 456, 792, 924, 623, 835, 729, 639, 293, 620, 248,   3,   7, 273, 885, 769,
        738, 601, 211, 595, 205, 985, 215, 767, 303, 114, 713, 546, 723, 962, 530, 453, 758, 633, 106, 284,
        567, 495, 760, 570, 682, 137, 631, 873, 856, 494, 949, 239, 244, 381, 334, 403, 945, 847, 801, 318,
        829, 346, 357, 247, 966,  63, 834,  30, 867, 808, 437, 580, 259, 940, 915, 929, 652,  77, 914,  33,
        309, 838, 699, 497, 372, 131, 391, 987, 531, 612, 777, 523, 283, 536,  15, 782, 830, 617, 249, 886,
        275, 489, 604, 578, 861, 820, 499, 348, 387, 957, 383, 366, 173, 272, 650, 771, 841, 428, 133, 290,
        491, 221, 960, 708, 573, 532, 898, 424, 522, 730, 790, 669, 404, 698,  62, 385, 925,  97, 571, 116,
        442, 225,  38, 909, 115,  32, 132, 181, 305, 694, 908, 999, 498, 191, 485, 586, 627,  34,  16, 218,
        568, 976, 624, 552, 843, 754, 136, 451, 599, 514, 101, 525, 602,  41,   2, 492,  90, 482, 950, 392,
        478, 953,  51, 319, 704, 311, 613, 930, 402, 400, 299, 540, 619, 506, 395, 235, 890, 605, 594, 200,
        933, 593, 521, 328, 980, 537, 253, 780,  48, 896, 446, 811, 582,  94, 806,  71, 237, 170, 490,  17,
        977, 472,  31, 899, 853, 887, 943, 954, 849, 362, 477, 240, 558, 779, 700, 827,   8, 797, 308, 154,
        433, 589,  52, 803, 937, 243, 800, 813, 110, 342, 663, 842, 816, 554, 986, 990,  55, 675, 628, 163,
        676, 733, 561, 162, 158, 330, 559, 206, 408, 282, 683, 302, 888, 662, 778,  14, 175,  42, 681, 764,
        880, 336, 217, 716, 128, 591, 793, 340, 220, 959, 883, 824, 881, 349, 622, 423, 258, 105, 689, 138,
        187, 180, 855, 564, 261, 928, 982, 384, 100, 551, 588,  93, 232, 572,   1, 329, 998, 197,  59, 742,
        375, 224, 560, 804, 397, 757, 113, 971, 260, 919,  58, 893, 839, 672, 144, 852, 894, 845,  64, 625,
        513,  26, 629, 462, 274, 208, 709,  44, 734, 245, 146, 783, 903, 389, 267, 879,  57, 250, 657, 794,
        860, 436, 167, 610, 936, 679, 439, 680, 878, 119, 969, 430, 912, 844, 534, 296, 665, 923, 487, 876,
        203, 674, 686, 906, 111, 685, 837, 678, 872, 123, 964, 344, 266, 703, 480, 718,  23,  45, 421, 951,
        519, 649, 768, 542, 327, 831, 469, 152, 376, 563, 616, 550, 688, 301, 934, 263, 467, 236, 882, 984,
        508, 471, 419,  21, 848, 461,  19, 756, 143, 818, 475, 673, 648, 805, 457, 140, 279, 770, 691, 634,
        707, 505, 278, 739, 712, 761, 427, 409, 569,   5, 420, 666, 654, 315, 772, 356,  27, 741, 926, 255,
        324, 865, 196, 952, 636,  39, 840, 252, 979, 821, 646, 655, 190, 373, 670,  68, 706, 736, 359, 479,
        630, 753, 529, 958,  24, 851,  99, 584, 668, 382, 223, 647, 434, 222, 997, 579, 850,  81, 828,  49,
        112, 380, 653, 955, 603,  47, 661, 737, 265,  29, 157, 993, 562, 176, 874, 396, 313,  75, 370, 185,
        264, 460, 149, 321, 875, 607, 697,  10, 326, 814, 445, 938,  43,  74, 172, 117, 970, 596,  82, 422,
        76, 251, 714, 178, 367, 920,  40,  37, 129, 740, 353, 207, 156, 124, 287, 545, 664, 802, 493, 702,
        484, 904, 526, 246, 775, 108, 921, 720, 233, 826, 183, 199, 473, 555,  20, 854,  86, 365, 809, 379,
        643, 209, 819, 931, 441, 332, 857, 608,  67, 978, 202, 774, 188,  91, 766, 939,  25, 784, 186, 817,
        405, 705, 799, 281,  46, 125, 230, 310, 316, 374, 825, 476, 214, 510, 645, 759,   0,  22, 583, 863,
        95, 295, 294,  60, 231, 527, 656, 900, 743, 544, 788, 416, 717,  88, 823, 547,  80, 468, 732, 731,
        470, 798, 257,  69, 322, 228, 358, 660, 553, 611, 517, 995, 107,  96, 151, 320, 695, 355, 323, 781,
        139, 609, 606, 177, 122, 169, 212, 184, 752, 297,  79, 201, 192, 314,  54,  66, 963, 735, 577, 911,
        331, 262, 862, 483, 285, 127, 286, 684, 748, 922, 411,  28, 728, 598, 363, 448, 465, 238, 378, 858,
        597, 815, 148, 333, 377, 277, 398, 587, 438, 935, 341, 345, 967, 364, 745, 795, 615, 464, 618, 516,
        543, 892, 877, 696, 394, 973, 822, 165, 710, 179, 134, 153,  35, 751, 504, 103, 897, 431, 659, 932,
        693, 455, 533, 226, 189, 763, 869, 216, 641, 968, 677, 902, 269,  36, 368, 142, 130, 432, 174, 864,
        227, 435, 406, 458,  56, 832, 524, 574, 762, 450, 749,  78, 947, 891, 807, 787, 941, 219, 994, 164,
        518, 350, 755, 195, 371, 166, 121,   6, 846,  87, 907, 210,  12, 946, 581, 810, 393, 500, 229, 640,
        651, 776,  53,   9, 535, 193, 171,  11, 727, 271, 194, 354, 399, 765, 965, 418, 585, 234, 276, 644,
        351, 786, 288, 512, 155, 488, 687, 836, 268, 621, 466, 812, 671, 118, 548, 161, 667, 747, 401, 910,
        991, 576,  18, 658, 528, 974,  73, 956, 725, 638, 444, 452, 145, 750, 503, 773, 592,  70, 632, 981,
        102, 889,   4, 361, 459, 352,  13, 566, 335,  89, 501, 715, 948, 983,  61, 298, 412, 317, 280, 150,
        918,  65, 575, 474, 109, 600, 410, 637, 539, 502,  50, 135, 343, 182, 785, 304, 291, 626,  85, 590,
        711, 486, 159, 557, 614, 325, 515, 426, 496, 289, 509, 307, 895, 390, 511, 270, 213, 120, 726, 789,
    };

    constexpr size_t top_num = 5;

    for (auto i = decltype(batch_size)(0); i < batch_size; ++i) {
        const float* p = probs.data() + class_num*i;
        auto tops = argmax(p, class_num, top_num);

        std::vector<std::string> labels = read_labels(label_file);

        for (auto i = decltype(tops.size())(0); i < tops.size(); ++i) {
            auto index = tops[i];
            auto label_i = label_indices[index];
            std::cout << std::fixed << std::setprecision(3) << p[index]*100 << ": "
                      << labels[label_i] << "\n";
        }
    }
}

}

int main(int argc, char **argv) {
    try {
        Buffer<int32_t> in = load_data<int32_t>("data/image_n03207743.bin");

        Buffer<int32_t> c1w = load_data<int32_t>("data/conv1_weight.bin");
        Buffer<int32_t> c1b = load_data<int32_t>("data/conv1_bias.bin");
        Buffer<int32_t> bn1m = load_data<int32_t>("data/bn1_mean.bin");
        Buffer<int32_t> bn1v = load_data<int32_t>("data/bn1_variance.bin");
        Buffer<int32_t> s1w = load_data<int32_t>("data/scale1_weight.bin");
        Buffer<int32_t> s1b = load_data<int32_t>("data/scale1_bias.bin");

        Buffer<int32_t> bn2m = load_data<int32_t>("data/bn2_mean.bin");
        Buffer<int32_t> bn2v = load_data<int32_t>("data/bn2_variance.bin");
        Buffer<int32_t> s2w = load_data<int32_t>("data/scale2_weight.bin");
        Buffer<int32_t> s2b = load_data<int32_t>("data/scale2_bias.bin");
        Buffer<bool> c2w = load_data<bool>("data/conv2_weight.bin");
        Buffer<int32_t> c2a = load_data<int32_t>("data/conv2_alpha.bin");
        Buffer<int32_t> c2b = load_data<int32_t>("data/conv2_bias.bin");

        Buffer<int32_t> bn3m = load_data<int32_t>("data/bn3_mean.bin");
        Buffer<int32_t> bn3v = load_data<int32_t>("data/bn3_variance.bin");
        Buffer<int32_t> s3w = load_data<int32_t>("data/scale3_weight.bin");
        Buffer<int32_t> s3b = load_data<int32_t>("data/scale3_bias.bin");
        Buffer<bool> c3w = load_data<bool>("data/conv3_weight.bin");
        Buffer<int32_t> c3a = load_data<int32_t>("data/conv3_alpha.bin");
        Buffer<int32_t> c3b = load_data<int32_t>("data/conv3_bias.bin");

        Buffer<int32_t> bn4m = load_data<int32_t>("data/bn4_mean.bin");
        Buffer<int32_t> bn4v = load_data<int32_t>("data/bn4_variance.bin");
        Buffer<int32_t> s4w = load_data<int32_t>("data/scale4_weight.bin");
        Buffer<int32_t> s4b = load_data<int32_t>("data/scale4_bias.bin");
        Buffer<bool> c4w = load_data<bool>("data/conv4_weight.bin");
        Buffer<int32_t> c4a = load_data<int32_t>("data/conv4_alpha.bin");
        Buffer<int32_t> c4b = load_data<int32_t>("data/conv4_bias.bin");

        Buffer<int32_t> bn5m = load_data<int32_t>("data/bn5_mean.bin");
        Buffer<int32_t> bn5v = load_data<int32_t>("data/bn5_variance.bin");
        Buffer<int32_t> s5w = load_data<int32_t>("data/scale5_weight.bin");
        Buffer<int32_t> s5b = load_data<int32_t>("data/scale5_bias.bin");
        Buffer<bool> c5w = load_data<bool>("data/conv5_weight.bin");
        Buffer<int32_t> c5a = load_data<int32_t>("data/conv5_alpha.bin");
        Buffer<int32_t> c5b = load_data<int32_t>("data/conv5_bias.bin");

        Buffer<int32_t> bn6m = load_data<int32_t>("data/bn6_mean.bin");
        Buffer<int32_t> bn6v = load_data<int32_t>("data/bn6_variance.bin");
        Buffer<int32_t> s6w = load_data<int32_t>("data/scale6_weight.bin");
        Buffer<int32_t> s6b = load_data<int32_t>("data/scale6_bias.bin");
        Buffer<bool> f6w = load_data<bool>("data/fc6_weight.bin");
        Buffer<int32_t> f6a = load_data<int32_t>("data/fc6_alpha.bin");
        Buffer<int32_t> f6b = load_data<int32_t>("data/fc6_bias.bin");

        Buffer<int32_t> bn7m = load_data<int32_t>("data/bn7_mean.bin");
        Buffer<int32_t> bn7v = load_data<int32_t>("data/bn7_variance.bin");
        Buffer<int32_t> s7w = load_data<int32_t>("data/scale7_weight.bin");
        Buffer<int32_t> s7b = load_data<int32_t>("data/scale7_bias.bin");
        Buffer<bool> f7w = load_data<bool>("data/fc7_weight.bin");
        Buffer<int32_t> f7a = load_data<int32_t>("data/fc7_alpha.bin");
        Buffer<int32_t> f7b = load_data<int32_t>("data/fc7_bias.bin");

        Buffer<int32_t> bn8m = load_data<int32_t>("data/bn8_mean.bin");
        Buffer<int32_t> bn8v = load_data<int32_t>("data/bn8_variance.bin");
        Buffer<int32_t> s8w = load_data<int32_t>("data/scale8_weight.bin");
        Buffer<int32_t> s8b = load_data<int32_t>("data/scale8_bias.bin");
        Buffer<int32_t> f8w = load_data<int32_t>("data/fc8_weight.bin");
        Buffer<int32_t> f8b = load_data<int32_t>("data/fc8_bias.bin");

        const int classes = 1000;
        const int batch_size = in.extent(3);

        Buffer<float> out(classes, batch_size);

        alexnet_xnor(
            in,
            c1w, c1b, bn1m, bn1v, s1w, s1b,
            bn2m, bn2v, s2w, s2b, c2w, c2a, c2b,
            bn3m, bn3v, s3w, s3b, c3w, c3a, c3b,
            bn4m, bn4v, s4w, s4b, c4w, c4a, c4b,
            bn5m, bn5v, s5w, s5b, c5w, c5a, c5b,
            bn6m, bn6v, s6w, s6b, f6w, f6a, f6b,
            bn7m, bn7v, s7w, s7b, f7w, f7a, f7b,
            bn8m, bn8v, s8w, s8b, f8w, f8b,
            out);

        std::string label_file("data/synset_words.txt");
        classify(out, classes, batch_size, label_file);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    printf("Success!\n");
    return 0;
}
