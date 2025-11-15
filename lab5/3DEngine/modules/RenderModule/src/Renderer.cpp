#include "Renderer.h"
#include "../../core/include/Model.h"
#include "../../core/include/Camera.h"
#include "../../core/include/Light.h"
#include "../../core/include/Mesh.h"
#include "../../core/include/Vec3.h"
#include <QOpenGLFunctions>
#include <QImage>
#include <QFileInfo>
#include <limits>
#include <QVector3D>
#include <QVector4D>
#include <QtMath>
#include <algorithm>

static const char* kVS = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
uniform bool uUseAttrNormal;
uniform float uPointSize;
uniform mat4 uMVP;
uniform mat4 uModel;
uniform vec3 uNormal; // model-space or world-space normal if uModel is identity
out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
void main(){
	vec4 worldPos = uModel * vec4(aPos, 1.0);
	vWorldPos = worldPos.xyz;
	vec3 N = uUseAttrNormal ? aNormal : uNormal;
	vNormal = normalize(N);
	vUV = aUV;
	gl_Position = uMVP * vec4(aPos, 1.0);
	gl_PointSize = uPointSize;
}
)";

static const char* kFS = R"(
#version 330 core
out vec4 FragColor;
uniform vec4 uColor;
uniform int  uLightCount;
uniform vec3 uLightPos[16];     // world space
uniform vec3 uLightColor[16];   // 0..1
uniform float uLightIntensity[16];
uniform float uAmbient;     // 0..1
uniform bool uUseTex;
uniform sampler2D uDiffuseTex;
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
void main(){
	// Minimal Lambert lighting with multiple lights
	vec3 N = normalize(vNormal);
	// Two-sided lighting: flip normal for back faces
	if(!gl_FrontFacing) N = -N;
	vec3 base = uColor.rgb;
	if(uUseTex){ base *= texture(uDiffuseTex, vUV).rgb; }
	vec3 lit = base * uAmbient;
	for(int i=0;i<uLightCount;i++){
		vec3 L = normalize(uLightPos[i] - vWorldPos);
		float ndotl = max(dot(N, L), 0.0);
		lit += base * (uLightColor[i] * uLightIntensity[i]) * ndotl;
	}
	FragColor = vec4(lit, uColor.a);
}
)";

void Renderer::ensureGL(){
	if(glReady) return;
	this->glEnable(GL_DEPTH_TEST);
	this->glEnable(GL_PROGRAM_POINT_SIZE);
	// this->glEnable(GL_CULL_FACE); // Disabled to render both faces

	program.create();
	program.addShaderFromSourceCode(QOpenGLShader::Vertex,   kVS);
	program.addShaderFromSourceCode(QOpenGLShader::Fragment, kFS);
	program.link();

	vao.create();
	vao.bind();

	vboTriangle.create();
	vboTriangle.bind();
	const float verts[] = {
		-0.6f, -0.5f, 0.0f,
		 0.6f, -0.5f, 0.0f,
		 0.0f,  0.6f, 0.0f
	};
	vboTriangle.allocate(verts, sizeof(verts));

	program.bind();
	this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), reinterpret_cast<void*>(0));
	this->glEnableVertexAttribArray(0);

	vboTriangle.release();
	vao.release();
	program.release();

	glReady = true;
}

void Renderer::drawTriangle(){
	vao.bind();
	vboTriangle.bind();
	this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), reinterpret_cast<void*>(0));
	this->glEnableVertexAttribArray(0);
	// Ensure other attributes are disabled for this simple triangle
	this->glDisableVertexAttribArray(1);
	this->glDisableVertexAttribArray(2);
	this->glDrawArrays(GL_TRIANGLES, 0, 3);
	vboTriangle.release();
	vao.release();
}

