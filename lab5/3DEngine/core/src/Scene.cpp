#include "Scene.h"
#include <fstream>
#include <sstream>
#include <string>
#include <limits>
#include "Model.h"
#include "Light.h"

static std::string trimLeft(const std::string& s){ size_t i=0; while(i<s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i; return s.substr(i); }

bool Scene::loadFromFile(const std::string& path){
	std::ifstream in(path);
	if(!in) return false;
	models.clear(); lights.clear();

	std::string line;
	int expectLights = -1;
	int expectModels = -1;
	while(std::getline(in, line)){
		if(line.empty()) continue;
		std::istringstream ss(line);
		std::string key; ss >> key;
		if(key == "CAMERA"){
			//keep current camera
			continue;
		} else if(key == "LIGHTS"){
			ss >> expectLights;
			for(int i=0;i<expectLights;i++){
				if(!std::getline(in, line)) break; if(line.empty()){ --i; continue; }
				std::istringstream ls(line); std::string lkey; ls >> lkey; if(lkey != "LIGHT"){ --i; continue; }
				auto lt = std::make_unique<Light>();
				int t=0; ls >> t;
				lt->type = (t==1) ? Light::Type::Directional : Light::Type::Point;
				ls >> lt->position.x >> lt->position.y >> lt->position.z;
				ls >> lt->direction.x >> lt->direction.y >> lt->direction.z;
				ls >> lt->color.r >> lt->color.g >> lt->color.b >> lt->color.a;
				ls >> lt->intensity;
				addLight(std::move(lt));
			}
		} else if(key == "MODELS"){
			ss >> expectModels;
			for(int mi=0; mi<expectModels; ++mi){
				auto md = std::make_unique<Model>();
				// NAME line
				if(!std::getline(in, line)) break; if(line.rfind("NAME",0)==0){ md->name = trimLeft(line.substr(4)); if(!md->name.empty() && md->name[0]==' ') md->name.erase(0,1);} else { md->name = "model"+std::to_string(mi); }
				// TEXTURE line
				if(std::getline(in, line) && line.rfind("TEXTURE",0)==0){ std::string pathPart = trimLeft(line.substr(7)); if(!pathPart.empty() && pathPart[0]==' ') pathPart.erase(0,1); if(pathPart != "-" && !pathPart.empty()){ md->texture.file = pathPart; md->texture.loaded = true; } }
				// MATERIAL line
				if(std::getline(in, line) && line.rfind("MATERIAL",0)==0){ std::istringstream ms(line.substr(8)); ms >> md->material.diffuse.r >> md->material.diffuse.g >> md->material.diffuse.b >> md->material.diffuse.a; }
				// MESHES line
				int meshCount = 0; if(std::getline(in, line) && line.rfind("MESHES",0)==0){ std::istringstream mcs(line.substr(6)); mcs >> meshCount; }
				for(int k=0;k<meshCount;k++){
					// VERTICES
					int vcount=0; if(!std::getline(in, line)) break; if(line.rfind("VERTICES",0)==0){ std::istringstream vs(line.substr(8)); vs >> vcount; }
					std::vector<Vec3> verts; verts.reserve(vcount);
					for(int vi=0; vi<vcount; ++vi){ if(!std::getline(in, line)) break; std::istringstream vls(line); char c; Vec3 v; vls >> c >> v.x >> v.y >> v.z; verts.push_back(v); }
					// INDICES
					int icount=0; if(!std::getline(in, line)) break; if(line.rfind("INDICES",0)==0){ std::istringstream is(line.substr(7)); is >> icount; }
					std::vector<unsigned> idx; idx.reserve(icount);
					for(int ii=0; ii<icount; ++ii){ if(!std::getline(in, line)) break; std::istringstream ils(line); char c; unsigned a; ils >> c >> a; idx.push_back(a); }
					Mesh m; m.vertices = std::move(verts); m.indices = std::move(idx); md->meshes.push_back(std::move(m));
				}
				addModel(std::move(md));
			}
		}
	}
	return true;
}

bool Scene::saveToFile(const std::string& path) const{
	std::ofstream f(path); if(!f) return false;
	// Camera
	f << "CAMERA " << camera.position.x << ' ' << camera.position.y << ' ' << camera.position.z
	  << ' ' << camera.yaw << ' ' << camera.pitch << ' ' << camera.fov << "\n";
	// Lights
	f << "LIGHTS " << lights.size() << "\n";
	for(const auto& lp : lights){ const Light* l = lp.get(); if(!l) continue;
		int t = (l->type == Light::Type::Directional) ? 1 : 0;
		f << "LIGHT " << t << ' ' << l->position.x << ' ' << l->position.y << ' ' << l->position.z
		  << ' ' << l->direction.x << ' ' << l->direction.y << ' ' << l->direction.z
		  << ' ' << l->color.r << ' ' << l->color.g << ' ' << l->color.b << ' ' << l->color.a
		  << ' ' << l->intensity << "\n";
	}
	// Models
	f << "MODELS " << models.size() << "\n";
	for(const auto& mp : models){ const Model* m = mp.get(); if(!m) continue;
		f << "NAME " << m->name << "\n";
		f << "TEXTURE " << (m->texture.file.empty() ? "-" : m->texture.file) << "\n";
		f << "MATERIAL " << m->material.diffuse.r << ' ' << m->material.diffuse.g << ' ' << m->material.diffuse.b << ' ' << m->material.diffuse.a << "\n";
		f << "MESHES " << m->meshes.size() << "\n";
		for(const auto& mesh : m->meshes){
			f << "VERTICES " << mesh.vertices.size() << "\n";
			for(const auto& v : mesh.vertices){ f << 'v' << ' ' << v.x << ' ' << v.y << ' ' << v.z << "\n"; }
			f << "INDICES " << mesh.indices.size() << "\n";
			for(const auto& ii : mesh.indices){ f << 'i' << ' ' << ii << "\n"; }
		}
	}
	return true;
}