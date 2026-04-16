#pragma once

namespace perception {

class KalmanFilter1D {
public:
    KalmanFilter1D(double q, double r, double p, double initial_value) : q_(q), r_(r), p_(p), x_(initial_value) {}

    double update(double measurement) {
        p_ = p_ + q_;
        double k = p_ / (p_ + r_);
        x_ = x_ + k * (measurement - x_);
        p_ = (1.0 - k) * p_;
        return x_;
    }

    void reset(double q, double r, double p, double initial_value) {
        q_ = q;
        r_ = r;
        p_ = p;
        x_ = initial_value;
    }

    double state() const { return x_; }

private:
    double q_;
    double r_;
    double p_;
    double x_;
};

} // namespace perception
