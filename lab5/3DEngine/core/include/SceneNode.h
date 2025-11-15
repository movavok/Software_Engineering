#ifndef SCENENODE_H
#define SCENENODE_H
#include <vector>
#include <memory>
#include "Vec3.h"
#include "Component.h"
class SceneNode {
public:
    Vec3 position{0,0,0};
    Vec3 rotation{0,0,0};
    Vec3 scale{1,1,1};
    std::vector<std::unique_ptr<Component>> components;
    std::vector<std::unique_ptr<SceneNode>> children;
};
#endif // SCENENODE_H
