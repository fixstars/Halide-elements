#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include <Halide.h>

namespace Halide {
namespace Element {

namespace {

void load_param(Buffer<>& param, std::ifstream& ifs)
{
    uint32_t dim;
    ifs.read(reinterpret_cast<char*>(&dim), sizeof(dim));

    std::vector<int32_t> extents(dim);
    for (size_t i=0; i<dim; i++) {
        uint32_t e;
        ifs.read(reinterpret_cast<char*>(&e), sizeof(e));
        extents[i] = static_cast<int32_t>(e);
    }

    ifs.seekg(0, std::ifstream::end);
    std::ifstream::pos_type end = ifs.tellg();

    ifs.seekg((1+dim)*sizeof(uint32_t), std::ifstream::beg);
    std::ifstream::pos_type beg = ifs.tellg();

    std::size_t buf_size_in_byte = end-beg;

    // if (std::accumulate(extents.begin(), extents.end(), sizeof(T), std::multiplies<int32_t>()) != buf_size_in_byte) {
    //     throw std::runtime_error("Unexpected file");
    // }

    ifs.read(reinterpret_cast<char*>(param.data()), buf_size_in_byte);
}

class Layer
{
protected:
    std::string name_;
    Func forward_;
    Var x{"x"}, y{"y"}, c{"c"}, n{"n"};

    std::vector<int32_t> bottom_shape_;    // (w, h, c, n)
    std::vector<int32_t> top_shape_;    // (w, h, c, n)

    virtual void setup_shape(const std::vector<int32_t>& bottom_shape)
    {
        top_shape_ = bottom_shape_ = bottom_shape;
    }

    virtual void setup_param() { /* Do nothing */ }

    virtual void setup_forward(Func bottom)
    {
        const int dim = bottom.dimensions();

        if (dim == 2) {
            forward_(c, n) = bottom(c, n);
        } else if (dim == 4) {
            forward_(c, x, y, n) = bottom(c, x, y, n);
        }
    }

    virtual void setup_schedule() { /* Do nothing */ }


public:
    explicit Layer(const std::string& name)
        : name_(name), forward_(Func(name))
    {}

    void setup(Func bottom_f, const std::vector<int32_t>& bottom_shape)
    {
        setup_shape(bottom_shape);
        setup_param();
        setup_forward(bottom_f);
        setup_schedule();
    }

    void setup(const Layer& bottom)
    {
        Func f = bottom.forward();
        const auto bottom_shape = bottom.top_shape();

        setup(f, bottom_shape);
    }

    virtual void load(std::ifstream& ifs) { /* Do nothing */ }

    inline const std::string& name() const { return name_; }
    inline const Func& forward() const { return forward_; }
    inline const std::vector<int32_t>& bottom_shape() const { return bottom_shape_; }
    inline const std::vector<int32_t>& top_shape() const { return top_shape_; }

};


template<typename T>
class Conv : public Layer
{
protected:
    std::vector<int32_t> kernel_shape_;
    int32_t kernel_num_;
    std::vector<int32_t> strides_;
    std::vector<int32_t> pads_;
    bool use_bias_;

    Func clamped;

    Buffer<> weight_;
    Buffer<> bias_;

    void setup_shape(const std::vector<int32_t>& bottom_shape) override
    {
        bottom_shape_ = bottom_shape;

        top_shape_ = {
            kernel_num_,
            (bottom_shape_[1] - kernel_shape_[0] + 2*pads_[0]) / strides_[0] + 1,
            (bottom_shape_[2] - kernel_shape_[1] + 2*pads_[1]) / strides_[1] + 1,
            bottom_shape_[3]
        };
    }

    void setup_param() override
    {
        const std::string weight_name = name_ + "_weight";
        const std::vector<int32_t> weight_shape = {bottom_shape_[0], kernel_shape_[0], kernel_shape_[1], kernel_num_};
        weight_ = Buffer<>(type_of<T>(), weight_shape, weight_name);

        const std::string bias_name = name_ + "_bias";
        const std::vector<int32_t> bias_shape = {kernel_num_};
        bias_ = Buffer<>(type_of<T>(), bias_shape, bias_name);
    }