void Renderer::drawMeshTriangles(const Mesh& mesh,
								 const Model* modelRef,
								 const QMatrix4x4& modelMat,
								 const QMatrix4x4& mvp,
								 const std::vector<QVector3D>& lpos,
								 const std::vector<QVector3D>& lcol,
								 const std::vector<float>& lint){
	if(mesh.indices.size() < 3 || mesh.vertices.size() < 3) return;
	// Compute per-vertex normals (averaged face normals)
	std::vector<QVector3D> normals(mesh.vertices.size(), QVector3D(0,0,0));
	for(size_t i=0; i+2 < mesh.indices.size(); i += 3){
		unsigned ia = mesh.indices[i];
		unsigned ib = mesh.indices[i+1];
		unsigned ic = mesh.indices[i+2];
		if(ia>=mesh.vertices.size() || ib>=mesh.vertices.size() || ic>=mesh.vertices.size()) continue;
		const Vec3& a = mesh.vertices[ia];
		const Vec3& b = mesh.vertices[ib];
		const Vec3& c = mesh.vertices[ic];
		QVector3D va(a.x, a.y, a.z);
		QVector3D vb(b.x, b.y, b.z);
		QVector3D vc(c.x, c.y, c.z);
		QVector3D n = QVector3D::crossProduct(vb - va, vc - va).normalized();
		normals[ia] += n; normals[ib] += n; normals[ic] += n;
	}
	for(auto& n : normals){ if(n.lengthSquared() > 0) n.normalize(); }

	// Generate simple planar UVs from XY bbox as fallback
	float minX=std::numeric_limits<float>::max(), minY=std::numeric_limits<float>::max();
	float maxX=-std::numeric_limits<float>::max(), maxY=-std::numeric_limits<float>::max();
	for(const auto& v : mesh.vertices){
		minX = std::min(minX, v.x); maxX = std::max(maxX, v.x);
		minY = std::min(minY, v.y); maxY = std::max(maxY, v.y);
	}
	float rx = std::max(1e-6f, maxX - minX);
	float ry = std::max(1e-6f, maxY - minY);
	std::vector<float> uv; uv.reserve(mesh.vertices.size()*2);
	for(const auto& v : mesh.vertices){
		float u = (v.x - minX)/rx;
		float vv = (v.y - minY)/ry;
		uv.push_back(u);
		uv.push_back(vv);
	}

	// Upload data
	vao.bind();
	QOpenGLBuffer vboPos(QOpenGLBuffer::VertexBuffer); vboPos.create(); vboPos.bind();
	std::vector<float> pos; pos.reserve(mesh.vertices.size()*3);
	for(const auto& v : mesh.vertices){ pos.push_back(v.x); pos.push_back(v.y); pos.push_back(v.z); }
	vboPos.allocate(pos.data(), static_cast<int>(pos.size()*sizeof(float)));
	this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), reinterpret_cast<void*>(0));
	this->glEnableVertexAttribArray(0);

	QOpenGLBuffer vboNrm(QOpenGLBuffer::VertexBuffer); vboNrm.create(); vboNrm.bind();
	std::vector<float> nbuf; nbuf.reserve(normals.size()*3);
	for(const auto& n : normals){ nbuf.push_back(n.x()); nbuf.push_back(n.y()); nbuf.push_back(n.z()); }
	vboNrm.allocate(nbuf.data(), static_cast<int>(nbuf.size()*sizeof(float)));
	this->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), reinterpret_cast<void*>(0));
	this->glEnableVertexAttribArray(1);

	QOpenGLBuffer vboUV(QOpenGLBuffer::VertexBuffer); vboUV.create(); vboUV.bind();
	vboUV.allocate(uv.data(), static_cast<int>(uv.size()*sizeof(float)));
	this->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), reinterpret_cast<void*>(0));
	this->glEnableVertexAttribArray(2);

	QOpenGLBuffer ebo(QOpenGLBuffer::IndexBuffer); ebo.create(); ebo.bind();
	ebo.allocate(mesh.indices.data(), static_cast<int>(mesh.indices.size()*sizeof(unsigned)));

	// Set uniforms
	program.bind();
	program.setUniformValue("uUseAttrNormal", true);
	program.setUniformValue("uPointSize", 1.0f);
	program.setUniformValue("uMVP", mvp);
	program.setUniformValue("uModel", modelMat);
	program.setUniformValue("uColor", QVector4D(0.7f, 0.7f, 0.75f, 1.0f));
	program.setUniformValue("uAmbient", 0.2f);
	int lc = static_cast<int>(lpos.size());
	program.setUniformValue("uLightCount", lc);
	if(lc>0){
		program.setUniformValueArray("uLightPos",  lpos.data(), lc);
		program.setUniformValueArray("uLightColor",lcol.data(), lc);
		program.setUniformValueArray("uLightIntensity", lint.data(), lc, 1);
	}

	// Texture binding
	bool useTex = false;
	if(modelRef && modelRef->texture.loaded && !modelRef->texture.file.empty()){
		useTex = bindTextureIfAvailable(modelRef->texture.file);
	}
	program.setUniformValue("uUseTex", useTex);
	if(useTex){ program.setUniformValue("uDiffuseTex", 0); }

	// Draw
	this->glDrawElements(GL_TRIANGLES, static_cast<int>(mesh.indices.size()), GL_UNSIGNED_INT, reinterpret_cast<void*>(0));

	// Cleanup - ВАЖЛИВО: destroy() звільняє GPU пам'ять, release() тільки відв'язує
	program.release();
	ebo.release(); ebo.destroy();
	vboUV.release(); vboUV.destroy();
	vboNrm.release(); vboNrm.destroy();
	vboPos.release(); vboPos.destroy();
	vao.release();
}

