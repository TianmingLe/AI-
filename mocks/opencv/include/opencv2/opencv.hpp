#pragma once

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
public:
    int rows, cols, type;
    Mat() : rows(0), cols(0), type(0) {}
    Mat(int _rows, int _cols, int _type, const Scalar& s)
        : rows(_rows), cols(_cols), type(_type) {}

    bool empty() const { return rows == 0 || cols == 0; }
    Mat clone() const { return *this; }
};

class Rect {
public:
    int x, y, width, height;
    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(int _x, int _y, int _width, int _height) 
        : x(_x), y(_y), width(_width), height(_height) {}
};

} // namespace cv

#define CV_8UC3 16
