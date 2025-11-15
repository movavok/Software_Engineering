#ifndef MESH_H
#define MESH_H
#include <vector>
#include "Vec3.h"
struct Mesh { std::vector<Vec3> vertices; std::vector<unsigned> indices; };
#endif // MESH_H
