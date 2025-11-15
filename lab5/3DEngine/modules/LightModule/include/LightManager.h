#ifndef LIGHTMANAGER_H
#define LIGHTMANAGER_H
#include "../../core/include/Light.h"
#include "../../core/include/Color.h"
#include "../../core/include/Vec3.h"
#include <memory>
#include <vector>
class LightManager {
public:
    std::vector<std::unique_ptr<Light>> lights;
    
    // Існуючі методи
    Light* addLight();
    void configure(Light* l, float intensity, const Color& color);
    void remove(Light* l);
    std::vector<Light*> all() const;
    
    // Нові зручні методи
    Light* addPointLight(const Vec3& position, float intensity, const Color& color);
    void setDefaultColor(const Color& color) { defaultColor = color; }
    void setDefaultIntensity(float intensity) { defaultIntensity = intensity; }
    Color getDefaultColor() const { return defaultColor; }
    float getDefaultIntensity() const { return defaultIntensity; }
    
private:
    Color defaultColor{1.0f, 1.0f, 1.0f, 1.0f};
    float defaultIntensity{1.0f};
};
#endif // LIGHTMANAGER_H
