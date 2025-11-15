#ifndef SCENE_H
#define SCENE_H
#include <vector>
#include <memory>
#include "Model.h"
#include "Light.h"
#include "Camera.h"
class Scene {
public:
    std::vector<std::unique_ptr<Model>> models;
    std::vector<std::unique_ptr<Light>> lights;
    Camera camera;
    bool addModel(std::unique_ptr<Model> m){ if(models.size()>=50) return false; models.push_back(std::move(m)); return true; }
    bool addLight(std::unique_ptr<Light> l){ if(lights.size()>=10) return false; lights.push_back(std::move(l)); return true; }
    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path) const;
};
#endif // SCENE_H
