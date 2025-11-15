#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVBoxLayout>
#include <QFileDialog>
#include <QColorDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QtMath>

namespace {
struct Basis { QVector3D f, r, u; };

static Vec3 toVec3(const QVector3D& v) { return {v.x(), v.y(), v.z()}; }
static Basis cameraBasis(const Camera& cam){
    const float yawRad = qDegreesToRadians(cam.yaw);
    const float pitchRad = qDegreesToRadians(cam.pitch);
    const float cy = std::cos(yawRad), sy = std::sin(yawRad);
    const float cp = std::cos(pitchRad), sp = std::sin(pitchRad);
    QVector3D f(cp*cy, sp, cp*sy);
    f.normalize();
    QVector3D worldUp(0,1,0);
    QVector3D r = QVector3D::crossProduct(f, worldUp);
    if(r.lengthSquared() < 1e-6f) r = QVector3D(1,0,0);
    else r.normalize();
    QVector3D u = QVector3D::crossProduct(r, f);
    u.normalize();
    return {f,r,u};
}
static void addModelWithMesh(Scene& scene, const QString& name, const std::vector<Vec3>& verts, const std::vector<unsigned>& idx){
    auto m = std::make_unique<Model>();
    m->name = name.toStdString();
    Mesh mesh; mesh.vertices = verts; mesh.indices = idx;
    m->meshes.push_back(std::move(mesh));
    scene.addModel(std::move(m));
}

static QVector3D forwardCenter(const Camera& cam, float dist){
    auto B = cameraBasis(cam);
    QVector3D pos(cam.position.x, cam.position.y, cam.position.z);
    return pos + B.f * dist;
}
static void addTriangleSample(Scene& scene){
    const auto B = cameraBasis(scene.camera);
    QVector3D center = forwardCenter(scene.camera, 5.0f);
    float sx=0.6f, sy=0.5f, su=0.6f;
    std::vector<Vec3> verts = {
        toVec3(center + (-B.r*sx - B.u*sy)),
        toVec3(center + ( B.r*sx - B.u*sy)),
        toVec3(center + ( B.u*su))
    };
    addModelWithMesh(scene, "Triangle", verts, {0,1,2});
}
static void addCubeSample(Scene& scene){
    const auto B = cameraBasis(scene.camera);
    QVector3D c = forwardCenter(scene.camera, 3.0f); float h=0.5f;
    QVector3D rx = B.r*h, uy = B.u*h, fz = B.f*h;
    QVector3D v[8] = {
        c - rx - uy - fz,
        c + rx - uy - fz,
        c + rx + uy - fz,
        c - rx + uy - fz,
        c - rx - uy + fz,
        c + rx - uy + fz,
        c + rx + uy + fz,
        c - rx + uy + fz
    };
    std::vector<Vec3> verts; verts.reserve(8);
    for(int i=0;i<8;i++) verts.push_back(toVec3(v[i]));
    std::vector<unsigned> idx = {
        0,1,2, 0,2,3,
        4,6,5, 4,7,6,
        0,4,5, 0,5,1,
        3,2,6, 3,6,7,
        0,3,7, 0,7,4,
        1,5,6, 1,6,2
    };
    addModelWithMesh(scene, "Cube", verts, idx);
}
static void addPyramidSample(Scene& scene){
    QVector3D center = forwardCenter(scene.camera, 3.0f);
    float baseSize = 1.0f; float half = baseSize * 0.5f; float height = 0.9f;
        QVector3D wx(1,0,0), wz(0,0,1), wy(0,1,0);
        QVector3D hr = wx * half; QVector3D hf = wz * half;
        QVector3D p0 = center - hr - hf;
        QVector3D p1 = center + hr - hf;
        QVector3D p2 = center + hr + hf;
        QVector3D p3 = center - hr + hf;
        QVector3D apex = center + wy * height;
    std::vector<Vec3> verts = {
        toVec3(p0), toVec3(p1), toVec3(p2), toVec3(p3), toVec3(apex)
    };
    std::vector<unsigned> idx = { 0,1,2, 0,2,3, 0,1,4, 1,2,4, 2,3,4, 3,0,4 };
    addModelWithMesh(scene, "Pyramid", verts, idx);
}
static void addSphereSample(Scene& scene){
    const auto B = cameraBasis(scene.camera);
    QVector3D center = forwardCenter(scene.camera, 3.0f);
    float radius = 0.7f; int stacks = 12; int slices = 18;
    std::vector<Vec3> verts; verts.reserve((stacks+1)*(slices+1));
    for(int i=0;i<=stacks;i++){
        float phi = M_PI * (float)i / (float)stacks;
        float y = std::cos(phi); float sinphi = std::sin(phi);
        for(int j=0;j<=slices;j++){
            float theta = 2.0f * M_PI * (float)j / (float)slices;
            float x = std::cos(theta) * sinphi;
            float z = std::sin(theta) * sinphi;
            QVector3D pos3 = center + B.r*(radius*x) + B.u*(radius*y) + B.f*(radius*z);
            verts.push_back(toVec3(pos3));
        }
    }
    std::vector<unsigned> idx;
    for(int i=0;i<stacks;i++){
        for(int j=0;j<slices;j++){
            int first = i*(slices+1) + j;
            int second = first + (slices+1);
            idx.push_back(first); idx.push_back(second); idx.push_back(first+1);
            idx.push_back(second); idx.push_back(second+1); idx.push_back(first+1);
        }
    }
    addModelWithMesh(scene, "Sphere", verts, idx);
}
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    
    // Create view as child of placeholder and make it fill the space
    view = new SceneViewWidget(ui->sceneViewPlaceholder);
    view->scene = &scene;
    cameraController.setCamera(&scene.camera);
    cameraController.setInitialPosition(4, 3, 4, -135.0f, -20.0f, 60.0f);
    cameraController.reset();
    
