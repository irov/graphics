#include "detail.h"

#include "struct.h"

#include <string.h>

//////////////////////////////////////////////////////////////////////////
typedef struct gp_scratch_allocation_t
{
    gp_size_t size;
} gp_scratch_allocation_t;
//////////////////////////////////////////////////////////////////////////
static gp_size_t gp_align_size( gp_size_t _value )
{
    const gp_size_t alignment = sizeof( void * );
    if( _value > SIZE_MAX - alignment + 1U )
    {
        return SIZE_MAX;
    }

    return (_value + alignment - 1U) & ~(alignment - 1U);
}
//////////////////////////////////////////////////////////////////////////
static void * gp_arena_allocate( gp_arena_t * _arena, gp_size_t _size )
{
    gp_size_t aligned_size = gp_align_size( _size );
    if( _arena->offset > _arena->capacity || aligned_size > _arena->capacity - _arena->offset )
    {
        return GP_NULLPTR;
    }

    void * memory = _arena->memory + _arena->offset;
    _arena->offset += aligned_size;
    if( _arena->peak < _arena->offset )
    {
        _arena->peak = _arena->offset;
    }

    return memory;
}
//////////////////////////////////////////////////////////////////////////
void * gp_canvas_recording_allocate( gp_canvas_t * _canvas, gp_size_t _size )
{
    return gp_arena_allocate( &_canvas->recording_arena, _size );
}
//////////////////////////////////////////////////////////////////////////
void * gp_canvas_scratch_reallocate( gp_canvas_t * _canvas, void * _pointer, gp_size_t _size )
{
    if( _size == 0 )
    {
        return GP_NULLPTR;
    }

    if( _size > SIZE_MAX - sizeof( gp_scratch_allocation_t ) )
    {
        return GP_NULLPTR;
    }

    gp_size_t allocation_size = sizeof( gp_scratch_allocation_t ) + _size;
    if( _pointer != GP_NULLPTR )
    {
        gp_scratch_allocation_t * previous = (gp_scratch_allocation_t *)_pointer - 1;
        if( previous->size <= SIZE_MAX - sizeof( gp_scratch_allocation_t ) )
        {
            const gp_size_t previous_allocation_size = gp_align_size( sizeof( gp_scratch_allocation_t ) + previous->size );
            const gp_size_t previous_offset = (gp_uint8_t *)previous - _canvas->scratch_arena.memory;
            if( previous_offset <= _canvas->scratch_arena.offset &&
                previous_allocation_size <= _canvas->scratch_arena.offset - previous_offset &&
                previous_offset + previous_allocation_size == _canvas->scratch_arena.offset )
            {
                const gp_size_t resized_allocation_size = gp_align_size( allocation_size );
                if( resized_allocation_size <= _canvas->scratch_arena.capacity - previous_offset )
                {
                    previous->size = _size;
                    _canvas->scratch_arena.offset = previous_offset + resized_allocation_size;
                    if( _canvas->scratch_arena.peak < _canvas->scratch_arena.offset )
                    {
                        _canvas->scratch_arena.peak = _canvas->scratch_arena.offset;
                    }
                    return previous + 1;
                }
            }
        }
    }

    gp_scratch_allocation_t * allocation = (gp_scratch_allocation_t *)gp_arena_allocate( &_canvas->scratch_arena, allocation_size );
    if( allocation == GP_NULLPTR )
    {
        return GP_NULLPTR;
    }

    allocation->size = _size;
    void * memory = allocation + 1;
    if( _pointer != GP_NULLPTR )
    {
        const gp_scratch_allocation_t * previous = (const gp_scratch_allocation_t *)_pointer - 1;
        gp_size_t copy_size = previous->size < _size ? previous->size : _size;
        memcpy( memory, _pointer, copy_size );
    }

    return memory;
}
//////////////////////////////////////////////////////////////////////////
void gp_canvas_scratch_free( gp_canvas_t * _canvas, void * _pointer )
{
    if( _canvas == GP_NULLPTR || _pointer == GP_NULLPTR )
    {
        return;
    }

    gp_scratch_allocation_t * allocation = (gp_scratch_allocation_t *)_pointer - 1;
    if( (gp_uint8_t *)allocation < _canvas->scratch_arena.memory ||
        (gp_uint8_t *)allocation >= _canvas->scratch_arena.memory + _canvas->scratch_arena.offset ||
        allocation->size > SIZE_MAX - sizeof( gp_scratch_allocation_t ) )
    {
        return;
    }

    const gp_size_t allocation_offset = (gp_uint8_t *)allocation - _canvas->scratch_arena.memory;
    const gp_size_t allocation_size = gp_align_size( sizeof( gp_scratch_allocation_t ) + allocation->size );
    if( allocation_size <= _canvas->scratch_arena.offset - allocation_offset &&
        allocation_offset + allocation_size == _canvas->scratch_arena.offset )
    {
        _canvas->scratch_arena.offset = allocation_offset;
    }
}
//////////////////////////////////////////////////////////////////////////
void * gp_canvas_scratch_malloc_callback( gp_size_t _size, void * _ud )
{
    return gp_canvas_scratch_reallocate( (gp_canvas_t *)_ud, GP_NULLPTR, _size );
}
//////////////////////////////////////////////////////////////////////////
void * gp_canvas_scratch_realloc_callback( void * _pointer, gp_size_t _size, void * _ud )
{
    return gp_canvas_scratch_reallocate( (gp_canvas_t *)_ud, _pointer, _size );
}
//////////////////////////////////////////////////////////////////////////
void gp_canvas_scratch_free_callback( void * _pointer, void * _ud )
{
    gp_canvas_scratch_free( (gp_canvas_t *)_ud, _pointer );
}
//////////////////////////////////////////////////////////////////////////
gp_size_t gp_canvas_scratch_mark( const gp_canvas_t * _canvas )
{
    return _canvas->scratch_arena.offset;
}
//////////////////////////////////////////////////////////////////////////
void gp_canvas_scratch_reset( gp_canvas_t * _canvas, gp_size_t _mark )
{
    if( _mark <= _canvas->scratch_arena.offset )
    {
        _canvas->scratch_arena.offset = _mark;
    }
}

