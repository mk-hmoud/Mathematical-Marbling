#include "Artwork.h"

Artwork::Artwork(int width, int height)
    : _width(width), _height(height), _rng(std::random_device{}()), _canvas(width, height)
{
}

void Artwork::addDrop(const Point &center, float radius, const Color &color)
{
    DropOp op{center, radius, color};
    apply(op);
    _ops.push_back(op);
}

void Artwork::addRandomDrop(float radius)
{
    // Resolve the randomness here, then record a plain DropOp so replay is
    // deterministic.
    std::uniform_real_distribution<float> xDist(0.0f, static_cast<float>(_width));
    std::uniform_real_distribution<float> yDist(0.0f, static_cast<float>(_height));
    std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);

    Point center{xDist(_rng), yDist(_rng)};
    Color color{colorDist(_rng), colorDist(_rng), colorDist(_rng)};
    addDrop(center, radius, color);
}

void Artwork::addTine(const Point &start, const Point &end, const TineSettings &settings)
{
    TineOp op{start, end, settings};
    apply(op);
    _ops.push_back(op);
}

void Artwork::addComb(const Point &start, const Point &end, const CombSettings &settings)
{
    CombOp op{start, end, settings};
    apply(op);
    _ops.push_back(op);
}

void Artwork::addVortex(const Point &pos, const VortexSettings &settings)
{
    VortexOp op{pos, settings};
    apply(op);
    _ops.push_back(op);
}

void Artwork::clear()
{
    ClearOp op{};
    apply(op);
    _ops.push_back(op);
}

void Artwork::setOperations(const std::vector<Operation> &ops)
{
    _ops = ops;
    _canvas.clear();
    for (const auto &op : _ops)
        apply(op);
}

void Artwork::load(int width, int height, const std::vector<Operation> &ops)
{
    _width = width;
    _height = height;
    setOperations(ops);
}

void Artwork::apply(const Operation &op)
{
    std::visit(
        [this](const auto &o)
        {
            using T = std::decay_t<decltype(o)>;
            if constexpr (std::is_same_v<T, DropOp>)
                _canvas.add_drop(o.center.x, o.center.y, o.radius, o.color);
            else if constexpr (std::is_same_v<T, TineOp>)
                _canvas.tine(o.start, o.end, o.settings);
            else if constexpr (std::is_same_v<T, CombOp>)
                _canvas.comb(o.start, o.end, o.settings);
            else if constexpr (std::is_same_v<T, VortexOp>)
                _canvas.vortex(o.pos, o.settings);
            else if constexpr (std::is_same_v<T, ClearOp>)
                _canvas.clear();
        },
        op);
}
