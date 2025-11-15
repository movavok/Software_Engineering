#include "ModelManager.h"
#include <fstream>
Model* ModelManager::loadModel(const std::string& filename){ auto m = std::make_unique<Model>(); m->name = filename; owned.push_back(std::move(m)); return owned.back().get(); }
bool ModelManager::saveModel(Model* model,const std::string& filename){ std::ofstream f(filename); if(!f) return false; f<<"MODEL "<<model->name; return true; }
bool ModelManager::applyTexture(const std::string& textureFile, Model* model){ model->texture.file = textureFile; model->texture.loaded = true; return true; }
