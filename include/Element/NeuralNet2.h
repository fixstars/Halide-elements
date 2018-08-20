#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include <Halide.h>

namespace Halide {
namespace Element {

namespace {

std::string vector_str(const std::vector<int32_t>& vec)
{
    if (vec.size() == 0) return "";

    std::string str = "(";

    for (size_t i = 0; i < vec.size(); i++) {
        str += std::to_string(vec[i]);
        if (i < vec.size()-1) {
            str += ", ";
        }
    }

    str += ")";

    return str;
}

template<typename T>
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

    std::size_t buf_size_in_byte = std::accumulate(extents.begin(), extents.end(), sizeof(T), std::multiplies<int32_t>());

    std::cerr << "  " << param.name() << " : " << dim << " " << vector_str(extents) << std::endl;

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

    static const std::vector<Expr> expr_shape(const std::vector<int32_t>& shape)
    {
        std::vector<Expr> e_shape(shape.size());

        for (size_t i = 0; i < e_shape.size(); i++) {
            e_shape[i] = shape[i];
        }

        return e_shape;
    }

    virtual void setup_shape(const std::vector<int32_t>& bottom_shape)
    {
        top_shape_ = bottom_shape_ = bottom_shape;
    }

    virtual void setup_param() { /* Do nothing */ }

    virtual void setup_forward(Func& bottom)
    {
        const int dim = bottom.dimensions();

        if (dim == 2) {
            forward_(c, n) = bottom(c, n);
        } else if (dim == 4) {
            forward_(c, x, y, n) = bottom(c, x, y, n);
        }
    }

    virtual void setup_schedule() { /* Do nothing */
        schedule(forward_, expr_shape(top_shape_));
    }

public:
    explicit Layer(const std::string& name)
        : name_(name), forward_(Func(name))
    {}

    virtual void setup(Func& bottom_f, const std::vector<int32_t>& bottom_shape)
    {
        setup_shape(bottom_shape);
        setup_param();
        setup_forward(bottom_f);
        setup_schedule();
    }

    virtual void setup(const Layer& bottom)
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
        weight_ = Buffer<>(type_of<float>(), weight_shape, weight_name);

        const std::string bias_name = name_ + "_bias";
        const std::vector<int32_t> bias_shape = {kernel_num_};
        bias_ = Buffer<>(type_of<float>(), bias_shape, bias_name);
    }

    void setup_forward(Func& bottom) override
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

    // void setup_schedule() override
    // {
    //     // schedule(clamped, bottom_shape_);
    // }

    void load(std::ifstream& ifs) override
    {
        load_param<float>(weight_, ifs);
        load_param<float>(bias_, ifs);
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
        : Conv(name, kernel_size, kernel_num, 1, kernel_size/2, true)
    {}

};


class Pool : public Layer
{
protected:
    std::vector<int32_t> window_shape_;
    std::vector<int32_t> strides_;
    std::vector<int32_t> pads_;

    Func clamped_;

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

    void setup_forward(Func& bottom) override
    {
        RDom r(0, window_shape_[0], 0, window_shape_[1]);

        clamped_ = BoundaryConditions::constant_exterior(bottom, Float(32).min(), 0, bottom_shape_[0], 0, bottom_shape_[1], 0, bottom_shape_[2]);

        forward_(c, x, y, n) = maximum(r, clamped_(c, x*strides_[0] - pads_[0] + r.x, y*strides_[1] - pads_[1] + r.y, n));
    }

    // void setup_schedule() override
    // {
    //     // schedule(clamped, bottom_shape_);
    // }


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

    void setup_forward(Func& bottom) override
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


class BatchNorm : public Layer
{
protected:
    Buffer<> mean_;
    Buffer<> variance_;

    void setup_param() override
    {
        const int32_t channel = bottom_shape_[0];

        const std::string mean_name = name_ + "_mean";
        mean_ = Buffer<>(type_of<float>(), {channel}, mean_name);

        const std::string variance_name = name_ + "_variance";
        variance_ = Buffer<>(type_of<float>(), {channel}, variance_name);
    }

    void setup_forward(Func& bottom) override
    {
        forward_(c, x, y, n) = (bottom(c, x, y, n) - mean_(c)) / (sqrt(variance_(c)) + Expr(.000001f));
    }

public:
    BatchNorm(const std::string &name)
        : Layer(name)
    {}

    void load(std::ifstream& ifs) override
    {
        load_param<float>(mean_, ifs);
        load_param<float>(variance_, ifs);
    }

};


class Scale : public Layer
{
protected:
    Buffer<> weight_;
    Buffer<> bias_;

    void setup_param() override
    {
        const int32_t channel = bottom_shape_[0];

        const std::string weight_name = name_ + "_weight";
        weight_ = Buffer<>(type_of<float>(), {channel}, weight_name);

        const std::string bias_name = name_ + "_bias";
        bias_ = Buffer<>(type_of<float>(), {channel}, bias_name);
    }

    void setup_forward(Func& bottom) override
    {
        forward_(c, x, y, n) = bottom(c, x, y, n) * weight_(c) + bias_(c);
    }

public:
    Scale(const std::string &name)
        : Layer(name)
    {}

