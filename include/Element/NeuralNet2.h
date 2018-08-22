#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include <Halide.h>

#include "Util.h"

namespace Halide {
namespace Element {

namespace {


void dprint(int thresh, const char* msg)
{
    const char* debug_env = std::getenv("HE_NN_DEBUG");
    const int level = std::atoi(debug_env);

    if (level >= thresh) {
        std::cerr << msg;
    }
}

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

std::vector<int32_t> extents_from_buffer(const Buffer<>& buffer)
{
    const auto buffer_ptr = buffer.get();
    const int dim = buffer_ptr->dimensions();

    std::vector<int32_t> e(dim);

    for (int i = 0; i < dim; i++) {
        e[i] = buffer_ptr->extent(i);
    }

    return e;
}

std::size_t rest_byte(std::ifstream& ifs)
{
    std::ifstream::pos_type beg = ifs.tellg();

    ifs.seekg(0, std::ifstream::end);
    std::ifstream::pos_type end = ifs.tellg();

    ifs.seekg(beg);

    std::size_t rest = end - beg;

    return rest;
}

template<typename T>
void load_param(Buffer<>& param, std::ifstream& ifs)
{
    try {
        uint32_t dim;
        ifs.read(reinterpret_cast<char*>(&dim), sizeof(dim));

        std::vector<int32_t> extents(dim);
        for (size_t i=0; i<dim; i++) {
            uint32_t e;
            ifs.read(reinterpret_cast<char*>(&e), sizeof(e));
            extents[i] = static_cast<int32_t>(e);
        }

        const auto expect_extents = extents_from_buffer(param);
        throw_assert(extents == expect_extents,
                     format("Unexpected extents: expect = %s, actual = %s", vector_str(expect_extents).c_str(), vector_str(extents).c_str()).c_str());

        std::size_t buf_size_in_byte = std::accumulate(extents.begin(), extents.end(), sizeof(T), std::multiplies<int32_t>());

        std::size_t rest = rest_byte(ifs);
        throw_assert(buf_size_in_byte <= rest,
                     format("Oversize read: rest = %d[Byte], read = %d[Byte]", rest, buf_size_in_byte).c_str());

        ifs.read(reinterpret_cast<char*>(param.data()), buf_size_in_byte);

    } catch (const std::exception& e) {
        std::cerr << "Loading parameter error : " << param.name() << std::endl;
        std::cerr << "  " << e.what() << std::endl;
        throw std::runtime_error("");
    }
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

    virtual void setup_schedule() { /* Do nothing */ }

    virtual std::string layer_kind() const { return "Layer"; }
    virtual void print_param() const { /* Do nothing */ }

public:
    explicit Layer(const std::string& name)
        : name_(name), forward_(Func(name))
    {}

    void setup(Func& bottom_f, const std::vector<int32_t>& bottom_shape)
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

    virtual void layer_schedule()
    {
        schedule(forward_, expr_shape(top_shape_));
    }

    void print_info() const
    {
        std::cout << format("%10s  (%10s)  :  %20s  ->  %20s",
                            name_.c_str(), layer_kind().c_str(), vector_str(bottom_shape_).c_str(), vector_str(top_shape_).c_str());
        print_param();
        std::cout << std::endl;
    }

    virtual void load(std::ifstream& ifs) { /* Do nothing */ }
    virtual bool is_stencil() const { return false; }

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

    Func clamped_;

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
        weight_ = Buffer<>(type_of<float>(), {bottom_shape_[0], kernel_shape_[0], kernel_shape_[1], kernel_num_}, name_ + "_weight");
        bias_ = Buffer<>(type_of<float>(), {kernel_num_}, name_ + "_bias");
    }

    void setup_forward(Func& bottom) override
    {
        RDom r(0, bottom_shape_[0], 0, kernel_shape_[0], 0, kernel_shape_[1]);

        clamped_ = BoundaryConditions::constant_exterior(bottom, Expr(0.0f), 0, bottom_shape_[0], 0, bottom_shape_[1], 0, bottom_shape_[2]);

        if (use_bias_) {
            forward_(c, x, y, n) = sum(r, clamped_(r.x, x*strides_[0] - pads_[0] + r.y, y*strides_[1] - pads_[1] + r.z, n) * weight_(r.x, r.y, r.z, c))
                + bias_(c);
        } else {
            forward_(c, x, y, n) = sum(r, clamped_(r.x, x*strides_[0] - pads_[0] + r.y, y*strides_[1] - pads_[1] + r.z, n) * weight_(r.x, r.y, r.z, c));
        }
    }

