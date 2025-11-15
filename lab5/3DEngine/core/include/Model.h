#ifndef MODEL_H
#define MODEL_H
#include <vector>
#include <string>
#include "Mesh.h"
#include "Material.h"
#include "Texture.h"
class Model {
public:
    std::string name;
    std::vector<Mesh> meshes;
    Material material; 
    Texture texture; 
};
#endif // MODEL_H
