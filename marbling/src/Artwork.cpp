#include "Artwork.h"

Artwork::Artwork(int width, int height)
    : _width(width), _height(height), _rng(std::random_device{}()), _canvas(width, height)
{
}

void Artwork::addDrop(const Point &center, float radius, const Color &color)
{
    record(DropOp{center, radius, color});
}

void Artwork::addRandomDrop(float radius, const std::vector<Color> &palette)
{
    // Resolve the randomness here, then record a plain DropOp so replay is
    // deterministic.
    std::uniform_real_distribution<float> xDist(0.0f, static_cast<float>(_width));
    std::uniform_real_distribution<float> yDist(0.0f, static_cast<float>(_height));
    Point center{xDist(_rng), yDist(_rng)};

    Color color;
    if (palette.empty())
    {
        std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);
        color = {colorDist(_rng), colorDist(_rng), colorDist(_rng)};
    }
    else
    {
        std::uniform_int_distribution<size_t> pick(0, palette.size() - 1);
        color = palette[pick(_rng)];
    }
    addDrop(center, radius, color);
}

void Artwork::addTine(const Point &start, const Point &end, const TineSettings &settings)
{
    record(TineOp{start, end, settings});
}

void Artwork::addComb(const Point &start, const Point &end, const CombSettings &settings)
{
    record(CombOp{start, end, settings});
}

void Artwork::addVortex(const Point &pos, const VortexSettings &settings)
{
    record(VortexOp{pos, settings});
}

void Artwork::clear()
{
    record(ClearOp{});
}

void Artwork::record(const Operation &op)
{
    // A fresh edit starts a new branch: anything that was undone is discarded.
    _redo.clear();
    apply(op); // incremental: the canvas already reflects _ops
    _ops.push_back(op);
}

void Artwork::setOperations(const std::vector<Operation> &ops)
{
    _ops = ops;
    _redo.clear();
    rebuild();
}

void Artwork::load(int width, int height, const std::vector<Operation> &ops)
{
    _width = width;
    _height = height;
    setOperations(ops);
}

void Artwork::undo()
{
    if (_ops.empty())
        return;

    // Operations are not individually invertible, so step back by dropping the
    // last one and replaying the rest from an empty canvas.
    _redo.push_back(_ops.back());
    _ops.pop_back();
    rebuild();
}

void Artwork::redo()
{
    if (_redo.empty())
        return;

    const Operation op = _redo.back();
    _redo.pop_back();
    apply(op); // incremental: the canvas already reflects _ops
    _ops.push_back(op);
}

void Artwork::rebuild()
{
    _canvas.clear();
    for (const auto &op : _ops)
        apply(op);
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
