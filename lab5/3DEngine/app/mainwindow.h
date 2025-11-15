#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "widgets/SceneViewWidget.h"
#include "../core/include/Scene.h"
#include "../modules/ModelManager/include/ModelManager.h"
#include "../modules/CameraModule/include/CameraController.h"
#include "../modules/LightModule/include/LightManager.h"
#include <QColor>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void loadSceneFile(const QString& path);
private:
    QString findDefaultScenePath() const;
private:
    Ui::MainWindow *ui; 
    SceneViewWidget* view{nullptr};
    Scene scene;
    ModelManager modelManager;
    CameraController cameraController;
    LightManager lightManager;
    QColor currentLightColor{255,255,255};
    double currentLightIntensity{1.0};
private slots:
    void updateFPSLabel(int fps);
    void updateZoomLabel(float fov);
};
#endif // MAINWINDOW_H
