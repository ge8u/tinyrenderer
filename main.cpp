#include <vector>
#include <cmath>
#include <cstdlib>
#include <limits>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
Model *model = NULL;
const int width  = 800;
const int height = 800;

using namespace std;

void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color) {
    bool steep = false;
    if (std::abs(x0-x1)<std::abs(y0-y1)) {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0>x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    for (int x=x0; x<=x1; x++) {
        float t = (x-x0)/(float)(x1-x0);
        int y = y0*(1.-t) + y1*t;
        if (steep) {
            image.set(y, x, color);
        } else {
            image.set(x, y, color);
        }
    }
}

Vec3f barycentric(Vec3f A, Vec3f B, Vec3f C, Vec3f P) {
    Vec3f s[2];
    for (int i=2; i--; ) {
        s[i][0] = C[i]-A[i];
        s[i][1] = B[i]-A[i];
        s[i][2] = A[i]-P[i];
    }
    Vec3f u = cross(s[0], s[1]);

    if (std::abs(u[2])>1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
        return Vec3f(1.f-(u.x+u.y)/u.z, u.y/u.z, u.x/u.z);
    return Vec3f(-1,1,1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
}

void triangle(Vec3f *pts, float *zbuffer, TGAImage &imageTexture,TGAImage &image,Vec2f texture[3],Vec3f normal[3]) {
    Vec2f bboxmin( std::numeric_limits<float>::max(),  std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width()-1, image.get_height()-1);
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            bboxmin[j] = std::max(0.f,      std::min(bboxmin[j], pts[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
        }
    }
    Vec3f P;
    for (P.x=bboxmin.x; P.x<=bboxmax.x; P.x++) {
        for (P.y=bboxmin.y; P.y<=bboxmax.y; P.y++) {
            Vec3f bc_screen  = barycentric(pts[0], pts[1], pts[2], P);
            if (bc_screen.x<0 || bc_screen.y<0 || bc_screen.z<0) continue;
            P.z = 0;
            for (int i=0; i<3; i++) P.z += pts[i][2]*bc_screen[i];
            if (zbuffer[int(P.x+P.y*width)]<P.z) {
                zbuffer[int(P.x+P.y*width)] = P.z;
                //texture
                TGAColor colorX=imageTexture.get(texture[0].x,texture[0].y)*bc_screen.x;
                TGAColor colorY=imageTexture.get(texture[1].x,texture[1].y)*bc_screen.y;
                TGAColor colorZ=imageTexture.get(texture[2].x,texture[2].y)*bc_screen.z;
                float colorRenderX=colorX[2]+colorY[2]+colorZ[2];
                float colorRenderY=colorX[1]+colorY[1]+colorZ[1];
                float colorRenderZ=colorX[0]+colorY[0]+colorZ[0];
                //itensity
                Vec3f light_dir(0,0,-1);
                float normalX=bc_screen.x*normal[0].x+bc_screen.y*normal[1].x+bc_screen.z*normal[2].x;
                float normalY=bc_screen.x*normal[0].y+bc_screen.y*normal[1].y+bc_screen.z*normal[2].y;
                float normalZ=bc_screen.x*normal[0].z+bc_screen.y*normal[1].z+bc_screen.z*normal[2].z;
                Vec3f n=Vec3f(normalX,normalY,normalZ).normalize();
                float  intensity= abs(n*light_dir);
                //shader
                image.set(P.x, P.y, TGAColor(colorRenderX*intensity,colorRenderY*intensity,colorRenderZ*intensity));
            }
        }
    }
}

Vec3f world2screen(Vec3f v) {
    return Vec3f(int((v.x+1.)*width/2.+.5), int((v.y+1.)*height/2.+.5), v.z);
}

Vec2f world2screen2(Vec2f v) {
    return Vec2f(float(v.x*1024),float(v.y*1024));
}

int main(int argc, char** argv) {
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/diablo3_pose/diablo3_pose.obj");
    }

    float *zbuffer = new float[width*height];
    for (int i=width*height; i--; zbuffer[i] = -std::numeric_limits<float>::max());
    TGAImage image(width, height, TGAImage::RGB);
    TGAImage imageTexture(width, height, TGAImage::RGB);
    imageTexture.read_tga_file("obj/diablo3_pose/diablo3_pose_diffuse.tga"); 
    Vec3f light_dir(0,0,-1);
    for (int i=0; i<model->nfaces(); i++) {
        Vec3f pts[3];
        Vec2f pts_texture[3];
        Vec3f pts_normal[3];
        for (int j=0; j<3; j++){ 
            pts[j] = world2screen(model->vert(i,j));
            pts_texture[j] = world2screen2(model->texture(i,j));
            pts_normal[j]=model->normal(i,j);
        }
        triangle(pts, zbuffer,imageTexture,image,pts_texture,pts_normal);
        
    }

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
    delete model;
    return 0;
}