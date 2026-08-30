#include "graphics/graphics.h"

#include "detail.h"
#include "effects.h"

#include <string.h>

//////////////////////////////////////////////////////////////////////////
typedef struct gp_effect_point_t
{
    float x;
    float y;
} gp_effect_point_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_box_payload_t
{
    float x;
    float y;
    float width;
    float height;
    float radius_top_left;
    float radius_top_right;
    float radius_bottom_right;
    float radius_bottom_left;
    const gp_state_t * state;
} gp_box_payload_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_ring_payload_t
{
    float x;
    float y;
    float inner_radius;
    float outer_radius;
    float angle_from;
    float angle_to;
    const gp_state_t * state;
} gp_ring_payload_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_rect_instance_t
{
    float x;
    float y;
    float width;
    float height;
    gp_color_t color;
} gp_rect_instance_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_rects_payload_t
{
    gp_uint32_t capacity;
    gp_uint32_t count;
    gp_rect_instance_t instances[1];
} gp_rects_payload_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_quad_vertex_t
{
    float x;
    float y;
    gp_color_t color;
    float u;
    float v;
} gp_quad_vertex_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_quad_t
{
    gp_quad_vertex_t vertices[4];
} gp_quad_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_quads_payload_t
{
    gp_uint32_t capacity;
    gp_uint32_t vertex_count;
    gp_quad_t quads[1];
} gp_quads_payload_t;
//////////////////////////////////////////////////////////////////////////
#if defined(GP_DEBUG)
static gp_bool_t __color_valid( const gp_color_t * _color )
{
    gp_bool_t valid = GP_MATH_ISFINITE( _color->r ) != 0 && GP_MATH_ISFINITE( _color->g ) != 0 &&
        GP_MATH_ISFINITE( _color->b ) != 0 && GP_MATH_ISFINITE( _color->a ) != 0 &&
        _color->r >= 0.f && _color->r <= 1.f && _color->g >= 0.f && _color->g <= 1.f &&
        _color->b >= 0.f && _color->b <= 1.f && _color->a >= 0.f && _color->a <= 1.f ? GP_TRUE : GP_FALSE;
    return valid;
}
//////////////////////////////////////////////////////////////////////////
static gp_bool_t __float_valid( float _value )
{
    gp_bool_t valid = GP_MATH_ISFINITE( _value ) != 0 ? GP_TRUE : GP_FALSE;
    return valid;
}
//////////////////////////////////////////////////////////////////////////
static gp_bool_t __spread_valid( gp_fill_spread_t _spread )
{
    gp_bool_t valid = _spread == GP_FILL_SPREAD_CLAMP || _spread == GP_FILL_SPREAD_REPEAT ||
        _spread == GP_FILL_SPREAD_MIRROR ? GP_TRUE : GP_FALSE;
    return valid;
}
#endif
//////////////////////////////////////////////////////////////////////////
static float __fill_spread( float _value, gp_fill_spread_t _spread )
{
    switch( _spread )
    {
    case GP_FILL_SPREAD_REPEAT:
    {
        float value = GP_MATH_FMODF( _value, 1.f );
        float spread = value < 0.f ? value + 1.f : value;
        return spread;
    }
    case GP_FILL_SPREAD_MIRROR:
    {
        float value = GP_MATH_FMODF( _value, 2.f );
        if( value < 0.f )
        {
            value += 2.f;
        }
        float spread = value <= 1.f ? value : 2.f - value;
        return spread;
    }
    case GP_FILL_SPREAD_CLAMP:
    default:
    {
        float spread = _value < 0.f ? 0.f : _value > 1.f ? 1.f : _value;
        return spread;
    }
    }
}
//////////////////////////////////////////////////////////////////////////
static gp_color_t __color_lerp( const gp_color_t * _first, const gp_color_t * _second, float _ratio )
{
    float inverse = 1.f - _ratio;
    gp_color_t color;
    color.r = _first->r * inverse + _second->r * _ratio;
    color.g = _first->g * inverse + _second->g * _ratio;
    color.b = _first->b * inverse + _second->b * _ratio;
    color.a = _first->a * inverse + _second->a * _ratio;
    return color;
}
//////////////////////////////////////////////////////////////////////////
static gp_color_t __fill_color( const gp_fill_t * _fill, float _x, float _y )
{
    if( _fill->type == GP_FILL_TYPE_SOLID || _fill->stop_count == 0 )
    {
        gp_color_t color = _fill->color;
        return color;
    }

    float ratio = 0.f;
    switch( _fill->type )
    {
    case GP_FILL_TYPE_LINEAR:
    {
        float dx = _fill->to_x - _fill->from_x;
        float dy = _fill->to_y - _fill->from_y;
        ratio = ((_x - _fill->from_x) * dx + (_y - _fill->from_y) * dy) / (dx * dx + dy * dy);
    } break;
    case GP_FILL_TYPE_RADIAL:
    {
        float dx = _x - _fill->from_x;
        float dy = _y - _fill->from_y;
        ratio = GP_MATH_SQRTF( dx * dx + dy * dy ) / _fill->to_x;
    } break;
    case GP_FILL_TYPE_CONIC:
        ratio = (GP_MATH_ATAN2F( _y - _fill->from_y, _x - _fill->from_x ) - _fill->to_x) /
            gp_constant_two_pi;
        break;
    case GP_FILL_TYPE_SOLID:
    default:
        break;
    }

    ratio = __fill_spread( ratio, _fill->spread );
    if( ratio <= _fill->stops[0].offset )
    {
        gp_color_t color = _fill->stops[0].color;
        return color;
    }
    if( ratio >= _fill->stops[_fill->stop_count - 1].offset )
    {
        gp_uint32_t last_index = _fill->stop_count - 1;
        gp_color_t color = _fill->stops[last_index].color;
        return color;
    }

    for( gp_uint32_t index = 1; index != _fill->stop_count; ++index )
    {
        const gp_fill_stop_t * next = _fill->stops + index;
        if( ratio <= next->offset )
        {
            const gp_fill_stop_t * previous = next - 1;
            float range = next->offset - previous->offset;
            float local = range <= 0.00000001f ? 0.f : (ratio - previous->offset) / range;
            gp_color_t color = __color_lerp( &previous->color, &next->color, local );
            return color;
        }
    }

    gp_uint32_t last_index = _fill->stop_count - 1;
    gp_color_t color = _fill->stops[last_index].color;
    return color;
}
//////////////////////////////////////////////////////////////////////////
static gp_argb_t __fill_argb( const gp_fill_t * _fill, const gp_color_t * _modulation, float _x, float _y, float _alpha )
{
    gp_color_t color = __fill_color( _fill, _x, _y );
    color.a *= _alpha;
    gp_color_t result;
    gp_color_mul( &result, &color, _modulation );
    gp_argb_t argb = gp_color_argb( &result );
    return argb;
}
//////////////////////////////////////////////////////////////////////////
static gp_argb_t __color_argb( const gp_color_t * _color, const gp_color_t * _modulation, float _alpha )
{
    gp_color_t color = *_color;
    color.a *= _alpha;
    gp_color_t result;
    gp_color_mul( &result, &color, _modulation );
    gp_argb_t argb = gp_color_argb( &result );
    return argb;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __writer_vertex( gp_mesh_writer_t * _writer, gp_uint32_t _vertex, float _x, float _y, gp_argb_t _color, float _origin_x, float _origin_y, float _width, float _height )
{
    float u = _width <= 0.f ? 0.f : (_x - _origin_x) / _width;
    float v = _height <= 0.f ? 0.f : (_y - _origin_y) / _height;
    if( gp_mesh_writer_position( _writer, _vertex, _x, _y ) == GP_FAILURE ||
        gp_mesh_writer_color( _writer, _vertex, _color ) == GP_FAILURE ||
        gp_mesh_writer_uv( _writer, _vertex, u, v ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static float __box_radius( float _radius, float _width, float _height )
{
    float maximum = (_width < _height ? _width : _height) * 0.5f;
    if( maximum < 0.f )
    {
        maximum = 0.f;
    }
    float radius = _radius < 0.f ? 0.f : _radius > maximum ? maximum : _radius;
    return radius;
}
//////////////////////////////////////////////////////////////////////////
static gp_effect_point_t __box_point( const gp_box_payload_t * _box, gp_uint8_t _quality, gp_uint32_t _point, float _inset )
{
    float x = _box->x + _inset;
    float y = _box->y + _inset;
    float width = _box->width - _inset * 2.f;
    float height = _box->height - _inset * 2.f;
    if( width < 0.f )
    {
        width = 0.f;
    }
    if( height < 0.f )
    {
        height = 0.f;
    }

    gp_uint32_t samples = (gp_uint32_t)_quality + 1U;
    gp_uint32_t corner = _point / samples;
    gp_uint32_t sample = _point % samples;
    float ratio = (float)sample / (float)_quality;
    float angle = 0.f;
    float center_x = x;
    float center_y = y;
    float radius = 0.f;

    switch( corner )
    {
    case 0:
        radius = __box_radius( _box->radius_top_left - _inset, width, height );
        center_x = x + radius;
        center_y = y + radius;
        angle = gp_constant_pi + gp_constant_half_pi * ratio;
        break;
    case 1:
        radius = __box_radius( _box->radius_top_right - _inset, width, height );
        center_x = x + width - radius;
        center_y = y + radius;
        angle = -gp_constant_half_pi + gp_constant_half_pi * ratio;
        break;
    case 2:
        radius = __box_radius( _box->radius_bottom_right - _inset, width, height );
        center_x = x + width - radius;
        center_y = y + height - radius;
        angle = gp_constant_half_pi * ratio;
        break;
    case 3:
    default:
        radius = __box_radius( _box->radius_bottom_left - _inset, width, height );
        center_x = x + radius;
        center_y = y + height - radius;
        angle = gp_constant_half_pi + gp_constant_half_pi * ratio;
        break;
    }

    gp_effect_point_t point;
    point.x = center_x + GP_MATH_COSF( angle ) * radius;
    point.y = center_y + GP_MATH_SINF( angle ) * radius;
    return point;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __box_write_contour( gp_mesh_writer_t * _writer, const gp_box_payload_t * _box, gp_uint8_t _quality, gp_uint32_t _vertex, float _inset, const gp_fill_t * _fill, const gp_color_t * _color, float _alpha )
{
    gp_uint32_t count = 4U * ((gp_uint32_t)_quality + 1U);
    for( gp_uint32_t index = 0; index != count; ++index )
    {
        gp_effect_point_t point = __box_point( _box, _quality, index, _inset );
        gp_argb_t color = _fill != GP_NULLPTR ? __fill_argb( _fill, &_writer->mesh->color, point.x, point.y, _alpha ) : __color_argb( _color, &_writer->mesh->color, _alpha );
        GP_CALL( __writer_vertex, (_writer, _vertex + index, point.x, point.y, color, _box->x, _box->y, _box->width, _box->height) );
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __box_write_fan( gp_mesh_writer_t * _writer, const gp_box_payload_t * _box, gp_uint8_t _quality, gp_uint32_t * _vertex, gp_uint32_t * _index, float _inset, const gp_fill_t * _fill, const gp_color_t * _color, float _alpha )
{
    gp_uint32_t count = 4U * ((gp_uint32_t)_quality + 1U);
    float center_x = _box->x + _box->width * 0.5f;
    float center_y = _box->y + _box->height * 0.5f;
    gp_argb_t center_color = _fill != GP_NULLPTR ? __fill_argb( _fill, &_writer->mesh->color, center_x, center_y, _alpha ) : __color_argb( _color, &_writer->mesh->color, _alpha );
    GP_CALL( __writer_vertex, (_writer, *_vertex, center_x, center_y, center_color, _box->x, _box->y, _box->width, _box->height) );
    GP_CALL( __box_write_contour, (_writer, _box, _quality, *_vertex + 1U, _inset, _fill, _color, _alpha) );

    for( gp_uint32_t point = 0; point != count; ++point )
    {
        GP_CALL( gp_mesh_writer_index, (_writer, *_index + point * 3U + 0U, *_vertex) );
        GP_CALL( gp_mesh_writer_index, (_writer, *_index + point * 3U + 1U, *_vertex + 1U + point) );
        GP_CALL( gp_mesh_writer_index, (_writer, *_index + point * 3U + 2U, *_vertex + 1U + (point + 1U) % count) );
    }

    *_vertex += count + 1U;
    *_index += count * 3U;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __box_write_band( gp_mesh_writer_t * _writer, const gp_box_payload_t * _box, gp_uint8_t _quality, gp_uint32_t * _vertex, gp_uint32_t * _index, float _inner_inset, float _outer_inset, const gp_fill_t * _inner_fill, const gp_color_t * _inner_color, float _inner_alpha, const gp_fill_t * _outer_fill, const gp_color_t * _outer_color, float _outer_alpha )
{
    gp_uint32_t count = 4U * ((gp_uint32_t)_quality + 1U);
    GP_CALL( __box_write_contour, (_writer, _box, _quality, *_vertex, _inner_inset, _inner_fill, _inner_color, _inner_alpha) );
    GP_CALL( __box_write_contour, (_writer, _box, _quality, *_vertex + count, _outer_inset, _outer_fill, _outer_color, _outer_alpha) );

    for( gp_uint32_t point = 0; point != count; ++point )
    {
        gp_uint32_t next = (point + 1U) % count;
        gp_uint32_t index = *_index + point * 6U;
        GP_CALL( gp_mesh_writer_index, (_writer, index + 0U, *_vertex + point) );
        GP_CALL( gp_mesh_writer_index, (_writer, index + 1U, *_vertex + count + point) );
        GP_CALL( gp_mesh_writer_index, (_writer, index + 2U, *_vertex + count + next) );
        GP_CALL( gp_mesh_writer_index, (_writer, index + 3U, *_vertex + point) );
        GP_CALL( gp_mesh_writer_index, (_writer, index + 4U, *_vertex + count + next) );
        GP_CALL( gp_mesh_writer_index, (_writer, index + 5U, *_vertex + next) );
    }

    *_vertex += count * 2U;
    *_index += count * 6U;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_bool_t __shadow_visible( const gp_shadow_t * _shadow )
{
    gp_bool_t visible = _shadow->color.a > 0.f && (_shadow->offset_x != 0.f || _shadow->offset_y != 0.f ||
        _shadow->spread > 0.f || _shadow->blur > 0.f) ? GP_TRUE : GP_FALSE;
    return visible;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __write_box( gp_mesh_writer_t * _writer, const void * _payload )
{
    const gp_box_payload_t * box = (const gp_box_payload_t *)_payload;
    const gp_state_t * state = box->state;
    gp_uint8_t quality = state->rect_quality > 32 ? 32 : state->rect_quality;
    gp_uint32_t vertex = 0;
    gp_uint32_t index = 0;

    if( __shadow_visible( &state->shadow ) == GP_TRUE )
    {
        gp_box_payload_t shadow_box = *box;
        shadow_box.x += state->shadow.offset_x - state->shadow.spread;
        shadow_box.y += state->shadow.offset_y - state->shadow.spread;
        shadow_box.width += state->shadow.spread * 2.f;
        shadow_box.height += state->shadow.spread * 2.f;
        shadow_box.radius_top_left += state->shadow.spread;
        shadow_box.radius_top_right += state->shadow.spread;
        shadow_box.radius_bottom_right += state->shadow.spread;
        shadow_box.radius_bottom_left += state->shadow.spread;
        GP_CALL( __box_write_fan, (_writer, &shadow_box, quality, &vertex, &index, 0.f, GP_NULLPTR, &state->shadow.color, 1.f) );
        if( state->shadow.blur > 0.f )
        {
            GP_CALL( __box_write_band, (_writer, &shadow_box, quality, &vertex, &index, 0.f, -state->shadow.blur, GP_NULLPTR, &state->shadow.color, 1.f, GP_NULLPTR, &state->shadow.color, 0.f) );
        }
    }

    float maximum_border = (box->width < box->height ? box->width : box->height) * 0.5f;
    float border = state->border_width < maximum_border ? state->border_width : maximum_border;
    if( border < 0.f )
    {
        border = 0.f;
    }
    GP_CALL( __box_write_fan, (_writer, box, quality, &vertex, &index, border, &state->fill, GP_NULLPTR, 1.f) );
    if( border > 0.f )
    {
        GP_CALL( __box_write_band, (_writer, box, quality, &vertex, &index, border, 0.f, GP_NULLPTR, &state->border_color, 1.f, GP_NULLPTR, &state->border_color, 1.f) );
    }
    if( state->feather > 0.f )
    {
        const gp_fill_t * fill = border > 0.f ? GP_NULLPTR : &state->fill;
        const gp_color_t * color = border > 0.f ? &state->border_color : GP_NULLPTR;
        GP_CALL( __box_write_band, (_writer, box, quality, &vertex, &index, 0.f, -state->feather, fill, color, 1.f, fill, color, 0.f) );
    }

    if( vertex != _writer->vertex_count || index != _writer->index_count )
    {
        return GP_FAILURE;
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __box_size( const gp_box_payload_t * _box, gp_uint32_t * _vertex_count, gp_uint32_t * _index_count )
{
    const gp_state_t * state = _box->state;
    gp_uint8_t quality = state->rect_quality > 32 ? 32 : state->rect_quality;
    gp_uint32_t contour = 4U * ((gp_uint32_t)quality + 1U);
    gp_uint32_t vertices = contour + 1U;
    gp_uint32_t indices = contour * 3U;
    if( state->border_width > 0.f )
    {
        vertices += contour * 2U;
        indices += contour * 6U;
    }
    if( state->feather > 0.f )
    {
        vertices += contour * 2U;
        indices += contour * 6U;
    }
    if( __shadow_visible( &state->shadow ) == GP_TRUE )
    {
        vertices += contour + 1U;
        indices += contour * 3U;
        if( state->shadow.blur > 0.f )
        {
            vertices += contour * 2U;
            indices += contour * 6U;
        }
    }

    *_vertex_count = vertices;
    *_index_count = indices;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
#if defined(GP_DEBUG)
static gp_bool_t __fill_valid( const gp_fill_t * _fill )
{
    if( _fill->type == GP_FILL_TYPE_SOLID )
    {
        gp_bool_t valid = __color_valid( &_fill->color );
        return valid;
    }
    if( (_fill->type != GP_FILL_TYPE_LINEAR && _fill->type != GP_FILL_TYPE_RADIAL &&
        _fill->type != GP_FILL_TYPE_CONIC) || __spread_valid( _fill->spread ) == GP_FALSE ||
        _fill->stop_count == 0 || _fill->stop_count > GP_FILL_STOP_CAPACITY )
    {
        return GP_FALSE;
    }

    if( __float_valid( _fill->from_x ) == GP_FALSE || __float_valid( _fill->from_y ) == GP_FALSE ||
        __float_valid( _fill->to_x ) == GP_FALSE || __float_valid( _fill->to_y ) == GP_FALSE )
    {
        return GP_FALSE;
    }
    if( _fill->type == GP_FILL_TYPE_LINEAR )
    {
        float dx = _fill->to_x - _fill->from_x;
        float dy = _fill->to_y - _fill->from_y;
        if( dx * dx + dy * dy <= 0.00000001f )
        {
            return GP_FALSE;
        }
    }
    else if( _fill->type == GP_FILL_TYPE_RADIAL && _fill->to_x <= 0.f )
    {
        return GP_FALSE;
    }

    for( gp_uint32_t index = 0; index != _fill->stop_count; ++index )
    {
        if( __float_valid( _fill->stops[index].offset ) == GP_FALSE ||
            _fill->stops[index].offset < 0.f || _fill->stops[index].offset > 1.f ||
            __color_valid( &_fill->stops[index].color ) == GP_FALSE ||
            (index != 0 && _fill->stops[index].offset < _fill->stops[index - 1].offset) )
        {
            return GP_FALSE;
        }
    }
    return GP_TRUE;
}
#endif
//////////////////////////////////////////////////////////////////////////
static gp_result_t __set_fill_gradient( gp_canvas_t * _canvas, gp_fill_type_e _type, float _from_x, float _from_y, float _to_x, float _to_y, gp_fill_spread_t _spread )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || __float_valid( _from_x ) == GP_FALSE ||
        __float_valid( _from_y ) == GP_FALSE || __float_valid( _to_x ) == GP_FALSE ||
        __float_valid( _to_y ) == GP_FALSE || __spread_valid( _spread ) == GP_FALSE )
    {
        return GP_FAILURE;
    }

    if( _type == GP_FILL_TYPE_LINEAR )
    {
        float dx = _to_x - _from_x;
        float dy = _to_y - _from_y;
        if( dx * dx + dy * dy <= 0.00000001f )
        {
            return GP_FAILURE;
        }
    }
    else if( _type == GP_FILL_TYPE_RADIAL && _to_x <= 0.f )
    {
        return GP_FAILURE;
    }
#endif

    gp_fill_t fill = {0};
    fill.type = _type;
    fill.spread = _spread;
    fill.from_x = _from_x;
    fill.from_y = _from_y;
    fill.to_x = _to_x;
    fill.to_y = _to_y;

    GP_GET_STATE( _canvas )->fill = fill;
    _canvas->state_invalidate = GP_TRUE;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_fill_color( gp_canvas_t * _canvas, float _r, float _g, float _b, float _a )
{
    gp_color_t color = {_r, _g, _b, _a};
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || __color_valid( &color ) == GP_FALSE )
    {
        return GP_FAILURE;
    }
#endif

    gp_state_t * state = GP_GET_STATE( _canvas );
    gp_fill_t * fill = &state->fill;
    memset( fill, 0, sizeof( *fill ) );
    fill->type = GP_FILL_TYPE_SOLID;
    fill->spread = GP_FILL_SPREAD_CLAMP;
    fill->color = color;
    state->color = color;
    _canvas->state_invalidate = GP_TRUE;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_fill_linear( gp_canvas_t * _canvas, float _from_x, float _from_y, float _to_x, float _to_y, gp_fill_spread_t _spread )
{
    gp_result_t result = __set_fill_gradient( _canvas, GP_FILL_TYPE_LINEAR, _from_x, _from_y, _to_x, _to_y, _spread );
    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_fill_radial( gp_canvas_t * _canvas, float _center_x, float _center_y, float _radius, gp_fill_spread_t _spread )
{
    gp_result_t result = __set_fill_gradient( _canvas, GP_FILL_TYPE_RADIAL, _center_x, _center_y, _radius, 0.f, _spread );
    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_fill_conic( gp_canvas_t * _canvas, float _center_x, float _center_y, float _start_angle, gp_fill_spread_t _spread )
{
    gp_result_t result = __set_fill_gradient( _canvas, GP_FILL_TYPE_CONIC, _center_x, _center_y, _start_angle, 0.f, _spread );
    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_clear_fill_stops( gp_canvas_t * _canvas )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    gp_fill_t * fill = &GP_GET_STATE( _canvas )->fill;
#if defined(GP_DEBUG)
    if( fill->type == GP_FILL_TYPE_SOLID )
    {
        return GP_FAILURE;
    }
#endif

    fill->stop_count = 0;
    _canvas->state_invalidate = GP_TRUE;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_add_fill_stop( gp_canvas_t * _canvas, float _offset, float _r, float _g, float _b, float _a )
{
    gp_color_t color = {_r, _g, _b, _a};
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || __float_valid( _offset ) == GP_FALSE ||
        _offset < 0.f || _offset > 1.f || __color_valid( &color ) == GP_FALSE )
    {
        return GP_FAILURE;
    }
#endif

    gp_fill_t * fill = &GP_GET_STATE( _canvas )->fill;
#if defined(GP_DEBUG)
    if( fill->type == GP_FILL_TYPE_SOLID ||
        (fill->stop_count != 0 && _offset < fill->stops[fill->stop_count - 1].offset) )
    {
        return GP_FAILURE;
    }
#endif
    if( fill->stop_count >= GP_FILL_STOP_CAPACITY )
    {
        return GP_FAILURE;
    }

    gp_fill_stop_t * stop = fill->stops + fill->stop_count;
    stop->offset = _offset;
    stop->color = color;
    ++fill->stop_count;
    _canvas->state_invalidate = GP_TRUE;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_border( gp_canvas_t * _canvas, float _width, float _r, float _g, float _b, float _a )
{
    gp_color_t color = {_r, _g, _b, _a};
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || __float_valid( _width ) == GP_FALSE || _width < 0.f ||
        __color_valid( &color ) == GP_FALSE )
    {
        return GP_FAILURE;
    }
#endif

    gp_state_t * state = GP_GET_STATE( _canvas );
    state->border_width = _width;
    state->border_color = color;
    _canvas->state_invalidate = GP_TRUE;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_feather( gp_canvas_t * _canvas, float _feather )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || __float_valid( _feather ) == GP_FALSE || _feather < 0.f )
    {
        return GP_FAILURE;
    }
#endif

    GP_GET_STATE( _canvas )->feather = _feather;
    _canvas->state_invalidate = GP_TRUE;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_shadow( gp_canvas_t * _canvas, float _offset_x, float _offset_y, float _spread, float _blur, float _r, float _g, float _b, float _a )
{
    gp_color_t color = {_r, _g, _b, _a};
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || __float_valid( _offset_x ) == GP_FALSE ||
        __float_valid( _offset_y ) == GP_FALSE || __float_valid( _spread ) == GP_FALSE ||
        __float_valid( _blur ) == GP_FALSE || _spread < 0.f || _blur < 0.f ||
        __color_valid( &color ) == GP_FALSE )
    {
        return GP_FAILURE;
    }
#endif

    gp_state_t * state = GP_GET_STATE( _canvas );
    state->shadow.offset_x = _offset_x;
    state->shadow.offset_y = _offset_y;
    state->shadow.spread = _spread;
    state->shadow.blur = _blur;
    state->shadow.color = color;
    _canvas->state_invalidate = GP_TRUE;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_uv( gp_canvas_t * _canvas, float _u, float _v )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || __float_valid( _u ) == GP_FALSE || __float_valid( _v ) == GP_FALSE )
    {
        return GP_FAILURE;
    }
#endif

    gp_state_t * state = GP_GET_STATE( _canvas );
    state->uv_u = _u;
    state->uv_v = _v;
    _canvas->state_invalidate = GP_TRUE;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_uv( const gp_canvas_t * _canvas, float * _u, float * _v )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _u == GP_NULLPTR || _v == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    const gp_state_t * state = GP_GET_STATE( _canvas );
    *_u = state->uv_u;
    *_v = state->uv_v;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_box( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _radius_top_left, float _radius_top_right, float _radius_bottom_right, float _radius_bottom_left )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || __float_valid( _x ) == GP_FALSE || __float_valid( _y ) == GP_FALSE ||
        __float_valid( _width ) == GP_FALSE || __float_valid( _height ) == GP_FALSE ||
        __float_valid( _radius_top_left ) == GP_FALSE || __float_valid( _radius_top_right ) == GP_FALSE ||
        __float_valid( _radius_bottom_right ) == GP_FALSE || __float_valid( _radius_bottom_left ) == GP_FALSE ||
        _width <= 0.f || _height <= 0.f || _radius_top_left < 0.f || _radius_top_right < 0.f ||
        _radius_bottom_right < 0.f || _radius_bottom_left < 0.f )
    {
        return GP_FAILURE;
    }
#endif

    gp_box_payload_t box;
    box.x = _x;
    box.y = _y;
    box.width = _width;
    box.height = _height;
    box.radius_top_left = _radius_top_left;
    box.radius_top_right = _radius_top_right;
    box.radius_bottom_right = _radius_bottom_right;
    box.radius_bottom_left = _radius_bottom_left;
    box.state = GP_COPY_STATE( _canvas );
    if( box.state == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#if defined(GP_DEBUG)
    if( __fill_valid( &box.state->fill ) == GP_FALSE )
    {
        return GP_FAILURE;
    }
#endif

    gp_uint32_t vertex_count;
    gp_uint32_t index_count;
    GP_CALL( __box_size, (&box, &vertex_count, &index_count) );
    gp_result_t result = gp_internal_geometry( _canvas, vertex_count, index_count, __write_box, &box, sizeof( box ) );
    return result;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __ring_vertex( gp_mesh_writer_t * _writer, gp_uint32_t _vertex, const gp_ring_payload_t * _ring, float _radius, float _angle, float _alpha )
{
    float x = _ring->x + GP_MATH_COSF( _angle ) * _radius;
    float y = _ring->y + GP_MATH_SINF( _angle ) * _radius;
    gp_argb_t color = __fill_argb( &_ring->state->fill, &_writer->mesh->color, x, y, _alpha );
    float diameter = _ring->outer_radius * 2.f;
    gp_result_t result = __writer_vertex( _writer, _vertex, x, y, color, _ring->x - _ring->outer_radius, _ring->y - _ring->outer_radius, diameter, diameter );
    return result;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __ring_band( gp_mesh_writer_t * _writer, const gp_ring_payload_t * _ring, gp_uint32_t * _vertex, gp_uint32_t * _index, float _inner_radius, float _outer_radius, float _inner_alpha, float _outer_alpha )
{
    gp_uint8_t quality = _ring->state->rect_quality < 3 ? 64 : _ring->state->rect_quality;
    gp_uint32_t columns = (gp_uint32_t)quality + 1U;
    float angle_delta = _ring->angle_to - _ring->angle_from;
    for( gp_uint32_t point = 0; point != columns; ++point )
    {
        float ratio = (float)point / (float)quality;
        float angle = _ring->angle_from + angle_delta * ratio;
        GP_CALL( __ring_vertex, (_writer, *_vertex + point, _ring, _inner_radius, angle, _inner_alpha) );
        GP_CALL( __ring_vertex, (_writer, *_vertex + columns + point, _ring, _outer_radius, angle, _outer_alpha) );
    }
    for( gp_uint32_t point = 0; point != quality; ++point )
    {
        gp_uint32_t offset = *_index + point * 6U;
        GP_CALL( gp_mesh_writer_index, (_writer, offset + 0U, *_vertex + point) );
        GP_CALL( gp_mesh_writer_index, (_writer, offset + 1U, *_vertex + columns + point) );
        GP_CALL( gp_mesh_writer_index, (_writer, offset + 2U, *_vertex + columns + point + 1U) );
        GP_CALL( gp_mesh_writer_index, (_writer, offset + 3U, *_vertex + point) );
        GP_CALL( gp_mesh_writer_index, (_writer, offset + 4U, *_vertex + columns + point + 1U) );
        GP_CALL( gp_mesh_writer_index, (_writer, offset + 5U, *_vertex + point + 1U) );
    }
    *_vertex += columns * 2U;
    *_index += (gp_uint32_t)quality * 6U;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __write_ring( gp_mesh_writer_t * _writer, const void * _payload )
{
    const gp_ring_payload_t * ring = (const gp_ring_payload_t *)_payload;
    float feather = ring->state->feather;
    gp_uint32_t vertex = 0;
    gp_uint32_t index = 0;
    if( feather > 0.f )
    {
        float inner = ring->inner_radius - feather;
        if( inner < 0.f )
        {
            inner = 0.f;
        }
        GP_CALL( __ring_band, (_writer, ring, &vertex, &index, inner, ring->inner_radius, 0.f, 1.f) );
    }
    GP_CALL( __ring_band, (_writer, ring, &vertex, &index, ring->inner_radius, ring->outer_radius, 1.f, 1.f) );
    if( feather > 0.f )
    {
        GP_CALL( __ring_band, (_writer, ring, &vertex, &index, ring->outer_radius, ring->outer_radius + feather, 1.f, 0.f) );
    }
    if( vertex != _writer->vertex_count || index != _writer->index_count )
    {
        return GP_FAILURE;
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_effect_ring( gp_canvas_t * _canvas, float _x, float _y, float _inner_radius, float _outer_radius, float _angle_from, float _angle_to )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || __float_valid( _x ) == GP_FALSE || __float_valid( _y ) == GP_FALSE ||
        __float_valid( _inner_radius ) == GP_FALSE || __float_valid( _outer_radius ) == GP_FALSE ||
        __float_valid( _angle_from ) == GP_FALSE || __float_valid( _angle_to ) == GP_FALSE ||
        _inner_radius < 0.f || _outer_radius <= _inner_radius || _angle_from == _angle_to )
    {
        return GP_FAILURE;
    }
#endif
    gp_ring_payload_t payload;
    payload.x = _x;
    payload.y = _y;
    payload.inner_radius = _inner_radius;
    payload.outer_radius = _outer_radius;
    payload.angle_from = _angle_from;
    payload.angle_to = _angle_to;
    payload.state = GP_COPY_STATE( _canvas );
    if( payload.state == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#if defined(GP_DEBUG)
    if( __fill_valid( &payload.state->fill ) == GP_FALSE )
    {
        return GP_FAILURE;
    }
#endif

    gp_uint8_t quality = payload.state->rect_quality < 3 ? 64 : payload.state->rect_quality;
    gp_uint32_t band_count = payload.state->feather > 0.f ? 3U : 1U;
    gp_uint32_t vertex_count = band_count * ((gp_uint32_t)quality + 1U) * 2U;
    gp_uint32_t index_count = band_count * (gp_uint32_t)quality * 6U;
    gp_result_t result = gp_internal_geometry( _canvas, vertex_count, index_count, __write_ring, &payload, sizeof( payload ) );
    return result;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __write_rects( gp_mesh_writer_t * _writer, const void * _payload )
{
    const gp_rects_payload_t * payload = (const gp_rects_payload_t *)_payload;
    for( gp_uint32_t rect_index = 0; rect_index != payload->count; ++rect_index )
    {
        const gp_rect_instance_t * rect = payload->instances + rect_index;
        gp_uint32_t vertex = rect_index * 4U;
        gp_uint32_t index = rect_index * 6U;
        gp_argb_t color = __color_argb( &rect->color, &_writer->mesh->color, 1.f );
        GP_CALL( __writer_vertex, (_writer, vertex + 0U, rect->x, rect->y, color, rect->x, rect->y, rect->width, rect->height) );
        GP_CALL( __writer_vertex, (_writer, vertex + 1U, rect->x + rect->width, rect->y, color, rect->x, rect->y, rect->width, rect->height) );
        GP_CALL( __writer_vertex, (_writer, vertex + 2U, rect->x + rect->width, rect->y + rect->height, color, rect->x, rect->y, rect->width, rect->height) );
        GP_CALL( __writer_vertex, (_writer, vertex + 3U, rect->x, rect->y + rect->height, color, rect->x, rect->y, rect->width, rect->height) );
        GP_CALL( gp_mesh_writer_index, (_writer, index + 0U, vertex + 0U) );
        GP_CALL( gp_mesh_writer_index, (_writer, index + 1U, vertex + 1U) );
        GP_CALL( gp_mesh_writer_index, (_writer, index + 2U, vertex + 2U) );
        GP_CALL( gp_mesh_writer_index, (_writer, index + 3U, vertex + 0U) );
        GP_CALL( gp_mesh_writer_index, (_writer, index + 4U, vertex + 2U) );
        GP_CALL( gp_mesh_writer_index, (_writer, index + 5U, vertex + 3U) );
    }
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_begin_rects( gp_canvas_t * _canvas, gp_size_t _rect_count )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _rect_count == 0 || _canvas->active_rects != GP_NULLPTR ||
        _canvas->active_quads != GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif
    if( _rect_count > UINT32_MAX / 6U ||
        _rect_count > (SIZE_MAX - offsetof( gp_rects_payload_t, instances )) / sizeof( gp_rect_instance_t ) )
    {
        return GP_FAILURE;
    }

    gp_size_t payload_size = offsetof( gp_rects_payload_t, instances ) +
        _rect_count * sizeof( gp_rect_instance_t );
    gp_rects_payload_t * payload = (gp_rects_payload_t *)gp_canvas_recording_allocate( _canvas, payload_size );
    if( payload == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    payload->capacity = (gp_uint32_t)_rect_count;
    payload->count = 0;
    _canvas->active_rects = payload;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_add_rect( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _canvas->active_rects == GP_NULLPTR ||
        __float_valid( _x ) == GP_FALSE || __float_valid( _y ) == GP_FALSE ||
        __float_valid( _width ) == GP_FALSE || __float_valid( _height ) == GP_FALSE ||
        _width <= 0.f || _height <= 0.f )
    {
        return GP_FAILURE;
    }
#endif

    gp_rects_payload_t * payload = _canvas->active_rects;
    const gp_state_t * state = GP_GET_STATE( _canvas );
#if defined(GP_DEBUG)
    if( __color_valid( &state->color ) == GP_FALSE )
    {
        return GP_FAILURE;
    }
#endif
    if( payload->count == payload->capacity )
    {
        return GP_FAILURE;
    }

    gp_rect_instance_t * instance = payload->instances + payload->count++;
    instance->x = _x;
    instance->y = _y;
    instance->width = _width;
    instance->height = _height;
    instance->color = state->color;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_end_rects( gp_canvas_t * _canvas )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _canvas->active_rects == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    gp_rects_payload_t * payload = _canvas->active_rects;
    _canvas->active_rects = GP_NULLPTR;
    if( payload->count != payload->capacity )
    {
        return GP_FAILURE;
    }

    gp_result_t result = gp_internal_geometry( _canvas, payload->count * 4U, payload->count * 6U, __write_rects, payload, 0 );
    return result;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __write_quads( gp_mesh_writer_t * _writer, const void * _payload )
{
    const gp_quads_payload_t * payload = (const gp_quads_payload_t *)_payload;
    for( gp_uint32_t quad_index = 0; quad_index != payload->capacity; ++quad_index )
    {
        const gp_quad_t * quad = payload->quads + quad_index;
        gp_uint32_t vertex = quad_index * 4U;
        gp_uint32_t index = quad_index * 6U;
        for( gp_uint32_t corner = 0; corner != 4U; ++corner )
        {
            const gp_quad_vertex_t * source = quad->vertices + corner;
            gp_argb_t color = __color_argb( &source->color, &_writer->mesh->color, 1.f );
            GP_CALL( gp_mesh_writer_position, (_writer, vertex + corner, source->x, source->y) );
            GP_CALL( gp_mesh_writer_color, (_writer, vertex + corner, color) );
            GP_CALL( gp_mesh_writer_uv, (_writer, vertex + corner, source->u, source->v) );
        }
        GP_CALL( gp_mesh_writer_index, (_writer, index + 0U, vertex + 0U) );
        GP_CALL( gp_mesh_writer_index, (_writer, index + 1U, vertex + 1U) );
        GP_CALL( gp_mesh_writer_index, (_writer, index + 2U, vertex + 2U) );
        GP_CALL( gp_mesh_writer_index, (_writer, index + 3U, vertex + 0U) );
        GP_CALL( gp_mesh_writer_index, (_writer, index + 4U, vertex + 2U) );
        GP_CALL( gp_mesh_writer_index, (_writer, index + 5U, vertex + 3U) );
    }
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_begin_quads( gp_canvas_t * _canvas, gp_size_t _quad_count )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _quad_count == 0 || _canvas->active_rects != GP_NULLPTR ||
        _canvas->active_quads != GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif
    if( _quad_count > UINT32_MAX / 6U || _quad_count >
            (SIZE_MAX - offsetof( gp_quads_payload_t, quads )) / sizeof( gp_quad_t ) )
    {
        return GP_FAILURE;
    }

    gp_size_t payload_size = offsetof( gp_quads_payload_t, quads ) + _quad_count * sizeof( gp_quad_t );
    gp_quads_payload_t * payload = (gp_quads_payload_t *)gp_canvas_recording_allocate( _canvas, payload_size );
    if( payload == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    payload->capacity = (gp_uint32_t)_quad_count;
    payload->vertex_count = 0;
    _canvas->active_quads = payload;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_add_quad_vertex( gp_canvas_t * _canvas, float _x, float _y )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _canvas->active_quads == GP_NULLPTR ||
        __float_valid( _x ) == GP_FALSE || __float_valid( _y ) == GP_FALSE )
    {
        return GP_FAILURE;
    }
#endif

    gp_quads_payload_t * payload = _canvas->active_quads;
    const gp_state_t * state = GP_GET_STATE( _canvas );
#if defined(GP_DEBUG)
    if( __float_valid( state->uv_u ) == GP_FALSE || __float_valid( state->uv_v ) == GP_FALSE ||
        __color_valid( &state->color ) == GP_FALSE )
    {
        return GP_FAILURE;
    }
#endif
    if( payload->vertex_count == payload->capacity * 4U )
    {
        return GP_FAILURE;
    }

    gp_uint32_t quad_index = payload->vertex_count / 4U;
    gp_uint32_t corner = payload->vertex_count % 4U;
    gp_quad_vertex_t * vertex = payload->quads[quad_index].vertices + corner;
    vertex->x = _x;
    vertex->y = _y;
    vertex->color = state->color;
    vertex->u = state->uv_u;
    vertex->v = state->uv_v;
    ++payload->vertex_count;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_end_quads( gp_canvas_t * _canvas )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _canvas->active_quads == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    gp_quads_payload_t * payload = _canvas->active_quads;
    _canvas->active_quads = GP_NULLPTR;
    if( payload->vertex_count != payload->capacity * 4U )
    {
        return GP_FAILURE;
    }

    gp_result_t result = gp_internal_geometry( _canvas, payload->capacity * 4U, payload->capacity * 6U, __write_quads, payload, 0 );
    return result;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __multicolor_rect_vertex( gp_canvas_t * _canvas, float _x, float _y, float _u, float _v, float _r, float _g, float _b, float _a )
{
    gp_result_t color_result = gp_set_color( _canvas, _r, _g, _b, _a );
    if( color_result != GP_SUCCESSFUL )
    {
        return GP_FAILURE;
    }

    gp_result_t uv_result = gp_set_uv( _canvas, _u, _v );
    if( uv_result != GP_SUCCESSFUL )
    {
        return GP_FAILURE;
    }

    gp_result_t vertex_result = gp_add_quad_vertex( _canvas, _x, _y );
    if( vertex_result != GP_SUCCESSFUL )
    {
        return GP_FAILURE;
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_multicolor_rect( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _top_left_r, float _top_left_g, float _top_left_b, float _top_left_a, float _top_right_r, float _top_right_g, float _top_right_b, float _top_right_a, float _bottom_right_r, float _bottom_right_g, float _bottom_right_b, float _bottom_right_a, float _bottom_left_r, float _bottom_left_g, float _bottom_left_b, float _bottom_left_a )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    if( __float_valid( _x ) == GP_FALSE )
    {
        return GP_FAILURE;
    }

    if( __float_valid( _y ) == GP_FALSE )
    {
        return GP_FAILURE;
    }

    if( __float_valid( _width ) == GP_FALSE )
    {
        return GP_FAILURE;
    }

    if( __float_valid( _height ) == GP_FALSE )
    {
        return GP_FAILURE;
    }
#endif

    gp_result_t push_result = gp_push_state( _canvas );
    if( push_result != GP_SUCCESSFUL )
    {
        return GP_FAILURE;
    }

    gp_result_t begin_result = gp_begin_quads( _canvas, 1 );
    if( begin_result != GP_SUCCESSFUL )
    {
        gp_result_t pop_result = gp_pop_state( _canvas );
        GP_UNUSED( pop_result );

        return GP_FAILURE;
    }

    gp_result_t result = __multicolor_rect_vertex( _canvas, _x, _y, 0.f, 0.f, _top_left_r, _top_left_g, _top_left_b, _top_left_a );

    if( result == GP_SUCCESSFUL )
    {
        result = __multicolor_rect_vertex( _canvas, _x + _width, _y, 1.f, 0.f, _top_right_r, _top_right_g, _top_right_b, _top_right_a );
    }

    if( result == GP_SUCCESSFUL )
    {
        result = __multicolor_rect_vertex( _canvas, _x + _width, _y + _height, 1.f, 1.f, _bottom_right_r, _bottom_right_g, _bottom_right_b, _bottom_right_a );
    }

    if( result == GP_SUCCESSFUL )
    {
        result = __multicolor_rect_vertex( _canvas, _x, _y + _height, 0.f, 1.f, _bottom_left_r, _bottom_left_g, _bottom_left_b, _bottom_left_a );
    }

    gp_result_t end_result = gp_end_quads( _canvas );
    if( end_result != GP_SUCCESSFUL )
    {
        result = GP_FAILURE;
    }

    gp_result_t pop_result = gp_pop_state( _canvas );
    if( pop_result != GP_SUCCESSFUL )
    {
        result = GP_FAILURE;
    }

    return result;
}
