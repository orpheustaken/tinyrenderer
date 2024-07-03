#include <cmath>
#include <iostream>
#include <vector>
#include "geometry.h"
#include "model.h"
#include "tgaimage.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);

Model *model = nullptr;
constexpr int width = 800;
constexpr int height = 800;

// TODO: Add asserts and checks on going beyond the borders of the image
// Bresenham's line algorithm
void line(int x0, int y0, int x1, int y1, TGAImage &image, const TGAColor &color)
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

int main(const int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Please, provide an object file to be rendered." << std::endl;
        return 1;
    }

    model = new Model(argv[1]);

    // TGAImage image(100, 100, TGAImage::RGB);

    // line(13, 20, 80, 40, image, red);
    // line(20, 13, 40, 80, image, red);
    // line(80, 40, 13, 20, image, white);


    // image.write_tga_file("img_output.tga");

    TGAImage image(width, height, TGAImage::RGB);

    for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> face = model->face(i);
        for (int j = 0; j < 3; j++)
        {
            const Vec3f v0 = model->vert(face[j]);
            const Vec3f v1 = model->vert(face[(j + 1) % 3]);
            const int x0 = (v0.x + 1.) * width / 2.;
            const int y0 = (v0.y + 1.) * height / 2.;
            const int x1 = (v1.x + 1.) * width / 2.;
            const int y1 = (v1.y + 1.) * height / 2.;
            line(x0, y0, x1, y1, image, white);
        }
    }

    // origin of y at the left bottom corner of the image
    image.flip_vertically();

    image.write_tga_file("img_output.tga");
    delete model;

    return 0;
}