    // Make view fill entire placeholder
    auto* layout = new QVBoxLayout(ui->sceneViewPlaceholder);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(view);
    
    // Raise labels to appear on top of view
    ui->labelFPS->raise();
    ui->labelZoom->raise();
    
    // Connect FPS and Zoom updates
    connect(view, &SceneViewWidget::fpsChanged, this, &MainWindow::updateFPSLabel);
    connect(view, &SceneViewWidget::fovChanged, this, &MainWindow::updateZoomLabel);
    float initialZoom = 60.0f / scene.camera.fov;
    ui->labelZoom->setText(QString("Zoom: x%1").arg(initialZoom, 0, 'f', 1));

    const auto def = findDefaultScenePath();
    if(!def.isEmpty()) loadSceneFile(def);
    if(scene.models.empty()){
        addTriangleSample(scene);
        view->update();
    }

    connect(ui->actionLoad_scene, &QAction::triggered, this, [this]{
        auto file = QFileDialog::getOpenFileName(this, tr("Open Scene"), QString(), tr("Scene Files (*.scene);;All Files (*.*)"));
        if(!file.isEmpty()) loadSceneFile(file);
    });
    connect(ui->actionDefault_scene, &QAction::triggered, this, [this]{
        view->clearTextures();
        scene.models.clear();
        scene.lights.clear();
        view->update();
    });
    connect(ui->actionImport_scene, &QAction::triggered, this, [this]{
        QString path = QFileDialog::getSaveFileName(this, tr("Export Current Scene"), QString(), tr("Scene Files (*.scene);;All Files (*.*)"));
        if(path.isEmpty()) return;
        if(QFileInfo(path).suffix().isEmpty()) path += ".scene";
        if(scene.saveToFile(path.toStdString())){
            QMessageBox::information(this, tr("Scene Exported"), tr("Saved: %1").arg(QDir::toNativeSeparators(path)));
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to save scene file."));
        }
    });
    connect(ui->actionReset_camera, &QAction::triggered, this, [this]{
        cameraController.reset();
        view->update();
    });
    connect(ui->actionPlace_here, &QAction::triggered, this, [this]{
        Color lightColor{currentLightColor.redF(), currentLightColor.greenF(), currentLightColor.blueF(), 1.0f};
        auto l = std::make_unique<Light>();
        l->type = Light::Type::Point;
        l->position = scene.camera.position;
        l->intensity = static_cast<float>(currentLightIntensity);
        l->color = lightColor;
        scene.addLight(std::move(l));
        view->update();
    });
    connect(ui->actionLight_color, &QAction::triggered, this, [this]{
        QColor c = QColorDialog::getColor(currentLightColor, this, tr("Select Light Color"));
        if(c.isValid()) currentLightColor = c;
    });
    connect(ui->actionIntensity, &QAction::triggered, this, [this]{
        bool ok=false;
        double v = QInputDialog::getDouble(this, tr("Light Intensity"), tr("Intensity (0..10)"), currentLightIntensity, 0.0, 10.0, 2, &ok);
        if(ok){ currentLightIntensity = v; }
    });
    connect(ui->actionChange_texture, &QAction::triggered, this, [this]{
        if(scene.models.empty()){
            QMessageBox::information(this, tr("Change Texture"), tr("No models in scene."));
            return;
        }
        // Build model list
        QStringList items;
        for(size_t i=0;i<scene.models.size();++i){
            const auto& m = scene.models[i];
            const QString name = m ? QString::fromStdString(m->name) : tr("(unnamed)");
            items << QString::number(i) + ": " + name;
        }
        bool ok=false;
        const QString choice = QInputDialog::getItem(this, tr("Select Model"), tr("Model:"), items, 0, false, &ok);
        if(!ok || choice.isEmpty()) return;
        bool okIndex=false;
        int idx = choice.section(':',0,0).toInt(&okIndex);
        if(!okIndex || idx<0 || idx >= static_cast<int>(scene.models.size())) return;
        auto* model = scene.models[idx].get();
        const QString img = QFileDialog::getOpenFileName(this, tr("Choose Texture"), QString(), tr("Images (*.png *.jpg *.jpeg *.bmp);;All Files (*.*)"));
        if(img.isEmpty()) return;
        modelManager.applyTexture(img.toStdString(), model);
        view->update();
    });