    std::string layer_kind() const override { return "Conv"; }
    void print_param() const override
    {
        std::cout << format("    kernel = %s,  stride = %s,  pad = %s",
                            vector_str(kernel_shape_).c_str(), vector_str(strides_).c_str(), vector_str(pads_).c_str());
    }

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

    bool is_stencil() const override { return true; }

};


class BinConv : public Conv
{
protected:
    Buffer<> alpha_;

    void setup_param() override
    {
        weight_ = Buffer<>(type_of<bool>(), {bottom_shape_[0], kernel_shape_[0], kernel_shape_[1], kernel_num_}, name_ + "_weight");
        alpha_ = Buffer<>(type_of<float>(), {kernel_num_}, name_ + "_alpha");
        bias_ = Buffer<>(type_of<float>(), {kernel_num_}, name_ + "_bias");
    }

    void setup_forward(Func& bottom) override
    {
        RDom r(0, bottom_shape_[0], 0, kernel_shape_[0], 0, kernel_shape_[1]);

        clamped_ = BoundaryConditions::constant_exterior(bottom, Expr(0), 0, bottom_shape_[0], 0, bottom_shape_[1], 0, bottom_shape_[2]);

        const float elem_num = static_cast<float>(bottom_shape_[0] * kernel_shape_[0] * kernel_shape_[1]);

        // TODO : use_bias
        if (pads_[0] == 0 && pads_[1] == 0) {
            forward_(c, x, y, n) = (-elem_num +
                             2 * sum(cast<float>(!clamped_(r.x, x*strides_[0] - pads_[0] + r.y, y*strides_[1] - pads_[1] + r.z, n) ^
                                                 weight_(r.x, r.y, r.z, c)))) *
                alpha_(c) + bias_(c);
        } else {
            Expr tx = x*strides_[0] - pads_[0] + r.y;
            Expr ty = y*strides_[1] - pads_[1] + r.z;

            Expr iw = !clamped_(r.x, tx, ty, n) ^ weight_(r.x, r.y, r.z, c);
            forward_(c, x, y, n) = sum(r, select(tx >= 0 && tx < bottom_shape_[1] && ty >= 0 && ty < bottom_shape_[2],
                                          cast<float>(select(iw, 1, -1)),
                                          cast<float>(0))) * alpha_(c) + bias_(c);
        }
    }

    std::string layer_kind() const override { return "BinConv"; }

    void load(std::ifstream& ifs) override
    {
        load_param<bool>(weight_, ifs);
        load_param<float>(alpha_, ifs);
        load_param<float>(bias_, ifs);
    }

public:
    using Conv::Conv;
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

    std::string layer_kind() const override { return "Pool"; }
    void print_param() const override
    {
        std::cout << format("    window = %s,  stride = %s,  pad = %s",
                            vector_str(window_shape_).c_str(), vector_str(strides_).c_str(), vector_str(pads_).c_str());
    }

public:
    Pool(const std::string &name, const std::vector<int32_t>& window_shape, const std::vector<int32_t>& strides, const std::vector<int32_t>& pads)
        : Layer(name), window_shape_(window_shape), strides_(strides), pads_(pads)
    {}

    Pool(const std::string &name, int window_size, int stride)
        : Pool(name, {window_size, window_size}, {stride, stride}, {0, 0})
    {}

    bool is_stencil() const override { return true; }

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

    std::string layer_kind() const override { return "Relu"; }

public:
    Relu(const std::string &name, float slope)
        : Layer(name), slope_(slope), leaky_(true)
    {}

    Relu(const std::string &name)
        : Relu(name, .0f)
    {}

};

class BinActive : public Layer
{
    void setup_forward(Func& bottom) override
    {
        int dim = bottom.dimensions();

        if (dim == 2) {
            forward_(c, n) = bottom(c, n) >= 0;
        } else if (dim == 4) {
            forward_(c, x, y, n) = bottom(c, x, y, n) >= 0;
        }
    }

    std::string layer_kind() const override { return "BinActive"; }

public:
    BinActive(const std::string &name)
        : Layer(name)
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

        mean_ = Buffer<>(type_of<float>(), {channel}, name_ + "_mean");
        variance_ = Buffer<>(type_of<float>(), {channel}, name_ + "_variance");
    }

    void setup_forward(Func& bottom) override
    {
        forward_(c, x, y, n) = (bottom(c, x, y, n) - mean_(c)) / (sqrt(variance_(c)) + Expr(.000001f));
    }

    std::string layer_kind() const override { return "BatchNorm"; }

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

