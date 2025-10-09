#include "Canvas.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <numeric>
#include <random>
#include <algorithm>

Canvas::Canvas(int width, int height) : width(width), height(height) {}

float Canvas::randomFloat(float lower, float upper)
{
    float random = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    return lower + random * (upper - lower);
}

void Canvas::add_drop(float x, float y, float radius)
{
    InkDrop new_drop(x, y, randomFloat(40, 100), randomFloat(0, 1), randomFloat(0, 1), randomFloat(0, 1));
    for (auto &drop : inkDrops)
    {
        drop.marbled(new_drop);
    }
    inkDrops.push_back(new_drop);
}

void Canvas::render()
{
    for (const auto &drop : inkDrops)
    {
        drop.render();
    }
}

void Canvas::tine(float tine_x, float tine_y, float z, float c)
{
    for (auto &drop : inkDrops)
    {
        drop.tine(Point(1, 1), tine_x, tine_y, z, c);
    }
}

void Canvas::vortex(float x, float y, float z, float c, float r)
{
    for (auto &drop : inkDrops)
    {
        drop.vortex(Point(x, y), z, c, r);
    }
}

void Canvas::clear()
{
    inkDrops.clear();
}

/*Perlin*/

float fade(float t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

float grad(int hash, float x, float y)
{
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : h == 12 || h == 14 ? x
                                             : 0;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float perlinNoise(float x, float y, const std::vector<int> &perm)
{
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;

    x -= std::floor(x);
    y -= std::floor(y);

    float u = fade(x);
    float v = fade(y);

    int aa = perm[X + perm[Y]];
    int ab = perm[X + perm[Y + 1]];
    int ba = perm[X + 1 + perm[Y]];
    int bb = perm[X + 1 + perm[Y + 1]];

    return lerp(v, lerp(u, grad(aa, x, y), grad(ba, x - 1, y)),
                lerp(u, grad(ab, x, y - 1), grad(bb, x - 1, y - 1)));
}

// permutation table
std::vector<int> generatePermutation()
{
    std::vector<int> perm(512);
    std::iota(perm.begin(), perm.begin() + 256, 0);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(perm.begin(), perm.begin() + 256, g);
    for (int i = 0; i < 256; ++i)
    {
        perm[256 + i] = perm[i];
    }
    return perm;
}

void Canvas::addDropsWithPerlinNoise(int numDrops, float scale, float z, float c)
{
    auto perm = generatePermutation();
    for (int i = 0; i < numDrops; ++i)
    {
        float nx = static_cast<float>(i) / numDrops * scale;
        float ny = static_cast<float>(i + 100) / numDrops * scale;

        float x = width * perlinNoise(nx, ny, perm);
        float y = height * perlinNoise(ny, nx, perm);

        add_drop(x, y, 10);
    }
}
