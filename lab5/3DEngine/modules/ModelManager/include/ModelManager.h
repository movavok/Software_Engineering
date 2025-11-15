#ifndef MODELMANAGER_H
#define MODELMANAGER_H
#include <memory>
#include <vector>
#include "../../core/include/Model.h"
class ModelManager {
public:
    std::vector<std::unique_ptr<Model>> owned;
    Model* loadModel(const std::string& filename);
    bool saveModel(Model* model,const std::string& filename);
    bool applyTexture(const std::string& textureFile, Model* model);
};
#endif // MODELMANAGER_H
