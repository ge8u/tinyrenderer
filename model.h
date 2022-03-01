#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"

class Model {
private:
	std::vector<Vec3f> verts_;
	std::vector<std::vector<int> > faces_;
	std::vector<Vec2f> textures_;
	std::vector<Vec3f> normals_;
	std::vector<int> facet_vrt{};
    std::vector<int> facet_tex{};  
    std::vector<int> facet_nrm{};
public:
	Model(const char *filename);
	~Model();
	int nverts();
	int nfaces();
	Vec3f vert(int iface, int nthvert);
	Vec2f texture(int iface,int nthvert) ;
	Vec3f normal(int iface, int nthvert) ;
};

#endif //__MODEL_H__