void Renderer::drawPoints(const std::vector<float>& data, GLenum primitive, int count, const QVector4D& color){
	if(count <= 0) return;
	program.bind();
	program.setUniformValue("uColor", color);
	vao.bind();
	// Створюємо тимчасовий буфер для кожного виклику - так надійніше
	QOpenGLBuffer vboTemp(QOpenGLBuffer::VertexBuffer);
	vboTemp.create();
	vboTemp.bind();
	vboTemp.allocate(data.data(), static_cast<int>(data.size()*sizeof(float)));
	this->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), reinterpret_cast<void*>(0));
	this->glEnableVertexAttribArray(0);
	// Disable normal attribute array slot when drawing points without normals
	this->glDisableVertexAttribArray(1);
	// Disable UV attribute for points
	this->glDisableVertexAttribArray(2);
	// Expect caller to set a desired point size beforehand if drawing GL_POINTS
	this->glDrawArrays(primitive, 0, count);
	vboTemp.release();
	vboTemp.destroy();
	vao.release();
	program.release();
}

void Renderer::renderScene(){
	ensureGL();

	this->glClearColor(0.1f,0.1f,0.15f,1.f);
	this->glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// Compute MVP once from camera (or identity if no camera provided)
	QMatrix4x4 proj; QMatrix4x4 view; QMatrix4x4 mvp;
	const float aspect = viewportH > 0 ? float(viewportW)/float(viewportH) : 1.0f;
	if(cam){
		proj.perspective(cam->fov, aspect, 0.1f, 500.0f);
		const float yawRad = qDegreesToRadians(cam->yaw);
		const float pitchRad = qDegreesToRadians(cam->pitch);
		const float cy = std::cos(yawRad), sy = std::sin(yawRad);
		const float cp = std::cos(pitchRad), sp = std::sin(pitchRad);
		const QVector3D pos(cam->position.x, cam->position.y, cam->position.z);
		const QVector3D forward(cp*cy, sp, cp*sy);
		const QVector3D target = pos + forward;
		view.lookAt(pos, target, QVector3D(0,1,0));
	} else {
		proj.setToIdentity(); view.setToIdentity();
	}
	mvp = proj * view;

	// Draw coordinate axes (X=red, Y=green, Z=blue) with simple arrows
	const float axisLen = 5.0f;
	const float arrowSize = 0.4f;
	program.bind();
	program.setUniformValue("uMVP", mvp);
	program.setUniformValue("uModel", QMatrix4x4());
	program.setUniformValue("uPointSize", 1.0f);
	program.setUniformValue("uAmbient", 1.0f);
	program.setUniformValue("uLightCount", 0);
	program.setUniformValue("uUseAttrNormal", false);
	program.setUniformValue("uUseTex", false);
	program.release();
	
	// X axis - Red
	std::vector<float> xData = {
		0.0f, 0.0f, 0.0f,  axisLen, 0.0f, 0.0f,  // main line
		axisLen, 0.0f, 0.0f,  axisLen-arrowSize, arrowSize*0.5f, 0.0f,  // arrow line 1
		axisLen, 0.0f, 0.0f,  axisLen-arrowSize, -arrowSize*0.5f, 0.0f  // arrow line 2
	};
	drawPoints(xData, GL_LINES, 6, QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
	
	// Y axis - Green
	std::vector<float> yData = {
		0.0f, 0.0f, 0.0f,  0.0f, axisLen, 0.0f,  // main line
		0.0f, axisLen, 0.0f,  arrowSize*0.5f, axisLen-arrowSize, 0.0f,  // arrow line 1
		0.0f, axisLen, 0.0f,  -arrowSize*0.5f, axisLen-arrowSize, 0.0f  // arrow line 2
	};
	drawPoints(yData, GL_LINES, 6, QVector4D(0.0f, 1.0f, 0.0f, 1.0f));
	
	// Z axis - Blue
	std::vector<float> zData = {
		0.0f, 0.0f, 0.0f,  0.0f, 0.0f, axisLen,  // main line
		0.0f, 0.0f, axisLen,  arrowSize*0.5f, 0.0f, axisLen-arrowSize,  // arrow line 1
		0.0f, 0.0f, axisLen,  -arrowSize*0.5f, 0.0f, axisLen-arrowSize  // arrow line 2
	};
	drawPoints(zData, GL_LINES, 6, QVector4D(0.0f, 0.0f, 1.0f, 1.0f));

	// Pack up to 16 lights (reused by model rendering)
	const int maxL = 16;
	std::vector<QVector3D> lpos; lpos.reserve(std::min((int)lights.size(), maxL));
	std::vector<QVector3D> lcol; lcol.reserve(std::min((int)lights.size(), maxL));
	std::vector<float> lint; lint.reserve(std::min((int)lights.size(), maxL));
	for(size_t i=0;i<lights.size() && (int)i<maxL;i++){
		auto* l = lights[i]; if(!l) continue;
		lpos.emplace_back(l->position.x, l->position.y, l->position.z);
		lcol.emplace_back(l->color.r, l->color.g, l->color.b);
		lint.emplace_back(l->intensity);
	}

	// Draw lights as points
	if(!lights.empty()){
		for(auto* l : lights){
			if(!l) continue;
			const float scale = std::max(0.0f, std::min(3.0f, l->intensity));
			QVector4D col(l->color.r * scale, l->color.g * scale, l->color.b * scale, 1.0f);
			program.bind();
			program.setUniformValue("uPointSize", 6.0f);
			program.setUniformValue("uMVP", mvp);
			// Render light points unlit (pure color)
			program.setUniformValue("uAmbient", 1.0f);
			program.setUniformValue("uLightCount", 0);
			program.setUniformValue("uUseAttrNormal", false);
			program.setUniformValue("uUseTex", false);
			program.release();
			const std::vector<float> one = { l->position.x, l->position.y, l->position.z };
			drawPoints(one, GL_POINTS, 1, col);
		}
	}

	// Draw models as lit triangle meshes (first mesh per model for now)
	for(auto* m : models){
		if(!m) continue; if(m->meshes.empty()) continue;
		const auto& mesh = m->meshes.front();
		QMatrix4x4 modelMat; // identity until we have per-model transforms
		drawMeshTriangles(mesh, m, modelMat, mvp, lpos, lcol, lint);
	}
}

// Create GL texture from image file and return id, 0 on failure
unsigned int Renderer::createTextureFromImage(const QString& qpath){
	QImage img(qpath);
	if(img.isNull()) return 0;
	QImage src = img.convertToFormat(QImage::Format_RGBA8888).mirrored();
	unsigned int texId = 0;
	this->glGenTextures(1, &texId);
	this->glActiveTexture(GL_TEXTURE0);
	this->glBindTexture(GL_TEXTURE_2D, texId);
	this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	this->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	this->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, src.width(), src.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, src.constBits());
	this->glGenerateMipmap(GL_TEXTURE_2D);
	return texId;
}

bool Renderer::bindTextureIfAvailable(const std::string& path){
	if(path.empty()) return false;
	auto it = textureCache.find(path);
	unsigned int id = 0;
	if(it == textureCache.end()){
		QFileInfo fi(QString::fromStdString(path));
		if(!fi.exists()) return false;
		id = createTextureFromImage(fi.absoluteFilePath());
		if(id == 0) return false;
		textureCache.emplace(path, id);
	} else {
		id = it->second;
	}
	this->glActiveTexture(GL_TEXTURE0);
	this->glBindTexture(GL_TEXTURE_2D, id);
	return true;
}

void Renderer::clearTextures(){
	if(!glReady) return;
	for(const auto& pair : textureCache){
		unsigned int texId = pair.second;
		this->glDeleteTextures(1, &texId);
	}
	textureCache.clear();
}
