# graphics

[![Build Status](https://travis-ci.org/irov/graphics.svg?branch=master)](https://travis-ci.org/irov/graphics) [![Build status](https://ci.appveyor.com/api/projects/status/j1o1386wwxnbu1iu/branch/master?svg=true)](https://ci.appveyor.com/project/irov/graphics/branch/master)

## Mesh contract

`gp_mesh_t::vertex_count` and `gp_mesh_t::index_count` are 32-bit counters. The
index buffer still stores `gp_uint16_t` values: one mesh may contain at most
65535 unique vertices, while `index_count` may be greater than 65535. The
vertex limit and API preconditions are checked only in Debug builds.

Bulk point APIs use `const void *`, point count, and byte stride. The first two
values of every point must be `float x` and `float y`; no public vector type is
required.

## Canvas memory

`gp_canvas_create` performs one allocation containing the canvas, a fixed
render-order array, recording memory, and tessellation scratch memory. Recording
and rendering do not fall back to the heap. If a fixed capacity is exhausted,
the operation returns `GP_FAILURE`.

The defaults are 65,536 render-order entries, 16 MiB of recording memory, and
64 MiB of reusable tessellation scratch memory. They can be configured with
`GP_CANVAS_RENDER_ORDER_CAPACITY`, `GP_CANVAS_RECORDING_MEMORY_SIZE`, and
`GP_CANVAS_SCRATCH_MEMORY_SIZE`. `gp_canvas_get_memory_stats` exposes current
usage and lifetime peaks so an application can size these blocks from real
frames.

`gp_inject_geometry` lets a caller write externally generated geometry directly
into the mesh. `gp_inject_branch` starts a new render-order range and invokes a
callback for every physical batch in that range; the callback payload can carry
renderer-owned state such as clip, texture, blend mode, or a debug label.

## Rounded polygons

`gp_rounded_polygon` preserves its original behavior: it creates a rounded
outward polygon offset. It does not round corners inside the input contour.
Use `gp_rounded_polyline` with `closed` set to `GP_TRUE` for corner rounding
contained by the original contour.
