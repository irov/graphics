#include "geometry.h"

#include "detail.h"

//////////////////////////////////////////////////////////////////////////
static const void * __geometry_element( const void * _buffer, gp_size_t _offset, gp_size_t _stride, gp_uint32_t _index )
{
    const gp_uint8_t * bytes = (const gp_uint8_t *)_buffer;
    const void * element = bytes + _offset + _stride * _index;

    return element;
}
//////////////////////////////////////////////////////////////////////////
static gp_color_t __source_color_float( const gp_state_t * _state, const void * _buffer, gp_size_t _offset, gp_size_t _stride, gp_uint32_t _index )
{
    gp_color_t color = _state->color;

    if( _buffer != GP_NULLPTR )
    {
        const float * source = (const float *)__geometry_element( _buffer, _offset, _stride, _index );
        color.r *= source[0];
        color.g *= source[1];
        color.b *= source[2];
        color.a *= source[3];
    }

    return color;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_geometry_write( gp_mesh_writer_t * _writer, const gp_geometry_t * _geometry )
{
    for( gp_uint32_t vertex = 0; vertex != _geometry->vertex_count; ++vertex )
    {
        const float * position = (const float *)__geometry_element( _geometry->positions_buffer, _geometry->positions_offset, _geometry->positions_stride, vertex );
        float x = _geometry->translation_x + _geometry->axis_x_x * position[0] + _geometry->axis_y_x * position[1];
        float y = _geometry->translation_y + _geometry->axis_x_y * position[0] + _geometry->axis_y_y * position[1];
        gp_color_t color = __source_color_float( _geometry->state, _geometry->colors_buffer, _geometry->colors_offset, _geometry->colors_stride, vertex );
        gp_color_mul( &color, &color, &_writer->mesh->color );
        gp_argb_t argb = gp_color_argb( &color );
        GP_CALL( gp_mesh_writer_position, (_writer, vertex, x, y) );
        GP_CALL( gp_mesh_writer_color, (_writer, vertex, argb) );

        if( _geometry->uv_buffer == GP_NULLPTR )
        {
            GP_CALL( gp_mesh_writer_uv, (_writer, vertex, _geometry->state->uv_u, _geometry->state->uv_v) );
        }
        else
        {
            const float * uv = (const float *)__geometry_element( _geometry->uv_buffer, _geometry->uv_offset, _geometry->uv_stride, vertex );
            GP_CALL( gp_mesh_writer_uv, (_writer, vertex, uv[0], uv[1]) );
        }
    }

    for( gp_uint32_t index = 0; index != _geometry->index_count; ++index )
    {
        const gp_uint32_t * source = (const gp_uint32_t *)__geometry_element( _geometry->indices_buffer, _geometry->indices_offset, _geometry->indices_stride, index );

#if defined(GP_DEBUG)
        if( *source >= _geometry->vertex_count )
        {
            return GP_FAILURE;
        }
#endif

        GP_CALL( gp_mesh_writer_index, (_writer, index, *source) );
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_textured_rects_write( gp_mesh_writer_t * _writer, const gp_textured_rects_t * _rects )
{
    for( gp_uint32_t rect = 0; rect != _rects->rect_count; ++rect )
    {
        const float * bounds = (const float *)__geometry_element( _rects->bounds_buffer, _rects->bounds_offset, _rects->bounds_stride, rect );
        const float * uv = (const float *)__geometry_element( _rects->uv_buffer, _rects->uv_offset, _rects->uv_stride, rect );
        float x0 = bounds[0];
        float y0 = bounds[1];
        float x1 = x0 + bounds[2];
        float y1 = y0 + bounds[3];
        float positions[8] = {x0, y0, x1, y0, x1, y1, x0, y1};
        float coordinates[8] = {uv[0], uv[1], uv[0] + uv[2], uv[1], uv[0] + uv[2], uv[1] + uv[3], uv[0], uv[1] + uv[3]};
        gp_color_t color = __source_color_float( _rects->state, _rects->colors_buffer, _rects->colors_offset, _rects->colors_stride, rect );
        gp_color_mul( &color, &color, &_writer->mesh->color );
        gp_argb_t argb = gp_color_argb( &color );
        gp_uint32_t vertex_offset = rect * 4U;
        gp_uint32_t index_offset = rect * 6U;

        for( gp_uint32_t corner = 0; corner != 4U; ++corner )
        {
            float source_x = positions[corner * 2U];
            float source_y = positions[corner * 2U + 1U];
            float x = _rects->translation_x + _rects->axis_x_x * source_x + _rects->axis_y_x * source_y;
            float y = _rects->translation_y + _rects->axis_x_y * source_x + _rects->axis_y_y * source_y;
            GP_CALL( gp_mesh_writer_position, (_writer, vertex_offset + corner, x, y) );
            GP_CALL( gp_mesh_writer_color, (_writer, vertex_offset + corner, argb) );
            GP_CALL( gp_mesh_writer_uv, (_writer, vertex_offset + corner, coordinates[corner * 2U], coordinates[corner * 2U + 1U]) );
        }

        GP_CALL( gp_mesh_writer_index, (_writer, index_offset + 0U, vertex_offset + 0U) );
        GP_CALL( gp_mesh_writer_index, (_writer, index_offset + 1U, vertex_offset + 1U) );
        GP_CALL( gp_mesh_writer_index, (_writer, index_offset + 2U, vertex_offset + 2U) );
        GP_CALL( gp_mesh_writer_index, (_writer, index_offset + 3U, vertex_offset + 0U) );
        GP_CALL( gp_mesh_writer_index, (_writer, index_offset + 4U, vertex_offset + 2U) );
        GP_CALL( gp_mesh_writer_index, (_writer, index_offset + 5U, vertex_offset + 3U) );
    }

    return GP_SUCCESSFUL;
}