    void load(std::ifstream& ifs) override
    {
        load_param<float>(weight_, ifs);
        load_param<float>(bias_, ifs);
    }

};

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
        weight_ = Buffer<>(type_of<float>(), weight_shape, weight_name);

        const std::string bias_name = name_ + "_bias";
        bias_ = Buffer<>(type_of<float>(), {output_num_}, bias_name);
    }

    void setup_forward(Func& bottom) override
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
        load_param<float>(weight_, ifs);
        load_param<float>(bias_, ifs);
    }

};

class Softmax : public Layer
{
protected:
    Func mins_{"mins"};
    Func norm_{"norm"};
    Func d_{"d"};

    void setup_forward(Func& bottom) override
    {
        RDom r(0, bottom_shape_[0]);
        const int dim = bottom.dimensions();

        if (dim == 2) {
            mins_(n) = minimum(r, bottom(r.x, n));
            norm_(c, n) = exp(bottom(c, n) - mins_(n));
            d_(n) = sum(r, norm_(r.x, n));
            forward_(c, n) = norm_(c, n) / d_(n);
        } else if (dim == 4) {
            mins_(x, y, n) = minimum(r, bottom(r.x, x, y, n));
            norm_(c, x, y, n) = exp(bottom(c, x, y, n) - mins_(x, y, n));
            d_(x, y, n) = sum(r, norm_(r.x, x, y, n));
            forward_(c, x, y, n) = norm_(c, x, y, n) / d_(x, y, n);
        }
    }

    void setup_schedule() override
    {
        const int dim = forward_.dimensions();
        if (dim == 2) {
            schedule(mins_, {bottom_shape_[1]});
            schedule(norm_, expr_shape(bottom_shape_));
            schedule(d_, {bottom_shape_[1]});
        } else if (dim == 4) {
            schedule(mins_, {bottom_shape_[1], bottom_shape_[2], bottom_shape_[3]});
            schedule(norm_, expr_shape(bottom_shape_));
            schedule(d_, {bottom_shape_[1], bottom_shape_[2], bottom_shape_[3]});
        }

        schedule(forward_, expr_shape(top_shape_));
    }

public:
    Softmax(const std::string &name)
        : Layer(name)
    {}
};


// TODO: Use register
class LayerFactory
{
private:
    template<class T, class... Args>
    static typename std::enable_if<std::is_constructible<T, Args...>::value, std::unique_ptr<T>>::type
    create_(Args&& ...args)
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    template<class T, class... Args>
    static typename std::enable_if<!std::is_constructible<T, Args...>::value, std::unique_ptr<T>>::type
    create_(...)
    {
        std::runtime_error("Invalid argmuent types.");
    }

public:
    template<class... Args>
    std::unique_ptr<Layer> create(const std::string& type, Args&&... params)
    {
        if (type == "Conv") {
            return create_<Conv>(std::forward<Args>(params)...);
        } else if (type == "Pool") {
            return create_<Pool>(std::forward<Args>(params)...);
        } else if (type == "Relu") {
            return create_<Relu>(std::forward<Args>(params)...);
        } else if (type == "BatchNorm") {
            return create_<BatchNorm>(std::forward<Args>(params)...);
        } else if (type == "Scale") {
            return create_<Scale>(std::forward<Args>(params)...);
        } else if (type == "Linear") {
            return create_<Linear>(std::forward<Args>(params)...);
        } else if (type == "Softmax") {
            return create_<Softmax>(std::forward<Args>(params)...);
        }

        std::runtime_error("Unknown layer : " + type);
    }

};




class Net {
protected:
    std::string name_;
    std::vector<std::shared_ptr<Layer>> layers_;
    Func output_;

    LayerFactory factory_;

public:
    Net(const std::string& name)
        : name_(name)
    {}

    template<class... Args>
    void add_layer(const std::string& type, Args&& ...params)
    {
        layers_.emplace_back(factory_.create(type, std::forward<Args>(params)...));
    }

    void setup(Func input, const std::vector<int32_t>& input_shape)
    {
        Func& bottom_f = input;
        auto bottom_shape = input_shape;

        for (size_t i = 0; i < layers_.size(); i++)  {
            auto l = layers_[i];
            l->setup(bottom_f, bottom_shape);

            bottom_f = l->forward();
            bottom_shape = l->top_shape();
        }

        output_ = bottom_f;
    }

    void load(const std::string& fname)
    {
        std::ifstream ifs(fname.c_str(), std::ios_base::binary);
        if (!ifs.is_open()) {
            throw std::runtime_error("File not found :" + fname);
        }

        uint32_t param_num;
        ifs.read(reinterpret_cast<char*>(&param_num), sizeof(param_num));
        std::cerr << "Loading parameters:" << std::endl;
        std::cerr << param_num << std::endl;

        for (auto& l : layers_)
        {
            l->load(ifs);
        }
    }

    inline Func output() const { return output_; }
};


} // anonymous
} // Element
} // Halide
