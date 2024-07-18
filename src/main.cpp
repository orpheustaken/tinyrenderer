#include <cmath>
#include <iostream>
#include <vector>
#include "geometry.h"
#include "model.h"
#include "tgaimage.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);

Model *model = nullptr;

constexpr int width = 800;
constexpr int height = 800;

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
    // multipling by 2 allows the code not to use floating points
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
Vec3f barycentric(Vec2i *pts, Vec2i P)
{
    const Vec3f u = Vec3f(pts[2].raw[0] - pts[0].raw[0], pts[1].raw[0] - pts[0].raw[0], pts[0].raw[0] - P.raw[0]) ^
        Vec3f(pts[2].raw[1] - pts[0].raw[1], pts[1].raw[1] - pts[0].raw[1], pts[0].raw[1] - P.raw[1]);
    // pts and P has integer value as coordinates
    // so abs(u[2]) < 1 means u[2] is 0, that means
    // triangle is degenerate, in this case return something with negative coordinates
    if (std::abs(u.z) < 1)
        return {-1, 1, 1};
    // return Vec3f(1.f-(u.x+u.y)/u.z, u.y/u.z, u.x/u.z);
    // as per clang-tidy: return braced initializer instead of declared type
    return {1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z};
}

// https://en.m.wikipedia.org/wiki/Cross_product
void draw_triangle(Vec2i *pts, const TGAImage &image, const TGAColor &color)
{
    Vec2i bboxmin(image.get_width() - 1, image.get_height() - 1);
    Vec2i bboxmax(0, 0);
    const Vec2i clamp(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++)
    {
        bboxmin.x = std::max(0, std::min(bboxmin.x, pts[i].x));
        bboxmin.y = std::max(0, std::min(bboxmin.y, pts[i].y));

        bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, pts[i].x));
        bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, pts[i].y));
    }
    Vec2i P;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++)
    {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++)
        {
            if (const Vec3f bc_screen = barycentric(pts, P); bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
                continue;
            image.set(P.x, P.y, color);
        }
    }
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

    // flat shading render
    // https://en.m.wikipedia.org/wiki/Back-face_culling
    for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> face = model->face(i);
        Vec2i screen_coords[3];
        Vec3f world_coords[3];
        for (int j = 0; j < 3; j++)
        {
            Vec3f v = model->vert(face[j]);
            screen_coords[j] = Vec2i((v.x + 1.) * width / 2., (v.y + 1.) * height / 2.);
            world_coords[j] = v;
        }
        Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        n.normalize();
        float intensity = n * light_dir;
        if (intensity > 0)
        {
            draw_triangle(screen_coords, image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
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