    void setup_forward(Func bottom) override
    {
        RDom r(0, bottom_shape_[3], 0, kernel_shape_[0], 0, kernel_shape_[1]);

        clamped = BoundaryConditions::constant_exterior(bottom, Expr(0.0f), 0, bottom_shape_[0], 0, bottom_shape_[1], 0, bottom_shape_[2]);

        if (use_bias_) {
            forward_(c, x, y, n) = sum(r, clamped(r.x, x*strides_[0] - pads_[0] + r.y, y*strides_[1] - pads_[1] + r.z, n) * weight_(r.x, r.y, r.z, c))
                + bias_(c);
        } else {
            forward_(c, x, y, n) = sum(r, clamped(r.x, x*strides_[0] - pads_[0] + r.y, y*strides_[1] - pads_[1] + r.z, n) * weight_(r.x, r.y, r.z, c));
        }
    }

    void setup_schedule() override
    {
        // schedule(clamped, bottom_shape_);
    }

    void load(std::ifstream& ifs) override
    {
        load_param(weight_, ifs);
        load_param(bias_, ifs);
    }

public:
    Conv(const std::string &name,
         const std::vector<int32_t>& kernel_shape, int32_t kernel_num,
         const std::vector<int32_t>& strides, const std::vector<int32_t>& pads,
         bool use_bias)
        : Layer(name), kernel_shape_(kernel_shape), kernel_num_(kernel_num), strides_(strides), pads_(pads), use_bias_(use_bias)
    {}

    Conv(const std::string &name, int32_t kernel_size, int32_t kernel_num, int32_t stride, int32_t pad, bool use_bias)
        : Conv(name, {kernel_size, kernel_size}, kernel_num, {stride, stride}, {pad, pad}, use_bias)
    {}

    Conv(const std::string &name, int32_t kernel_size, int32_t kernel_num)
        : Conv(name, kernel_size, kernel_num, kernel_size/2, 1, true)
    {}

};


class Pool : public Layer
{
protected:
    std::vector<int32_t> window_shape_;
    std::vector<int32_t> strides_;
    std::vector<int32_t> pads_;

    Func clamped;

    void setup_shape(const std::vector<int32_t>& bottom_shape) override
    {
        bottom_shape_ = bottom_shape;

        top_shape_ = {
            bottom_shape_[0],
            (bottom_shape_[1] - window_shape_[0] + 2*pads_[0]) / strides_[0] + 1,
            (bottom_shape_[2] - window_shape_[1] + 2*pads_[1]) / strides_[1] + 1,
            bottom_shape_[3]
        };
    }

    void setup_forward(Func bottom) override
    {
        RDom r(0, window_shape_[0], 0, window_shape_[1]);

        clamped = BoundaryConditions::constant_exterior(bottom, Float(32).min(), 0, bottom_shape_[0], 0, bottom_shape_[1], 0, bottom_shape_[2]);

        forward_(c, x, y, n) = maximum(r, bottom(c, x*strides_[0] - pads_[0] + r.x, y*strides_[1] - pads_[1] + r.y, n));
    }

    void setup_schedule() override
    {
        // schedule(clamped, bottom_shape_);
    }


public:
    Pool(const std::string &name, const std::vector<int32_t>& window_shape, const std::vector<int32_t>& strides, const std::vector<int32_t>& pads)
        : Layer(name), window_shape_(window_shape), strides_(strides), pads_(pads)
    {}

    Pool(const std::string &name, int window_size, int stride)
        : Pool(name, {window_size, window_size}, {stride, stride}, {0, 0})
    {}
};


class Relu : public Layer
{
    float slope_;
    bool leaky_;

    void setup_forward(Func bottom) override
    {
        int dim = bottom.dimensions();

        if (leaky_) {
            if (dim == 2) {
                forward_(c, n) = select(bottom(c, n) > 0, bottom(c, n), slope_ * bottom(c, n));
            } else if (dim == 4) {
                forward_(c, x, y, n) = select(bottom(c, x, y, n) > 0, bottom(c, x, y, n), slope_ * bottom(c, x, y, n));
            }
        } else {
            if (dim == 2) {
                forward_(c, n) = max(bottom(c, n), 0);
            } else if (dim == 4) {
                forward_(c, x, y, n) = max(bottom(c, x, y, n), 0);
            }
        }
    }

public:
    Relu(const std::string &name, float slope)
        : Layer(name), slope_(slope), leaky_(true)
    {}

    Relu(const std::string &name)
        : Relu(name, .0f)
    {}

};


template<typename  T>
class BatchNorm : public Layer
{
protected:
    Buffer<> mean_;
    Buffer<> variance_;

