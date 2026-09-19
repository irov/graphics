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

`gp_canvas_create` allocates only the canvas header and its state stack.
Recording memory is allocated on demand in linked blocks, so recorded pointers
remain stable as the canvas grows. The recording size passed to
`gp_graphics_create` is now the block size, not a total recording limit. The
default block size is 4 KiB; larger individual payloads get a larger block.
Render orders use a contiguous array allocated on the first command, starting
with up to 16 entries and growing up to the configured render-order limit.

Each canvas keeps a pointer to its `gp_graphics_t`, which must remain alive until
all canvases created from it have been destroyed.

Scratch memory belongs to `gp_graphics_t` and is shared by all its canvases.
It grows in blocks on demand and is reused after each temporary path or mesh
operation, including failures. Clearing or destroying a canvas does not reset
another operation's scratch memory. Operations using scratch on the same
graphics object must be serialized across threads. Separate graphics objects
have independent scratch memory.

`GP_CAPACITY_DEFAULT` selects the compile-time defaults:
`GP_CANVAS_RECORDING_MEMORY_SIZE` (4 KiB blocks),
`GP_CANVAS_RENDER_ORDER_CAPACITY` (65,536 orders per canvas), and
`GP_CANVAS_SCRATCH_MEMORY_SIZE` (64 MiB maximum shared scratch capacity).
These capacities are not reserved in advance. Allocation failures and exhausted
render-order or scratch limits return `GP_FAILURE`.
`gp_canvas_get_memory_stats` reports allocated capacities and lifetime peaks;
scratch statistics describe the shared graphics object and must not be summed
across its canvases. Arena usage includes alignment and unused block tails;
allocator metadata, block headers, and the canvas header are not included.

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
buffers, then frees recording blocks and the render-order array. The logical
canvas state and state stack are preserved, so recording may continue after a
renderer-state boundary without clearing or recreating the canvas. Output mesh
memory remains entirely owned and lifetime-managed by the caller.
`gp_canvas_clear` also frees recording blocks and render orders, and restores
the default canvas state. Shared scratch blocks remain cached until
`gp_graphics_destroy`.

## Rounded polygons

`gp_rounded_polygon` preserves its original behavior: it creates a rounded
outward polygon offset. It does not round corners inside the input contour.
Use `gp_rounded_polyline` with `closed` set to `GP_TRUE` for corner rounding
contained by the original contour.
