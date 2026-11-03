#ifndef INKDROP_H
#define INKDROP_H

#include <vector>
#include "DataTypes.h"

class InkDrop
{
public:
    InkDrop(float x, float y, float radius, float r = 1.0f, float g = 1.0f, float b = 1.0f);

    // --- Math Marbling Functions ---
    void marbled(const InkDrop &other);
    void vortex(const Point &pos, float strength, float coreRadius);
    void tine(const Point &m, float x, float y, float z, float c);
    void wave(const Point &m, float bx, float by, float amplitude, float wavelength);

    // Finite, localized "short stroke" (Oseen flow) from a to b. Unlike tine()
    // (an infinite line), this is a local deformation that decays in every
    // direction with falloff length L, integrated in `steps` sub-steps as the
    // stylus moves along the path. `strength` scales how far core fluid is
    // carried (strength*|b-a|).
    void oseenStroke(const Point &a, const Point &b, float L, float strength, int steps);

    // Re-sample the boundary so no edge is longer than maxEdgeLen, inserting
    // points on stretched edges. Marbling operations stretch the boundary into
    // thin filaments; without re-sampling the fixed vertex set spreads apart
    // and smooth curves collapse into visible straight chords. Call after each
    // deformation. Deterministic; bounded by a hard per-drop vertex cap.
    void resample(float maxEdgeLen);

    // --- Data access methods for the Renderer ---
    const std::vector<Point> &getBoundary() const;
    const Color &getColor() const;
    Point getInitialCentre() const;
    float getInitialRadius() const;

private:
    Point _initialCentre;
    float _initialRadius;
    Color _color;
    std::vector<Point> _boundary;
};

#endif // INKDROP_H