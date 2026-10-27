#include "Artwork.h"

Artwork::Artwork(int width, int height)
    : _width(width), _height(height), _rng(std::random_device{}()),
      _canvas(width, height), _previewCanvas(width, height)
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
    // (For a batch this was already cleared in beginBatch.)
    if (!_batchActive)
    {
        _redo.clear();
        _redoSteps.clear();
    }

    apply(op); // incremental: the canvas already reflects _ops
    _ops.push_back(op);

    if (_batchActive)
        ++_batchCount;
    else
        _undoSteps.push_back(1);

    _dirty = true;
}

void Artwork::beginBatch()
{
    // A batch is a fresh edit, so it discards any redo branch up front.
    _redo.clear();
    _redoSteps.clear();
    _batchActive = true;
    _batchCount = 0;
}

void Artwork::endBatch()
{
    if (!_batchActive)
        return;
    if (_batchCount > 0)
        _undoSteps.push_back(_batchCount);
    _batchActive = false;
    _batchCount = 0;
}

void Artwork::previewTine(const Point &start, const Point &end, const TineSettings &settings)
{
    _previewCanvas = _canvas; // copy committed state, then apply the gesture
    _previewCanvas.tine(start, end, settings);
    _previewActive = true;
    _dirty = true;
}

void Artwork::previewComb(const Point &start, const Point &end, const CombSettings &settings)
{
    _previewCanvas = _canvas;
    _previewCanvas.comb(start, end, settings);
    _previewActive = true;
    _dirty = true;
}

void Artwork::previewVortex(const Point &pos, const VortexSettings &settings)
{
    _previewCanvas = _canvas;
    _previewCanvas.vortex(pos, settings);
    _previewActive = true;
    _dirty = true;
}

void Artwork::endPreview()
{
    if (_previewActive)
    {
        _previewActive = false;
        _dirty = true;
    }
}

void Artwork::setOperations(const std::vector<Operation> &ops)
{
    _ops = ops;
    _redo.clear();
    _redoSteps.clear();
    // Each loaded op becomes its own undo step.
    _undoSteps.assign(_ops.size(), 1);
    _batchActive = false;
    _batchCount = 0;
    _previewActive = false;
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
    if (_undoSteps.empty())
        return;

    // Operations are not individually invertible, so step back by dropping the
    // last step's ops and replaying the rest from an empty canvas. The group is
    // moved to the redo stack preserving its original order.
    const size_t n = _undoSteps.back();
    _undoSteps.pop_back();
    const size_t start = _ops.size() - n;
    _redo.insert(_redo.end(), _ops.begin() + start, _ops.end());
    _redoSteps.push_back(n);
    _ops.erase(_ops.begin() + start, _ops.end());
    rebuild();
}

void Artwork::redo()
{
    if (_redoSteps.empty())
        return;

    const size_t n = _redoSteps.back();
    _redoSteps.pop_back();
    const size_t start = _redo.size() - n;
    for (size_t i = start; i < _redo.size(); ++i)
    {
        apply(_redo[i]); // incremental, in original order
        _ops.push_back(_redo[i]);
    }
    _redo.erase(_redo.begin() + start, _redo.end());
    _undoSteps.push_back(n);
    _dirty = true;
}

void Artwork::rebuild()
{
    _canvas.clear();
    for (const auto &op : _ops)
        apply(op);
    _dirty = true;
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
