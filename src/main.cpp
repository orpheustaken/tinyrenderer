#include <cmath>
#include <iostream>
#include <vector>
#include "geometry.h"
#include "model.h"
#include "tgaimage.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);

Model *model = nullptr;

constexpr int width = 800;
constexpr int height = 800;

// Cross Product
// https://en.m.wikipedia.org/wiki/Cross_product
Vec3f cross(const Vec3f &v1, const Vec3f &v2)
{
    return Vec3f(v1.raw[1] * v2.raw[2] - v1.raw[2] * v2.raw[1], v1.raw[2] * v2.raw[0] - v1.raw[0] * v2.raw[2],
                 v1.raw[0] * v2.raw[1] - v1.raw[1] * v2.raw[0]);
}

// TODO: Add asserts and checks on going beyond the borders of the image
// https://en.m.wikipedia.org/wiki/Bresenham's_line_algorithm
void draw_line(int x0, int y0, int x1, int y1, const TGAImage &image, const TGAColor &color)
{
    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1))
    {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0 > x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    const int dx = x1 - x0;
    const int dy = y1 - y0;
    // float derror = std::abs(dy/float(dx));
    // multiplying by 2 allows the code not to use floating points
    int derror = std::abs(dy) * 2;
    int error = 0;
    int y = y0;
    // adding the steep check to outside the draw loop greatly increases performance
    // see: https://github.com/ssloy/tinyrenderer/issues/28
    for (int x = x0; x <= x1; x++)
    {
        if (steep)
        {
            image.set(y, x, color);
        }
        else
        {
            image.set(x, y, color);
        }
        error += derror;
        // if (error>.5) {
        if (error > dx)
        {
            y += (y1 > y0 ? 1 : -1);
            // error -= 1.;
            error -= dx * 2;
        }
    }
}

// https://en.m.wikipedia.org/wiki/Barycentric_coordinate_system
Vec3f barycentric(Vec3f A, Vec3f B, Vec3f C, Vec3f P)
{
    Vec3f s[2];

    for (int i = 2; i--;)
    {
        s[i].raw[0] = C.raw[i] - A.raw[i];
        s[i].raw[1] = B.raw[i] - A.raw[i];
        s[i].raw[2] = A.raw[i] - P.raw[i];
    }

    Vec3f u = cross(s[0], s[1]);

    // pts and P has integer value as coordinates
    // so abs(u[2]) < 1 means u[2] is 0, that means
    // triangle is degenerate, in this case return something with negative coordinates

    if (std::abs(u.raw[2]) > 1e-2) // don't forget that u[2] is integer. If it is zero then triangle ABC is degenerate
        return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);

    // return Vec3f(-1,1,1);
    // as per clang-tidy: return braced initializer instead of declared typeA
    return {-1, 1, 1}; // in this case generate negative coordinates, it will be thrown away by the rasterizer
}

// the idea is to take the barycentric coordinates version of triangle rasterization, and for every pixel we want to
// draw simply to multiply its barycentric coordinates by the z-values of the vertices of the triangle we rasterize

void draw_triangle(Vec3f *screen_coords, float *zbuffer, const TGAImage &image, const TGAColor &color)
{
    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1);

    for (int i = 0; i < 3; i++)
    {
        bboxmin.x = std::max(0.f, std::min(bboxmin.x, static_cast<float>(screen_coords[i].x)));
        bboxmin.y = std::max(0.f, std::min(bboxmin.y, static_cast<float>(screen_coords[i].y)));
        bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, static_cast<float>(screen_coords[i].x)));
        bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, static_cast<float>(screen_coords[i].y)));
    }
    Vec3f P;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++)
    {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++)
        {
            Vec3f bc_screen = barycentric(screen_coords[0], screen_coords[1], screen_coords[2], P);
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
                continue;
            P.z = 0;
            for (int i = 0; i < 3; i++)
                P.z += screen_coords[i].raw[2] * bc_screen.raw[i];
            if (zbuffer[int(P.x + P.y * width)] < P.z)
            {
                zbuffer[int(P.x + P.y * width)] = P.z;
                image.set(P.x, P.y, color);
            }
        }
    }
}

void rasterize(Vec2i p0, Vec2i p1, const TGAImage &image, const TGAColor &color, int ybuffer[])
{
    if (p0.x > p1.x)
    {
        std::swap(p0, p1);
    }
    for (int x = p0.x; x <= p1.x; x++)
    {
        float t = (x - p0.x) / (float)(p1.x - p0.x);
        int y = p0.y * (1. - t) + p1.y * t + .5;
        if (ybuffer[x] < y)
        {
            ybuffer[x] = y;
            image.set(x, 0, color);
        }
    }
}

Vec3f world2screen(Vec3f v)
{
    return Vec3f(int((v.x + 1.) * width / 2. + .5), int((v.y + 1.) * height / 2. + .5), v.z);
}

int main(const int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Please, provide an object file to be rendered." << std::endl;
        return 1;
    }

    model = new Model(argv[1]);

    TGAImage image(width, height, TGAImage::RGB);

    const Vec3f light_dir(0, 0, -1);

    // z-buffer
    // https://en.m.wikipedia.org/wiki/Z-buffering
    float *zbuffer = new float[width * height];
    for (int i = width * height; i--; zbuffer[i] = -std::numeric_limits<float>::max())
        ;

    // flat shading render
    // https://en.m.wikipedia.org/wiki/Back-face_culling
    for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> face = model->face(i);
        Vec3f world_coords[3];
        for (int j = 0; j < 3; j++)
        {
            Vec3f v = model->vert(face[j]);
            world_coords[j] = v;
        }
        Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        n.normalize();
        float intensity = n * light_dir;
        Vec3f pts[3];
        for (int i = 0; i < 3; i++)
            pts[i] = world2screen(model->vert(face[i]));
        if (intensity > 0)
        {
            draw_triangle(pts, zbuffer, image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
        }
    }

    // origin of y at the left bottom corner of the image
    const bool flip_result = image.flip_vertically();

    image.write_tga_file("img_output.tga");
    delete model;

    if (flip_result)
    {
        return 0;
    }

    return 2;
}