//////////////////////////////////////////////////////////////////////////
void gp_color_mul( gp_color_t * _c, const gp_color_t * _c0, const gp_color_t * _c1 )
{
    _c->r = _c0->r * _c1->r;
    _c->g = _c0->g * _c1->g;
    _c->b = _c0->b * _c1->b;
    _c->a = _c0->a * _c1->a;
}
//////////////////////////////////////////////////////////////////////////
gp_uint32_t gp_color_argb( const gp_color_t * _c )
{
    const float rgba_255 = 255.5f;

    gp_uint8_t r8 = (gp_uint8_t)(GP_COLOR_R( _c ) * rgba_255);
    gp_uint8_t g8 = (gp_uint8_t)(GP_COLOR_G( _c ) * rgba_255);
    gp_uint8_t b8 = (gp_uint8_t)(GP_COLOR_B( _c ) * rgba_255);
    gp_uint8_t a8 = (gp_uint8_t)(GP_COLOR_A( _c ) * rgba_255);

    gp_uint32_t argb = ((gp_uint32_t)a8 << 24) | ((gp_uint32_t)r8 << 16) |
        ((gp_uint32_t)g8 << 8) | (gp_uint32_t)b8;

    return argb;
}
//////////////////////////////////////////////////////////////////////////
gp_uint32_t gp_ellipse_arc_segment_count( float _angle_delta, gp_uint8_t _quality )
{
    float angle_delta_abs = _angle_delta < 0.f ? -_angle_delta : _angle_delta;
    float segment_factor = angle_delta_abs / gp_constant_two_pi * (float)_quality;
    gp_uint32_t segment_count = (gp_uint32_t)segment_factor;
    if( (float)segment_count < segment_factor )
    {
        ++segment_count;
    }

    return segment_count == 0 ? 1 : segment_count;
}
//////////////////////////////////////////////////////////////////////////
float gp_ellipse_arc_sample_t( float _angle_delta, gp_uint8_t _quality, gp_uint32_t _index )
{
    float angle_delta_abs = _angle_delta < 0.f ? -_angle_delta : _angle_delta;
    float angle_offset = gp_constant_two_pi / (float)_quality * (float)_index;
    if( angle_offset > angle_delta_abs )
    {
        angle_offset = angle_delta_abs;
    }

    return angle_offset / angle_delta_abs;
}
//////////////////////////////////////////////////////////////////////////
float gp_ellipse_arc_sample_angle( float _angle_from, float _angle_to, gp_uint8_t _quality, gp_uint32_t _index )
{
    float angle_delta = _angle_to - _angle_from;
    if( _index >= gp_ellipse_arc_segment_count( angle_delta, _quality ) )
    {
        return _angle_to;
    }

    float angle_offset = gp_constant_two_pi / (float)_quality * (float)_index;

    return angle_delta < 0.f
        ? _angle_from - angle_offset
        : _angle_from + angle_offset;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_mesh_push_index( const gp_mesh_t * _mesh, gp_uint32_t _iterator, gp_uint32_t _index )
{
    if( _mesh->indices_buffer == GP_NULLPTR )
    {
        return GP_SUCCESSFUL;
    }

#if defined(GP_DEBUG)
    if( _mesh->index_count <= _iterator || _index >= _mesh->vertex_count ||
        (_mesh->indices_stride != sizeof( gp_uint32_t ) && _index >= UINT16_MAX) )
    {
        return GP_FAILURE;
    }
#endif

    gp_uint8_t * destination = (gp_uint8_t *)_mesh->indices_buffer + _mesh->indices_offset + _mesh->indices_stride * _iterator;
    if( _mesh->indices_stride == sizeof( gp_uint32_t ) )
    {
        *(gp_uint32_t *)destination = _index;
    }
    else
    {
        *(gp_uint16_t *)destination = (gp_uint16_t)_index;
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_mesh_push_position( const gp_mesh_t * _mesh, gp_uint32_t _iterator, float _x, float _y )
{
#if defined(GP_DEBUG)
    if( _mesh->vertex_count <= _iterator )
    {
        return GP_FAILURE;
    }
#endif

    gp_vec2f_t p;
    p.x = _x;
    p.y = _y;

    if( _mesh->positions_buffer != GP_NULLPTR )
    {
        *(gp_vec2f_t *)((gp_uint8_t *)_mesh->positions_buffer + _mesh->positions_offset + _mesh->positions_stride * _iterator) = p;
    }

    if( _mesh->uv_buffer != GP_NULLPTR )
    {
        *(gp_vec2f_t *)((gp_uint8_t *)_mesh->uv_buffer + _mesh->uv_offset + _mesh->uv_stride * _iterator) = p;
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_mesh_push_color( const gp_mesh_t * _mesh, gp_uint32_t _iterator, gp_argb_t _c )
{
    if( _mesh->colors_buffer == GP_NULLPTR )
    {
        return GP_SUCCESSFUL;
    }

#if defined(GP_DEBUG)
    if( _mesh->vertex_count <= _iterator )
    {
        return GP_FAILURE;
    }
#endif

    * (gp_argb_t *)((gp_uint8_t *)_mesh->colors_buffer + _mesh->colors_offset + _mesh->colors_stride * _iterator) = _c;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_mesh_push_uv( const gp_state_t * _state, const gp_mesh_t * _mesh, gp_uint32_t _iterator, float _u, float _v )
{
    if( _mesh->uv_buffer == GP_NULLPTR )
    {
        return GP_SUCCESSFUL;
    }

#if defined(GP_DEBUG)
    if( _mesh->vertex_count <= _iterator )
    {
        return GP_FAILURE;
    }
#endif

    gp_vec2f_t uv;
    uv.x = _state->uv_ou + _u * _state->uv_su;
    uv.y = _state->uv_ov + _v * _state->uv_sv;

    *(gp_vec2f_t *)((gp_uint8_t *)_mesh->uv_buffer + _mesh->uv_offset + _mesh->uv_stride * _iterator) = uv;

    return GP_SUCCESSFUL;
}
gp_result_t gp_mesh_push_uv_map( const gp_state_t * _state, const gp_mesh_t * _mesh, gp_uint32_t _iterator, float _x, float _y, float _ox, float _oy, float _w, float _h )
{
    float u = (_x + _ox) / _w;
    float v = (_y + _oy) / _h;

    gp_result_t result = gp_mesh_push_uv( _state, _mesh, _iterator, u, v );

    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_mesh_get_position( const gp_mesh_t * _mesh, gp_uint32_t _iterator, gp_vec2f_t * const _pos )
{
#if defined(GP_DEBUG)
    if( _mesh->vertex_count <= _iterator )
    {
        return GP_FAILURE;
    }
#endif

    const gp_vec2f_t * pos = (const gp_vec2f_t *)((gp_uint8_t *)_mesh->positions_buffer + _mesh->positions_offset + _mesh->positions_stride * _iterator);

    *_pos = *pos;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
void * gp_canvas_scratch_reallocate_array( gp_canvas_t * _canvas, void * _pointer, gp_size_t _element_size, gp_size_t _count )
{
    if( _element_size != 0 && _count > SIZE_MAX / _element_size )
    {
        return GP_NULLPTR;
    }

    return gp_canvas_scratch_reallocate( _canvas, _pointer, _element_size * _count );
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_mesh_writer_position( gp_mesh_writer_t * _writer, gp_uint32_t _vertex, float _x, float _y )
{
    if( _writer == GP_NULLPTR || _writer->mesh == GP_NULLPTR || _vertex >= _writer->vertex_count )
    {
        return GP_FAILURE;
    }

    return gp_mesh_push_position( _writer->mesh, _writer->vertex_offset + _vertex, _x, _y );
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_mesh_writer_color( gp_mesh_writer_t * _writer, gp_uint32_t _vertex, gp_argb_t _color )
{
    if( _writer == GP_NULLPTR || _writer->mesh == GP_NULLPTR || _vertex >= _writer->vertex_count )
    {
        return GP_FAILURE;
    }

    return gp_mesh_push_color( _writer->mesh, _writer->vertex_offset + _vertex, _color );
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_mesh_writer_uv( gp_mesh_writer_t * _writer, gp_uint32_t _vertex, float _u, float _v )
{
    if( _writer == GP_NULLPTR || _writer->mesh == GP_NULLPTR || _vertex >= _writer->vertex_count )
    {
        return GP_FAILURE;
    }

    if( _writer->mesh->uv_buffer == GP_NULLPTR )
    {
        return GP_SUCCESSFUL;
    }

    gp_vec2f_t uv;
    uv.x = _u;
    uv.y = _v;
    *(gp_vec2f_t *)((gp_uint8_t *)_writer->mesh->uv_buffer + _writer->mesh->uv_offset + _writer->mesh->uv_stride * (_writer->vertex_offset + _vertex)) = uv;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_mesh_writer_index( gp_mesh_writer_t * _writer, gp_uint32_t _index, gp_uint32_t _vertex )
{
    if( _writer == GP_NULLPTR || _writer->mesh == GP_NULLPTR ||
        _index >= _writer->index_count || _vertex >= _writer->vertex_count )
    {
        return GP_FAILURE;
    }

    return gp_mesh_push_index( _writer->mesh, _writer->index_offset + _index, _writer->vertex_offset + _vertex );
}
//////////////////////////////////////////////////////////////////////////
