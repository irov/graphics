#include "graphics/graphics.h"

#include "detail.h"
#include "struct.h"

#include <float.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////
static void __primitive_get_point( const void * _points, gp_size_t _point_stride, gp_size_t _index, float * const _x, float * const _y )
{
    const gp_uint8_t * point_bytes = (const gp_uint8_t *)_points + _index * _point_stride;
    memcpy( _x, point_bytes + sizeof( float ) * 0, sizeof( float ) );
    memcpy( _y, point_bytes + sizeof( float ) * 1, sizeof( float ) );
}
//////////////////////////////////////////////////////////////////////////
static float __primitive_length( float _x, float _y )
{
    return GP_MATH_SQRTF( _x * _x + _y * _y );
}
//////////////////////////////////////////////////////////////////////////
typedef struct gp_primitive_style_t
{
    float width;
    gp_color_t color;
    gp_color_t outline_color;
} gp_primitive_style_t;
//////////////////////////////////////////////////////////////////////////
#if defined(GP_DEBUG)
static gp_bool_t __primitive_finite( float _value )
{
    return _value == _value && _value >= -FLT_MAX && _value <= FLT_MAX ? GP_TRUE : GP_FALSE;
}
#endif
//////////////////////////////////////////////////////////////////////////
static gp_result_t __primitive_validate_layout( const gp_path_layout_t * _layout, gp_size_t _point_stride )
{
#if defined(GP_DEBUG)
    if( _layout == GP_NULLPTR )
    {
        return GP_SUCCESSFUL;
    }

    if( _layout->width_offset != GP_PATH_ATTRIBUTE_NONE &&
        (_point_stride < sizeof( float ) || _layout->width_offset < sizeof( float ) * 2 || _layout->width_offset > _point_stride - sizeof( float )) )
    {
        return GP_FAILURE;
    }

    if( _layout->color_offset != GP_PATH_ATTRIBUTE_NONE &&
        (_point_stride < sizeof( gp_color_t ) || _layout->color_offset < sizeof( float ) * 2 || _layout->color_offset > _point_stride - sizeof( gp_color_t )) )
    {
        return GP_FAILURE;
    }

    if( _layout->outline_color_offset != GP_PATH_ATTRIBUTE_NONE &&
        (_point_stride < sizeof( gp_color_t ) || _layout->outline_color_offset < sizeof( float ) * 2 || _layout->outline_color_offset > _point_stride - sizeof( gp_color_t )) )
    {
        return GP_FAILURE;
    }
#else
    GP_UNUSED( _layout );
    GP_UNUSED( _point_stride );
#endif

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static void __primitive_get_style( const gp_canvas_t * _canvas, const void * _points, gp_size_t _point_stride, gp_size_t _index, const gp_path_layout_t * _layout, gp_primitive_style_t * const _style )
{
    const gp_state_t * state = GP_GET_STATE( _canvas );
    _style->width = state->thickness;
    _style->color = state->color;
    _style->outline_color = state->outline_color;

    if( _layout == GP_NULLPTR )
    {
        return;
    }

    const gp_uint8_t * point_bytes = (const gp_uint8_t *)_points + _index * _point_stride;
    if( _layout->width_offset != GP_PATH_ATTRIBUTE_NONE )
    {
        memcpy( &_style->width, point_bytes + _layout->width_offset, sizeof( float ) );
    }
    if( _layout->color_offset != GP_PATH_ATTRIBUTE_NONE )
    {
        memcpy( &_style->color, point_bytes + _layout->color_offset, sizeof( gp_color_t ) );
    }
    if( _layout->outline_color_offset != GP_PATH_ATTRIBUTE_NONE )
    {
        memcpy( &_style->outline_color, point_bytes + _layout->outline_color_offset, sizeof( gp_color_t ) );
    }
}
//////////////////////////////////////////////////////////////////////////
#if defined(GP_DEBUG)
static gp_result_t __primitive_validate_style( const gp_primitive_style_t * _style )
{
    if( __primitive_finite( _style->width ) == GP_FALSE || _style->width <= 0.f ||
        __primitive_finite( _style->color.r ) == GP_FALSE || __primitive_finite( _style->color.g ) == GP_FALSE ||
        __primitive_finite( _style->color.b ) == GP_FALSE || __primitive_finite( _style->color.a ) == GP_FALSE ||
        _style->color.r < 0.f || _style->color.r > 1.f || _style->color.g < 0.f || _style->color.g > 1.f ||
        _style->color.b < 0.f || _style->color.b > 1.f || _style->color.a < 0.f || _style->color.a > 1.f )
    {
        return GP_FAILURE;
    }

    if( __primitive_finite( _style->outline_color.r ) == GP_FALSE || __primitive_finite( _style->outline_color.g ) == GP_FALSE ||
        __primitive_finite( _style->outline_color.b ) == GP_FALSE || __primitive_finite( _style->outline_color.a ) == GP_FALSE ||
        _style->outline_color.r < 0.f || _style->outline_color.r > 1.f || _style->outline_color.g < 0.f || _style->outline_color.g > 1.f ||
        _style->outline_color.b < 0.f || _style->outline_color.b > 1.f || _style->outline_color.a < 0.f || _style->outline_color.a > 1.f )
    {
        return GP_FAILURE;
    }
    return GP_SUCCESSFUL;
}
#endif
//////////////////////////////////////////////////////////////////////////
static gp_result_t __primitive_apply_style( gp_path_t * _path, const gp_path_layout_t * _layout, const gp_primitive_style_t * _style )
{
    if( _layout == GP_NULLPTR )
    {
        return GP_SUCCESSFUL;
    }

    gp_path_style_t style;
    memset( &style, 0, sizeof( style ) );

    if( _layout->width_offset != GP_PATH_ATTRIBUTE_NONE )
    {
        style.attributes |= GP_PATH_STYLE_WIDTH;
        style.width = _style->width;
    }
    if( _layout->color_offset != GP_PATH_ATTRIBUTE_NONE )
    {
        style.attributes |= GP_PATH_STYLE_COLOR;
        style.color = _style->color;
    }

    if( _layout->outline_color_offset != GP_PATH_ATTRIBUTE_NONE )
    {
        style.attributes |= GP_PATH_STYLE_OUTLINE_COLOR;
        style.outline_color = _style->outline_color;
    }

    GP_CALL( gp_path_set_style, (_path, &style) );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static void __primitive_lerp_style( gp_primitive_style_t * const _style, const gp_primitive_style_t * _from, const gp_primitive_style_t * _to, float _t )
{
    float inverse_t = 1.f - _t;
    _style->width = _from->width * inverse_t + _to->width * _t;
    _style->color.r = _from->color.r * inverse_t + _to->color.r * _t;
    _style->color.g = _from->color.g * inverse_t + _to->color.g * _t;
    _style->color.b = _from->color.b * inverse_t + _to->color.b * _t;
    _style->color.a = _from->color.a * inverse_t + _to->color.a * _t;
    _style->outline_color.r = _from->outline_color.r * inverse_t + _to->outline_color.r * _t;
    _style->outline_color.g = _from->outline_color.g * inverse_t + _to->outline_color.g * _t;
    _style->outline_color.b = _from->outline_color.b * inverse_t + _to->outline_color.b * _t;
    _style->outline_color.a = _from->outline_color.a * inverse_t + _to->outline_color.a * _t;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __primitive_create_path( gp_canvas_t * _canvas, gp_path_t ** _path )
{
    const gp_state_t * state = GP_GET_STATE( _canvas );
    return gp_path_create( _path, _canvas->malloc, _canvas->realloc, _canvas->free, _canvas->ud, state->curve_quality, state->ellipse_quality );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __primitive_draw_destroy_path( gp_canvas_t * _canvas, gp_path_t * _path )
{
    gp_result_t result = gp_draw_path( _canvas, _path );
    GP_CALL( gp_path_destroy, (_path) );

    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_polyline( gp_canvas_t * _canvas, const void * _points, gp_size_t _point_count, gp_size_t _point_stride, const gp_path_layout_t * _layout, gp_bool_t _closed )
{
    if( _point_count == 0 )
    {
        return GP_SUCCESSFUL;
    }

#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _points == GP_NULLPTR || _point_stride < sizeof( float ) * 2 ||
        _point_count < 2 || (_closed == GP_TRUE && _point_count < 3) )
    {
        return GP_FAILURE;
    }

    gp_size_t validation_segment_count = _closed == GP_TRUE ? _point_count : _point_count - 1;
    for( gp_size_t segment = 0; segment != validation_segment_count; ++segment )
    {
        float from_x;
        float from_y;
        float to_x;
        float to_y;
        __primitive_get_point( _points, _point_stride, segment, &from_x, &from_y );
        __primitive_get_point( _points, _point_stride, (segment + 1) % _point_count, &to_x, &to_y );
        if( __primitive_length( to_x - from_x, to_y - from_y ) <= 0.0001f )
        {
            return GP_FAILURE;
        }
    }
#endif

    GP_CALL( __primitive_validate_layout, (_layout, _point_stride) );

#if defined(GP_DEBUG)
    for( gp_size_t index = 0; index != _point_count; ++index )
    {
        gp_primitive_style_t style;
        __primitive_get_style( _canvas, _points, _point_stride, index, _layout, &style );
        GP_CALL( __primitive_validate_style, (&style) );
    }
#endif

    gp_path_t * path;
    GP_CALL( __primitive_create_path, (_canvas, &path) );
    GP_CALL( gp_path_polyline, (path, _points, _point_count, _point_stride, _layout, _closed) );

    return __primitive_draw_destroy_path( _canvas, path );
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_rounded_rect_varying( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _radius_top_left, float _radius_top_right, float _radius_bottom_right, float _radius_bottom_left )
{
#if defined(GP_DEBUG)
    if( _width < 0.f || _height < 0.f ||
        _radius_top_left < 0.f || _radius_top_right < 0.f ||
        _radius_bottom_right < 0.f || _radius_bottom_left < 0.f )
    {
        return GP_FAILURE;
    }
#endif

    float maximum_radius = (_width < _height ? _width : _height) * 0.5f;
    float rtl = _radius_top_left > maximum_radius ? maximum_radius : _radius_top_left;
    float rtr = _radius_top_right > maximum_radius ? maximum_radius : _radius_top_right;
    float rbr = _radius_bottom_right > maximum_radius ? maximum_radius : _radius_bottom_right;
    float rbl = _radius_bottom_left > maximum_radius ? maximum_radius : _radius_bottom_left;

    gp_path_t * path;
    GP_CALL( __primitive_create_path, (_canvas, &path) );
    GP_CALL( gp_path_move_to, (path, _x, _y + rtl) );
    if( rtl > 0.f )
    {
        GP_CALL( gp_path_quadratic_curve_to, (path, _x, _y, _x + rtl, _y) );
    }
    GP_CALL( gp_path_line_to, (path, _x + _width - rtr, _y) );
    if( rtr > 0.f )
    {
        GP_CALL( gp_path_quadratic_curve_to, (path, _x + _width, _y, _x + _width, _y + rtr) );
    }
    GP_CALL( gp_path_line_to, (path, _x + _width, _y + _height - rbr) );
    if( rbr > 0.f )
    {
        GP_CALL( gp_path_quadratic_curve_to, (path, _x + _width, _y + _height, _x + _width - rbr, _y + _height) );
    }
    GP_CALL( gp_path_line_to, (path, _x + rbl, _y + _height) );
    if( rbl > 0.f )
    {
        GP_CALL( gp_path_quadratic_curve_to, (path, _x, _y + _height, _x, _y + _height - rbl) );
    }
    GP_CALL( gp_path_close, (path) );

    return __primitive_draw_destroy_path( _canvas, path );
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_capsule( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height )
{
#if defined(GP_DEBUG)
    if( _width <= 0.f || _height <= 0.f )
    {
        return GP_FAILURE;
    }
#endif

    float radius = (_width < _height ? _width : _height) * 0.5f;
    gp_result_t result = gp_rounded_rect( _canvas, _x, _y, _width, _height, radius );

    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_ring( gp_canvas_t * _canvas, float _x, float _y, float _inner_radius, float _outer_radius, float _angle_from, float _angle_to )
{
#if defined(GP_DEBUG)
    if( _inner_radius < 0.f || _outer_radius <= 0.f || _inner_radius >= _outer_radius )
    {
        return GP_FAILURE;
    }
#endif

    GP_CALL( gp_push_state, (_canvas) );

    gp_state_t * state = GP_GET_STATE( _canvas );
    if( state->fill == GP_FALSE )
    {
        GP_CALL( gp_begin_fill, (_canvas) );
    }

    GP_CALL( gp_set_inner_radius, (_canvas, _inner_radius / _outer_radius) );
    GP_CALL( gp_arc, (_canvas, _x, _y, _outer_radius, _angle_from, _angle_to) );
    GP_CALL( gp_pop_state, (_canvas) );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_regular_polygon( gp_canvas_t * _canvas, float _x, float _y, float _radius, gp_uint32_t _vertex_count, float _rotation )
{
#if defined(GP_DEBUG)
    if( _vertex_count < 3 || _vertex_count > UINT16_MAX || _radius < 0.f )
    {
        return GP_FAILURE;
    }
#endif

    float * points = GP_REALLOC( _canvas, GP_NULLPTR, float, _vertex_count * 2 );
    for( gp_uint32_t index = 0; index != _vertex_count; ++index )
    {
        float angle = _rotation + gp_constant_two_pi * (float)index / (float)_vertex_count;
        points[index * 2 + 0] = _x + GP_MATH_COSF( angle ) * _radius;
        points[index * 2 + 1] = _y + GP_MATH_SINF( angle ) * _radius;
    }

    gp_result_t result = gp_polyline( _canvas, points, _vertex_count, sizeof( float ) * 2, GP_NULLPTR, GP_TRUE );
    GP_FREE( _canvas, points );

    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_star( gp_canvas_t * _canvas, float _x, float _y, float _inner_radius, float _outer_radius, gp_uint32_t _ray_count, float _rotation )
{
#if defined(GP_DEBUG)
    if( _ray_count < 2 || _ray_count > UINT16_MAX / 2 || _inner_radius < 0.f || _outer_radius <= 0.f || _inner_radius > _outer_radius )
    {
        return GP_FAILURE;
    }
#endif

    gp_uint32_t point_count = _ray_count * 2;
    float * points = GP_REALLOC( _canvas, GP_NULLPTR, float, point_count * 2 );
    for( gp_uint32_t index = 0; index != point_count; ++index )
    {
        float radius = index % 2 == 0 ? _outer_radius : _inner_radius;
        float angle = _rotation + gp_constant_two_pi * (float)index / (float)point_count;
        points[index * 2 + 0] = _x + GP_MATH_COSF( angle ) * radius;
        points[index * 2 + 1] = _y + GP_MATH_SINF( angle ) * radius;
    }

    gp_result_t result = gp_polyline( _canvas, points, point_count, sizeof( float ) * 2, GP_NULLPTR, GP_TRUE );
    GP_FREE( _canvas, points );

    return result;
}
//////////////////////////////////////////////////////////////////////////
static float __primitive_signed_power( float _value, float _power )
{
    float absolute = _value < 0.f ? -_value : _value;
    float result = GP_MATH_POWF( absolute, _power );

    return _value < 0.f ? -result : result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_superellipse( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _exponent, float _rotation )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _width <= 0.f || _height <= 0.f || _exponent <= 0.f )
    {
        return GP_FAILURE;
    }
#endif

    const gp_state_t * state = GP_GET_STATE( _canvas );
    gp_uint32_t point_count = state->ellipse_quality;
    float * points = GP_REALLOC( _canvas, GP_NULLPTR, float, point_count * 2 );
    float power = 2.f / _exponent;
    float cosine_rotation = GP_MATH_COSF( _rotation );
    float sine_rotation = GP_MATH_SINF( _rotation );
    for( gp_uint32_t index = 0; index != point_count; ++index )
    {
        float angle = gp_constant_two_pi * (float)index / (float)point_count;
        float local_x = __primitive_signed_power( GP_MATH_COSF( angle ), power ) * _width;
        float local_y = __primitive_signed_power( GP_MATH_SINF( angle ), power ) * _height;
        points[index * 2 + 0] = _x + local_x * cosine_rotation - local_y * sine_rotation;
        points[index * 2 + 1] = _y + local_x * sine_rotation + local_y * cosine_rotation;
    }

    gp_result_t result = gp_polyline( _canvas, points, point_count, sizeof( float ) * 2, GP_NULLPTR, GP_TRUE );
    GP_FREE( _canvas, points );

    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_rounded_polyline( gp_canvas_t * _canvas, const void * _points, gp_size_t _point_count, gp_size_t _point_stride, const gp_path_layout_t * _layout, float _radius, gp_bool_t _closed )
{
#if defined(GP_DEBUG)
    if( _points == GP_NULLPTR || _point_count < (_closed == GP_TRUE ? 3U : 2U) || _point_stride < sizeof( float ) * 2 || _radius < 0.f )
    {
        return GP_FAILURE;
    }

    gp_size_t validation_segment_count = _closed == GP_TRUE ? _point_count : _point_count - 1;
    for( gp_size_t segment = 0; segment != validation_segment_count; ++segment )
    {
        float from_x;
        float from_y;
        float to_x;
        float to_y;
        __primitive_get_point( _points, _point_stride, segment, &from_x, &from_y );
        __primitive_get_point( _points, _point_stride, (segment + 1) % _point_count, &to_x, &to_y );
        if( __primitive_length( to_x - from_x, to_y - from_y ) <= 0.0001f )
        {
            return GP_FAILURE;
        }
    }
#endif

    GP_CALL( __primitive_validate_layout, (_layout, _point_stride) );

#if defined(GP_DEBUG)
    for( gp_size_t index = 0; index != _point_count; ++index )
    {
        gp_primitive_style_t style;
        __primitive_get_style( _canvas, _points, _point_stride, index, _layout, &style );
        GP_CALL( __primitive_validate_style, (&style) );
    }
#endif

    if( _radius <= 0.0001f )
    {
        return gp_polyline( _canvas, _points, _point_count, _point_stride, _layout, _closed );
    }

    gp_path_t * path;
    GP_CALL( __primitive_create_path, (_canvas, &path) );

    gp_size_t first_index = _closed == GP_TRUE ? 0 : 1;
    float first_x;
    float first_y;
    __primitive_get_point( _points, _point_stride, 0, &first_x, &first_y );

    if( _closed == GP_FALSE )
    {
        gp_primitive_style_t first_style;
        __primitive_get_style( _canvas, _points, _point_stride, 0, _layout, &first_style );
        GP_CALL( __primitive_apply_style, (path, _layout, &first_style) );
        GP_CALL( gp_path_move_to, (path, first_x, first_y) );
    }

    gp_size_t corner_count = _closed == GP_TRUE ? _point_count : _point_count - 2;
    for( gp_size_t offset = 0; offset != corner_count; ++offset )
    {
        gp_size_t index = (first_index + offset) % _point_count;
        gp_size_t previous_index = (index + _point_count - 1) % _point_count;
        gp_size_t next_index = (index + 1) % _point_count;

        float previous_x;
        float previous_y;
        float point_x;
        float point_y;
        float next_x;
        float next_y;
        __primitive_get_point( _points, _point_stride, previous_index, &previous_x, &previous_y );
        __primitive_get_point( _points, _point_stride, index, &point_x, &point_y );
        __primitive_get_point( _points, _point_stride, next_index, &next_x, &next_y );

        float incoming_x = previous_x - point_x;
        float incoming_y = previous_y - point_y;
        float outgoing_x = next_x - point_x;
        float outgoing_y = next_y - point_y;
        float incoming_length = __primitive_length( incoming_x, incoming_y );
        float outgoing_length = __primitive_length( outgoing_x, outgoing_y );
        float distance = _radius;
        float maximum_distance = (incoming_length < outgoing_length ? incoming_length : outgoing_length) * 0.5f;
        if( distance > maximum_distance )
        {
            distance = maximum_distance;
        }

        float entry_x = point_x + incoming_x / incoming_length * distance;
        float entry_y = point_y + incoming_y / incoming_length * distance;
        float exit_x = point_x + outgoing_x / outgoing_length * distance;
        float exit_y = point_y + outgoing_y / outgoing_length * distance;

        gp_primitive_style_t previous_style;
        gp_primitive_style_t point_style;
        gp_primitive_style_t next_style;
        gp_primitive_style_t entry_style;
        gp_primitive_style_t exit_style;
        __primitive_get_style( _canvas, _points, _point_stride, previous_index, _layout, &previous_style );
        __primitive_get_style( _canvas, _points, _point_stride, index, _layout, &point_style );
        __primitive_get_style( _canvas, _points, _point_stride, next_index, _layout, &next_style );
        __primitive_lerp_style( &entry_style, &previous_style, &point_style, (incoming_length - distance) / incoming_length );
        __primitive_lerp_style( &exit_style, &point_style, &next_style, distance / outgoing_length );

        GP_CALL( __primitive_apply_style, (path, _layout, &entry_style) );
        if( _closed == GP_TRUE && offset == 0 )
        {
            GP_CALL( gp_path_move_to, (path, entry_x, entry_y) );
        }
        else
        {
            GP_CALL( gp_path_line_to, (path, entry_x, entry_y) );
        }

        GP_CALL( __primitive_apply_style, (path, _layout, &exit_style) );
        GP_CALL( gp_path_quadratic_curve_to, (path, point_x, point_y, exit_x, exit_y) );
    }

    if( _closed == GP_TRUE )
    {
        GP_CALL( gp_path_close, (path) );
    }
    else
    {
        float last_x;
        float last_y;
        __primitive_get_point( _points, _point_stride, _point_count - 1, &last_x, &last_y );
        gp_primitive_style_t last_style;
        __primitive_get_style( _canvas, _points, _point_stride, _point_count - 1, _layout, &last_style );
        GP_CALL( __primitive_apply_style, (path, _layout, &last_style) );
        GP_CALL( gp_path_line_to, (path, last_x, last_y) );
    }

    return __primitive_draw_destroy_path( _canvas, path );
}
//////////////////////////////////////////////////////////////////////////
typedef struct gp_offset_vertex_t
{
    float entry_x;
    float entry_y;
    float exit_x;
    float exit_y;
    float control_x;
    float control_y;
    gp_bool_t separated;
    gp_bool_t rounded;
} gp_offset_vertex_t;
//////////////////////////////////////////////////////////////////////////
static gp_result_t __primitive_offset_vertex( const void * _points, gp_size_t _point_count, gp_size_t _point_stride, gp_size_t _index, float _offset, float _orientation, gp_line_join_t _join, float _miter_limit, gp_offset_vertex_t * const _vertex )
{
    float previous_x;
    float previous_y;
    float point_x;
    float point_y;
    float next_x;
    float next_y;
    __primitive_get_point( _points, _point_stride, (_index + _point_count - 1) % _point_count, &previous_x, &previous_y );
    __primitive_get_point( _points, _point_stride, _index, &point_x, &point_y );
    __primitive_get_point( _points, _point_stride, (_index + 1) % _point_count, &next_x, &next_y );

    float incoming_x = point_x - previous_x;
    float incoming_y = point_y - previous_y;
    float outgoing_x = next_x - point_x;
    float outgoing_y = next_y - point_y;
    float incoming_length = __primitive_length( incoming_x, incoming_y );
    float outgoing_length = __primitive_length( outgoing_x, outgoing_y );

#if defined(GP_DEBUG)
    if( incoming_length <= 0.0001f || outgoing_length <= 0.0001f )
    {
        return GP_FAILURE;
    }
#endif

    float incoming_normal_x = _orientation * incoming_y / incoming_length;
    float incoming_normal_y = _orientation * -incoming_x / incoming_length;
    float outgoing_normal_x = _orientation * outgoing_y / outgoing_length;
    float outgoing_normal_y = _orientation * -outgoing_x / outgoing_length;
    float entry_x = point_x + incoming_normal_x * _offset;
    float entry_y = point_y + incoming_normal_y * _offset;
    float exit_x = point_x + outgoing_normal_x * _offset;
    float exit_y = point_y + outgoing_normal_y * _offset;
    float turn = incoming_x * outgoing_y - incoming_y * outgoing_x;
    float intersection_x = entry_x;
    float intersection_y = entry_y;

    if( turn < -0.0001f || turn > 0.0001f )
    {
        float factor = ((exit_x - entry_x) * outgoing_y - (exit_y - entry_y) * outgoing_x) / turn;
        intersection_x = entry_x + incoming_x * factor;
        intersection_y = entry_y + incoming_y * factor;
    }

    gp_bool_t outer_corner = _orientation * turn * _offset > 0.0001f ? GP_TRUE : GP_FALSE;
    gp_bool_t separated = GP_FALSE;
    gp_bool_t rounded = GP_FALSE;

    if( outer_corner == GP_TRUE && _join != GP_LINE_JOIN_MITER )
    {
        separated = GP_TRUE;
        rounded = _join == GP_LINE_JOIN_ROUND ? GP_TRUE : GP_FALSE;
    }
    else if( outer_corner == GP_TRUE && _join == GP_LINE_JOIN_MITER )
    {
        float dx = intersection_x - point_x;
        float dy = intersection_y - point_y;
        float distance = __primitive_length( dx, dy );
        float limit = (_offset < 0.f ? -_offset : _offset) * _miter_limit;
        if( distance > limit )
        {
            separated = GP_TRUE;
        }
    }

    if( separated == GP_TRUE )
    {
        _vertex->entry_x = entry_x;
        _vertex->entry_y = entry_y;
        _vertex->exit_x = exit_x;
        _vertex->exit_y = exit_y;
    }
    else
    {
        _vertex->entry_x = intersection_x;
        _vertex->entry_y = intersection_y;
        _vertex->exit_x = intersection_x;
        _vertex->exit_y = intersection_y;
    }

    _vertex->control_x = intersection_x;
    _vertex->control_y = intersection_y;
    _vertex->separated = separated;
    _vertex->rounded = rounded;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_polygon_offset( gp_canvas_t * _canvas, const void * _points, gp_size_t _point_count, gp_size_t _point_stride, float _offset, gp_line_join_t _join )
{
#if defined(GP_DEBUG)
    if( _points == GP_NULLPTR || _point_count < 3 || _point_stride < sizeof( float ) * 2 ||
        (_join != GP_LINE_JOIN_MITER && _join != GP_LINE_JOIN_BEVEL && _join != GP_LINE_JOIN_ROUND) )
    {
        return GP_FAILURE;
    }
#endif

    float area = 0.f;
    for( gp_size_t index = 0; index != _point_count; ++index )
    {
        float x0;
        float y0;
        float x1;
        float y1;
        __primitive_get_point( _points, _point_stride, index, &x0, &y0 );
        __primitive_get_point( _points, _point_stride, (index + 1) % _point_count, &x1, &y1 );
        area += x0 * y1 - y0 * x1;
    }

    float orientation = area >= 0.f ? 1.f : -1.f;
    float miter_limit = GP_GET_STATE( _canvas )->miter_limit;
    gp_path_t * path;
    GP_CALL( __primitive_create_path, (_canvas, &path) );
    gp_offset_vertex_t vertex;
    GP_CALL( __primitive_offset_vertex, (_points, _point_count, _point_stride, 0, _offset, orientation, _join, miter_limit, &vertex) );
    GP_CALL( gp_path_move_to, (path, vertex.entry_x, vertex.entry_y) );

    if( vertex.rounded == GP_TRUE )
    {
        GP_CALL( gp_path_quadratic_curve_to, (path, vertex.control_x, vertex.control_y, vertex.exit_x, vertex.exit_y) );
    }
    else if( vertex.separated == GP_TRUE )
    {
        GP_CALL( gp_path_line_to, (path, vertex.exit_x, vertex.exit_y) );
    }

    for( gp_size_t index = 1; index != _point_count; ++index )
    {
        GP_CALL( __primitive_offset_vertex, (_points, _point_count, _point_stride, index, _offset, orientation, _join, miter_limit, &vertex) );
        GP_CALL( gp_path_line_to, (path, vertex.entry_x, vertex.entry_y) );

        if( vertex.rounded == GP_TRUE )
        {
            GP_CALL( gp_path_quadratic_curve_to, (path, vertex.control_x, vertex.control_y, vertex.exit_x, vertex.exit_y) );
        }
        else if( vertex.separated == GP_TRUE )
        {
            GP_CALL( gp_path_line_to, (path, vertex.exit_x, vertex.exit_y) );
        }
    }

    GP_CALL( gp_path_close, (path) );

    return __primitive_draw_destroy_path( _canvas, path );
}
