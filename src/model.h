#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"

class Model
{
private:
    std::vector<Vec3f> verts_;
    std::vector<std::vector<int>> faces_;

public:
    explicit Model(const char *filename);
    ~Model();
    int nverts() const;
    int nfaces() const;
    Vec3f vert(int i) const;
    std::vector<int> face(int idx);
};

#endif //__MODEL_H__
