#include "geometry.h"

#include "detail.h"

#include <string.h>

//////////////////////////////////////////////////////////////////////////
static gp_result_t __geometry_reserve( gp_geometry_t * _geometry, gp_uint32_t _vertex_count, gp_uint32_t _index_count )
{
    if( _vertex_count > _geometry->vertex_capacity )
    {
        gp_size_t size = (gp_size_t)_vertex_count * sizeof( gp_geometry_vertex_t );
        if( _vertex_count != 0 && size / sizeof( gp_geometry_vertex_t ) != _vertex_count )
        {
            return GP_FAILURE;
        }

        gp_geometry_vertex_t * vertices = (gp_geometry_vertex_t *)(*_geometry->realloc)( _geometry->vertices, size, _geometry->ud );
        if( vertices == GP_NULLPTR )
        {
            return GP_FAILURE;
        }

        _geometry->vertices = vertices;
        _geometry->vertex_capacity = _vertex_count;
    }

    if( _index_count > _geometry->index_capacity )
    {
        gp_size_t size = (gp_size_t)_index_count * sizeof( gp_uint32_t );
        if( _index_count != 0 && size / sizeof( gp_uint32_t ) != _index_count )
        {
            return GP_FAILURE;
        }

        gp_uint32_t * indices = (gp_uint32_t *)(*_geometry->realloc)( _geometry->indices, size, _geometry->ud );
        if( indices == GP_NULLPTR )
        {
            return GP_FAILURE;
        }

        _geometry->indices = indices;
        _geometry->index_capacity = _index_count;
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_geometry_create( gp_geometry_t ** _geometry, gp_malloc_t _malloc, gp_realloc_t _realloc, gp_free_t _free, void * _ud )
{
#if defined(GP_DEBUG)
    if( _geometry == GP_NULLPTR || _malloc == GP_NULLPTR || _realloc == GP_NULLPTR || _free == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    gp_geometry_t * geometry = (gp_geometry_t *)(*_malloc)( sizeof( gp_geometry_t ), _ud );
    if( geometry == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    memset( geometry, 0, sizeof( gp_geometry_t ) );
    geometry->malloc = _malloc;
    geometry->realloc = _realloc;
    geometry->free = _free;
    geometry->ud = _ud;
    *_geometry = geometry;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_geometry_destroy( gp_geometry_t * _geometry )
{
#if defined(GP_DEBUG)
    if( _geometry == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    if( _geometry->vertices != GP_NULLPTR )
    {
        (*_geometry->free)( _geometry->vertices, _geometry->ud );
    }

    if( _geometry->indices != GP_NULLPTR )
    {
        (*_geometry->free)( _geometry->indices, _geometry->ud );
    }

    (*_geometry->free)( _geometry, _geometry->ud );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_geometry_clear( gp_geometry_t * _geometry )
{
#if defined(GP_DEBUG)
    if( _geometry == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    _geometry->vertex_count = 0;
    _geometry->index_count = 0;
    _geometry->expected_rect_count = 0;
    _geometry->rect_count = 0;
    _geometry->building = GP_FALSE;
    _geometry->complete = GP_FALSE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_geometry_begin_textured_rects( gp_geometry_t * _geometry, gp_size_t _rect_count )
{
#if defined(GP_DEBUG)
    if( _geometry == GP_NULLPTR || _rect_count == 0 || _geometry->building == GP_TRUE )
    {
        return GP_FAILURE;
    }
#endif

    if( _rect_count > UINT32_MAX / 6U )
    {
        return GP_FAILURE;
    }

    gp_uint32_t rect_count = (gp_uint32_t)_rect_count;
    gp_uint32_t vertex_count = rect_count * 4U;
    gp_uint32_t index_count = rect_count * 6U;
    if( __geometry_reserve( _geometry, vertex_count, index_count ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    _geometry->vertex_count = 0;
    _geometry->index_count = 0;
    _geometry->expected_rect_count = rect_count;
    _geometry->rect_count = 0;
    _geometry->building = GP_TRUE;
    _geometry->complete = GP_FALSE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_geometry_add_textured_rect( gp_geometry_t * _geometry, float _x, float _y, float _width, float _height, float _u0, float _v0, float _u1, float _v1, float _r, float _g, float _b, float _a )
{
#if defined(GP_DEBUG)
    if( _geometry == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    if( _geometry->building == GP_FALSE )
    {
        return GP_FAILURE;
    }

    if( GP_MATH_ISFINITE( _x ) == 0 || GP_MATH_ISFINITE( _y ) == 0 )
    {
        return GP_FAILURE;
    }

    if( GP_MATH_ISFINITE( _width ) == 0 || GP_MATH_ISFINITE( _height ) == 0 )
    {
        return GP_FAILURE;
    }

    if( GP_MATH_ISFINITE( _u0 ) == 0 || GP_MATH_ISFINITE( _v0 ) == 0 )
    {
        return GP_FAILURE;
    }

    if( GP_MATH_ISFINITE( _u1 ) == 0 || GP_MATH_ISFINITE( _v1 ) == 0 )
    {
        return GP_FAILURE;
    }

    if( GP_MATH_ISFINITE( _r ) == 0 || GP_MATH_ISFINITE( _g ) == 0 )
    {
        return GP_FAILURE;
    }

    if( GP_MATH_ISFINITE( _b ) == 0 || GP_MATH_ISFINITE( _a ) == 0 )
    {
        return GP_FAILURE;
    }
#endif

    if( _geometry->rect_count == _geometry->expected_rect_count )
    {
        return GP_FAILURE;
    }

    gp_uint32_t vertex = _geometry->rect_count * 4U;
    gp_uint32_t index = _geometry->rect_count * 6U;
    gp_color_t color;
    color.r = _r;
    color.g = _g;
    color.b = _b;
    color.a = _a;
    gp_geometry_vertex_t * vertices = _geometry->vertices + vertex;
    vertices[0].x = _x;
    vertices[0].y = _y;
    vertices[0].color = color;
    vertices[0].u = _u0;
    vertices[0].v = _v0;
    vertices[1].x = _x + _width;
    vertices[1].y = _y;
    vertices[1].color = color;
    vertices[1].u = _u1;
    vertices[1].v = _v0;
    vertices[2].x = _x + _width;
    vertices[2].y = _y + _height;
    vertices[2].color = color;
    vertices[2].u = _u1;
    vertices[2].v = _v1;
    vertices[3].x = _x;
    vertices[3].y = _y + _height;
    vertices[3].color = color;
    vertices[3].u = _u0;
    vertices[3].v = _v1;
    _geometry->indices[index + 0U] = vertex + 0U;
    _geometry->indices[index + 1U] = vertex + 1U;
    _geometry->indices[index + 2U] = vertex + 2U;
    _geometry->indices[index + 3U] = vertex + 0U;
    _geometry->indices[index + 4U] = vertex + 2U;
    _geometry->indices[index + 5U] = vertex + 3U;
    ++_geometry->rect_count;
    _geometry->vertex_count += 4U;
    _geometry->index_count += 6U;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_geometry_end( gp_geometry_t * _geometry )
{
#if defined(GP_DEBUG)
    if( _geometry == GP_NULLPTR || _geometry->building == GP_FALSE )
    {
        return GP_FAILURE;
    }
#endif

    if( _geometry->rect_count != _geometry->expected_rect_count )
    {
        return GP_FAILURE;
    }

    _geometry->building = GP_FALSE;
    _geometry->complete = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_geometry_get_memory_size( const gp_geometry_t * _geometry, gp_size_t * _size )
{
#if defined(GP_DEBUG)
    if( _geometry == GP_NULLPTR || _size == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    gp_size_t vertex_size = (gp_size_t)_geometry->vertex_capacity * sizeof( gp_geometry_vertex_t );
    gp_size_t index_size = (gp_size_t)_geometry->index_capacity * sizeof( gp_uint32_t );
    *_size = sizeof( gp_geometry_t ) + vertex_size + index_size;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_geometry_write( gp_mesh_writer_t * _writer, const gp_geometry_draw_t * _draw )
{
    const gp_geometry_t * geometry = _draw->geometry;
    for( gp_uint32_t vertex = 0; vertex != geometry->vertex_count; ++vertex )
    {
        const gp_geometry_vertex_t * source = geometry->vertices + vertex;
        float x = _draw->translation_x + _draw->axis_x_x * source->x + _draw->axis_y_x * source->y;
        float y = _draw->translation_y + _draw->axis_x_y * source->x + _draw->axis_y_y * source->y;
        gp_color_t color;
        gp_color_mul( &color, &source->color, &_draw->tint );
        gp_color_mul( &color, &color, &_writer->mesh->color );
        gp_uint32_t argb = gp_color_argb( &color );
        GP_CALL( gp_mesh_writer_position, (_writer, vertex, x, y) );
        GP_CALL( gp_mesh_writer_color, (_writer, vertex, argb) );
        GP_CALL( gp_mesh_writer_uv, (_writer, vertex, source->u, source->v) );
    }

    for( gp_uint32_t index = 0; index != geometry->index_count; ++index )
    {
        GP_CALL( gp_mesh_writer_index, (_writer, index, geometry->indices[index]) );
    }

    return GP_SUCCESSFUL;
}
