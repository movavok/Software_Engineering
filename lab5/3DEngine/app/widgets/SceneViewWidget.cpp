#include "SceneViewWidget.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QtMath>
SceneViewWidget::SceneViewWidget(QWidget* parent):QOpenGLWidget(parent){
	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);
	fpsTimer.start();
}
void SceneViewWidget::initializeGL(){ renderer.initialize(); renderer.setViewportSize(width(), height()); }
void SceneViewWidget::resizeGL(int w,int h){
	if(QOpenGLContext* ctx = QOpenGLContext::currentContext()){
		if(QOpenGLFunctions* f = ctx->functions()){
			f->glViewport(0,0,w,h);
		}
	}
	renderer.setViewportSize(w,h);
}
void SceneViewWidget::mousePressEvent(QMouseEvent* e){ lastPos = e->pos(); }
void SceneViewWidget::mouseMoveEvent(QMouseEvent* e){
	if(!scene) return;
	if(e->buttons() & Qt::LeftButton){
		QPoint delta = e->pos() - lastPos;
		// Simple sensitivity factors (degrees per pixel)
		const float yawSens = 0.2f;
		const float pitchSens = 0.2f;
		scene->camera.rotate(delta.x() * yawSens, -delta.y() * pitchSens);
		update();
	}
	if(e->buttons() & Qt::RightButton){
		QPoint delta = e->pos() - lastPos;
		// Pan: move along camera right/up axes
		const float panSens = 0.02f; // world units per pixel
		const float yawRad = qDegreesToRadians(scene->camera.yaw);
		const float pitchRad = qDegreesToRadians(scene->camera.pitch);
		const float cy = std::cos(yawRad), sy = std::sin(yawRad);
		const float cp = std::cos(pitchRad), sp = std::sin(pitchRad);
		// Forward and basis
		QVector3D f(cp*cy, sp, cp*sy);
		QVector3D worldUp(0,1,0);
		QVector3D r = QVector3D::crossProduct(f, worldUp).normalized();
		QVector3D u = QVector3D::crossProduct(r, f).normalized();
		// Apply pan (note: screen Y up decreases world up)
		QVector3D deltaWorld = r * (delta.x() * panSens) + u * (-delta.y() * panSens);
		scene->camera.position.x += deltaWorld.x();
		scene->camera.position.y += deltaWorld.y();
		scene->camera.position.z += deltaWorld.z();
		update();
	}
	if(e->buttons() & Qt::MiddleButton){
		QPoint delta = e->pos() - lastPos;
		// Dolly: move along camera forward
		const float dollySens = 0.05f; // world units per pixel
		const float yawRad = qDegreesToRadians(scene->camera.yaw);
		const float pitchRad = qDegreesToRadians(scene->camera.pitch);
		const float cy = std::cos(yawRad), sy = std::sin(yawRad);
		const float cp = std::cos(pitchRad), sp = std::sin(pitchRad);
		QVector3D f(cp*cy, sp, cp*sy);
		QVector3D deltaWorld = f * (-delta.y() * dollySens);
		scene->camera.position.x += deltaWorld.x();
		scene->camera.position.y += deltaWorld.y();
		scene->camera.position.z += deltaWorld.z();
		update();
	}
	lastPos = e->pos();
}
void SceneViewWidget::wheelEvent(QWheelEvent* e){
	if(!scene) return;
	const float step = e->angleDelta().y() / 120.0f; // 1 step = 120
	const float scale = (step > 0) ? 0.9f : 1.1f;
	scene->camera.zoom(scale);
	emit fovChanged(scene->camera.fov);
	update();
}
void SceneViewWidget::paintGL(){
	if(scene){
		renderer.setCamera(&scene->camera);
		std::vector<Light*> ls;
		for(auto& l: scene->lights) ls.push_back(l.get());
		renderer.setLights(ls);
		renderer.clearModels();
		for(auto& m: scene->models) renderer.addModel(m.get());
	}
	renderer.renderScene();
	// FPS tracking
	frameCount++;
	qint64 elapsed = fpsTimer.elapsed();
	if(elapsed - lastFPSUpdate >= 500){
		currentFPS = static_cast<int>(frameCount * 1000.0 / (elapsed - lastFPSUpdate));
		emit fpsChanged(currentFPS);
		frameCount = 0;
		lastFPSUpdate = elapsed;
	}
	update();
}
