#include "graphics/graphics.h"

#include "path.h"
#include "detail.h"

//////////////////////////////////////////////////////////////////////////
typedef struct gp_rounded_polygon_vertex_t
{
    float entry_x;
    float entry_y;
    float exit_x;
    float exit_y;
    float control_x;
    float control_y;
    gp_bool_t rounded;
} gp_rounded_polygon_vertex_t;
//////////////////////////////////////////////////////////////////////////
static void __rounded_polygon_get_point( const void * _points, gp_size_t _point_stride, gp_size_t _index, float * const _x, float * const _y )
{
    const gp_uint8_t * point_bytes = (const gp_uint8_t *)_points + _index * _point_stride;
    const float * point = (const float *)point_bytes;

    *_x = point[0];
    *_y = point[1];
}
//////////////////////////////////////////////////////////////////////////
static float __rounded_polygon_cross( float _lhs_x, float _lhs_y, float _rhs_x, float _rhs_y )
{
    float cross = _lhs_x * _rhs_y - _lhs_y * _rhs_x;
    return cross;
}
//////////////////////////////////////////////////////////////////////////
static void __rounded_polygon_calculate_vertex( const void * _points, gp_size_t _point_count, gp_size_t _point_stride, gp_size_t _index, float _radius, float _signed_area, gp_rounded_polygon_vertex_t * const _vertex )
{
    gp_size_t previous_index = (_index + _point_count - 1) % _point_count;
    gp_size_t next_index = (_index + 1) % _point_count;

    float previous_x;
    float previous_y;
    __rounded_polygon_get_point( _points, _point_stride, previous_index, &previous_x, &previous_y );

    float point_x;
    float point_y;
    __rounded_polygon_get_point( _points, _point_stride, _index, &point_x, &point_y );

    float next_x;
    float next_y;
    __rounded_polygon_get_point( _points, _point_stride, next_index, &next_x, &next_y );

    float incoming_x = point_x - previous_x;
    float incoming_y = point_y - previous_y;
    float outgoing_x = next_x - point_x;
    float outgoing_y = next_y - point_y;
    float incoming_length = GP_MATH_SQRTF( incoming_x * incoming_x + incoming_y * incoming_y );
    float outgoing_length = GP_MATH_SQRTF( outgoing_x * outgoing_x + outgoing_y * outgoing_y );

    if( incoming_length <= 0.0001f || outgoing_length <= 0.0001f )
    {
        _vertex->entry_x = point_x;
        _vertex->entry_y = point_y;
        _vertex->exit_x = point_x;
        _vertex->exit_y = point_y;
        _vertex->control_x = point_x;
        _vertex->control_y = point_y;
        _vertex->rounded = GP_FALSE;

        return;
    }

    float turn = __rounded_polygon_cross( incoming_x, incoming_y, outgoing_x, outgoing_y );
    float orientation = _signed_area >= 0.f ? 1.f : -1.f;
    float incoming_normal_x = orientation * incoming_y / incoming_length;
    float incoming_normal_y = orientation * -incoming_x / incoming_length;
    float outgoing_normal_x = orientation * outgoing_y / outgoing_length;
    float outgoing_normal_y = orientation * -outgoing_x / outgoing_length;
    float incoming_offset_x = point_x + incoming_normal_x * _radius;
    float incoming_offset_y = point_y + incoming_normal_y * _radius;
    float outgoing_offset_x = point_x + outgoing_normal_x * _radius;
    float outgoing_offset_y = point_y + outgoing_normal_y * _radius;
    float intersection_x = point_x + (incoming_normal_x + outgoing_normal_x) * (_radius * 0.5f);
    float intersection_y = point_y + (incoming_normal_y + outgoing_normal_y) * (_radius * 0.5f);

    if( turn < -0.0001f || turn > 0.0001f )
    {
        float offset_delta_x = outgoing_offset_x - incoming_offset_x;
        float offset_delta_y = outgoing_offset_y - incoming_offset_y;
        float intersection_factor = __rounded_polygon_cross( offset_delta_x, offset_delta_y, outgoing_x, outgoing_y ) / turn;

        intersection_x = incoming_offset_x + incoming_x * intersection_factor;
        intersection_y = incoming_offset_y + incoming_y * intersection_factor;
    }

    if( _signed_area * turn > 0.0001f )
    {
        _vertex->entry_x = incoming_offset_x;
        _vertex->entry_y = incoming_offset_y;
        _vertex->exit_x = outgoing_offset_x;
        _vertex->exit_y = outgoing_offset_y;
        _vertex->control_x = intersection_x;
        _vertex->control_y = intersection_y;
        _vertex->rounded = _radius > 0.0001f ? GP_TRUE : GP_FALSE;
    }
    else
    {
        _vertex->entry_x = intersection_x;
        _vertex->entry_y = intersection_y;
        _vertex->exit_x = intersection_x;
        _vertex->exit_y = intersection_y;
        _vertex->control_x = intersection_x;
        _vertex->control_y = intersection_y;
        _vertex->rounded = GP_FALSE;
    }
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __rounded_polygon( gp_canvas_t * _canvas, const void * _points, gp_size_t _point_count, gp_size_t _point_stride, float _radius )
{
    if( _point_count < 3 )
    {
        return GP_SUCCESSFUL;
    }

#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _points == GP_NULLPTR || _point_stride < sizeof( float ) * 2 )
    {
        return GP_FAILURE;
    }
#endif

    float radius = _radius < 0.f ? 0.f : _radius;
    float signed_area = 0.f;

    for( gp_size_t index = 0; index != _point_count; ++index )
    {
        gp_size_t next_index = (index + 1) % _point_count;

        float point_x;
        float point_y;
        __rounded_polygon_get_point( _points, _point_stride, index, &point_x, &point_y );

        float next_x;
        float next_y;
        __rounded_polygon_get_point( _points, _point_stride, next_index, &next_x, &next_y );

        signed_area += __rounded_polygon_cross( point_x, point_y, next_x, next_y );
    }

    gp_rounded_polygon_vertex_t vertex;
    __rounded_polygon_calculate_vertex( _points, _point_count, _point_stride, 0, radius, signed_area, &vertex );

    gp_path_t * path;
    GP_CALL( gp_path_create_scratch_internal, (_canvas, &path) );

    GP_CALL( gp_path_move_to, (path, vertex.entry_x, vertex.entry_y) );
    if( vertex.rounded == GP_TRUE )
    {
        GP_CALL( gp_path_quadratic_curve_to, (path, vertex.control_x, vertex.control_y, vertex.exit_x, vertex.exit_y) );
    }

    for( gp_size_t index = 1; index != _point_count; ++index )
    {
        __rounded_polygon_calculate_vertex( _points, _point_count, _point_stride, index, radius, signed_area, &vertex );

        GP_CALL( gp_path_line_to, (path, vertex.entry_x, vertex.entry_y) );

        if( vertex.rounded == GP_TRUE )
        {
            GP_CALL( gp_path_quadratic_curve_to, (path, vertex.control_x, vertex.control_y, vertex.exit_x, vertex.exit_y) );
        }
    }

    GP_CALL( gp_path_close, (path) );
    gp_result_t result = gp_draw_path( _canvas, path );
    GP_CALL( gp_path_destroy, (path) );

    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_rounded_polygon( gp_canvas_t * _canvas, const void * _points, gp_size_t _point_count, gp_size_t _point_stride, float _radius )
{
    if( _canvas == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    gp_size_t scratch_mark = gp_canvas_scratch_mark( _canvas );
    gp_result_t result = __rounded_polygon( _canvas, _points, _point_count, _point_stride, _radius );
    gp_canvas_scratch_reset( _canvas, scratch_mark );

    return result;
}
