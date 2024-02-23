#include <fft.h>

#include <fftw3.h>

#include <glog/logging.h>
#include <cmath>
#include <cstddef>
#include <memory>

class DctCalculator::Impl {
public:
    Impl() = delete;

    Impl(size_t width, std::vector<double> *input, std::vector<double> *output)
        : plan_(fftw_plan_r2r_2d(width, width, input->data(), output->data(), FFTW_REDFT01,
                                 FFTW_REDFT01, 0)),
          width_(width),
          input_(input) {
    }

    void Execute() {
        for (size_t i = 0; i < width_; ++i) {
            for (size_t j = 0; j < width_; ++j) {
                if (i > 0) {
                    (*input_)[i * width_ + j] /= 2;
                } else {
                    (*input_)[i * width_ + j] /= std::sqrt(2);
                }
                if (j > 0) {
                    (*input_)[i * width_ + j] /= 2;
                } else {
                    (*input_)[i * width_ + j] /= std::sqrt(2);
                }
                (*input_)[i * width_ + j] /= 4;
            }
        }
        fftw_execute(plan_);
    }

    ~Impl() {
        fftw_destroy_plan(plan_);
    }

private:
    fftw_plan plan_;
    size_t width_;
    std::vector<double> *input_;
};

DctCalculator::DctCalculator(size_t width, std::vector<double> *input,
                             std::vector<double> *output) {
    if (width * width != input->size() || input->size() != output->size()) {
        DLOG(ERROR) << "Incorrect size fft\n";
        throw std::invalid_argument("");
    }
    impl_ = std::make_unique<Impl>(width, input, output);
}

void DctCalculator::Inverse() {
    impl_->Execute();
}

DctCalculator::~DctCalculator() = default;
