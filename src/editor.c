#include "graphics/graphics.h"

#include "detail.h"
#include "struct.h"

#include <float.h>

//////////////////////////////////////////////////////////////////////////
typedef struct gp_nine_slice_payload_t
{
    float destination_x[4];
    float destination_y[4];
    float source_x[4];
    float source_y[4];
    float uv_x;
    float uv_y;
    float uv_width;
    float uv_height;
    gp_nine_slice_mode_t edge_mode;
    gp_nine_slice_mode_t center_mode;
    gp_color_t color;
} gp_nine_slice_payload_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_grid_payload_t
{
    float minimum_x;
    float minimum_y;
    float maximum_x;
    float maximum_y;
    float origin_x;
    float origin_y;
    float axis_x_x;
    float axis_x_y;
    float axis_y_x;
    float axis_y_y;
    float minor_step;
    gp_uint32_t major_interval;
    float minor_thickness;
    float major_thickness;
    gp_color_t minor_color;
    gp_color_t major_color;
    int64_t first_x;
    int64_t last_x;
    int64_t first_y;
    int64_t last_y;
} gp_grid_payload_t;
//////////////////////////////////////////////////////////////////////////
static gp_uint32_t __tile_count( float _destination_extent, float _source_extent, gp_nine_slice_mode_t _mode )
{
    if( _destination_extent <= 0.f || _source_extent <= 0.f )
    {
        return 0;
    }

    if( _mode == GP_NINE_SLICE_STRETCH )
    {
        return 1;
    }

    gp_uint32_t count = (gp_uint32_t)(_destination_extent / _source_extent);

    if( (float)count * _source_extent < _destination_extent )
    {
        ++count;
    }

    return count;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __write_quad( gp_mesh_writer_t * _writer, gp_uint32_t _quad, float _x0, float _y0, float _x1, float _y1, float _u0, float _v0, float _u1, float _v1, gp_argb_t _color )
{
    gp_uint32_t vertex = _quad * 4U;
    gp_uint32_t index = _quad * 6U;

    GP_CALL( gp_mesh_writer_position, (_writer, vertex + 0U, _x0, _y0) );
    GP_CALL( gp_mesh_writer_position, (_writer, vertex + 1U, _x1, _y0) );
    GP_CALL( gp_mesh_writer_position, (_writer, vertex + 2U, _x1, _y1) );
    GP_CALL( gp_mesh_writer_position, (_writer, vertex + 3U, _x0, _y1) );
    GP_CALL( gp_mesh_writer_color, (_writer, vertex + 0U, _color) );
    GP_CALL( gp_mesh_writer_color, (_writer, vertex + 1U, _color) );
    GP_CALL( gp_mesh_writer_color, (_writer, vertex + 2U, _color) );
    GP_CALL( gp_mesh_writer_color, (_writer, vertex + 3U, _color) );
    GP_CALL( gp_mesh_writer_uv, (_writer, vertex + 0U, _u0, _v0) );
    GP_CALL( gp_mesh_writer_uv, (_writer, vertex + 1U, _u1, _v0) );
    GP_CALL( gp_mesh_writer_uv, (_writer, vertex + 2U, _u1, _v1) );
    GP_CALL( gp_mesh_writer_uv, (_writer, vertex + 3U, _u0, _v1) );
    GP_CALL( gp_mesh_writer_index, (_writer, index + 0U, vertex + 0U) );
    GP_CALL( gp_mesh_writer_index, (_writer, index + 1U, vertex + 1U) );
    GP_CALL( gp_mesh_writer_index, (_writer, index + 2U, vertex + 2U) );
    GP_CALL( gp_mesh_writer_index, (_writer, index + 3U, vertex + 0U) );
    GP_CALL( gp_mesh_writer_index, (_writer, index + 4U, vertex + 2U) );
    GP_CALL( gp_mesh_writer_index, (_writer, index + 5U, vertex + 3U) );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_nine_slice_mode_t __nine_slice_x_mode( const gp_nine_slice_payload_t * _payload, gp_uint32_t _column, gp_uint32_t _row )
{
    if( _column != 1U )
    {
        return GP_NINE_SLICE_STRETCH;
    }

    if( _row == 1U )
    {
        return _payload->center_mode;
    }

    return _payload->edge_mode;
}
//////////////////////////////////////////////////////////////////////////
static gp_nine_slice_mode_t __nine_slice_y_mode( const gp_nine_slice_payload_t * _payload, gp_uint32_t _column, gp_uint32_t _row )
{
    if( _row != 1U )
    {
        return GP_NINE_SLICE_STRETCH;
    }

    if( _column == 1U )
    {
        return _payload->center_mode;
    }

    return _payload->edge_mode;
}
//////////////////////////////////////////////////////////////////////////
static gp_uint32_t __nine_slice_quad_count( const gp_nine_slice_payload_t * _payload )
{
    gp_uint32_t count = 0;

    for( gp_uint32_t row = 0; row != 3U; ++row )
    {
        float destination_height = _payload->destination_y[row + 1U] - _payload->destination_y[row];
        float source_height = _payload->source_y[row + 1U] - _payload->source_y[row];

        for( gp_uint32_t column = 0; column != 3U; ++column )
        {
            float destination_width = _payload->destination_x[column + 1U] - _payload->destination_x[column];
            float source_width = _payload->source_x[column + 1U] - _payload->source_x[column];
            gp_uint32_t columns = __tile_count( destination_width, source_width, __nine_slice_x_mode( _payload, column, row ) );
            gp_uint32_t rows = __tile_count( destination_height, source_height, __nine_slice_y_mode( _payload, column, row ) );

            if( columns == 0 || rows == 0 )
            {
                continue;
            }

            if( columns > UINT32_MAX / rows || count > UINT32_MAX - columns * rows )
            {
                return 0;
            }

            count += columns * rows;
        }
    }

    return count;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __write_nine_slice( gp_mesh_writer_t * _writer, const void * _payload_pointer )
{
    const gp_nine_slice_payload_t * payload = (const gp_nine_slice_payload_t *)_payload_pointer;
    gp_color_t color;
    gp_color_mul( &color, &payload->color, &_writer->mesh->color );
    gp_argb_t argb = gp_color_argb( &color );
    float source_width = payload->source_x[3] - payload->source_x[0];
    float source_height = payload->source_y[3] - payload->source_y[0];
    gp_uint32_t quad = 0;

    for( gp_uint32_t row = 0; row != 3U; ++row )
    {
        float destination_region_height = payload->destination_y[row + 1U] - payload->destination_y[row];
        float source_region_height = payload->source_y[row + 1U] - payload->source_y[row];
        for( gp_uint32_t column = 0; column != 3U; ++column )
        {
            float destination_region_width = payload->destination_x[column + 1U] - payload->destination_x[column];
            float source_region_width = payload->source_x[column + 1U] - payload->source_x[column];
            gp_nine_slice_mode_t x_mode = __nine_slice_x_mode( payload, column, row );
            gp_nine_slice_mode_t y_mode = __nine_slice_y_mode( payload, column, row );
            gp_uint32_t columns = __tile_count( destination_region_width, source_region_width, x_mode );
            gp_uint32_t rows = __tile_count( destination_region_height, source_region_height, y_mode );

            for( gp_uint32_t tile_y = 0; tile_y != rows; ++tile_y )
            {
                float destination_y0 = payload->destination_y[row];
                float destination_y1 = payload->destination_y[row + 1U];
                float source_fraction_y = 1.f;

                if( y_mode == GP_NINE_SLICE_TILE )
                {
                    destination_y0 += (float)tile_y * source_region_height;
                    destination_y1 = destination_y0 + source_region_height;

                    if( destination_y1 > payload->destination_y[row + 1U] )
                    {
                        destination_y1 = payload->destination_y[row + 1U];
                    }

                    source_fraction_y = (destination_y1 - destination_y0) / source_region_height;
                }

                for( gp_uint32_t tile_x = 0; tile_x != columns; ++tile_x )
                {
                    float destination_x0 = payload->destination_x[column];
                    float destination_x1 = payload->destination_x[column + 1U];
                    float source_fraction_x = 1.f;

                    if( x_mode == GP_NINE_SLICE_TILE )
                    {
                        destination_x0 += (float)tile_x * source_region_width;
                        destination_x1 = destination_x0 + source_region_width;

                        if( destination_x1 > payload->destination_x[column + 1U] )
                        {
                            destination_x1 = payload->destination_x[column + 1U];
                        }

                        source_fraction_x = (destination_x1 - destination_x0) / source_region_width;
                    }

                    float source_x0 = payload->source_x[column];
                    float source_y0 = payload->source_y[row];
                    float source_x1 = source_x0 + source_region_width * source_fraction_x;
                    float source_y1 = source_y0 + source_region_height * source_fraction_y;
                    float u0 = payload->uv_x + source_x0 / source_width * payload->uv_width;
                    float v0 = payload->uv_y + source_y0 / source_height * payload->uv_height;
                    float u1 = payload->uv_x + source_x1 / source_width * payload->uv_width;
                    float v1 = payload->uv_y + source_y1 / source_height * payload->uv_height;
                    GP_CALL( __write_quad, (_writer, quad, destination_x0, destination_y0, destination_x1, destination_y1, u0, v0, u1, v1, argb) );
                    ++quad;
                }
            }
        }
    }

#if defined(GP_DEBUG)
    if( quad * 4U != _writer->vertex_count || quad * 6U != _writer->index_count )
    {
        return GP_FAILURE;
    }
#endif

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_nine_slice( gp_canvas_t * _canvas, float _destination_x, float _destination_y, float _destination_width, float _destination_height, float _source_width, float _source_height, float _uv_x, float _uv_y, float _uv_width, float _uv_height, float _inset_left, float _inset_top, float _inset_right, float _inset_bottom, gp_nine_slice_mode_t _edge_mode, gp_nine_slice_mode_t _center_mode )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _destination_width <= 0.f || _destination_height <= 0.f || _source_width <= 0.f || _source_height <= 0.f )
    {
        return GP_FAILURE;
    }

    if( _inset_left < 0.f || _inset_top < 0.f || _inset_right < 0.f || _inset_bottom < 0.f )
    {
        return GP_FAILURE;
    }

    if( _inset_left + _inset_right > _source_width || _inset_top + _inset_bottom > _source_height )
    {
        return GP_FAILURE;
    }

    if( (_edge_mode != GP_NINE_SLICE_STRETCH && _edge_mode != GP_NINE_SLICE_TILE) || (_center_mode != GP_NINE_SLICE_STRETCH && _center_mode != GP_NINE_SLICE_TILE) )
    {
        return GP_FAILURE;
    }
#endif

    float horizontal_scale = 1.f;
    float vertical_scale = 1.f;

    if( _inset_left + _inset_right > _destination_width )
    {
        horizontal_scale = _destination_width / (_inset_left + _inset_right);
    }

    if( _inset_top + _inset_bottom > _destination_height )
    {
        vertical_scale = _destination_height / (_inset_top + _inset_bottom);
    }

    gp_nine_slice_payload_t payload;
    payload.destination_x[0] = _destination_x;
    payload.destination_x[1] = _destination_x + _inset_left * horizontal_scale;
    payload.destination_x[2] = _destination_x + _destination_width - _inset_right * horizontal_scale;
    payload.destination_x[3] = _destination_x + _destination_width;
    payload.destination_y[0] = _destination_y;
    payload.destination_y[1] = _destination_y + _inset_top * vertical_scale;
    payload.destination_y[2] = _destination_y + _destination_height - _inset_bottom * vertical_scale;
    payload.destination_y[3] = _destination_y + _destination_height;
    payload.source_x[0] = 0.f;
    payload.source_x[1] = _inset_left;
    payload.source_x[2] = _source_width - _inset_right;
    payload.source_x[3] = _source_width;
    payload.source_y[0] = 0.f;
    payload.source_y[1] = _inset_top;
    payload.source_y[2] = _source_height - _inset_bottom;
    payload.source_y[3] = _source_height;
    payload.uv_x = _uv_x;
    payload.uv_y = _uv_y;
    payload.uv_width = _uv_width;
    payload.uv_height = _uv_height;
    payload.edge_mode = _edge_mode;
    payload.center_mode = _center_mode;
    payload.color = GP_GET_STATE( _canvas )->color;

    gp_uint32_t quad_count = __nine_slice_quad_count( &payload );

    if( quad_count == 0 || quad_count > UINT32_MAX / 6U )
    {
        return GP_FAILURE;
    }

    gp_result_t result = gp_internal_geometry( _canvas, quad_count * 4U, quad_count * 6U, __write_nine_slice, &payload, sizeof( payload ) );

    return result;
}
//////////////////////////////////////////////////////////////////////////
static int64_t __floor_to_int64( float _value )
{
    int64_t result = (int64_t)_value;

    if( (float)result > _value )
    {
        --result;
    }

    return result;
}
//////////////////////////////////////////////////////////////////////////
static int64_t __ceil_to_int64( float _value )
{
    int64_t result = (int64_t)_value;

    if( (float)result < _value )
    {
        ++result;
    }

    return result;
}
//////////////////////////////////////////////////////////////////////////
static gp_bool_t __clip_grid_line( const gp_grid_payload_t * _payload, float _point_x, float _point_y, float _direction_x, float _direction_y, float * const _from_x, float * const _from_y, float * const _to_x, float * const _to_y )
{
    float minimum_t = -FLT_MAX;
    float maximum_t = FLT_MAX;

    if( GP_EQUAL_F( _direction_x, 0.000001f ) == GP_TRUE )
    {
        if( _point_x < _payload->minimum_x || _point_x > _payload->maximum_x )
        {
            return GP_FALSE;
        }
    }
    else
    {
        float first = (_payload->minimum_x - _point_x) / _direction_x;
        float second = (_payload->maximum_x - _point_x) / _direction_x;

        if( first > second )
        {
            float temporary = first;
            first = second;
            second = temporary;
        }

        minimum_t = first > minimum_t ? first : minimum_t;
        maximum_t = second < maximum_t ? second : maximum_t;
    }

    if( GP_EQUAL_F( _direction_y, 0.000001f ) == GP_TRUE )
    {
        if( _point_y < _payload->minimum_y || _point_y > _payload->maximum_y )
        {
            return GP_FALSE;
        }
    }
    else
    {
        float first = (_payload->minimum_y - _point_y) / _direction_y;
        float second = (_payload->maximum_y - _point_y) / _direction_y;

        if( first > second )
        {
            float temporary = first;
            first = second;
            second = temporary;
        }

        minimum_t = first > minimum_t ? first : minimum_t;
        maximum_t = second < maximum_t ? second : maximum_t;
    }

    if( minimum_t > maximum_t )
    {
        return GP_FALSE;
    }

    *_from_x = _point_x + _direction_x * minimum_t;
    *_from_y = _point_y + _direction_y * minimum_t;
    *_to_x = _point_x + _direction_x * maximum_t;
    *_to_y = _point_y + _direction_y * maximum_t;

    return GP_TRUE;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __write_grid_line( gp_mesh_writer_t * _writer, gp_uint32_t _quad, float _from_x, float _from_y, float _to_x, float _to_y, float _thickness, gp_argb_t _color )
{
    float direction_x = _to_x - _from_x;
    float direction_y = _to_y - _from_y;
    float length = GP_MATH_SQRTF( direction_x * direction_x + direction_y * direction_y );

    if( length <= 0.f )
    {
        return GP_FAILURE;
    }

    float offset_x = -direction_y / length * _thickness * 0.5f;
    float offset_y = direction_x / length * _thickness * 0.5f;
    gp_uint32_t vertex = _quad * 4U;
    gp_uint32_t index = _quad * 6U;
    GP_CALL( gp_mesh_writer_position, (_writer, vertex + 0U, _from_x + offset_x, _from_y + offset_y) );
    GP_CALL( gp_mesh_writer_position, (_writer, vertex + 1U, _to_x + offset_x, _to_y + offset_y) );
    GP_CALL( gp_mesh_writer_position, (_writer, vertex + 2U, _to_x - offset_x, _to_y - offset_y) );
    GP_CALL( gp_mesh_writer_position, (_writer, vertex + 3U, _from_x - offset_x, _from_y - offset_y) );
    GP_CALL( gp_mesh_writer_color, (_writer, vertex + 0U, _color) );
    GP_CALL( gp_mesh_writer_color, (_writer, vertex + 1U, _color) );
    GP_CALL( gp_mesh_writer_color, (_writer, vertex + 2U, _color) );
    GP_CALL( gp_mesh_writer_color, (_writer, vertex + 3U, _color) );
    GP_CALL( gp_mesh_writer_uv, (_writer, vertex + 0U, 0.f, 0.f) );
    GP_CALL( gp_mesh_writer_uv, (_writer, vertex + 1U, 0.f, 0.f) );
    GP_CALL( gp_mesh_writer_uv, (_writer, vertex + 2U, 0.f, 0.f) );
    GP_CALL( gp_mesh_writer_uv, (_writer, vertex + 3U, 0.f, 0.f) );
    GP_CALL( gp_mesh_writer_index, (_writer, index + 0U, vertex + 0U) );
    GP_CALL( gp_mesh_writer_index, (_writer, index + 1U, vertex + 1U) );
    GP_CALL( gp_mesh_writer_index, (_writer, index + 2U, vertex + 2U) );
    GP_CALL( gp_mesh_writer_index, (_writer, index + 3U, vertex + 0U) );
    GP_CALL( gp_mesh_writer_index, (_writer, index + 4U, vertex + 2U) );
    GP_CALL( gp_mesh_writer_index, (_writer, index + 5U, vertex + 3U) );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_bool_t __grid_line( const gp_grid_payload_t * _payload, gp_bool_t _x_axis, int64_t _index, float * const _from_x, float * const _from_y, float * const _to_x, float * const _to_y )
{
    float coordinate = (float)_index * _payload->minor_step;
    float point_x;
    float point_y;
    float direction_x;
    float direction_y;

    if( _x_axis == GP_TRUE )
    {
        point_x = _payload->origin_x + _payload->axis_x_x * coordinate;
        point_y = _payload->origin_y + _payload->axis_x_y * coordinate;
        direction_x = _payload->axis_y_x;
        direction_y = _payload->axis_y_y;
    }
    else
    {
        point_x = _payload->origin_x + _payload->axis_y_x * coordinate;
        point_y = _payload->origin_y + _payload->axis_y_y * coordinate;
        direction_x = _payload->axis_x_x;
        direction_y = _payload->axis_x_y;
    }

    gp_bool_t clipped = __clip_grid_line( _payload, point_x, point_y, direction_x, direction_y, _from_x, _from_y, _to_x, _to_y );

    return clipped;
}
//////////////////////////////////////////////////////////////////////////
static gp_uint32_t __grid_line_count( const gp_grid_payload_t * _payload )
{
    gp_uint32_t count = 0;

    for( int64_t index = _payload->first_x; index <= _payload->last_x; ++index )
    {
        float from_x;
        float from_y;
        float to_x;
        float to_y;

        if( __grid_line( _payload, GP_TRUE, index, &from_x, &from_y, &to_x, &to_y ) == GP_TRUE )
        {
            ++count;
        }
    }

    for( int64_t index = _payload->first_y; index <= _payload->last_y; ++index )
    {
        float from_x;
        float from_y;
        float to_x;
        float to_y;

        if( __grid_line( _payload, GP_FALSE, index, &from_x, &from_y, &to_x, &to_y ) == GP_TRUE )
        {
            ++count;
        }
    }

    return count;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __write_grid( gp_mesh_writer_t * _writer, const void * _payload_pointer )
{
    const gp_grid_payload_t * payload = (const gp_grid_payload_t *)_payload_pointer;
    gp_color_t minor_color;
    gp_color_t major_color;
    gp_color_mul( &minor_color, &payload->minor_color, &_writer->mesh->color );
    gp_color_mul( &major_color, &payload->major_color, &_writer->mesh->color );
    gp_argb_t minor_argb = gp_color_argb( &minor_color );
    gp_argb_t major_argb = gp_color_argb( &major_color );
    gp_uint32_t quad = 0;

    for( gp_uint32_t axis = 0; axis != 2U; ++axis )
    {
        int64_t first = axis == 0U ? payload->first_x : payload->first_y;
        int64_t last = axis == 0U ? payload->last_x : payload->last_y;

        for( int64_t line = first; line <= last; ++line )
        {
            float from_x;
            float from_y;
            float to_x;
            float to_y;

            if( __grid_line( payload, axis == 0U ? GP_TRUE : GP_FALSE, line, &from_x, &from_y, &to_x, &to_y ) == GP_FALSE )
            {
                continue;
            }

            gp_bool_t major = line % (int64_t)payload->major_interval == 0 ? GP_TRUE : GP_FALSE;
            float thickness = major == GP_TRUE ? payload->major_thickness : payload->minor_thickness;
            gp_argb_t color = major == GP_TRUE ? major_argb : minor_argb;
            GP_CALL( __write_grid_line, (_writer, quad, from_x, from_y, to_x, to_y, thickness, color) );
            ++quad;
        }
    }

#if defined(GP_DEBUG)
    if( quad * 4U != _writer->vertex_count || quad * 6U != _writer->index_count )
    {
        return GP_FAILURE;
    }
#endif

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __grid( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _origin_x, float _origin_y, float _axis_x_x, float _axis_x_y, float _axis_y_x, float _axis_y_y, float _minor_step, gp_uint32_t _major_interval, float _minor_thickness, float _major_thickness, float _minor_r, float _minor_g, float _minor_b, float _minor_a, float _major_r, float _major_g, float _major_b, float _major_a )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _width <= 0.f || _height <= 0.f || _minor_step <= 0.f || _major_interval == 0U || _minor_thickness <= 0.f || _major_thickness <= 0.f )
    {
        return GP_FAILURE;
    }
#endif

    float determinant = _axis_x_x * _axis_y_y - _axis_x_y * _axis_y_x;

    if( GP_EQUAL_F( determinant, 0.000001f ) == GP_TRUE )
    {
        return GP_FAILURE;
    }

    gp_grid_payload_t payload;
    payload.minimum_x = _x;
    payload.minimum_y = _y;
    payload.maximum_x = _x + _width;
    payload.maximum_y = _y + _height;
    payload.origin_x = _origin_x;
    payload.origin_y = _origin_y;
    payload.axis_x_x = _axis_x_x;
    payload.axis_x_y = _axis_x_y;
    payload.axis_y_x = _axis_y_x;
    payload.axis_y_y = _axis_y_y;
    payload.minor_step = _minor_step;
    payload.major_interval = _major_interval;
    payload.minor_thickness = _minor_thickness;
    payload.major_thickness = _major_thickness;
    payload.minor_color.r = _minor_r;
    payload.minor_color.g = _minor_g;
    payload.minor_color.b = _minor_b;
    payload.minor_color.a = _minor_a;
    payload.major_color.r = _major_r;
    payload.major_color.g = _major_g;
    payload.major_color.b = _major_b;
    payload.major_color.a = _major_a;

    float inverse_determinant = 1.f / determinant;
    float corners[4][2] = {{_x, _y}, {_x + _width, _y}, {_x + _width, _y + _height}, {_x, _y + _height}};
    float minimum_u = FLT_MAX;
    float maximum_u = -FLT_MAX;
    float minimum_v = FLT_MAX;
    float maximum_v = -FLT_MAX;

    for( gp_uint32_t corner = 0; corner != 4U; ++corner )
    {
        float x = corners[corner][0] - _origin_x;
        float y = corners[corner][1] - _origin_y;
        float u = (x * _axis_y_y - y * _axis_y_x) * inverse_determinant / _minor_step;
        float v = (y * _axis_x_x - x * _axis_x_y) * inverse_determinant / _minor_step;
        minimum_u = u < minimum_u ? u : minimum_u;
        maximum_u = u > maximum_u ? u : maximum_u;
        minimum_v = v < minimum_v ? v : minimum_v;
        maximum_v = v > maximum_v ? v : maximum_v;
    }

    payload.first_x = __floor_to_int64( minimum_u ) - 1;
    payload.last_x = __ceil_to_int64( maximum_u ) + 1;
    payload.first_y = __floor_to_int64( minimum_v ) - 1;
    payload.last_y = __ceil_to_int64( maximum_v ) + 1;

    if( payload.last_x - payload.first_x > 1000000 || payload.last_y - payload.first_y > 1000000 )
    {
        return GP_FAILURE;
    }

    const gp_state_t * state = GP_GET_STATE( _canvas );
    gp_color_mul( &payload.minor_color, &payload.minor_color, &state->color );
    gp_color_mul( &payload.major_color, &payload.major_color, &state->color );

    if( state->stroke_scale == GP_STROKE_SCALE_SCREEN )
    {
        float axis_x_length = GP_MATH_SQRTF( state->axis_x_x * state->axis_x_x + state->axis_x_y * state->axis_x_y );
        float axis_y_length = GP_MATH_SQRTF( state->axis_y_x * state->axis_y_x + state->axis_y_y * state->axis_y_y );
        float scale = axis_x_length > axis_y_length ? axis_x_length : axis_y_length;

        if( scale > 0.f )
        {
            payload.minor_thickness /= scale;
            payload.major_thickness /= scale;
        }
    }

    gp_uint32_t line_count = __grid_line_count( &payload );

    if( line_count == 0 || line_count > UINT32_MAX / 6U )
    {
        return GP_FAILURE;
    }

    gp_result_t result = gp_internal_geometry( _canvas, line_count * 4U, line_count * 6U, __write_grid, &payload, sizeof( payload ) );

    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_grid( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _origin_x, float _origin_y, float _axis_x_x, float _axis_x_y, float _axis_y_x, float _axis_y_y, float _minor_step, gp_uint32_t _major_interval, float _minor_thickness, float _major_thickness, float _minor_r, float _minor_g, float _minor_b, float _minor_a, float _major_r, float _major_g, float _major_b, float _major_a )
{
    gp_result_t result = __grid( _canvas, _x, _y, _width, _height, _origin_x, _origin_y, _axis_x_x, _axis_x_y, _axis_y_x, _axis_y_y, _minor_step, _major_interval, _minor_thickness, _major_thickness, _minor_r, _minor_g, _minor_b, _minor_a, _major_r, _major_g, _major_b, _major_a );

    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_grid_bounded( gp_canvas_t * _canvas, float _viewport_x, float _viewport_y, float _viewport_width, float _viewport_height, float _range_x, float _range_y, float _range_width, float _range_height, float _origin_x, float _origin_y, float _axis_x_x, float _axis_x_y, float _axis_y_x, float _axis_y_y, float _minor_step, gp_uint32_t _major_interval, float _minor_thickness, float _major_thickness, float _minor_r, float _minor_g, float _minor_b, float _minor_a, float _major_r, float _major_g, float _major_b, float _major_a )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _viewport_width <= 0.f || _viewport_height <= 0.f || _range_width <= 0.f || _range_height <= 0.f )
    {
        return GP_FAILURE;
    }
#endif

    float minimum_x = _viewport_x > _range_x ? _viewport_x : _range_x;
    float minimum_y = _viewport_y > _range_y ? _viewport_y : _range_y;
    float viewport_maximum_x = _viewport_x + _viewport_width;
    float viewport_maximum_y = _viewport_y + _viewport_height;
    float range_maximum_x = _range_x + _range_width;
    float range_maximum_y = _range_y + _range_height;
    float maximum_x = viewport_maximum_x < range_maximum_x ? viewport_maximum_x : range_maximum_x;
    float maximum_y = viewport_maximum_y < range_maximum_y ? viewport_maximum_y : range_maximum_y;

    if( maximum_x <= minimum_x || maximum_y <= minimum_y )
    {
        return GP_SUCCESSFUL;
    }

    float width = maximum_x - minimum_x;
    float height = maximum_y - minimum_y;
    gp_result_t result = __grid( _canvas, minimum_x, minimum_y, width, height, _origin_x, _origin_y, _axis_x_x, _axis_x_y, _axis_y_x, _axis_y_y, _minor_step, _major_interval, _minor_thickness, _major_thickness, _minor_r, _minor_g, _minor_b, _minor_a, _major_r, _major_g, _major_b, _major_a );

    return result;
}
