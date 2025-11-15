#ifndef LIGHT_H
#define LIGHT_H
#include "Color.h"
#include "Vec3.h"
#include <string>
class Light {
public:
    enum class Type { Point, Directional };
    Type type{Type::Point};
    Vec3 position{0,0,0};
    Vec3 direction{0,-1,0};
    Color color{Color::White()};
    float intensity{1.f};
};
#endif // LIGHT_H