    std::string layer_kind() const override { return "Scale"; }

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

        weight_ = Buffer<>(type_of<float>(), weight_shape, name_ + "_weight");
        bias_ = Buffer<>(type_of<float>(), {output_num_}, name_ + "_bias");
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

    std::string layer_kind() const override { return "Linear"; }

public:
    Linear(const std::string &name, int output_num)
        : Layer(name), output_num_(output_num)
    {}

    void load(std::ifstream& ifs) override
    {
        load_param<float>(weight_, ifs);
        load_param<float>(bias_, ifs);
    }

    bool is_stencil() const override { return true; }

};

class BinLinear : public Linear
{
protected:
    Buffer<> alpha_;

    void setup_param() override
    {
        const size_t dim = bottom_shape_.size();
        std::vector<int32_t> weight_shape;

        if (dim == 2) {
            weight_shape = {bottom_shape_[0], output_num_};
        } else if (dim == 4) {
            weight_shape = {bottom_shape_[0], bottom_shape_[1], bottom_shape_[2], output_num_};
        }

        weight_ = Buffer<>(type_of<bool>(), weight_shape, name_ + "_weight");
        alpha_ = Buffer<>(type_of<float>(), {output_num_}, name_ + "_alpha");
        bias_ = Buffer<>(type_of<float>(), {output_num_}, name_ + "_bias");
    }

    void setup_forward(Func& bottom) override
    {
        const int dim = bottom.dimensions();

        // TODO: use_bias
        if (dim == 2) {
            RDom r(0, bottom_shape_[0]);
            const float elem_num = static_cast<float>(bottom_shape_[0]);
            forward_(c, n) = (-elem_num + 2 * sum(r, cast<float>(!bottom(r.x, n) ^ weight_(r.x, c)))) * alpha_(c) + bias_(c);
        } else if (dim == 4) {
            RDom r(0, bottom_shape_[0], 0, bottom_shape_[1], 0, bottom_shape_[2]);
            const float elem_num = static_cast<float>(bottom_shape_[0] * bottom_shape_[1] * bottom_shape_[2]);
            forward_(c, n) = (-elem_num + 2 * sum(r, cast<float>(!bottom(r.x, r.y, r.z, n) ^ weight_(r.x, r.y, r.z, c)))) * alpha_(c) + bias_(c);
        }
    }

    std::string layer_kind() const override { return "BinLinear"; }

public:
    using Linear::Linear;

    void load(std::ifstream& ifs) override
    {
        load_param<bool>(weight_, ifs);
        load_param<float>(alpha_, ifs);
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

    std::string layer_kind() const override { return "Softmax"; }

public:
    Softmax(const std::string &name)
        : Layer(name)
    {}

    bool is_stencil() const override { return true; }

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
        } else if (type == "BinConv") {
            return create_<BinConv>(std::forward<Args>(params)...);
        } else if (type == "Pool") {
            return create_<Pool>(std::forward<Args>(params)...);
        } else if (type == "Relu") {
            return create_<Relu>(std::forward<Args>(params)...);
        } else if (type == "BinActive") {
            return create_<BinActive>(std::forward<Args>(params)...);
        } else if (type == "BatchNorm") {
            return create_<BatchNorm>(std::forward<Args>(params)...);
        } else if (type == "Scale") {
            return create_<Scale>(std::forward<Args>(params)...);
        } else if (type == "Linear") {
            return create_<Linear>(std::forward<Args>(params)...);
        } else if (type == "BinLinear") {
            return create_<BinLinear>(std::forward<Args>(params)...);
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

    void setup_schedule()
    {
        for (size_t i = 0; i < layers_.size()-1; i++) {
            auto l = layers_[i];
            auto succ = layers_[i+1];

            if (succ->is_stencil()) {
                l->layer_schedule();
            }
        }

        auto last = layers_.back();
        last->layer_schedule();
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

        setup_schedule();
    }

    void load(const std::string& fname)
    {
        std::ifstream ifs(fname.c_str(), std::ios_base::binary);
        if (!ifs.is_open()) {
            throw std::runtime_error("File not found :" + fname);
        }

        uint32_t param_num;
        ifs.read(reinterpret_cast<char*>(&param_num), sizeof(param_num));

        for (auto& l : layers_)
        {
            l->load(ifs);
        }
    }

    void print_info() const {
        for (size_t i = 0; i < layers_.size(); i++) {
            layers_[i]->print_info();
        }
    }

    inline Func output() const { return output_; }
};


} // anonymous
} // Element
} // Halide
