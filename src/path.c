#include "path.h"

#include "detail.h"

#include <float.h>
#include <string.h>

typedef struct gp_flat_path_point_t
{
    gp_vec2f_t point;
    float distance;
    gp_uint32_t contour_index;
    gp_bool_t contour_start;
} gp_flat_path_point_t;

typedef struct gp_flat_path_t
{
    gp_flat_path_point_t * points;
    gp_uint32_t count;
    gp_uint32_t capacity;
    float length;
} gp_flat_path_t;

static gp_bool_t __path_same_point( const gp_vec2f_t * _a, const gp_vec2f_t * _b )
{
    return GP_EQUAL_F( _a->x - _b->x, 0.00001f ) == GP_TRUE &&
        GP_EQUAL_F( _a->y - _b->y, 0.00001f ) == GP_TRUE
        ? GP_TRUE
        : GP_FALSE;
}

static float __path_distance( const gp_vec2f_t * _a, const gp_vec2f_t * _b )
{
    float dx = _b->x - _a->x;
    float dy = _b->y - _a->y;

    return GP_MATH_SQRTF( dx * dx + dy * dy );
}

static void __path_destroy_contour( gp_path_t * _path, gp_path_contour_t * _contour )
{
    for( gp_path_anchor_t * anchor = _contour->anchors; anchor != GP_NULLPTR; )
    {
        gp_path_anchor_t * next = anchor->next;
        (*_path->free)(anchor, _path->ud);
        anchor = next;
    }

    for( gp_path_edge_t * edge = _contour->edges; edge != GP_NULLPTR; )
    {
        gp_path_edge_t * next = edge->next;
        (*_path->free)(edge, _path->ud);
        edge = next;
    }

    (*_path->free)(_contour, _path->ud);
}

static gp_path_anchor_t * __path_push_anchor( gp_path_t * _path, gp_path_contour_t * _contour, float _x, float _y )
{
    gp_path_anchor_t * anchor = (gp_path_anchor_t *)(*_path->malloc)(sizeof( gp_path_anchor_t ), _path->ud);
    anchor->next = GP_NULLPTR;
    anchor->prev = GP_NULLPTR;
    anchor->point.x = _x;
    anchor->point.y = _y;
    anchor->style = _path->current_style;

    GP_LIST_PUSHBACK( gp_path_anchor_t, _contour->anchors, anchor );

    return anchor;
}

static gp_result_t __path_push_edge( gp_path_t * _path, gp_uint32_t _controls, const gp_vec2f_t * _points )
{
#if defined(GP_DEBUG)
    if( _path->current_contour == GP_NULLPTR || _path->current_contour->anchors == GP_NULLPTR ||
        _path->current_contour->closed == GP_TRUE )
    {
        return GP_FAILURE;
    }
#endif

    gp_path_edge_t * edge = (gp_path_edge_t *)(*_path->malloc)(sizeof( gp_path_edge_t ), _path->ud);
    edge->next = GP_NULLPTR;
    edge->prev = GP_NULLPTR;
    edge->controls = _controls;
    for( gp_uint32_t index = 0; index != _controls; ++index )
    {
        edge->point[index] = _points[index];
    }

    GP_LIST_PUSHBACK( gp_path_edge_t, _path->current_contour->edges, edge );

    return GP_SUCCESSFUL;
}

gp_result_t gp_path_create( gp_path_t ** _path, gp_malloc_t _malloc, gp_realloc_t _realloc, gp_free_t _free, void * _ud, gp_uint8_t _curve_quality, gp_uint8_t _ellipse_quality )
{
#if defined(GP_DEBUG)
    if( _path == GP_NULLPTR || _malloc == GP_NULLPTR || _realloc == GP_NULLPTR || _free == GP_NULLPTR ||
        _curve_quality == 0 || _ellipse_quality == 0 )
    {
        return GP_FAILURE;
    }
#endif

    gp_path_t * path = (gp_path_t *)(*_malloc)(sizeof( gp_path_t ), _ud);
    path->contours = GP_NULLPTR;
    path->current_contour = GP_NULLPTR;
    memset( &path->current_style, 0, sizeof( path->current_style ) );
    path->curve_quality = _curve_quality;
    path->ellipse_quality = _ellipse_quality;
    path->malloc = _malloc;
    path->realloc = _realloc;
    path->free = _free;
    path->ud = _ud;

    *_path = path;

    return GP_SUCCESSFUL;
}

