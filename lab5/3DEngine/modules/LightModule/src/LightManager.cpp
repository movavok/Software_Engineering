#include "LightManager.h"
#include <algorithm>

Light* LightManager::addLight() {
    auto l = std::make_unique<Light>();
    lights.push_back(std::move(l));
    return lights.back().get();
}

void LightManager::configure(Light* l, float intensity, const Color& color) {
    if(l) {
        l->intensity = intensity;
        l->color = color;
    }
}

void LightManager::remove(Light* l) {
    lights.erase(
        std::remove_if(lights.begin(), lights.end(),
            [l](const auto& ptr) { return ptr.get() == l; }),
        lights.end()
    );
}

std::vector<Light*> LightManager::all() const {
    std::vector<Light*> out;
    out.reserve(lights.size());
    for(const auto& x : lights) {
        out.push_back(x.get());
    }
    return out;
}

Light* LightManager::addPointLight(const Vec3& position, float intensity, const Color& color) {
    auto l = std::make_unique<Light>();
    l->type = Light::Type::Point;
    l->position = position;
    l->intensity = intensity;
    l->color = color;
    lights.push_back(std::move(l));
    return lights.back().get();
}