    auto connectSamples = [this]{
        QMenuBar* mb = this->menuBar(); if(!mb) return;
        QMenu* samples = nullptr;
        for(QMenu* m : mb->findChildren<QMenu*>()){
            if(m && m->title().compare("Samples", Qt::CaseInsensitive)==0){ samples = m; break; }
        }
        if(!samples) return;
        for(QAction* a : samples->actions()){
            if(!a) continue; QString t = a->text().toLower();
            auto bind = [&](auto fn){ connect(a, &QAction::triggered, this, [this, fn]{ fn(scene); view->update(); }); };
            if(t.contains("triangle")) bind(addTriangleSample);
            else if(t.contains("cube")) bind(addCubeSample);
            else if(t.contains("pyramid")) bind(addPyramidSample);
            else if(t.contains("sphere")) bind(addSphereSample);
        }
    };
    connectSamples();
}

void MainWindow::loadSceneFile(const QString& path){ 
    view->clearTextures(); 
    scene.loadFromFile(path.toStdString()); 
    view->update(); 
}

MainWindow::~MainWindow(){ delete ui; }

QString MainWindow::findDefaultScenePath() const{
    const QString exeDir = QCoreApplication::applicationDirPath();
    auto resolveIfExists = [](const QString& path) -> QString {
        QFileInfo fi(path);
        return (fi.exists() && fi.isFile()) ? fi.absoluteFilePath() : QString();
    };
    
    // Try relative to current working directory
    QString hit = resolveIfExists("data/default.scene");
    if(!hit.isEmpty()) return hit;
    
    // Try in exe directory
    hit = resolveIfExists(exeDir + "/data/default.scene");
    if(!hit.isEmpty()) return hit;
    
    // Try going up from exe directory (for build folders like build/Desktop_*/app/release/)
    QDir d(exeDir);
    for(int i=0;i<6;i++){
        if(!d.cdUp()) break;
        hit = resolveIfExists(d.absolutePath() + "/data/default.scene");
        if(!hit.isEmpty()) return hit;
    }
    return QString();
}

void MainWindow::updateFPSLabel(int fps) {
    ui->labelFPS->setText(QString("FPS: %1").arg(fps));
}

void MainWindow::updateZoomLabel(float fov) {
    // Default FOV is 60, calculate zoom as inverse ratio
    float zoom = 60.0f / fov;
    ui->labelZoom->setText(QString("Zoom: x%1").arg(zoom, 0, 'f', 1));
}