gp_result_t gp_path_destroy( gp_path_t * _path )
{
#if defined(GP_DEBUG)
    if( _path == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    GP_CALL( gp_path_clear, (_path) );
    (*_path->free)(_path, _path->ud);

    return GP_SUCCESSFUL;
}

gp_result_t gp_path_clear( gp_path_t * _path )
{
#if defined(GP_DEBUG)
    if( _path == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    for( gp_path_contour_t * contour = _path->contours; contour != GP_NULLPTR; )
    {
        gp_path_contour_t * next = contour->next;
        __path_destroy_contour( _path, contour );
        contour = next;
    }

    _path->contours = GP_NULLPTR;
    _path->current_contour = GP_NULLPTR;
    memset( &_path->current_style, 0, sizeof( _path->current_style ) );

    return GP_SUCCESSFUL;
}

gp_result_t gp_path_set_style( gp_path_t * _path, const gp_path_style_t * _style )
{
#if defined(GP_DEBUG)
    if( _path == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    if( _style != GP_NULLPTR )
    {
        gp_uint32_t valid_attributes = GP_PATH_STYLE_WIDTH | GP_PATH_STYLE_COLOR | GP_PATH_STYLE_OUTLINE_COLOR;
        if( (_style->attributes & ~valid_attributes) != 0 ||
            ((_style->attributes & GP_PATH_STYLE_WIDTH) != 0 &&
                (_style->width <= 0.f || GP_MATH_ISFINITE( _style->width ) == 0)) )
        {
            return GP_FAILURE;
        }

        if( (_style->attributes & GP_PATH_STYLE_COLOR) != 0 &&
            (GP_MATH_ISFINITE( _style->color.r ) == 0 || GP_MATH_ISFINITE( _style->color.g ) == 0 ||
                GP_MATH_ISFINITE( _style->color.b ) == 0 || GP_MATH_ISFINITE( _style->color.a ) == 0 ||
                _style->color.r < 0.f || _style->color.r > 1.f || _style->color.g < 0.f || _style->color.g > 1.f ||
                _style->color.b < 0.f || _style->color.b > 1.f || _style->color.a < 0.f || _style->color.a > 1.f) )
        {
            return GP_FAILURE;
        }

        if( (_style->attributes & GP_PATH_STYLE_OUTLINE_COLOR) != 0 &&
            (GP_MATH_ISFINITE( _style->outline_color.r ) == 0 || GP_MATH_ISFINITE( _style->outline_color.g ) == 0 ||
                GP_MATH_ISFINITE( _style->outline_color.b ) == 0 || GP_MATH_ISFINITE( _style->outline_color.a ) == 0 ||
                _style->outline_color.r < 0.f || _style->outline_color.r > 1.f || _style->outline_color.g < 0.f || _style->outline_color.g > 1.f ||
                _style->outline_color.b < 0.f || _style->outline_color.b > 1.f || _style->outline_color.a < 0.f || _style->outline_color.a > 1.f) )
        {
            return GP_FAILURE;
        }
    }
#endif

    if( _style == GP_NULLPTR )
    {
        memset( &_path->current_style, 0, sizeof( _path->current_style ) );
    }
    else
    {
        _path->current_style = *_style;
    }

    return GP_SUCCESSFUL;
}

gp_result_t gp_path_move_to( gp_path_t * _path, float _x, float _y )
{
#if defined(GP_DEBUG)
    if( _path == GP_NULLPTR || GP_MATH_ISFINITE( _x ) == 0 || GP_MATH_ISFINITE( _y ) == 0 )
    {
        return GP_FAILURE;
    }
#endif

    gp_path_contour_t * contour = (gp_path_contour_t *)(*_path->malloc)(sizeof( gp_path_contour_t ), _path->ud);
    contour->anchors = GP_NULLPTR;
    contour->edges = GP_NULLPTR;
    contour->closed = GP_FALSE;
    contour->next = GP_NULLPTR;
    contour->prev = GP_NULLPTR;
    __path_push_anchor( _path, contour, _x, _y );

    GP_LIST_PUSHBACK( gp_path_contour_t, _path->contours, contour );
    _path->current_contour = contour;

    return GP_SUCCESSFUL;
}

gp_result_t gp_path_line_to( gp_path_t * _path, float _x, float _y )
{
#if defined(GP_DEBUG)
    if( _path == GP_NULLPTR || GP_MATH_ISFINITE( _x ) == 0 || GP_MATH_ISFINITE( _y ) == 0 )
    {
        return GP_FAILURE;
    }
#endif

    gp_vec2f_t unused[1];
    GP_CALL( __path_push_edge, (_path, 0, unused) );
    __path_push_anchor( _path, _path->current_contour, _x, _y );

    return GP_SUCCESSFUL;
}

gp_result_t gp_path_quadratic_curve_to( gp_path_t * _path, float _p0x, float _p0y, float _x, float _y )
{
#if defined(GP_DEBUG)
    if( _path == GP_NULLPTR || GP_MATH_ISFINITE( _p0x ) == 0 || GP_MATH_ISFINITE( _p0y ) == 0 ||
        GP_MATH_ISFINITE( _x ) == 0 || GP_MATH_ISFINITE( _y ) == 0 )
    {
        return GP_FAILURE;
    }
#endif

    gp_vec2f_t controls[1] = {{_p0x, _p0y}};
    GP_CALL( __path_push_edge, (_path, 1, controls) );
    __path_push_anchor( _path, _path->current_contour, _x, _y );

    return GP_SUCCESSFUL;
}

gp_result_t gp_path_bezier_curve_to( gp_path_t * _path, float _p0x, float _p0y, float _p1x, float _p1y, float _x, float _y )
{
#if defined(GP_DEBUG)
    if( _path == GP_NULLPTR || GP_MATH_ISFINITE( _p0x ) == 0 || GP_MATH_ISFINITE( _p0y ) == 0 ||
        GP_MATH_ISFINITE( _p1x ) == 0 || GP_MATH_ISFINITE( _p1y ) == 0 ||
        GP_MATH_ISFINITE( _x ) == 0 || GP_MATH_ISFINITE( _y ) == 0 )
    {
        return GP_FAILURE;
    }
#endif

    gp_vec2f_t controls[2] = {{_p0x, _p0y}, {_p1x, _p1y}};
    GP_CALL( __path_push_edge, (_path, 2, controls) );
    __path_push_anchor( _path, _path->current_contour, _x, _y );

    return GP_SUCCESSFUL;
}

gp_result_t gp_path_ellipse_arc( gp_path_t * _path, float _x, float _y, float _width, float _height, float _angle_from, float _angle_to )
{
#if defined(GP_DEBUG)
    if( _path == GP_NULLPTR || GP_MATH_ISFINITE( _x ) == 0 || GP_MATH_ISFINITE( _y ) == 0 ||
        GP_MATH_ISFINITE( _width ) == 0 || GP_MATH_ISFINITE( _height ) == 0 ||
        GP_MATH_ISFINITE( _angle_from ) == 0 || GP_MATH_ISFINITE( _angle_to ) == 0 ||
        _width <= 0.f || _height <= 0.f )
    {
        return GP_FAILURE;
    }
#endif

    float angle_delta = _angle_to - _angle_from;
    if( GP_EQUAL_F( angle_delta, 0.000001f ) == GP_TRUE )
    {
        return GP_SUCCESSFUL;
    }

    float start_x = _x + GP_MATH_COSF( _angle_from ) * _width;
    float start_y = _y + GP_MATH_SINF( _angle_from ) * _height;
    gp_bool_t new_contour = _path->current_contour == GP_NULLPTR || _path->current_contour->closed == GP_TRUE
        ? GP_TRUE
        : GP_FALSE;
    if( new_contour == GP_TRUE )
    {
        GP_CALL( gp_path_move_to, (_path, start_x, start_y) );
    }
    else
    {
        const gp_path_anchor_t * current = GP_LIST_BACK( _path->current_contour->anchors );
        gp_vec2f_t start = {start_x, start_y};
        if( __path_same_point( &current->point, &start ) == GP_FALSE )
        {
            GP_CALL( gp_path_line_to, (_path, start_x, start_y) );
        }
    }

    gp_vec2f_t controls[3] = {{_x, _y}, {_width, _height}, {_angle_from, _angle_to}};
    GP_CALL( __path_push_edge, (_path, 3, controls) );
    __path_push_anchor( _path, _path->current_contour,
        _x + GP_MATH_COSF( _angle_to ) * _width,
        _y + GP_MATH_SINF( _angle_to ) * _height );

    float angle_delta_abs = angle_delta < 0.f ? -angle_delta : angle_delta;
    if( new_contour == GP_TRUE && GP_EQUAL_F( angle_delta_abs - gp_constant_two_pi, 0.000001f ) == GP_TRUE )
    {
        _path->current_contour->closed = GP_TRUE;
    }

    return GP_SUCCESSFUL;
}

gp_result_t gp_path_tangent_arc_to( gp_path_t * _path, float _x1, float _y1, float _x2, float _y2, float _radius )
{
#if defined(GP_DEBUG)
    if( _path == GP_NULLPTR || _path->current_contour == GP_NULLPTR ||
        GP_MATH_ISFINITE( _x1 ) == 0 || GP_MATH_ISFINITE( _y1 ) == 0 ||
        GP_MATH_ISFINITE( _x2 ) == 0 || GP_MATH_ISFINITE( _y2 ) == 0 ||
        GP_MATH_ISFINITE( _radius ) == 0 || _radius < 0.f )
    {
        return GP_FAILURE;
    }
#endif

    const gp_path_anchor_t * current = GP_LIST_BACK( _path->current_contour->anchors );
    float incoming_x = current->point.x - _x1;
    float incoming_y = current->point.y - _y1;
    float outgoing_x = _x2 - _x1;
    float outgoing_y = _y2 - _y1;
    float incoming_length = GP_MATH_SQRTF( incoming_x * incoming_x + incoming_y * incoming_y );
    float outgoing_length = GP_MATH_SQRTF( outgoing_x * outgoing_x + outgoing_y * outgoing_y );
    if( incoming_length <= 0.0001f || outgoing_length <= 0.0001f || _radius <= 0.0001f )
    {
        return gp_path_line_to( _path, _x1, _y1 );
    }

    incoming_x /= incoming_length;
    incoming_y /= incoming_length;
    outgoing_x /= outgoing_length;
    outgoing_y /= outgoing_length;

    float dot = incoming_x * outgoing_x + incoming_y * outgoing_y;
    dot = dot < -1.f ? -1.f : (dot > 1.f ? 1.f : dot);
    float angle = GP_MATH_ACOSF( dot );
    float half_angle = angle * 0.5f;
    float sin_half_angle = GP_MATH_SINF( half_angle );
    float cos_half_angle = GP_MATH_COSF( half_angle );
    float tan_half_angle = sin_half_angle / cos_half_angle;
    if( sin_half_angle <= 0.0001f || tan_half_angle <= 0.0001f )
    {
        return gp_path_line_to( _path, _x1, _y1 );
    }

    float tangent_distance = _radius / tan_half_angle;
    tangent_distance = tangent_distance > incoming_length ? incoming_length : tangent_distance;
    tangent_distance = tangent_distance > outgoing_length ? outgoing_length : tangent_distance;
    float effective_radius = tangent_distance * tan_half_angle;
    float tangent0_x = _x1 + incoming_x * tangent_distance;
    float tangent0_y = _y1 + incoming_y * tangent_distance;
    float tangent1_x = _x1 + outgoing_x * tangent_distance;
    float tangent1_y = _y1 + outgoing_y * tangent_distance;
    float bisector_x = incoming_x + outgoing_x;
    float bisector_y = incoming_y + outgoing_y;
    float bisector_length = GP_MATH_SQRTF( bisector_x * bisector_x + bisector_y * bisector_y );
    if( bisector_length <= 0.0001f )
    {
        return gp_path_line_to( _path, _x1, _y1 );
    }

    bisector_x /= bisector_length;
    bisector_y /= bisector_length;
    float center_distance = effective_radius / sin_half_angle;
    float center_x = _x1 + bisector_x * center_distance;
    float center_y = _y1 + bisector_y * center_distance;
    float angle_from = GP_MATH_ATAN2F( tangent0_y - center_y, tangent0_x - center_x );
    float angle_to = GP_MATH_ATAN2F( tangent1_y - center_y, tangent1_x - center_x );
    float turn = (-incoming_x) * outgoing_y - (-incoming_y) * outgoing_x;
    if( turn > 0.f )
    {
        while( angle_to < angle_from )
        {
            angle_to += gp_constant_two_pi;
        }
    }
    else
    {
        while( angle_to > angle_from )
        {
            angle_to -= gp_constant_two_pi;
        }
    }

    GP_CALL( gp_path_line_to, (_path, tangent0_x, tangent0_y) );
    GP_CALL( gp_path_ellipse_arc, (_path, center_x, center_y, effective_radius, effective_radius, angle_from, angle_to) );

    return GP_SUCCESSFUL;
}

gp_result_t gp_path_close( gp_path_t * _path )
{
#if defined(GP_DEBUG)
    if( _path == GP_NULLPTR || _path->current_contour == GP_NULLPTR ||
        _path->current_contour->edges == GP_NULLPTR || _path->current_contour->closed == GP_TRUE )
    {
        return GP_FAILURE;
    }
#endif

    _path->current_contour->closed = GP_TRUE;

    return GP_SUCCESSFUL;
}

static gp_result_t __path_validate_layout( const gp_path_layout_t * _layout, gp_size_t _stride )
{
#if defined(GP_DEBUG)
    if( _layout == GP_NULLPTR )
    {
        return GP_SUCCESSFUL;
    }

    if( _layout->width_offset != GP_PATH_ATTRIBUTE_NONE &&
        (_layout->width_offset < sizeof( float ) * 2 || _layout->width_offset > _stride - sizeof( float )) )
    {
        return GP_FAILURE;
    }
    if( _layout->color_offset != GP_PATH_ATTRIBUTE_NONE &&
        (_stride < sizeof( gp_color_t ) || _layout->color_offset < sizeof( float ) * 2 ||
            _layout->color_offset > _stride - sizeof( gp_color_t )) )
    {
        return GP_FAILURE;
    }
    if( _layout->outline_color_offset != GP_PATH_ATTRIBUTE_NONE &&
        (_stride < sizeof( gp_color_t ) || _layout->outline_color_offset < sizeof( float ) * 2 ||
            _layout->outline_color_offset > _stride - sizeof( gp_color_t )) )
    {
        return GP_FAILURE;
    }
#else
    GP_UNUSED( _layout );
    GP_UNUSED( _stride );
#endif

    return GP_SUCCESSFUL;
}

static void __path_read_point( const void * _points, gp_size_t _stride, gp_size_t _index, float * _x, float * _y, const gp_path_layout_t * _layout, gp_path_style_t * _style )
{
    const gp_uint8_t * bytes = (const gp_uint8_t *)_points + _stride * _index;
    memcpy( _x, bytes, sizeof( float ) );
    memcpy( _y, bytes + sizeof( float ), sizeof( float ) );
    memset( _style, 0, sizeof( *_style ) );

    if( _layout == GP_NULLPTR )
    {
        return;
    }

    if( _layout->width_offset != GP_PATH_ATTRIBUTE_NONE )
    {
        _style->attributes |= GP_PATH_STYLE_WIDTH;
        memcpy( &_style->width, bytes + _layout->width_offset, sizeof( float ) );
    }
    if( _layout->color_offset != GP_PATH_ATTRIBUTE_NONE )
    {
        _style->attributes |= GP_PATH_STYLE_COLOR;
        memcpy( &_style->color, bytes + _layout->color_offset, sizeof( gp_color_t ) );
    }
    if( _layout->outline_color_offset != GP_PATH_ATTRIBUTE_NONE )
    {
        _style->attributes |= GP_PATH_STYLE_OUTLINE_COLOR;
        memcpy( &_style->outline_color, bytes + _layout->outline_color_offset, sizeof( gp_color_t ) );
    }
}

gp_result_t gp_path_polyline( gp_path_t * _path, const void * _points, gp_size_t _point_count, gp_size_t _point_stride, const gp_path_layout_t * _layout, gp_bool_t _closed )
{
#if defined(GP_DEBUG)
    if( _path == GP_NULLPTR || _points == GP_NULLPTR || _point_stride < sizeof( float ) * 2 ||
        _point_count < (_closed == GP_TRUE ? 3U : 2U) )
    {
        return GP_FAILURE;
    }
#endif

    GP_CALL( __path_validate_layout, (_layout, _point_stride) );

    gp_path_style_t saved_style = _path->current_style;
    for( gp_size_t index = 0; index != _point_count; ++index )
    {
        float x;
        float y;
        gp_path_style_t style;
        __path_read_point( _points, _point_stride, index, &x, &y, _layout, &style );
        GP_CALL( gp_path_set_style, (_path, &style) );
        if( index == 0 )
        {
            GP_CALL( gp_path_move_to, (_path, x, y) );
        }
        else
        {
            GP_CALL( gp_path_line_to, (_path, x, y) );
        }
    }

    if( _closed == GP_TRUE )
    {
        GP_CALL( gp_path_close, (_path) );
    }

    _path->current_style = saved_style;

    return GP_SUCCESSFUL;
}

static void __path_extrapolate( gp_vec2f_t * _result, const gp_vec2f_t * _endpoint, const gp_vec2f_t * _neighbor )
{
    _result->x = _endpoint->x * 2.f - _neighbor->x;
    _result->y = _endpoint->y * 2.f - _neighbor->y;
}

static float __path_knot( float _previous, const gp_vec2f_t * _a, const gp_vec2f_t * _b )
{
    float distance = __path_distance( _a, _b );

    return _previous + GP_MATH_SQRTF( distance );
}

static void __path_catmull_controls( const gp_vec2f_t * _p0, const gp_vec2f_t * _p1, const gp_vec2f_t * _p2, const gp_vec2f_t * _p3, gp_vec2f_t * _c0, gp_vec2f_t * _c1 )
{
    float t0 = 0.f;
    float t1 = __path_knot( t0, _p0, _p1 );
    float t2 = __path_knot( t1, _p1, _p2 );
    float t3 = __path_knot( t2, _p2, _p3 );
    float dt = t2 - t1;

    float m1x = dt * ((_p1->x - _p0->x) / (t1 - t0) - (_p2->x - _p0->x) / (t2 - t0) + (_p2->x - _p1->x) / (t2 - t1));
    float m1y = dt * ((_p1->y - _p0->y) / (t1 - t0) - (_p2->y - _p0->y) / (t2 - t0) + (_p2->y - _p1->y) / (t2 - t1));
    float m2x = dt * ((_p2->x - _p1->x) / (t2 - t1) - (_p3->x - _p1->x) / (t3 - t1) + (_p3->x - _p2->x) / (t3 - t2));
    float m2y = dt * ((_p2->y - _p1->y) / (t2 - t1) - (_p3->y - _p1->y) / (t3 - t1) + (_p3->y - _p2->y) / (t3 - t2));

    _c0->x = _p1->x + m1x / 3.f;
    _c0->y = _p1->y + m1y / 3.f;
    _c1->x = _p2->x - m2x / 3.f;
    _c1->y = _p2->y - m2y / 3.f;
}

gp_result_t gp_path_catmull_rom( gp_path_t * _path, const void * _points, gp_size_t _point_count, gp_size_t _point_stride, const gp_path_layout_t * _layout, gp_bool_t _closed )
{
#if defined(GP_DEBUG)
    if( _path == GP_NULLPTR || _points == GP_NULLPTR || _point_stride < sizeof( float ) * 2 ||
        _point_count < (_closed == GP_TRUE ? 3U : 2U) )
    {
        return GP_FAILURE;
    }
#endif

    GP_CALL( __path_validate_layout, (_layout, _point_stride) );

    gp_vec2f_t * positions = (gp_vec2f_t *)(*_path->realloc)(GP_NULLPTR, sizeof( gp_vec2f_t ) * _point_count, _path->ud);
    gp_path_style_t * styles = (gp_path_style_t *)(*_path->realloc)(GP_NULLPTR, sizeof( gp_path_style_t ) * _point_count, _path->ud);
    for( gp_size_t index = 0; index != _point_count; ++index )
    {
        __path_read_point( _points, _point_stride, index, &positions[index].x, &positions[index].y, _layout, styles + index );
#if defined(GP_DEBUG)
        if( index != 0 && __path_same_point( positions + index - 1, positions + index ) == GP_TRUE )
        {
            (*_path->free)(styles, _path->ud);
            (*_path->free)(positions, _path->ud);
            return GP_FAILURE;
        }
#endif
    }
#if defined(GP_DEBUG)
    if( _closed == GP_TRUE && __path_same_point( positions, positions + _point_count - 1 ) == GP_TRUE )
    {
        (*_path->free)(styles, _path->ud);
        (*_path->free)(positions, _path->ud);
        return GP_FAILURE;
    }
#endif

    gp_path_style_t saved_style = _path->current_style;
    GP_CALL( gp_path_set_style, (_path, styles) );
    GP_CALL( gp_path_move_to, (_path, positions[0].x, positions[0].y) );

    gp_size_t segment_count = _closed == GP_TRUE ? _point_count : _point_count - 1;
    for( gp_size_t segment = 0; segment != segment_count; ++segment )
    {
        gp_size_t i1 = segment;
        gp_size_t i2 = (segment + 1) % _point_count;
        gp_vec2f_t p0;
        gp_vec2f_t p3;
        if( _closed == GP_TRUE )
        {
            p0 = positions[(i1 + _point_count - 1) % _point_count];
            p3 = positions[(i2 + 1) % _point_count];
        }
        else
        {
            if( i1 == 0 )
            {
                __path_extrapolate( &p0, positions + i1, positions + i2 );
            }
            else
            {
                p0 = positions[i1 - 1];
            }
            if( i2 + 1 == _point_count )
            {
                __path_extrapolate( &p3, positions + i2, positions + i1 );
            }
            else
            {
                p3 = positions[i2 + 1];
            }
        }

        gp_vec2f_t c0;
        gp_vec2f_t c1;
        __path_catmull_controls( &p0, positions + i1, positions + i2, &p3, &c0, &c1 );
        GP_CALL( gp_path_set_style, (_path, styles + i2) );
        GP_CALL( gp_path_bezier_curve_to, (_path, c0.x, c0.y, c1.x, c1.y, positions[i2].x, positions[i2].y) );
    }

    if( _closed == GP_TRUE )
    {
        GP_CALL( gp_path_close, (_path) );
    }

    _path->current_style = saved_style;
    (*_path->free)(styles, _path->ud);
    (*_path->free)(positions, _path->ud);

    return GP_SUCCESSFUL;
}

static void __path_flat_reserve( const gp_path_t * _path, gp_flat_path_t * _flat, gp_uint32_t _required )
{
    if( _required <= _flat->capacity )
    {
        return;
    }

    gp_uint32_t capacity = _flat->capacity == 0 ? 32 : _flat->capacity * 2;
    while( capacity < _required )
    {
        capacity *= 2;
    }

    gp_flat_path_point_t * points = (gp_flat_path_point_t *)(*_path->realloc)(GP_NULLPTR, sizeof( gp_flat_path_point_t ) * capacity, _path->ud);
    for( gp_uint32_t index = 0; index != _flat->count; ++index )
    {
        points[index] = _flat->points[index];
    }
    if( _flat->points != GP_NULLPTR )
    {
        (*_path->free)(_flat->points, _path->ud);
    }

    _flat->points = points;
    _flat->capacity = capacity;
}

static void __path_flat_push( const gp_path_t * _path, gp_flat_path_t * _flat, const gp_vec2f_t * _point, gp_uint32_t _contour_index, gp_bool_t _contour_start )
{
    __path_flat_reserve( _path, _flat, _flat->count + 1 );
    gp_flat_path_point_t * output = _flat->points + _flat->count;
    output->point = *_point;
    output->contour_index = _contour_index;
    output->contour_start = _contour_start;
    if( _flat->count == 0 || _contour_start == GP_TRUE )
    {
        output->distance = _flat->length;
    }
    else
    {
        _flat->length += __path_distance( &_flat->points[_flat->count - 1].point, _point );
        output->distance = _flat->length;
    }
    ++_flat->count;
}

static void __path_evaluate_edge( gp_vec2f_t * _point, const gp_path_anchor_t * _from, const gp_path_anchor_t * _to, const gp_path_edge_t * _edge, float _t )
{
    float inverse_t = 1.f - _t;
    switch( _edge->controls )
    {
    case 0:
        _point->x = _from->point.x * inverse_t + _to->point.x * _t;
        _point->y = _from->point.y * inverse_t + _to->point.y * _t;
        break;
    case 1:
        _point->x = inverse_t * inverse_t * _from->point.x + 2.f * inverse_t * _t * _edge->point[0].x + _t * _t * _to->point.x;
        _point->y = inverse_t * inverse_t * _from->point.y + 2.f * inverse_t * _t * _edge->point[0].y + _t * _t * _to->point.y;
        break;
    case 2:
        _point->x = inverse_t * inverse_t * inverse_t * _from->point.x + 3.f * inverse_t * inverse_t * _t * _edge->point[0].x + 3.f * inverse_t * _t * _t * _edge->point[1].x + _t * _t * _t * _to->point.x;
        _point->y = inverse_t * inverse_t * inverse_t * _from->point.y + 3.f * inverse_t * inverse_t * _t * _edge->point[0].y + 3.f * inverse_t * _t * _t * _edge->point[1].y + _t * _t * _t * _to->point.y;
        break;
    case 3:
        {
            float angle = _edge->point[2].x * inverse_t + _edge->point[2].y * _t;
            _point->x = _edge->point[0].x + GP_MATH_COSF( angle ) * _edge->point[1].x;
            _point->y = _edge->point[0].y + GP_MATH_SINF( angle ) * _edge->point[1].y;
        }break;
    default:
        _point->x = _from->point.x;
        _point->y = _from->point.y;
        break;
    }
}

float gp_path_contour_get_length_internal( const gp_path_t * _path, const gp_path_contour_t * _contour )
{
    if( _contour->edges == GP_NULLPTR )
    {
        return 0.f;
    }

    float length = 0.f;
    gp_vec2f_t previous = _contour->anchors->point;
    const gp_path_anchor_t * from = _contour->anchors;
    for( const gp_path_edge_t * edge = _contour->edges; edge != GP_NULLPTR; edge = edge->next )
    {
        const gp_path_anchor_t * to = from->next;
        gp_uint32_t samples = edge->controls == 0
            ? 1
            : (edge->controls == 3
                ? gp_ellipse_arc_segment_count( edge->point[2].y - edge->point[2].x, _path->ellipse_quality )
                : _path->curve_quality);
        for( gp_uint32_t index = 1; index <= samples; ++index )
        {
            float t = edge->controls == 3
                ? gp_ellipse_arc_sample_t( edge->point[2].y - edge->point[2].x, _path->ellipse_quality, index )
                : (float)index / (float)samples;
            gp_vec2f_t point;
            __path_evaluate_edge( &point, from, to, edge, t );
            if( edge->controls == 3 )
            {
                float angle = gp_ellipse_arc_sample_angle( edge->point[2].x, edge->point[2].y, _path->ellipse_quality, index );
                point.x = edge->point[0].x + GP_MATH_COSF( angle ) * edge->point[1].x;
                point.y = edge->point[0].y + GP_MATH_SINF( angle ) * edge->point[1].y;
            }
            length += __path_distance( &previous, &point );
            previous = point;
        }
        from = to;
    }

    if( _contour->closed == GP_TRUE && __path_same_point( &previous, &_contour->anchors->point ) == GP_FALSE )
    {
        length += __path_distance( &previous, &_contour->anchors->point );
    }

    return length;
}

static gp_result_t __path_flatten( const gp_path_t * _path, gp_flat_path_t * _flat )
{
    memset( _flat, 0, sizeof( *_flat ) );
    gp_uint32_t contour_index = 0;
    for( const gp_path_contour_t * contour = _path->contours; contour != GP_NULLPTR; contour = contour->next, ++contour_index )
    {
        if( contour->edges == GP_NULLPTR )
        {
            continue;
        }

        const gp_path_anchor_t * from = contour->anchors;
        __path_flat_push( _path, _flat, &from->point, contour_index, GP_TRUE );
        for( const gp_path_edge_t * edge = contour->edges; edge != GP_NULLPTR; edge = edge->next )
        {
            const gp_path_anchor_t * to = from->next;
            gp_uint32_t samples;
            if( edge->controls == 0 )
            {
                samples = 1;
            }
            else if( edge->controls == 3 )
            {
                samples = gp_ellipse_arc_segment_count( edge->point[2].y - edge->point[2].x, _path->ellipse_quality );
            }
            else
            {
                samples = _path->curve_quality;
            }

            for( gp_uint32_t index = 1; index <= samples; ++index )
            {
                float t = edge->controls == 3
                    ? gp_ellipse_arc_sample_t( edge->point[2].y - edge->point[2].x, _path->ellipse_quality, index )
                    : (float)index / (float)samples;
                gp_vec2f_t point;
                __path_evaluate_edge( &point, from, to, edge, t );
                if( edge->controls == 3 )
                {
                    float angle = gp_ellipse_arc_sample_angle( edge->point[2].x, edge->point[2].y, _path->ellipse_quality, index );
                    point.x = edge->point[0].x + GP_MATH_COSF( angle ) * edge->point[1].x;
                    point.y = edge->point[0].y + GP_MATH_SINF( angle ) * edge->point[1].y;
                }
                __path_flat_push( _path, _flat, &point, contour_index, GP_FALSE );
            }

            from = to;
        }

        if( contour->closed == GP_TRUE && __path_same_point( &_flat->points[_flat->count - 1].point, &contour->anchors->point ) == GP_FALSE )
        {
            __path_flat_push( _path, _flat, &contour->anchors->point, contour_index, GP_FALSE );
        }
    }

    return GP_SUCCESSFUL;
}

gp_result_t gp_path_get_length( const gp_path_t * _path, float * _length )
{
#if defined(GP_DEBUG)
    if( _path == GP_NULLPTR || _length == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    gp_flat_path_t flat;
    GP_CALL( __path_flatten, (_path, &flat) );
    *_length = flat.length;
    if( flat.points != GP_NULLPTR )
    {
        (*_path->free)(flat.points, _path->ud);
    }

    return GP_SUCCESSFUL;
}

gp_result_t gp_path_get_bounds( const gp_path_t * _path, gp_bounds_t * _bounds )
{
#if defined(GP_DEBUG)
    if( _path == GP_NULLPTR || _bounds == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    gp_flat_path_t flat;
    GP_CALL( __path_flatten, (_path, &flat) );
    if( flat.count == 0 )
    {
        _bounds->minimum_x = 0.f;
        _bounds->minimum_y = 0.f;
        _bounds->maximum_x = 0.f;
        _bounds->maximum_y = 0.f;
        return GP_SUCCESSFUL;
    }

    _bounds->minimum_x = flat.points[0].point.x;
    _bounds->minimum_y = flat.points[0].point.y;
    _bounds->maximum_x = flat.points[0].point.x;
    _bounds->maximum_y = flat.points[0].point.y;
    for( gp_uint32_t index = 1; index != flat.count; ++index )
    {
        const gp_vec2f_t * point = &flat.points[index].point;
        _bounds->minimum_x = point->x < _bounds->minimum_x ? point->x : _bounds->minimum_x;
        _bounds->minimum_y = point->y < _bounds->minimum_y ? point->y : _bounds->minimum_y;
        _bounds->maximum_x = point->x > _bounds->maximum_x ? point->x : _bounds->maximum_x;
        _bounds->maximum_y = point->y > _bounds->maximum_y ? point->y : _bounds->maximum_y;
    }

    (*_path->free)(flat.points, _path->ud);

    return GP_SUCCESSFUL;
}

gp_result_t gp_path_sample( const gp_path_t * _path, float _value, gp_path_sample_unit_t _unit, gp_path_sample_t * _sample )
{
#if defined(GP_DEBUG)
    if( _path == GP_NULLPTR || _sample == GP_NULLPTR ||
        (_unit != GP_PATH_SAMPLE_NORMALIZED && _unit != GP_PATH_SAMPLE_DISTANCE) ||
        GP_MATH_ISFINITE( _value ) == 0 ||
        (_unit == GP_PATH_SAMPLE_NORMALIZED && (_value < 0.f || _value > 1.f)) || _value < 0.f )
    {
        return GP_FAILURE;
    }
#endif

    gp_flat_path_t flat;
    GP_CALL( __path_flatten, (_path, &flat) );
#if defined(GP_DEBUG)
    if( flat.count < 2 || flat.length <= 0.f || (_unit == GP_PATH_SAMPLE_DISTANCE && _value > flat.length) )
    {
        if( flat.points != GP_NULLPTR )
        {
            (*_path->free)(flat.points, _path->ud);
        }
        return GP_FAILURE;
    }
#endif

    float distance = _unit == GP_PATH_SAMPLE_NORMALIZED ? _value * flat.length : _value;
    gp_uint32_t to_index = 1;
    while( to_index < flat.count && (flat.points[to_index].contour_start == GP_TRUE || flat.points[to_index].distance < distance) )
    {
        ++to_index;
    }
    if( to_index == flat.count )
    {
        to_index = flat.count - 1;
    }

    gp_uint32_t from_index = to_index - 1;
    while( flat.points[to_index].contour_start == GP_TRUE && to_index + 1 < flat.count )
    {
        ++to_index;
        from_index = to_index - 1;
    }

    const gp_flat_path_point_t * from = flat.points + from_index;
    const gp_flat_path_point_t * to = flat.points + to_index;
    float segment_length = to->distance - from->distance;
    float t = segment_length > 0.f ? (distance - from->distance) / segment_length : 0.f;
    t = t < 0.f ? 0.f : (t > 1.f ? 1.f : t);
    _sample->x = from->point.x + (to->point.x - from->point.x) * t;
    _sample->y = from->point.y + (to->point.y - from->point.y) * t;
    float tangent_x = to->point.x - from->point.x;
    float tangent_y = to->point.y - from->point.y;
    float tangent_length = GP_MATH_SQRTF( tangent_x * tangent_x + tangent_y * tangent_y );
    _sample->tangent_x = tangent_x / tangent_length;
    _sample->tangent_y = tangent_y / tangent_length;
    _sample->distance = distance;
    _sample->ratio = distance / flat.length;
    _sample->contour_index = to->contour_index;

    (*_path->free)(flat.points, _path->ud);

    return GP_SUCCESSFUL;
}
