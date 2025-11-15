#ifndef RENDERER_H
#define RENDERER_H
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <vector>
#include <unordered_map>
#include <string>
#include <QString>
class Model; class Camera; class Light;
struct Mesh;
class Renderer : public QOpenGLFunctions {
public:
    Renderer() { }
    Camera* cam{nullptr};
    std::vector<Light*> lights;
    std::vector<Model*> models;
    void initialize(){ initializeOpenGLFunctions(); }
    void renderScene(); 
    void setCamera(Camera* camera) { cam = camera; }
    void setLights(const std::vector<Light*>& l) { lights = l; }
    void addModel(Model* m) { models.push_back(m); }
    void clearModels() { models.clear(); }
    void setViewportSize(int w, int h){ viewportW = (w>0?w:1); viewportH = (h>0?h:1); }
    void clearTextures();
private:
    bool glReady{false};
    QOpenGLShaderProgram program;
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vboTriangle{QOpenGLBuffer::VertexBuffer};
    int viewportW{1}, viewportH{1};
    // Simple texture cache by file path
    std::unordered_map<std::string, unsigned int> textureCache;
    bool bindTextureIfAvailable(const std::string& path);
    unsigned int createTextureFromImage(const QString& qpath);
    void ensureGL();
    void drawTriangle();
    void drawPoints(const std::vector<float>& data, GLenum primitive, int count, const QVector4D& color = QVector4D(1,1,1,1));
    void drawMeshTriangles(const Mesh& mesh,
                           const Model* modelRef,
                           const QMatrix4x4& modelMat,
                           const QMatrix4x4& mvp,
                           const std::vector<QVector3D>& lpos,
                           const std::vector<QVector3D>& lcol,
                           const std::vector<float>& lint);
};
#endif // RENDERER_H
