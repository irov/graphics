# graphics

[![Build Status](https://travis-ci.org/irov/graphics.svg?branch=master)](https://travis-ci.org/irov/graphics) [![Build status](https://ci.appveyor.com/api/projects/status/j1o1386wwxnbu1iu/branch/master?svg=true)](https://ci.appveyor.com/project/irov/graphics/branch/master)

## Mesh contract

`gp_mesh_t::vertex_count`, `gp_mesh_t::index_count`, and
`gp_mesh_t::batch_count` are 32-bit counters. An index buffer may store either
`gp_uint16_t` or `gp_uint32_t` values, selected by `indices_stride`. Every
physical batch uses local indices and is limited to 65,535 vertices; a mesh may
contain any number of such batches within its 32-bit counters.

Call `gp_calculate_mesh_size` first to obtain the required counts. Every output
stream in `gp_mesh_t`, including batches, is optional: leave its buffer null
when the caller does not need that result. A non-null buffer uses its own byte
offset and stride.

Bulk point APIs use `const void *`, point count, and byte stride. The first two
values of every point must be `float x` and `float y`; no public vector type is
required.

## Canvas memory

`gp_graphics_create` owns the allocator and the canvas memory limits, and every
canvas is created from it. `gp_canvas_create` performs one allocation containing
the canvas, a fixed render-order array, recording memory, and tessellation
scratch memory. Recording and rendering do not fall back to the heap. If a fixed
capacity is exhausted, the operation returns `GP_FAILURE`.

Every limit is passed to `gp_graphics_create`, where `GP_CAPACITY_DEFAULT`
selects the compile-time default. The defaults are 65,536 render-order entries,
64 MiB of recording memory, and 64 MiB of reusable tessellation scratch memory,
configured with `GP_CANVAS_RENDER_ORDER_CAPACITY`,
`GP_CANVAS_RECORDING_MEMORY_SIZE`, and `GP_CANVAS_SCRATCH_MEMORY_SIZE`.
`gp_canvas_get_memory_stats` exposes current usage and lifetime peaks so an
application can size these blocks from real frames.

`gp_geometry` records borrowed vertex and index buffers without taking ownership
or copying them. Positions and indices are required; color and UV streams are
optional and may be null. Every borrowed buffer must remain valid until the
matching `gp_render` or `gp_flush` call returns. A position begins with two
floats, a color with four RGBA floats, a UV with two floats, and every index is
`gp_uint32_t`; byte offsets and strides describe interleaved source records.
Source colors are multiplied by the current canvas color.

`gp_textured_rects` accepts the same borrowed strided layout for logical
textured rectangles. Bounds and UV records each begin with four floats in
`x, y, width, height` order; an optional color record begins with RGBA. Graphics
expands these logical rectangles into vertices and indices during rendering.

`gp_flush` writes the currently recorded canvas commands into caller-owned mesh
buffers, then resets only the recording and tessellation arenas. The logical
canvas state and state stack are preserved, so recording may continue after a
renderer-state boundary without clearing or recreating the canvas. Output mesh
memory remains entirely owned and lifetime-managed by the caller.

## Rounded polygons

`gp_rounded_polygon` preserves its original behavior: it creates a rounded
outward polygon offset. It does not round corners inside the input contour.
Use `gp_rounded_polyline` with `closed` set to `GP_TRUE` for corner rounding
contained by the original contour.
