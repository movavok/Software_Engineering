#ifndef COLOR_H
#define COLOR_H
struct Color { float r{1.f}, g{1.f}, b{1.f}, a{1.f}; static Color White() { return {1.f,1.f,1.f,1.f}; } static Color Black(){return {0.f,0.f,0.f,1.f};}};
#endif // COLOR_H
