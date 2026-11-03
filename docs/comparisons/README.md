# Boundary re-sampling A/B

Identical deterministic op-scripts (grid of drops → combs → vortex → wave)
rendered by two versions of the marbling engine, to show the effect of adaptive
boundary re-sampling.

| file | engine |
|------|--------|
| `01-old-*` | **before** — fixed vertex set per drop (vertices only moved, never added) |
| `03-v2-decimated-*` | **after** — re-sample after each op: subdivide stretched edges, decimate near-collinear points |

`*-normal` is a light script; `*-stress` stacks many combs + tines (this is where
the difference shows). Compare `01-old-stress.png` with `03-v2-decimated-stress.png`:
the old center is jagged/faceted where filaments stretched past their vertex
resolution; the new one stays a smooth liquid swirl.

Measured (stress script): verts 10,512 → 13,614, render ~92 ms → ~136 ms — i.e.
the smooth result at roughly baseline cost, because decimation keeps straight
runs cheap while curves keep their detail.

Regenerate with the headless harness (see the marbling engine; `InkDrop::resample`).
