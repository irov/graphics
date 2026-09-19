#include "detail.h"

#include "struct.h"

#include <string.h>

//////////////////////////////////////////////////////////////////////////
typedef struct gp_scratch_allocation_t
{
    gp_arena_block_t * block;
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

    gp_size_t aligned_value = (_value + alignment - 1U) & ~(alignment - 1U);
    return aligned_value;
}
//////////////////////////////////////////////////////////////////////////
static void * gp_arena_allocate( gp_arena_t * _arena, const gp_graphics_t * _graphics, gp_size_t _size, gp_size_t _block_size, gp_size_t _limit )
{
    gp_size_t aligned_size = gp_align_size( _size );
    if( aligned_size == SIZE_MAX )
    {
        return GP_NULLPTR;
    }

    gp_arena_block_t * block = _arena->current;
    gp_size_t offset = block == GP_NULLPTR ? 0 : _arena->offset - block->begin;
    while( block != GP_NULLPTR && aligned_size > block->capacity - offset )
    {
        if( block->next == GP_NULLPTR )
        {
            break;
        }

        block = block->next;
        offset = 0;
    }

    if( block == GP_NULLPTR || aligned_size > block->capacity - offset )
    {
        gp_arena_block_t * unused = _arena->current;
        if( unused != GP_NULLPTR && _arena->offset != unused->begin )
        {
            unused = unused->next;
        }

        gp_size_t begin = unused == GP_NULLPTR ? _arena->capacity : unused->begin;
        gp_size_t capacity = gp_align_size( _block_size );
        if( capacity < aligned_size )
        {
            capacity = aligned_size;
        }

        gp_size_t available = _limit - begin;
        if( capacity > available )
        {
            capacity = available;
        }

        if( capacity < aligned_size || capacity > SIZE_MAX - sizeof( gp_arena_block_t ) )
        {
            return GP_NULLPTR;
        }

        gp_arena_block_t * next = (gp_arena_block_t *)(*_graphics->malloc)(sizeof( gp_arena_block_t ) + capacity, _graphics->ud);
        if( next == GP_NULLPTR )
        {
            return GP_NULLPTR;
        }

        if( unused != GP_NULLPTR )
        {
            block = unused->prev;
            while( unused != GP_NULLPTR )
            {
                gp_arena_block_t * following = unused->next;
                (*_graphics->free)(unused, _graphics->ud);
                unused = following;
            }
        }

        next->next = GP_NULLPTR;
        next->prev = block;
        next->begin = begin;
        next->capacity = capacity;
        if( block == GP_NULLPTR )
        {
            _arena->blocks = next;
        }
        else
        {
            block->next = next;
        }

        _arena->capacity = begin + capacity;
        block = next;
        offset = 0;
    }

    _arena->current = block;
    _arena->offset = block->begin + offset + aligned_size;
    if( _arena->peak < _arena->offset )
    {
        _arena->peak = _arena->offset;
    }

    void * memory = (gp_uint8_t *)(block + 1) + offset;
    return memory;
}
//////////////////////////////////////////////////////////////////////////
void gp_arena_clear( gp_arena_t * _arena, const gp_graphics_t * _graphics )
{
    gp_arena_block_t * block = _arena->blocks;
    while( block != GP_NULLPTR )
    {
        gp_arena_block_t * next = block->next;
        (*_graphics->free)(block, _graphics->ud);
        block = next;
    }

    _arena->blocks = GP_NULLPTR;
    _arena->current = GP_NULLPTR;
    _arena->capacity = 0;
    _arena->offset = 0;
}
//////////////////////////////////////////////////////////////////////////
void * gp_canvas_recording_allocate( gp_canvas_t * _canvas, gp_size_t _size )
{
    const gp_graphics_t * graphics = _canvas->graphics;
    void * memory = gp_arena_allocate( &_canvas->recording_arena, graphics, _size, graphics->recording_memory_size, SIZE_MAX );
    return memory;
}
//////////////////////////////////////////////////////////////////////////
const gp_state_t * gp_canvas_copy_state( gp_canvas_t * _canvas )
{
    if( _canvas->state_invalidate == GP_FALSE )
    {
        const gp_state_t * state = GP_LIST_BACK( _canvas->states );
        return state;
    }

    gp_state_t * state = GP_NEW( _canvas, gp_state_t );
    if( state == GP_NULLPTR )
    {
        return GP_NULLPTR;
    }

    *state = *GP_GET_STATE( _canvas );
    gp_state_apply_stroke_scale( state );
    state->next = GP_NULLPTR;
    state->prev = GP_NULLPTR;
    GP_LIST_PUSHBACK( gp_state_t, _canvas->states, state );
    _canvas->state_invalidate = GP_FALSE;
    return state;
}
//////////////////////////////////////////////////////////////////////////
void gp_state_apply_stroke_scale( gp_state_t * _state )
{
    if( _state->stroke_scale == GP_STROKE_SCALE_WORLD )
    {
        return;
    }

    float axis_x_length = GP_MATH_SQRTF( _state->axis_x_x * _state->axis_x_x + _state->axis_x_y * _state->axis_x_y );
    float axis_y_length = GP_MATH_SQRTF( _state->axis_y_x * _state->axis_y_x + _state->axis_y_y * _state->axis_y_y );
    float scale = axis_x_length > axis_y_length ? axis_x_length : axis_y_length;

    if( scale <= 0.f )
    {
        return;
    }

    float inverse_scale = 1.f / scale;
    _state->thickness *= inverse_scale;
    _state->outline_width *= inverse_scale;
    _state->penumbra *= inverse_scale;
    _state->border_width *= inverse_scale;
    _state->feather *= inverse_scale;
    _state->shadow.offset_x *= inverse_scale;
    _state->shadow.offset_y *= inverse_scale;
    _state->shadow.spread *= inverse_scale;
    _state->shadow.blur *= inverse_scale;
    _state->start_marker.length *= inverse_scale;
    _state->start_marker.width *= inverse_scale;
    _state->start_marker.inset *= inverse_scale;
    _state->end_marker.length *= inverse_scale;
    _state->end_marker.width *= inverse_scale;
    _state->end_marker.inset *= inverse_scale;
}
//////////////////////////////////////////////////////////////////////////
void * gp_canvas_scratch_reallocate( gp_canvas_t * _canvas, void * _pointer, gp_size_t _size )
{
    if( _size == 0 || _size > SIZE_MAX - sizeof( gp_scratch_allocation_t ) )
    {
        return GP_NULLPTR;
    }

    gp_graphics_t * graphics = _canvas->graphics;
    gp_arena_t * arena = &graphics->scratch_arena;
    gp_size_t allocation_size = gp_align_size( sizeof( gp_scratch_allocation_t ) + _size );
    if( allocation_size == SIZE_MAX )
    {
        return GP_NULLPTR;
    }

    if( _pointer != GP_NULLPTR )
    {
        gp_scratch_allocation_t * previous = (gp_scratch_allocation_t *)_pointer - 1;
        gp_arena_block_t * block = previous->block;
        gp_size_t previous_offset = (gp_uint8_t *)previous - (gp_uint8_t *)(block + 1);
        gp_size_t previous_size = gp_align_size( sizeof( gp_scratch_allocation_t ) + previous->size );
        if( block == arena->current && block->begin + previous_offset + previous_size == arena->offset &&
            allocation_size <= block->capacity - previous_offset )
        {
            previous->size = _size;
            arena->offset = block->begin + previous_offset + allocation_size;
            if( arena->peak < arena->offset )
            {
                arena->peak = arena->offset;
            }

            return _pointer;
        }
    }

    gp_scratch_allocation_t * allocation = (gp_scratch_allocation_t *)gp_arena_allocate( arena, graphics, allocation_size, 4096U, graphics->scratch_memory_size );
    if( allocation == GP_NULLPTR )
    {
        return GP_NULLPTR;
    }

    allocation->block = arena->current;
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

    gp_arena_t * arena = &_canvas->graphics->scratch_arena;
    gp_scratch_allocation_t * allocation = (gp_scratch_allocation_t *)_pointer - 1;
    gp_arena_block_t * block = allocation->block;
    gp_size_t offset = (gp_uint8_t *)allocation - (gp_uint8_t *)(block + 1);
    gp_size_t allocation_size = gp_align_size( sizeof( gp_scratch_allocation_t ) + allocation->size );
    if( block == arena->current && block->begin + offset + allocation_size == arena->offset )
    {
        gp_canvas_scratch_reset( _canvas, block->begin + offset );
    }
}
//////////////////////////////////////////////////////////////////////////
void * gp_canvas_scratch_malloc_callback( gp_size_t _size, void * _ud )
{
    void * memory = gp_canvas_scratch_reallocate( (gp_canvas_t *)_ud, GP_NULLPTR, _size );
    return memory;
}
//////////////////////////////////////////////////////////////////////////
void * gp_canvas_scratch_realloc_callback( void * _pointer, gp_size_t _size, void * _ud )
{
    void * memory = gp_canvas_scratch_reallocate( (gp_canvas_t *)_ud, _pointer, _size );
    return memory;
}
//////////////////////////////////////////////////////////////////////////
void gp_canvas_scratch_free_callback( void * _pointer, void * _ud )
{
    gp_canvas_scratch_free( (gp_canvas_t *)_ud, _pointer );
}
//////////////////////////////////////////////////////////////////////////
gp_size_t gp_canvas_scratch_mark( const gp_canvas_t * _canvas )
{
    gp_size_t mark = _canvas->graphics->scratch_arena.offset;
    return mark;
}
//////////////////////////////////////////////////////////////////////////
void gp_canvas_scratch_reset( gp_canvas_t * _canvas, gp_size_t _mark )
{
    gp_arena_t * arena = &_canvas->graphics->scratch_arena;
    if( _mark > arena->offset )
    {
        return;
    }

    while( arena->current != GP_NULLPTR && arena->current->prev != GP_NULLPTR && arena->current->begin >= _mark )
    {
        arena->current = arena->current->prev;
    }

    arena->offset = _mark;
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

    if( segment_count == 0 )
    {
        return 1;
    }

    return segment_count;
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

    float sample = angle_offset / angle_delta_abs;
    return sample;
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

    if( angle_delta < 0.f )
    {
        float angle = _angle_from - angle_offset;
        return angle;
    }

    float angle = _angle_from + angle_offset;
    return angle;
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

    gp_size_t size = _element_size * _count;
    void * memory = gp_canvas_scratch_reallocate( _canvas, _pointer, size );
    return memory;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_mesh_writer_position( gp_mesh_writer_t * _writer, gp_uint32_t _vertex, float _x, float _y )
{
    if( _writer == GP_NULLPTR || _writer->mesh == GP_NULLPTR || _vertex >= _writer->vertex_count )
    {
        return GP_FAILURE;
    }

    gp_uint32_t vertex = _writer->vertex_offset + _vertex;
    gp_result_t result = gp_mesh_push_position( _writer->mesh, vertex, _x, _y );
    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_mesh_writer_color( gp_mesh_writer_t * _writer, gp_uint32_t _vertex, gp_argb_t _color )
{
    if( _writer == GP_NULLPTR || _writer->mesh == GP_NULLPTR || _vertex >= _writer->vertex_count )
    {
        return GP_FAILURE;
    }

    gp_uint32_t vertex = _writer->vertex_offset + _vertex;
    gp_result_t result = gp_mesh_push_color( _writer->mesh, vertex, _color );
    return result;
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

    gp_uint32_t index = _writer->index_offset + _index;
    gp_uint32_t vertex = _writer->vertex_offset + _vertex;
    gp_result_t result = gp_mesh_push_index( _writer->mesh, index, vertex );
    return result;
}
//////////////////////////////////////////////////////////////////////////
