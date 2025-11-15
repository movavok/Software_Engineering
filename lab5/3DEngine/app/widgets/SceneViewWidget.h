#ifndef SCENEVIEWWIDGET_H
#define SCENEVIEWWIDGET_H
#include <QOpenGLWidget>
#include "../../modules/RenderModule/include/Renderer.h"
#include "../../core/include/Scene.h"
#include <QPoint>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QElapsedTimer>
class SceneViewWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit SceneViewWidget(QWidget* parent=nullptr);
    Scene* scene{nullptr};
    int getFPS() const { return currentFPS; }
    void clearTextures() { renderer.clearTextures(); }
signals:
    void fpsChanged(int fps);
    void fovChanged(float fov);
protected:
    void initializeGL() override;
    void resizeGL(int w,int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
private:
    Renderer renderer;
    QPoint lastPos;
    QElapsedTimer fpsTimer;
    int frameCount{0};
    int currentFPS{0};
    qint64 lastFPSUpdate{0};
};
#endif // SCENEVIEWWIDGET_H