    void setup_param() override
    {
        const int32_t channel = bottom_shape_[0];

        const std::string mean_name = name_ + "_mean";
        mean_ = Buffer<>(type_of<T>(), {channel}, mean_name);

        const std::string variance_name = name_ + "_variance";
        variance_ = Buffer<>(type_of<T>(), {channel}, variance_name);
    }

    void setup_forward(Func bottom) override
    {
        forward_(c, x, y, n) = (bottom(c, x, y, n) - mean_(c)) / (sqrt(variance_(c)) + Expr(.000001f));
    }

public:
    BatchNorm(const std::string &name)
        : Layer(name)
    {}

    void load(std::ifstream& ifs) override
    {
        load_param(mean_, ifs);
        load_param(variance_, ifs);
    }

};



template<typename T>
class Scale : public Layer
{
protected:
    Buffer<> weight_;
    Buffer<> bias_;

    void setup_param() override
    {
        const int32_t channel = bottom_shape_[0];

        const std::string weight_name = name_ + "_weight";
        weight_ = Buffer<>(type_of<T>(), {channel}, weight_name);

        const std::string bias_name = name_ + "_bias";
        bias_ = Buffer<>(type_of<T>(), {channel}, bias_name);
    }

    void setup_forward(Func bottom) override
    {
        forward_(c, x, y, n) = bottom(c, x, y, n) * weight_(c) + bias_(c);
    }

public:
    Scale(const std::string &name)
        : Layer(name)
    {}

    void load(std::ifstream& ifs) override
    {
        load_param(weight_, ifs);
        load_param(bias_, ifs);
    }

};

template<typename T>
class Linear : public Layer
{
protected:
    int32_t output_num_;

    Buffer<> weight_;
    Buffer<> bias_;

    void setup_shape(const std::vector<int32_t>& bottom_shape) override
    {
        const int32_t num = bottom_shape.back();

        bottom_shape_ = bottom_shape;
        top_shape_ = {output_num_, num};
    }

    void setup_param() override
    {
        const size_t dim = bottom_shape_.size();
        std::vector<int32_t> weight_shape;

        if (dim == 2) {
            weight_shape = {bottom_shape_[0], output_num_};
        } else if (dim == 4) {
            weight_shape = {bottom_shape_[0], bottom_shape_[1], bottom_shape_[2], output_num_};
        }

        const std::string weight_name = name_ + "_weight";
        weight_ = Buffer<>(type_of<T>(), weight_shape, weight_name);

        const std::string bias_name = name_ + "_bias";
        bias_ = Buffer<>(type_of<T>(), {output_num_}, bias_name);
    }

    void setup_forward(Func bottom) override
    {
        const int dim = bottom.dimensions();

        if (dim == 2) {
            RDom r(0, bottom_shape_[0]);
            forward_(c, n) = sum(r, bottom(r.x, n) * weight_(r.x, c)) + bias_(c);
        } else if (dim == 4) {
            RDom r(0, bottom_shape_[0], 0, bottom_shape_[1], 0, bottom_shape_[2]);
            forward_(c, n) = sum(r, bottom(r.x, r.y, r.z, n) * weight_(r.x, r.y, r.z, c)) + bias_(c);
        }
    }

public:
    Linear(const std::string &name, int output_num)
        : Layer(name), output_num_(output_num)
    {}

    void load(std::ifstream& ifs) override
    {
        load_param(weight_, ifs);
        load_param(bias_, ifs);
    }

};




class Net {
protected:
    std::vector<Layer> layers_;
    Func output_;

public:
    Net(const std::vector<Layer>& layers)
        : layers_(layers)
    {}

    void setup(Func input, const std::vector<int32_t>& input_shape)
    {
        Func bottom_f = input;
        auto bottom_shape = input_shape;

        for (auto& l : layers_) {
            l.setup(bottom_f, bottom_shape);
            bottom_f = l.forward();
            bottom_shape = l.bottom_shape();
        }

        output_ = bottom_f;
    }

    void load(const std::string& fname)
    {
        std::ifstream ifs(fname.c_str(), std::ios_base::binary);
        if (!ifs.is_open()) {
            throw std::runtime_error("File not found :" + fname);
        }

        for (auto& l : layers_)
        {
            l.load(ifs);
        }
    }

    inline Func output() const { return output_; }
};


} // anonymous
} // Element
} // Halide
