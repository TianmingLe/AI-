#pragma once
#include <memory>
#include <cstring>
#include <stdexcept>
#include <iostream>

#define CV_8UC1 0
#define CV_8UC3 16
#define CV_8UC4 24

namespace cv {

class Scalar {
public:
    double val[4];
    Scalar() { val[0] = 0; val[1] = 0; val[2] = 0; val[3] = 0; }
    Scalar(double v0, double v1, double v2, double v3 = 0) {
        val[0] = v0; val[1] = v1; val[2] = v2; val[3] = v3;
    }
};

class Mat {
private:
    std::shared_ptr<unsigned char[]> data_ptr_;
    
    int getChannels(int type) const {
        if (type == CV_8UC1) return 1;
        if (type == CV_8UC3) return 3;
        if (type == CV_8UC4) return 4;
        return 3; // fallback to 3 channels for unknown types in this mock
    }

    void allocate(int r, int c, int t) {
        if (r < 0 || c < 0) {
            std::cerr << "cv::Mat warning: negative dimensions (" << r << "x" << c << ")\n";
            rows = 0; cols = 0; type = 0;
            return;
        }
        rows = r; cols = c; type = t;
        if (rows > 0 && cols > 0) {
            try {
                size_t size = static_cast<size_t>(rows) * cols * getChannels(type);
                data_ptr_ = std::shared_ptr<unsigned char[]>(new unsigned char[size]());
            } catch (const std::bad_alloc& e) {
                std::cerr << "cv::Mat allocation failed: " << e.what() << "\n";
                rows = 0; cols = 0; type = 0;
            }
        }
    }

public:
    int rows, cols, type;
    
    Mat() : rows(0), cols(0), type(0) {}
    
    Mat(int _rows, int _cols, int _type) {
        allocate(_rows, _cols, _type);
    }
    
    Mat(int _rows, int _cols, int _type, const Scalar& s) {
        allocate(_rows, _cols, _type);
        // We omit actual scalar filling in this mock for simplicity,
        // but memory is at least zero-initialized.
    }
    
    // Copy constructor: shallow copy using shared_ptr
    Mat(const Mat& other) = default;
    
    // Assignment operator: shallow copy using shared_ptr
    Mat& operator=(const Mat& other) = default;
    
    ~Mat() = default;

    bool empty() const { return rows == 0 || cols == 0 || !data_ptr_; }
    
    // Deep copy using memcpy
    Mat clone() const { 
        Mat m;
        m.rows = rows;
        m.cols = cols;
        m.type = type;
        if (!empty()) {
            try {
                size_t size = static_cast<size_t>(rows) * cols * getChannels(type);
                m.data_ptr_ = std::shared_ptr<unsigned char[]>(new unsigned char[size]);
                std::memcpy(m.data_ptr_.get(), data_ptr_.get(), size);
            } catch (const std::bad_alloc& e) {
                std::cerr << "cv::Mat clone allocation failed: " << e.what() << "\n";
                m.rows = 0; m.cols = 0; m.type = 0;
            }
        }
        return m;
    }
    
    unsigned char* ptr() { return data_ptr_ ? data_ptr_.get() : nullptr; }
    const unsigned char* ptr() const { return data_ptr_ ? data_ptr_.get() : nullptr; }
};

class Rect {
public:
    int x, y, width, height;
    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(int _x, int _y, int _width, int _height) 
        : x(_x), y(_y), width(_width), height(_height) {}
};

} // namespace cv
