#include "line.h"

#include "struct.h"
#include "detail.h"

//////////////////////////////////////////////////////////////////////////
typedef struct gp_cooked_point_t
{
    gp_vec2f_t position;
    float half_width;
    gp_color_t color;
    gp_color_t outline_color;
    float length;
    float source_length;
    gp_bool_t join;
} gp_cooked_point_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_cooked_path_t
{
    gp_cooked_point_t * points;
    gp_uint32_t count;
    gp_uint32_t capacity;
    gp_bool_t closed;
    float source_offset;
    float source_total_length;
    gp_bool_t source_ccw;
} gp_cooked_path_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_cooked_span_t
{
    gp_cooked_point_t * points;
    gp_uint32_t count;
    gp_uint32_t capacity;
    gp_bool_t closed;
    float source_total_length;
    gp_bool_t source_closed;
    gp_bool_t source_ccw;
} gp_cooked_span_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_cooked_stroke_t
{
    gp_cooked_span_t * spans;
    gp_uint32_t count;
    gp_uint32_t capacity;
} gp_cooked_stroke_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_round_sector_t
{
    float angle_from;
    float angle_delta;
    gp_uint32_t segment_count;
} gp_round_sector_t;
//////////////////////////////////////////////////////////////////////////
static void __line_interpolate_cooked_point( gp_cooked_point_t * const _point, const gp_cooked_point_t * _from, const gp_cooked_point_t * _to, float _t );
//////////////////////////////////////////////////////////////////////////
static float __line_lerp( float _from, float _to, float _t )
{
    float value = _from * (1.f - _t) + _to * _t;
    return value;
}
//////////////////////////////////////////////////////////////////////////
static void __line_lerp_color( gp_color_t * const _color, const gp_color_t * _from, const gp_color_t * _to, float _t )
{
    _color->r = __line_lerp( _from->r, _to->r, _t );
    _color->g = __line_lerp( _from->g, _to->g, _t );
    _color->b = __line_lerp( _from->b, _to->b, _t );
    _color->a = __line_lerp( _from->a, _to->a, _t );
}
//////////////////////////////////////////////////////////////////////////
static float __line_distance( const gp_vec2f_t * _from, const gp_vec2f_t * _to )
{
    float dx = _to->x - _from->x;
    float dy = _to->y - _from->y;

    float distance = GP_MATH_SQRTF( dx * dx + dy * dy );
    return distance;
}
//////////////////////////////////////////////////////////////////////////
static gp_bool_t __line_same_point( const gp_vec2f_t * _from, const gp_vec2f_t * _to )
{
    gp_bool_t equal = GP_EQUAL_F( _from->x - _to->x, 0.00001f ) && GP_EQUAL_F( _from->y - _to->y, 0.00001f );
    return equal;
}
//////////////////////////////////////////////////////////////////////////
static void __line_direction( gp_vec2f_t * const _direction, const gp_vec2f_t * _from, const gp_vec2f_t * _to )
{
    float dx = _to->x - _from->x;
    float dy = _to->y - _from->y;
    float length = GP_MATH_SQRTF( dx * dx + dy * dy );
    float length_inv = 1.f / length;

    _direction->x = dx * length_inv;
    _direction->y = dy * length_inv;
}
//////////////////////////////////////////////////////////////////////////
static void __line_perpendicular( gp_vec2f_t * const _perpendicular, const gp_vec2f_t * _from, const gp_vec2f_t * _to )
{
    gp_vec2f_t direction;
    __line_direction( &direction, _from, _to );

    _perpendicular->x = -direction.y;
    _perpendicular->y = direction.x;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_reserve_points( const gp_line_t * _line, gp_cooked_point_t ** _points, gp_uint32_t _count, gp_uint32_t * _capacity, gp_uint32_t _required )
{
    if( _required <= *_capacity )
    {
        return GP_SUCCESSFUL;
    }

    gp_uint32_t capacity = *_capacity == 0 ? 16 : *_capacity;
    while( capacity < _required )
    {
        if( capacity > UINT32_MAX / 2U )
        {
            return GP_FAILURE;
        }
        capacity *= 2;
    }

    gp_cooked_point_t * points = (gp_cooked_point_t *)(*_line->realloc)(GP_NULLPTR, sizeof( gp_cooked_point_t ) * capacity, _line->ud);
    if( points == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    for( gp_uint32_t index = 0; index != _count; ++index )
    {
        points[index] = (*_points)[index];
    }
    if( *_points != GP_NULLPTR )
    {
        (*_line->free)(*_points, _line->ud);
    }

    *_points = points;
    *_capacity = capacity;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_push_point( const gp_line_t * _line, gp_cooked_path_t * _path, const gp_cooked_point_t * _point )
{
    if( _path->count != 0 && __line_same_point( &_path->points[_path->count - 1].position, &_point->position ) == GP_TRUE )
    {
        gp_bool_t join = _path->points[_path->count - 1].join == GP_TRUE || _point->join == GP_TRUE
            ? GP_TRUE
            : GP_FALSE;
        _path->points[_path->count - 1] = *_point;
        _path->points[_path->count - 1].join = join;
        if( _path->count == 1 )
        {
            _path->points[0].length = 0.f;
        }
        else
        {
            _path->points[_path->count - 1].length = _path->points[_path->count - 2].length +
                __line_distance( &_path->points[_path->count - 2].position, &_point->position );
        }

        _path->points[_path->count - 1].source_length = _path->points[_path->count - 1].length;

        return GP_SUCCESSFUL;
    }

    GP_CALL( __line_reserve_points, (_line, &_path->points, _path->count, &_path->capacity, _path->count + 1) );

    gp_cooked_point_t * point = _path->points + _path->count;
    *point = *_point;
    point->length = _path->count == 0
        ? 0.f
        : _path->points[_path->count - 1].length + __line_distance( &_path->points[_path->count - 1].position, &_point->position );
    point->source_length = point->length;
    ++_path->count;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static void __line_edge_tangent( gp_vec2f_t * const _tangent, const gp_line_point_t * _from, const gp_line_point_t * _to, const gp_line_edge_t * _edge, gp_bool_t _end )
{
    switch( _edge->controls )
    {
    case 0:
        _tangent->x = _to->p.x - _from->p.x;
        _tangent->y = _to->p.y - _from->p.y;
        break;
    case 1:
        if( _end == GP_TRUE )
        {
            _tangent->x = _to->p.x - _edge->p[0].x;
            _tangent->y = _to->p.y - _edge->p[0].y;
        }
        else
        {
            _tangent->x = _edge->p[0].x - _from->p.x;
            _tangent->y = _edge->p[0].y - _from->p.y;
        }
        break;
    case 2:
        if( _end == GP_TRUE )
        {
            _tangent->x = _to->p.x - _edge->p[1].x;
            _tangent->y = _to->p.y - _edge->p[1].y;
        }
        else
        {
            _tangent->x = _edge->p[0].x - _from->p.x;
            _tangent->y = _edge->p[0].y - _from->p.y;
        }
        break;
    case 3:
        {
            float angle = _end == GP_TRUE ? _edge->p[2].y : _edge->p[2].x;
            float angle_delta = _edge->p[2].y - _edge->p[2].x;
            _tangent->x = -GP_MATH_SINF( angle ) * _edge->p[1].x * angle_delta;
            _tangent->y = GP_MATH_COSF( angle ) * _edge->p[1].y * angle_delta;
        }break;
    default:
        _tangent->x = _to->p.x - _from->p.x;
        _tangent->y = _to->p.y - _from->p.y;
        break;
    }

    if( GP_EQUAL_F( _tangent->x, 0.00001f ) == GP_TRUE && GP_EQUAL_F( _tangent->y, 0.00001f ) == GP_TRUE )
    {
        _tangent->x = _to->p.x - _from->p.x;
        _tangent->y = _to->p.y - _from->p.y;
    }
}
//////////////////////////////////////////////////////////////////////////
static gp_bool_t __line_tangents_join( const gp_vec2f_t * _incoming, const gp_vec2f_t * _outgoing )
{
    float incoming_length2 = _incoming->x * _incoming->x + _incoming->y * _incoming->y;
    float outgoing_length2 = _outgoing->x * _outgoing->x + _outgoing->y * _outgoing->y;
    if( incoming_length2 <= 0.0000000001f || outgoing_length2 <= 0.0000000001f )
    {
        return GP_FALSE;
    }

    float cross = _incoming->x * _outgoing->y - _incoming->y * _outgoing->x;
    float dot = _incoming->x * _outgoing->x + _incoming->y * _outgoing->y;
    float angle = GP_MATH_ATAN2F( cross, dot );

    if( GP_EQUAL_F( angle, 0.00001f ) == GP_TRUE )
    {
        return GP_FALSE;
    }

    return GP_TRUE;
}
//////////////////////////////////////////////////////////////////////////
static gp_bool_t __line_point_join( const gp_line_t * _line, const gp_line_point_t * _point, const gp_line_edge_t * _outgoing_edge )
{
    gp_vec2f_t incoming;
    if( _outgoing_edge->prev != GP_NULLPTR )
    {
        __line_edge_tangent( &incoming, _point->prev, _point, _outgoing_edge->prev, GP_TRUE );
    }
    else
    {
        if( _line->closed == GP_FALSE )
        {
            return GP_FALSE;
        }

        const gp_line_point_t * last_point = GP_LIST_BACK( _line->points );
        if( __line_same_point( &_point->p, &last_point->p ) == GP_TRUE )
        {
            const gp_line_edge_t * last_edge = GP_LIST_BACK( _line->edges );
            __line_edge_tangent( &incoming, last_point->prev, last_point, last_edge, GP_TRUE );
        }
        else
        {
            incoming.x = _point->p.x - last_point->p.x;
            incoming.y = _point->p.y - last_point->p.y;
        }
    }

    gp_vec2f_t outgoing;
    __line_edge_tangent( &outgoing, _point, _point->next, _outgoing_edge, GP_FALSE );

    gp_bool_t join = __line_tangents_join( &incoming, &outgoing );
    return join;
}
//////////////////////////////////////////////////////////////////////////
static gp_bool_t __line_last_point_join( const gp_line_t * _line, const gp_line_point_t * _point )
{
    if( _line->closed == GP_FALSE )
    {
        return GP_FALSE;
    }

    const gp_line_edge_t * incoming_edge = GP_LIST_BACK( _line->edges );
    gp_vec2f_t incoming;
    __line_edge_tangent( &incoming, _point->prev, _point, incoming_edge, GP_TRUE );

    const gp_line_point_t * first_point = _line->points;
    if( __line_same_point( &_point->p, &first_point->p ) == GP_TRUE )
    {
        const gp_line_edge_t * outgoing_edge = _line->edges;
        gp_vec2f_t outgoing;
        __line_edge_tangent( &outgoing, first_point, first_point->next, outgoing_edge, GP_FALSE );

        gp_bool_t join = __line_tangents_join( &incoming, &outgoing );
        return join;
    }

    gp_vec2f_t outgoing;
    outgoing.x = first_point->p.x - _point->p.x;
    outgoing.y = first_point->p.y - _point->p.y;

    gp_bool_t join = __line_tangents_join( &incoming, &outgoing );
    return join;
}
//////////////////////////////////////////////////////////////////////////
static void __line_make_point( gp_cooked_point_t * const _point, const gp_line_point_t * _from, const gp_line_point_t * _to, const gp_line_edge_t * _edge, float _t )
{
    float inverse_t = 1.f - _t;
    switch( _edge->controls )
    {
    case 0:
        _point->position.x = __line_lerp( _from->p.x, _to->p.x, _t );
        _point->position.y = __line_lerp( _from->p.y, _to->p.y, _t );
        break;
    case 1:
        _point->position.x = inverse_t * inverse_t * _from->p.x + 2.f * inverse_t * _t * _edge->p[0].x + _t * _t * _to->p.x;
        _point->position.y = inverse_t * inverse_t * _from->p.y + 2.f * inverse_t * _t * _edge->p[0].y + _t * _t * _to->p.y;
        break;
    case 2:
        _point->position.x = inverse_t * inverse_t * inverse_t * _from->p.x + 3.f * inverse_t * inverse_t * _t * _edge->p[0].x +
            3.f * inverse_t * _t * _t * _edge->p[1].x + _t * _t * _t * _to->p.x;
        _point->position.y = inverse_t * inverse_t * inverse_t * _from->p.y + 3.f * inverse_t * inverse_t * _t * _edge->p[0].y +
            3.f * inverse_t * _t * _t * _edge->p[1].y + _t * _t * _t * _to->p.y;
        break;
    case 3:
        {
            float angle = __line_lerp( _edge->p[2].x, _edge->p[2].y, _t );
            _point->position.x = _edge->p[0].x + GP_MATH_COSF( angle ) * _edge->p[1].x;
            _point->position.y = _edge->p[0].y + GP_MATH_SINF( angle ) * _edge->p[1].y;
        }break;
    default:
        _point->position = _from->p;
        break;
    }

    _point->half_width = __line_lerp( _from->state->thickness, _to->state->thickness, _t ) * 0.5f;
    __line_lerp_color( &_point->color, &_from->state->color, &_to->state->color, _t );
    __line_lerp_color( &_point->outline_color, &_from->state->outline_color, &_to->state->outline_color, _t );
    _point->length = 0.f;
    _point->source_length = 0.f;
    _point->join = GP_FALSE;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_cook_path( const gp_line_t * _line, gp_cooked_path_t * const _path )
{
    _path->points = GP_NULLPTR;
    _path->count = 0;
    _path->capacity = 0;
    _path->closed = _line->closed;
    _path->source_offset = _line->state->path_length_offset;
    _path->source_total_length = 0.f;
    _path->source_ccw = GP_TRUE;

    if( _line->edges == GP_NULLPTR )
    {
        return GP_SUCCESSFUL;
    }

    const gp_line_point_t * from = _line->points;
    for( const gp_line_edge_t * edge = _line->edges; edge != GP_NULLPTR; edge = edge->next )
    {
#if defined(GP_DEBUG)
        if( edge->controls > 3 || from->next == GP_NULLPTR )
        {
            return GP_FAILURE;
        }
#endif

        const gp_line_point_t * to = from->next;
        gp_uint32_t sample_count;
        if( edge->controls == 0 )
        {
            sample_count = 1;
        }
        else if( edge->controls == 3 )
        {
            float angle_delta = edge->p[2].y - edge->p[2].x;
            sample_count = gp_ellipse_arc_segment_count( angle_delta, _line->state->ellipse_quality );
        }
        else
        {
            sample_count = _line->state->curve_quality;
        }
        for( gp_uint32_t index = 0; index != sample_count; ++index )
        {
            float t;
            if( edge->controls == 3 )
            {
                float angle_delta = edge->p[2].y - edge->p[2].x;
                t = gp_ellipse_arc_sample_t( angle_delta, _line->state->ellipse_quality, index );
            }
            else
            {
                t = (float)index / (float)sample_count;
            }

            gp_cooked_point_t point;
            __line_make_point( &point, from, to, edge, t );
            if( index == 0 )
            {
                point.join = __line_point_join( _line, from, edge );
            }
            if( edge->controls == 3 )
            {
                float angle = gp_ellipse_arc_sample_angle( edge->p[2].x, edge->p[2].y, _line->state->ellipse_quality, index );
                point.position.x = edge->p[0].x + GP_MATH_COSF( angle ) * edge->p[1].x;
                point.position.y = edge->p[0].y + GP_MATH_SINF( angle ) * edge->p[1].y;
            }
            GP_CALL( __line_push_point, (_line, _path, &point) );
        }

        from = to;
    }

    gp_cooked_point_t last;
    last.position = from->p;
    last.half_width = from->state->thickness * 0.5f;
    last.color = from->state->color;
    last.outline_color = from->state->outline_color;
    last.length = 0.f;
    last.source_length = 0.f;
    last.join = __line_last_point_join( _line, from );
    GP_CALL( __line_push_point, (_line, _path, &last) );

    if( _line->closed == GP_TRUE && (_line->state->flags & GP_STATE_FLAG_FILL) == 0 && _path->count >= 2 )
    {
        if( __line_same_point( &_path->points[0].position, &_path->points[_path->count - 1].position ) == GP_TRUE )
        {
            --_path->count;
        }

        gp_cooked_point_t closing = _path->points[0];
        GP_CALL( __line_push_point, (_line, _path, &closing) );
    }

    if( _path->count != 0 )
    {
        float local_length = _path->points[_path->count - 1].length;
        _path->source_total_length = _line->state->path_total_length > 0.f
            ? _line->state->path_total_length
            : local_length;
        for( gp_uint32_t index = 0; index != _path->count; ++index )
        {
            _path->points[index].source_length += _path->source_offset;
        }
        float area = 0.f;
        for( gp_uint32_t index = 0; index + 1 < _path->count; ++index )
        {
            const gp_vec2f_t * from = &_path->points[index].position;
            const gp_vec2f_t * to = &_path->points[index + 1].position;
            area += from->x * to->y - from->y * to->x;
        }
        _path->source_ccw = area >= 0.f ? GP_TRUE : GP_FALSE;
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static void __line_destroy_path( const gp_line_t * _line, gp_cooked_path_t * _path )
{
    if( _path->points != GP_NULLPTR )
    {
        (*_line->free)(_path->points, _line->ud);
    }
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_trim_push( const gp_line_t * _line, gp_cooked_path_t * _trimmed, const gp_cooked_point_t * _point, float _source_length )
{
    gp_cooked_point_t point = *_point;
    GP_CALL( __line_push_point, (_line, _trimmed, &point) );
    _trimmed->points[_trimmed->count - 1].source_length = _source_length;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static void __line_trim_sample( const gp_cooked_path_t * _path, float _distance, gp_cooked_point_t * const _point )
{
    if( _distance <= 0.f )
    {
        *_point = _path->points[0];
        return;
    }

    for( gp_uint32_t index = 0; index + 1 < _path->count; ++index )
    {
        const gp_cooked_point_t * from = _path->points + index;
        const gp_cooked_point_t * to = _path->points + index + 1;
        if( _distance > to->length + 0.00001f )
        {
            continue;
        }

        if( GP_EQUAL_F( _distance - from->length, 0.00001f ) == GP_TRUE )
        {
            *_point = *from;
        }
        else if( GP_EQUAL_F( _distance - to->length, 0.00001f ) == GP_TRUE )
        {
            *_point = *to;
        }
        else
        {
            float segment_length = to->length - from->length;
            __line_interpolate_cooked_point( _point, from, to, (_distance - from->length) / segment_length );
            _point->join = GP_FALSE;
        }
        return;
    }

    *_point = _path->points[_path->count - 1];
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_trim_append_range( const gp_line_t * _line, const gp_cooked_path_t * _path, gp_cooked_path_t * _trimmed, float _from, float _to, float _source_offset, gp_bool_t _include_from )
{
    if( _to < _from || GP_EQUAL_F( _to - _from, 0.00001f ) == GP_TRUE )
    {
        return GP_SUCCESSFUL;
    }

    if( _include_from == GP_TRUE )
    {
        gp_cooked_point_t boundary;
        __line_trim_sample( _path, _from, &boundary );
        GP_CALL( __line_trim_push, (_line, _trimmed, &boundary, _from + _source_offset) );
    }

    for( gp_uint32_t index = 0; index != _path->count; ++index )
    {
        const gp_cooked_point_t * point = _path->points + index;
        if( point->length <= _from + 0.00001f || point->length >= _to - 0.00001f )
        {
            continue;
        }

        GP_CALL( __line_trim_push, (_line, _trimmed, point, point->length + _source_offset) );
    }

    gp_cooked_point_t boundary;
    __line_trim_sample( _path, _to, &boundary );
    GP_CALL( __line_trim_push, (_line, _trimmed, &boundary, _to + _source_offset) );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_trim_path( const gp_line_t * _line, const gp_cooked_path_t * _path, gp_cooked_path_t * const _trimmed )
{
    _trimmed->points = GP_NULLPTR;
    _trimmed->count = 0;
    _trimmed->capacity = 0;
    _trimmed->closed = GP_FALSE;
    _trimmed->source_offset = _path->source_offset;
    _trimmed->source_total_length = _path->source_total_length;
    _trimmed->source_ccw = _path->source_ccw;

    if( _path->count < 2 || _path->source_total_length <= 0.f ||
        GP_EQUAL_F( _line->state->trim_from - _line->state->trim_to, 0.000001f ) == GP_TRUE )
    {
        return GP_SUCCESSFUL;
    }

    float local_length = _path->points[_path->count - 1].length;
    float from_absolute = _line->state->trim_from * _path->source_total_length;
    float to_absolute = _line->state->trim_to * _path->source_total_length;
    if( _path->closed == GP_TRUE && _line->state->trim_from > _line->state->trim_to )
    {
        GP_CALL( __line_trim_append_range, (_line, _path, _trimmed, from_absolute, local_length, _path->source_offset, GP_TRUE) );
        GP_CALL( __line_trim_append_range, (_line, _path, _trimmed, 0.f, to_absolute, _path->source_total_length, GP_FALSE) );
        return GP_SUCCESSFUL;
    }

    float from = from_absolute - _path->source_offset;
    float to = to_absolute - _path->source_offset;
    from = from < 0.f ? 0.f : from;
    to = to > local_length ? local_length : to;
    GP_CALL( __line_trim_append_range, (_line, _path, _trimmed, from, to, _path->source_offset, GP_TRUE) );
    if( _path->closed == GP_TRUE && GP_EQUAL_F( from, 0.000001f ) == GP_TRUE &&
        GP_EQUAL_F( to - local_length, 0.000001f ) == GP_TRUE )
    {
        _trimmed->closed = GP_TRUE;
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_span_push( const gp_line_t * _line, gp_cooked_span_t * _span, const gp_cooked_point_t * _point )
{
    if( _span->count != 0 && __line_same_point( &_span->points[_span->count - 1].position, &_point->position ) == GP_TRUE )
    {
        gp_bool_t join = _span->points[_span->count - 1].join == GP_TRUE || _point->join == GP_TRUE
            ? GP_TRUE
            : GP_FALSE;
        _span->points[_span->count - 1] = *_point;
        _span->points[_span->count - 1].join = join;
        return GP_SUCCESSFUL;
    }

    GP_CALL( __line_reserve_points, (_line, &_span->points, _span->count, &_span->capacity, _span->count + 1) );
    _span->points[_span->count] = *_point;
    _span->points[_span->count].length = _span->count == 0
        ? 0.f
        : _span->points[_span->count - 1].length + __line_distance( &_span->points[_span->count - 1].position, &_point->position );
    ++_span->count;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_new_span( const gp_line_t * _line, gp_cooked_stroke_t * _stroke, gp_cooked_span_t ** _span )
{
    if( _stroke->count == _stroke->capacity )
    {
        if( _stroke->capacity > UINT32_MAX / 2U )
        {
            return GP_FAILURE;
        }

        gp_uint32_t capacity = _stroke->capacity == 0 ? 8 : _stroke->capacity * 2;
        gp_cooked_span_t * spans = (gp_cooked_span_t *)(*_line->realloc)(GP_NULLPTR, sizeof( gp_cooked_span_t ) * capacity, _line->ud);
        if( spans == GP_NULLPTR )
        {
            return GP_FAILURE;
        }

        for( gp_uint32_t index = 0; index != _stroke->count; ++index )
        {
            spans[index] = _stroke->spans[index];
        }
        if( _stroke->spans != GP_NULLPTR )
        {
            (*_line->free)(_stroke->spans, _line->ud);
        }

        _stroke->spans = spans;
        _stroke->capacity = capacity;
    }

    gp_cooked_span_t * span = _stroke->spans + _stroke->count++;
    span->points = GP_NULLPTR;
    span->count = 0;
    span->capacity = 0;
    span->closed = GP_FALSE;
    span->source_total_length = 0.f;
    span->source_closed = GP_FALSE;
    span->source_ccw = GP_TRUE;

    *_span = span;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static void __line_interpolate_cooked_point( gp_cooked_point_t * const _point, const gp_cooked_point_t * _from, const gp_cooked_point_t * _to, float _t )
{
    _point->position.x = __line_lerp( _from->position.x, _to->position.x, _t );
    _point->position.y = __line_lerp( _from->position.y, _to->position.y, _t );
    _point->half_width = __line_lerp( _from->half_width, _to->half_width, _t );
    __line_lerp_color( &_point->color, &_from->color, &_to->color, _t );
    __line_lerp_color( &_point->outline_color, &_from->outline_color, &_to->outline_color, _t );
    _point->length = __line_lerp( _from->length, _to->length, _t );
    _point->source_length = __line_lerp( _from->source_length, _to->source_length, _t );
    if( GP_EQUAL_F( _t, 0.00001f ) == GP_TRUE )
    {
        _point->join = _from->join;
    }
    else if( GP_EQUAL_F( _t - 1.f, 0.00001f ) == GP_TRUE )
    {
        _point->join = _to->join;
    }
    else
    {
        _point->join = GP_FALSE;
    }
}
//////////////////////////////////////////////////////////////////////////
static void __line_recalculate_span_lengths( gp_cooked_span_t * _span )
{
    if( _span->count == 0 )
    {
        return;
    }

    _span->points[0].length = 0.f;
    for( gp_uint32_t index = 1; index != _span->count; ++index )
    {
        _span->points[index].length = _span->points[index - 1].length +
            __line_distance( &_span->points[index - 1].position, &_span->points[index].position );
    }
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_merge_closed_dash_seam( const gp_line_t * _line, gp_cooked_stroke_t * _stroke )
{
    if( _stroke->count < 2 )
    {
        return GP_SUCCESSFUL;
    }

    gp_cooked_span_t * first = _stroke->spans;
    gp_cooked_span_t * last = _stroke->spans + _stroke->count - 1;
    gp_uint32_t merged_count = last->count + first->count - 1;
    gp_cooked_point_t * merged = (gp_cooked_point_t *)(*_line->realloc)(GP_NULLPTR, sizeof( gp_cooked_point_t ) * merged_count, _line->ud);
    if( merged == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    for( gp_uint32_t index = 0; index != last->count; ++index )
    {
        merged[index] = last->points[index];
    }
    for( gp_uint32_t index = 1; index != first->count; ++index )
    {
        merged[last->count + index - 1] = first->points[index];
    }

    (*_line->free)(first->points, _line->ud);
    (*_line->free)(last->points, _line->ud);
    first->points = merged;
    first->count = merged_count;
    first->capacity = merged_count;
    first->closed = GP_FALSE;
    first->source_total_length = last->source_total_length;
    first->source_closed = last->source_closed;
    first->source_ccw = last->source_ccw;
    __line_recalculate_span_lengths( first );

    --_stroke->count;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_cook_stroke( const gp_line_t * _line, const gp_cooked_path_t * _path, gp_cooked_stroke_t * const _stroke )
{
    _stroke->spans = GP_NULLPTR;
    _stroke->count = 0;
    _stroke->capacity = 0;

    if( _path->count < 2 )
    {
        return GP_SUCCESSFUL;
    }

    gp_bool_t source_ccw = _path->source_ccw;

    const gp_state_t * state = _line->state;
    if( state->dash_count == 0 )
    {
        gp_cooked_span_t * span;
        GP_CALL( __line_new_span, (_line, _stroke, &span) );
        for( gp_uint32_t index = 0; index != _path->count; ++index )
        {
            GP_CALL( __line_span_push, (_line, span, _path->points + index) );
        }
        span->closed = _path->closed;
        span->source_total_length = _path->source_total_length;
        span->source_closed = _line->closed;
        span->source_ccw = source_ccw;

        return GP_SUCCESSFUL;
    }

    gp_uint32_t pattern_count = state->dash_count % 2 == 0 ? state->dash_count : state->dash_count * 2;
    float pattern_length = 0.f;
    for( gp_uint32_t index = 0; index != pattern_count; ++index )
    {
        pattern_length += state->dash_pattern[index % state->dash_count];
    }

    float phase = GP_MATH_FMODF( state->dash_offset + _path->points[0].source_length, pattern_length );
    if( phase < 0.f )
    {
        phase += pattern_length;
    }

    gp_uint32_t pattern_index = 0;
    while( phase >= state->dash_pattern[pattern_index % state->dash_count] )
    {
        phase -= state->dash_pattern[pattern_index % state->dash_count];
        pattern_index = (pattern_index + 1) % pattern_count;
    }

    float pattern_remaining = state->dash_pattern[pattern_index % state->dash_count] - phase;
    gp_bool_t drawing = pattern_index % 2 == 0 ? GP_TRUE : GP_FALSE;
    gp_cooked_span_t * active_span = GP_NULLPTR;

    for( gp_uint32_t segment = 0; segment + 1 < _path->count; ++segment )
    {
        const gp_cooked_point_t * from = _path->points + segment;
        const gp_cooked_point_t * to = _path->points + segment + 1;
        float segment_length = __line_distance( &from->position, &to->position );
        if( segment_length <= 0.00001f )
        {
            continue;
        }

        float consumed = 0.f;
        while( consumed < segment_length - 0.00001f )
        {
            float available = segment_length - consumed;
            float step = pattern_remaining < available ? pattern_remaining : available;

            gp_cooked_point_t piece_from;
            gp_cooked_point_t piece_to;
            __line_interpolate_cooked_point( &piece_from, from, to, consumed / segment_length );
            __line_interpolate_cooked_point( &piece_to, from, to, (consumed + step) / segment_length );

            if( drawing == GP_TRUE )
            {
                if( active_span == GP_NULLPTR )
                {
                    GP_CALL( __line_new_span, (_line, _stroke, &active_span) );
                    active_span->source_total_length = _path->source_total_length;
                    active_span->source_closed = _line->closed;
                    active_span->source_ccw = source_ccw;
                    GP_CALL( __line_span_push, (_line, active_span, &piece_from) );
                }
                GP_CALL( __line_span_push, (_line, active_span, &piece_to) );
            }

            consumed += step;
            pattern_remaining -= step;
            if( pattern_remaining <= 0.00001f )
            {
                pattern_index = (pattern_index + 1) % pattern_count;
                pattern_remaining = state->dash_pattern[pattern_index % state->dash_count];
                drawing = pattern_index % 2 == 0 ? GP_TRUE : GP_FALSE;
                if( drawing == GP_FALSE )
                {
                    active_span = GP_NULLPTR;
                }
            }
        }
    }

    if( _path->closed == GP_TRUE && _stroke->count != 0 )
    {
        gp_cooked_span_t * first = _stroke->spans;
        gp_cooked_span_t * last = _stroke->spans + _stroke->count - 1;
        gp_bool_t first_at_seam = __line_same_point( &first->points[0].position, &_path->points[0].position );
        gp_bool_t last_at_seam = __line_same_point( &last->points[last->count - 1].position, &_path->points[_path->count - 1].position );

        if( _stroke->count == 1 && first_at_seam == GP_TRUE && last_at_seam == GP_TRUE )
        {
            first->closed = GP_TRUE;
        }
        else if( _stroke->count > 1 && first_at_seam == GP_TRUE && last_at_seam == GP_TRUE )
        {
            GP_CALL( __line_merge_closed_dash_seam, (_line, _stroke) );
        }
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static void __line_destroy_stroke( const gp_line_t * _line, gp_cooked_stroke_t * _stroke )
{
    for( gp_uint32_t index = 0; index != _stroke->count; ++index )
    {
        if( _stroke->spans[index].points != GP_NULLPTR )
        {
            (*_line->free)(_stroke->spans[index].points, _line->ud);
        }
    }

    if( _stroke->spans != GP_NULLPTR )
    {
        (*_line->free)(_stroke->spans, _line->ud);
    }
}
//////////////////////////////////////////////////////////////////////////
static gp_uint32_t __line_round_sector_segment_count( const gp_state_t * _state, float _angle_delta )
{
    float angle_delta_abs = _angle_delta < 0.f ? -_angle_delta : _angle_delta;
    float segment_factor = angle_delta_abs / gp_constant_two_pi * (float)((gp_uint32_t)_state->rect_quality * 4);
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
static void __line_make_cap_sector( const gp_cooked_span_t * _span, gp_bool_t _start, const gp_state_t * _state, gp_round_sector_t * const _sector )
{
    gp_vec2f_t direction;
    if( _start == GP_TRUE )
    {
        __line_direction( &direction, &_span->points[0].position, &_span->points[1].position );
        _sector->angle_from = GP_MATH_ATAN2F( direction.y, direction.x ) - gp_constant_pi * 0.5f;
    }
    else
    {
        gp_uint32_t last = _span->count - 1;
        __line_direction( &direction, &_span->points[last - 1].position, &_span->points[last].position );
        _sector->angle_from = GP_MATH_ATAN2F( direction.y, direction.x ) + gp_constant_pi * 0.5f;
    }

    _sector->angle_delta = -gp_constant_pi;
    _sector->segment_count = __line_round_sector_segment_count( _state, _sector->angle_delta );
}
//////////////////////////////////////////////////////////////////////////
static gp_bool_t __line_make_join_sector( const gp_cooked_span_t * _span, gp_uint32_t _index, const gp_state_t * _state, gp_round_sector_t * const _sector )
{
    if( _span->points[_index].join == GP_FALSE )
    {
        return GP_FALSE;
    }

    gp_uint32_t unique_count = _span->closed == GP_TRUE ? _span->count - 1 : _span->count;
    gp_uint32_t previous_index = (_index + unique_count - 1) % unique_count;
    gp_uint32_t next_index = (_index + 1) % unique_count;
    const gp_vec2f_t * previous = &_span->points[previous_index].position;
    const gp_vec2f_t * point = &_span->points[_index].position;
    const gp_vec2f_t * next = &_span->points[next_index].position;
    gp_vec2f_t incoming;
    gp_vec2f_t outgoing;
    __line_direction( &incoming, previous, point );
    __line_direction( &outgoing, point, next );

    float cross = incoming.x * outgoing.y - incoming.y * outgoing.x;
    float dot = incoming.x * outgoing.x + incoming.y * outgoing.y;
    float angle_delta = GP_MATH_ATAN2F( cross, dot );
    if( GP_EQUAL_F( angle_delta, 0.00001f ) == GP_TRUE )
    {
        return GP_FALSE;
    }

    gp_vec2f_t incoming_perpendicular;
    incoming_perpendicular.x = -incoming.y;
    incoming_perpendicular.y = incoming.x;
    float side = angle_delta > 0.f ? -1.f : 1.f;
    _sector->angle_from = GP_MATH_ATAN2F( incoming_perpendicular.y * side, incoming_perpendicular.x * side );
    _sector->angle_delta = angle_delta;
    _sector->segment_count = __line_round_sector_segment_count( _state, angle_delta );

    return GP_TRUE;
}
//////////////////////////////////////////////////////////////////////////
static void __line_calculate_round_sector_size( const gp_state_t * _state, const gp_round_sector_t * _sector, gp_uint32_t * _vertex_count, gp_uint32_t * _index_count )
{
    if( _state->penumbra > 0.f )
    {
        *_vertex_count += _sector->segment_count * 2 + 3;
        *_index_count += _sector->segment_count * 9;
    }
    else
    {
        gp_uint32_t layers = _state->outline_width > 0.f ? 2 : 1;
        *_vertex_count += layers * (_sector->segment_count + 2);
        *_index_count += layers * _sector->segment_count * 3;
    }
}
//////////////////////////////////////////////////////////////////////////
static gp_uint32_t __line_point_section_count( const gp_cooked_span_t * _span, gp_uint32_t _index, const gp_state_t * _state )
{
    if( _state->line_join != GP_LINE_JOIN_ROUND || _index == 0 || _index + 1 == _span->count )
    {
        return 1;
    }

    gp_round_sector_t sector;
    if( __line_make_join_sector( _span, _index, _state, &sector ) == GP_TRUE )
    {
        return 2;
    }

    return 1;
}
//////////////////////////////////////////////////////////////////////////
static gp_uint32_t __line_body_section_count( const gp_cooked_span_t * _span, const gp_state_t * _state )
{
    gp_uint32_t section_count = 0;
    for( gp_uint32_t index = 0; index != _span->count; ++index )
    {
        section_count += __line_point_section_count( _span, index, _state );
    }

    return section_count;
}
//////////////////////////////////////////////////////////////////////////
static void __line_calculate_span_size( const gp_cooked_span_t * _span, const gp_state_t * _state, gp_uint32_t * _vertex_count, gp_uint32_t * _index_count )
{
    if( _span->count < 2 )
    {
        return;
    }

    gp_uint32_t section_count = __line_body_section_count( _span, _state );

    if( _state->penumbra > 0.f )
    {
        *_vertex_count += section_count * 4;
        *_index_count += (_span->count - 1) * 18;
    }
    else if( _state->outline_width > 0.f )
    {
        *_vertex_count += section_count * 6;
        *_index_count += (_span->count - 1) * 18;
    }
    else
    {
        *_vertex_count += section_count * 2;
        *_index_count += (_span->count - 1) * 6;
    }

    if( _span->closed == GP_FALSE && _state->line_cap == GP_LINE_CAP_ROUND )
    {
        gp_round_sector_t start_sector;
        gp_round_sector_t end_sector;
        __line_make_cap_sector( _span, GP_TRUE, _state, &start_sector );
        __line_make_cap_sector( _span, GP_FALSE, _state, &end_sector );
        __line_calculate_round_sector_size( _state, &start_sector, _vertex_count, _index_count );
        __line_calculate_round_sector_size( _state, &end_sector, _vertex_count, _index_count );
    }

    if( _state->line_join == GP_LINE_JOIN_ROUND )
    {
        gp_uint32_t begin = _span->closed == GP_TRUE ? 0 : 1;
        gp_uint32_t end = _span->count - 1;
        for( gp_uint32_t index = begin; index < end; ++index )
        {
            gp_round_sector_t sector;
            if( __line_make_join_sector( _span, index, _state, &sector ) == GP_TRUE )
            {
                __line_calculate_round_sector_size( _state, &sector, _vertex_count, _index_count );
            }
        }
    }
}
//////////////////////////////////////////////////////////////////////////
static void __line_calculate_marker_size( const gp_path_marker_t * _marker, const gp_state_t * _state, gp_uint32_t * _vertex_count, gp_uint32_t * _index_count )
{
    gp_uint32_t point_count;
    gp_uint32_t shape_count = 1;
    switch( _marker->type )
    {
    case GP_PATH_MARKER_NONE:
        return;
    case GP_PATH_MARKER_FILLED_ARROW:
    case GP_PATH_MARKER_TRIANGLE:
        point_count = 3;
        break;
    case GP_PATH_MARKER_OPEN_ARROW:
        point_count = 4;
        shape_count = 2;
        break;
    case GP_PATH_MARKER_CIRCLE:
        point_count = (gp_uint32_t)_state->rect_quality;
        break;
    case GP_PATH_MARKER_DIAMOND:
        point_count = 4;
        break;
    default:
        return;
    }

    if( _state->penumbra > 0.f || _state->outline_width > 0.f )
    {
        *_vertex_count += shape_count * point_count * 2;
        *_index_count += shape_count * (point_count * 6 + (point_count - 2) * 3);
    }
    else
    {
        *_vertex_count += shape_count * point_count;
        *_index_count += shape_count * (point_count - 2) * 3;
    }
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_calculate_size( const gp_line_t * _line, gp_uint32_t * _vertex_count, gp_uint32_t * _index_count )
{
    if( _line->edges == GP_NULLPTR || _line->compound_fill != GP_NULLPTR )
    {
        return GP_SUCCESSFUL;
    }

    gp_cooked_path_t path;
    GP_CALL( __line_cook_path, (_line, &path) );

    if( (_line->state->flags & GP_STATE_FLAG_FILL) != 0 )
    {
        if( path.count >= 3 )
        {
            *_vertex_count += path.count;
            *_index_count += (path.count - 2) * 3;
        }

        __line_destroy_path( _line, &path );
        return GP_SUCCESSFUL;
    }

    gp_cooked_path_t trimmed;
    GP_CALL( __line_trim_path, (_line, &path, &trimmed) );
    gp_cooked_stroke_t stroke;
    GP_CALL( __line_cook_stroke, (_line, &trimmed, &stroke) );
    for( gp_uint32_t index = 0; index != stroke.count; ++index )
    {
        __line_calculate_span_size( stroke.spans + index, _line->state, _vertex_count, _index_count );
    }

    if( trimmed.count >= 2 && trimmed.closed == GP_FALSE )
    {
        __line_calculate_marker_size( &_line->state->start_marker, _line->state, _vertex_count, _index_count );
        __line_calculate_marker_size( &_line->state->end_marker, _line->state, _vertex_count, _index_count );
    }

    __line_destroy_stroke( _line, &stroke );
    __line_destroy_path( _line, &trimmed );
    __line_destroy_path( _line, &path );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_calculate_mesh_line_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh )
{
    gp_uint32_t vertex_count = 0;
    gp_uint32_t index_count = 0;

    for( const gp_line_t * line = _canvas->lines; line != GP_NULLPTR; line = line->next )
    {
        GP_CALL( __line_calculate_size, (line, &vertex_count, &index_count) );
    }

    _mesh->vertex_count += vertex_count;
    _mesh->index_count += index_count;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static float __line_triangle_area( const gp_vec2f_t * _a, const gp_vec2f_t * _b, const gp_vec2f_t * _c )
{
    float area = (_b->x - _a->x) * (_c->y - _a->y) - (_b->y - _a->y) * (_c->x - _a->x);
    return area;
}
//////////////////////////////////////////////////////////////////////////
static gp_bool_t __line_point_in_triangle( const gp_vec2f_t * _point, const gp_vec2f_t * _a, const gp_vec2f_t * _b, const gp_vec2f_t * _c )
{
    float c0 = __line_triangle_area( _a, _b, _point );
    float c1 = __line_triangle_area( _b, _c, _point );
    float c2 = __line_triangle_area( _c, _a, _point );
    gp_bool_t negative = c0 < -0.00001f || c1 < -0.00001f || c2 < -0.00001f ? GP_TRUE : GP_FALSE;
    gp_bool_t positive = c0 > 0.00001f || c1 > 0.00001f || c2 > 0.00001f ? GP_TRUE : GP_FALSE;

    if( negative == GP_TRUE && positive == GP_TRUE )
    {
        return GP_FALSE;
    }

    return GP_TRUE;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_render_fill_indices( const gp_line_t * _line, const gp_cooked_path_t * _path, const gp_mesh_t * _mesh, gp_uint32_t _vertex_begin, gp_uint32_t * _index_iterator )
{
    gp_uint32_t * order = (gp_uint32_t *)(*_line->realloc)(GP_NULLPTR, sizeof( gp_uint32_t ) * _path->count, _line->ud);
    if( order == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    float area = 0.f;
    for( gp_uint32_t index = 0; index != _path->count; ++index )
    {
        order[index] = index;
        const gp_vec2f_t * p0 = &_path->points[index].position;
        const gp_vec2f_t * p1 = &_path->points[(index + 1) % _path->count].position;
        area += p0->x * p1->y - p1->x * p0->y;
    }

    gp_bool_t ccw = area >= 0.f ? GP_TRUE : GP_FALSE;
    gp_uint32_t remaining = _path->count;
    while( remaining > 2 )
    {
        gp_bool_t clipped = GP_FALSE;
        for( gp_uint32_t index = 0; index != remaining; ++index )
        {
            gp_uint32_t i0 = order[(index + remaining - 1) % remaining];
            gp_uint32_t i1 = order[index];
            gp_uint32_t i2 = order[(index + 1) % remaining];
            const gp_vec2f_t * p0 = &_path->points[i0].position;
            const gp_vec2f_t * p1 = &_path->points[i1].position;
            const gp_vec2f_t * p2 = &_path->points[i2].position;
            float triangle_area = __line_triangle_area( p0, p1, p2 );
            if( (ccw == GP_TRUE && triangle_area <= 0.00001f) || (ccw == GP_FALSE && triangle_area >= -0.00001f) )
            {
                continue;
            }

            gp_bool_t contains = GP_FALSE;
            for( gp_uint32_t test = 0; test != remaining; ++test )
            {
                gp_uint32_t candidate = order[test];
                if( candidate == i0 || candidate == i1 || candidate == i2 )
                {
                    continue;
                }
                if( __line_point_in_triangle( &_path->points[candidate].position, p0, p1, p2 ) == GP_TRUE )
                {
                    contains = GP_TRUE;
                    break;
                }
            }
            if( contains == GP_TRUE )
            {
                continue;
            }

            GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 0, _vertex_begin + i0) );
            GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 1, _vertex_begin + (ccw == GP_TRUE ? i1 : i2)) );
            GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 2, _vertex_begin + (ccw == GP_TRUE ? i2 : i1)) );
            *_index_iterator += 3;

            for( gp_uint32_t move = index; move + 1 < remaining; ++move )
            {
                order[move] = order[move + 1];
            }
            --remaining;
            clipped = GP_TRUE;
            break;
        }

        if( clipped == GP_FALSE )
        {
            for( gp_uint32_t index = 1; index + 1 < remaining; ++index )
            {
                GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 0, _vertex_begin + order[0]) );
                GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 1, _vertex_begin + order[index]) );
                GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 2, _vertex_begin + order[index + 1]) );
                *_index_iterator += 3;
            }
            break;
        }
    }

    (*_line->free)(order, _line->ud);

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_render_fill( const gp_line_t * _line, const gp_cooked_path_t * _path, const gp_state_t * _state, const gp_mesh_t * _mesh, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator )
{
    if( _path->count < 3 )
    {
        return GP_SUCCESSFUL;
    }

    gp_uint32_t vertex_begin = *_vertex_iterator;
    float minimum_x = _path->points[0].position.x;
    float minimum_y = _path->points[0].position.y;
    float maximum_x = minimum_x;
    float maximum_y = minimum_y;
    for( gp_uint32_t index = 1; index != _path->count; ++index )
    {
        const gp_vec2f_t * position = &_path->points[index].position;
        minimum_x = position->x < minimum_x ? position->x : minimum_x;
        minimum_y = position->y < minimum_y ? position->y : minimum_y;
        maximum_x = position->x > maximum_x ? position->x : maximum_x;
        maximum_y = position->y > maximum_y ? position->y : maximum_y;
    }
    float width = maximum_x - minimum_x;
    float height = maximum_y - minimum_y;
    for( gp_uint32_t index = 0; index != _path->count; ++index )
    {
        const gp_cooked_point_t * point = _path->points + index;
        gp_color_t total_color;
        gp_color_mul( &total_color, &_mesh->color, &point->color );
        GP_CALL( gp_mesh_push_position, (_mesh, vertex_begin + index, point->position.x, point->position.y) );
        GP_CALL( gp_mesh_push_color, (_mesh, vertex_begin + index, gp_color_argb( &total_color )) );
        float u = width > 0.f ? (point->position.x - minimum_x) / width : 0.f;
        float v = height > 0.f ? (point->position.y - minimum_y) / height : 0.f;
        GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex_begin + index, u, v) );
    }

    GP_CALL( __line_render_fill_indices, (_line, _path, _mesh, vertex_begin, _index_iterator) );
    *_vertex_iterator += _path->count;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static float __line_point_width( const gp_cooked_point_t * _point, float _addition, float _subtraction )
{
    float width = _point->half_width + _addition - _subtraction;

    if( width < 0.f )
    {
        return 0.f;
    }

    return width;
}
//////////////////////////////////////////////////////////////////////////
static void __line_make_offset_line( gp_linef_t * const _line, const gp_vec2f_t * _from, const gp_vec2f_t * _to, const gp_vec2f_t * _perpendicular, float _from_width, float _to_width, float _side )
{
    gp_vec2f_t a;
    a.x = _from->x + _perpendicular->x * _from_width * _side;
    a.y = _from->y + _perpendicular->y * _from_width * _side;
    gp_vec2f_t b;
    b.x = _to->x + _perpendicular->x * _to_width * _side;
    b.y = _to->y + _perpendicular->y * _to_width * _side;

    _line->a = b.y - a.y;
    _line->b = a.x - b.x;
    _line->c = _line->a * a.x + _line->b * a.y;
}
//////////////////////////////////////////////////////////////////////////
static gp_bool_t __line_intersect( const gp_linef_t * _a, const gp_linef_t * _b, gp_vec2f_t * const _point )
{
    float determinant = _a->a * _b->b - _b->a * _a->b;
    if( GP_EQUAL_F( determinant, 0.00001f ) == GP_TRUE )
    {
        return GP_FALSE;
    }

    float determinant_inv = 1.f / determinant;
    _point->x = (_a->c * _b->b - _b->c * _a->b) * determinant_inv;
    _point->y = (_a->a * _b->c - _b->a * _a->c) * determinant_inv;

    return GP_TRUE;
}
//////////////////////////////////////////////////////////////////////////
static void __line_limit_join( gp_vec2f_t * _join, const gp_vec2f_t * _center, const gp_vec2f_t * _perpendicular, float _width, float _side, const gp_state_t * _state )
{
    float dx = _join->x - _center->x;
    float dy = _join->y - _center->y;
    float distance = GP_MATH_SQRTF( dx * dx + dy * dy );
    if( _state->line_join != GP_LINE_JOIN_MITER || distance > _width * _state->miter_limit )
    {
        _join->x = _center->x + _perpendicular->x * _width * _side;
        _join->y = _center->y + _perpendicular->y * _width * _side;
    }
}
//////////////////////////////////////////////////////////////////////////
static void __line_cross_section( const gp_cooked_span_t * _span, gp_uint32_t _index, float _addition, float _subtraction, const gp_state_t * _state, gp_vec2f_t * const _left, gp_vec2f_t * const _right )
{
    gp_uint32_t last = _span->count - 1;
    gp_uint32_t index = _span->closed == GP_TRUE && _index == last ? 0 : _index;
    const gp_cooked_point_t * point = _span->points + index;
    float width = __line_point_width( point, _addition, _subtraction );

    if( _span->closed == GP_FALSE && (index == 0 || index == last) )
    {
        gp_uint32_t other = index == 0 ? 1 : last - 1;
        gp_vec2f_t perpendicular;
        if( index == 0 )
        {
            __line_perpendicular( &perpendicular, &point->position, &_span->points[other].position );
        }
        else
        {
            __line_perpendicular( &perpendicular, &_span->points[other].position, &point->position );
        }

        _left->x = point->position.x - perpendicular.x * width;
        _left->y = point->position.y - perpendicular.y * width;
        _right->x = point->position.x + perpendicular.x * width;
        _right->y = point->position.y + perpendicular.y * width;
        return;
    }

    gp_uint32_t unique_count = _span->closed == GP_TRUE ? last : _span->count;
    gp_uint32_t previous_index = (index + unique_count - 1) % unique_count;
    gp_uint32_t next_index = (index + 1) % unique_count;
    const gp_cooked_point_t * previous = _span->points + previous_index;
    const gp_cooked_point_t * next = _span->points + next_index;
    float previous_width = __line_point_width( previous, _addition, _subtraction );
    float next_width = __line_point_width( next, _addition, _subtraction );

    gp_vec2f_t incoming_perpendicular;
    gp_vec2f_t outgoing_perpendicular;
    __line_perpendicular( &incoming_perpendicular, &previous->position, &point->position );
    __line_perpendicular( &outgoing_perpendicular, &point->position, &next->position );

    gp_linef_t incoming_left;
    gp_linef_t outgoing_left;
    gp_linef_t incoming_right;
    gp_linef_t outgoing_right;
    __line_make_offset_line( &incoming_left, &previous->position, &point->position, &incoming_perpendicular, previous_width, width, -1.f );
    __line_make_offset_line( &outgoing_left, &point->position, &next->position, &outgoing_perpendicular, width, next_width, -1.f );
    __line_make_offset_line( &incoming_right, &previous->position, &point->position, &incoming_perpendicular, previous_width, width, 1.f );
    __line_make_offset_line( &outgoing_right, &point->position, &next->position, &outgoing_perpendicular, width, next_width, 1.f );

    if( __line_intersect( &incoming_left, &outgoing_left, _left ) == GP_FALSE )
    {
        _left->x = point->position.x - incoming_perpendicular.x * width;
        _left->y = point->position.y - incoming_perpendicular.y * width;
    }
    if( __line_intersect( &incoming_right, &outgoing_right, _right ) == GP_FALSE )
    {
        _right->x = point->position.x + incoming_perpendicular.x * width;
        _right->y = point->position.y + incoming_perpendicular.y * width;
    }

    __line_limit_join( _left, &point->position, &incoming_perpendicular, width, -1.f, _state );
    __line_limit_join( _right, &point->position, &incoming_perpendicular, width, 1.f, _state );
}
//////////////////////////////////////////////////////////////////////////
static void __line_join_inner( const gp_cooked_span_t * _span, gp_uint32_t _index, float _addition, float _subtraction, float _side, const gp_state_t * _state, gp_vec2f_t * const _inner )
{
    gp_uint32_t unique_count = _span->closed == GP_TRUE ? _span->count - 1 : _span->count;
    gp_uint32_t previous_index = (_index + unique_count - 1) % unique_count;
    gp_uint32_t next_index = (_index + 1) % unique_count;
    const gp_cooked_point_t * previous = _span->points + previous_index;
    const gp_cooked_point_t * point = _span->points + _index;
    const gp_cooked_point_t * next = _span->points + next_index;
    float previous_width = __line_point_width( previous, _addition, _subtraction );
    float width = __line_point_width( point, _addition, _subtraction );
    float next_width = __line_point_width( next, _addition, _subtraction );
    gp_vec2f_t incoming_perpendicular;
    gp_vec2f_t outgoing_perpendicular;
    __line_perpendicular( &incoming_perpendicular, &previous->position, &point->position );
    __line_perpendicular( &outgoing_perpendicular, &point->position, &next->position );
    gp_linef_t incoming_line;
    gp_linef_t outgoing_line;
    __line_make_offset_line( &incoming_line, &previous->position, &point->position, &incoming_perpendicular, previous_width, width, _side );
    __line_make_offset_line( &outgoing_line, &point->position, &next->position, &outgoing_perpendicular, width, next_width, _side );

    if( __line_intersect( &incoming_line, &outgoing_line, _inner ) == GP_FALSE )
    {
        _inner->x = point->position.x + incoming_perpendicular.x * width * _side;
        _inner->y = point->position.y + incoming_perpendicular.y * width * _side;
        return;
    }

    float dx = _inner->x - point->position.x;
    float dy = _inner->y - point->position.y;
    float distance = GP_MATH_SQRTF( dx * dx + dy * dy );
    if( distance > width * _state->miter_limit )
    {
        _inner->x = point->position.x + incoming_perpendicular.x * width * _side;
        _inner->y = point->position.y + incoming_perpendicular.y * width * _side;
    }
}
//////////////////////////////////////////////////////////////////////////
static void __line_smooth_cross_section( const gp_cooked_span_t * _span, gp_uint32_t _point_index, float _addition, float _subtraction, const gp_state_t * _state, gp_vec2f_t * const _left, gp_vec2f_t * const _right )
{
    gp_uint32_t last = _span->count - 1;
    if( _span->closed == GP_FALSE && (_point_index == 0 || _point_index == last) )
    {
        __line_cross_section( _span, _point_index, _addition, _subtraction, _state, _left, _right );
        return;
    }

    gp_uint32_t point_index = _span->closed == GP_TRUE && _point_index == last ? 0 : _point_index;
    __line_join_inner( _span, point_index, _addition, _subtraction, -1.f, _state, _left );
    __line_join_inner( _span, point_index, _addition, _subtraction, 1.f, _state, _right );
}
//////////////////////////////////////////////////////////////////////////
static void __line_segment_cross_section( const gp_cooked_span_t * _span, gp_uint32_t _segment_index, gp_uint32_t _point_index, float _addition, float _subtraction, const gp_state_t * _state, gp_vec2f_t * const _left, gp_vec2f_t * const _right )
{
    const gp_cooked_point_t * segment_from = _span->points + _segment_index;
    const gp_cooked_point_t * segment_to = segment_from + 1;
    const gp_cooked_point_t * point = _span->points + _point_index;
    float width = __line_point_width( point, _addition, _subtraction );
    gp_vec2f_t perpendicular;
    __line_perpendicular( &perpendicular, &segment_from->position, &segment_to->position );
    _left->x = point->position.x - perpendicular.x * width;
    _left->y = point->position.y - perpendicular.y * width;
    _right->x = point->position.x + perpendicular.x * width;
    _right->y = point->position.y + perpendicular.y * width;

    gp_uint32_t last = _span->count - 1;
    if( _span->closed == GP_FALSE && (_point_index == 0 || _point_index == last) )
    {
        return;
    }

    gp_uint32_t join_index = _span->closed == GP_TRUE && _point_index == last ? 0 : _point_index;
    gp_round_sector_t sector;
    if( __line_make_join_sector( _span, join_index, _state, &sector ) == GP_FALSE )
    {
        return;
    }

    if( sector.angle_delta > 0.f )
    {
        __line_join_inner( _span, join_index, _addition, _subtraction, 1.f, _state, _right );
    }
    else
    {
        __line_join_inner( _span, join_index, _addition, _subtraction, -1.f, _state, _left );
    }
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_push_quad( const gp_mesh_t * _mesh, gp_uint32_t * _index_iterator, gp_uint32_t _a, gp_uint32_t _b, gp_uint32_t _c, gp_uint32_t _d )
{
    GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 0, _a) );
    GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 1, _b) );
    GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 2, _c) );
    GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 3, _c) );
    GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 4, _b) );
    GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 5, _d) );
    *_index_iterator += 6;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static void __line_round_sector_uv( const gp_cooked_point_t * _point, const gp_vec2f_t * _cap_tangent,
    float _source_total_length, float _angle_delta, const gp_state_t * _state, float _x, float _y,
    float _fallback_u, float _fallback_v, float * _u, float * _v )
{
    if( _state->uv_mode != GP_UV_MODE_PATH || _source_total_length <= 0.f )
    {
        *_u = _fallback_u;
        *_v = _fallback_v;
        return;
    }

    float source_length = _point->source_length;
    if( source_length > _source_total_length )
    {
        source_length = GP_MATH_FMODF( source_length, _source_total_length );
    }

    if( _cap_tangent == GP_NULLPTR )
    {
        *_u = source_length / _source_total_length;
        float offset_x = _x - _point->position.x;
        float offset_y = _y - _point->position.y;
        float radius = GP_MATH_SQRTF( offset_x * offset_x + offset_y * offset_y );
        float side = _angle_delta > 0.f ? 1.f : -1.f;
        *_v = _point->half_width > 0.f
            ? 0.5f + side * radius / (_point->half_width * 2.f)
            : 0.5f;
        return;
    }

    float offset_x = _x - _point->position.x;
    float offset_y = _y - _point->position.y;
    float normal_x = -_cap_tangent->y;
    float normal_y = _cap_tangent->x;
    *_u = (source_length + offset_x * _cap_tangent->x + offset_y * _cap_tangent->y) /
        _source_total_length;
    *_v = _point->half_width > 0.f
        ? 0.5f + (offset_x * normal_x + offset_y * normal_y) / (_point->half_width * 2.f)
        : 0.5f;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_render_solid_sector( const gp_mesh_t * _mesh, const gp_state_t * _state,
    const gp_cooked_point_t * _point, const gp_vec2f_t * _cap_tangent, float _source_total_length, float _radius,
    const gp_round_sector_t * _sector, gp_argb_t _color,
    gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator )
{
    gp_uint32_t vertex_begin = *_vertex_iterator;
    float center_u;
    float center_v;
    __line_round_sector_uv( _point, _cap_tangent, _source_total_length, _sector->angle_delta, _state,
        _point->position.x, _point->position.y,
        0.5f, 0.5f, &center_u, &center_v );
    GP_CALL( gp_mesh_push_position, (_mesh, vertex_begin, _point->position.x, _point->position.y) );
    GP_CALL( gp_mesh_push_color, (_mesh, vertex_begin, _color) );
    GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex_begin, center_u, center_v) );

    for( gp_uint32_t index = 0; index != _sector->segment_count + 1; ++index )
    {
        float angle = _sector->angle_from + _sector->angle_delta * (float)index / (float)_sector->segment_count;
        float cosine = GP_MATH_COSF( angle );
        float sine = GP_MATH_SINF( angle );
        float x = _point->position.x + cosine * _radius;
        float y = _point->position.y + sine * _radius;
        float u;
        float v;
        __line_round_sector_uv( _point, _cap_tangent, _source_total_length, _sector->angle_delta, _state, x, y,
            cosine * 0.5f + 0.5f, sine * 0.5f + 0.5f, &u, &v );
        GP_CALL( gp_mesh_push_position, (_mesh, vertex_begin + index + 1, x, y) );
        GP_CALL( gp_mesh_push_color, (_mesh, vertex_begin + index + 1, _color) );
        GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex_begin + index + 1, u, v) );
    }

    for( gp_uint32_t index = 0; index != _sector->segment_count; ++index )
    {
        GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 0, vertex_begin) );
        GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 1, vertex_begin + index + 1) );
        GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 2, vertex_begin + index + 2) );
        *_index_iterator += 3;
    }

    *_vertex_iterator += _sector->segment_count + 2;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_render_soft_sector( const gp_mesh_t * _mesh, const gp_state_t * _state,
    const gp_cooked_point_t * _point, const gp_vec2f_t * _cap_tangent, float _source_total_length,
    float _radius, float _inner_radius,
    const gp_round_sector_t * _sector, gp_argb_t _color, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator )
{
    gp_uint32_t vertex_begin = *_vertex_iterator;
    gp_argb_t transparent = _color & 0x00ffffffU;
    float center_u;
    float center_v;
    __line_round_sector_uv( _point, _cap_tangent, _source_total_length, _sector->angle_delta, _state,
        _point->position.x, _point->position.y,
        0.5f, 0.5f, &center_u, &center_v );
    GP_CALL( gp_mesh_push_position, (_mesh, vertex_begin, _point->position.x, _point->position.y) );
    GP_CALL( gp_mesh_push_color, (_mesh, vertex_begin, _color) );
    GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex_begin, center_u, center_v) );

    for( gp_uint32_t index = 0; index != _sector->segment_count + 1; ++index )
    {
        float angle = _sector->angle_from + _sector->angle_delta * (float)index / (float)_sector->segment_count;
        float cosine = GP_MATH_COSF( angle );
        float sine = GP_MATH_SINF( angle );
        gp_uint32_t inner = vertex_begin + 1 + index;
        gp_uint32_t outer = vertex_begin + 2 + _sector->segment_count + index;
        float inner_x = _point->position.x + cosine * _inner_radius;
        float inner_y = _point->position.y + sine * _inner_radius;
        float outer_x = _point->position.x + cosine * _radius;
        float outer_y = _point->position.y + sine * _radius;
        float inner_u;
        float inner_v;
        float outer_u;
        float outer_v;
        __line_round_sector_uv( _point, _cap_tangent, _source_total_length, _sector->angle_delta, _state, inner_x, inner_y,
            cosine * 0.5f + 0.5f, sine * 0.5f + 0.5f, &inner_u, &inner_v );
        __line_round_sector_uv( _point, _cap_tangent, _source_total_length, _sector->angle_delta, _state, outer_x, outer_y,
            cosine * 0.5f + 0.5f, sine * 0.5f + 0.5f, &outer_u, &outer_v );
        GP_CALL( gp_mesh_push_position, (_mesh, inner, inner_x, inner_y) );
        GP_CALL( gp_mesh_push_color, (_mesh, inner, _color) );
        GP_CALL( gp_mesh_push_uv, (_state, _mesh, inner, inner_u, inner_v) );
        GP_CALL( gp_mesh_push_position, (_mesh, outer, outer_x, outer_y) );
        GP_CALL( gp_mesh_push_color, (_mesh, outer, transparent) );
        GP_CALL( gp_mesh_push_uv, (_state, _mesh, outer, outer_u, outer_v) );
    }

    for( gp_uint32_t index = 0; index != _sector->segment_count; ++index )
    {
        gp_uint32_t inner = vertex_begin + 1 + index;
        gp_uint32_t outer = vertex_begin + 2 + _sector->segment_count + index;
        gp_uint32_t next_inner = inner + 1;
        gp_uint32_t next_outer = outer + 1;
        GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 0, vertex_begin) );
        GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 1, inner) );
        GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 2, next_inner) );
        *_index_iterator += 3;
        GP_CALL( __line_push_quad, (_mesh, _index_iterator, inner, outer, next_inner, next_outer) );
    }

    *_vertex_iterator += _sector->segment_count * 2 + 3;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_render_round_sector( const gp_cooked_point_t * _point, const gp_round_sector_t * _sector,
    const gp_vec2f_t * _cap_tangent, float _source_total_length, const gp_state_t * _state, const gp_mesh_t * _mesh,
    gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator )
{
    gp_color_t total_color;
    gp_color_mul( &total_color, &_mesh->color, &_point->color );
    gp_argb_t color = gp_color_argb( &total_color );

    if( _state->penumbra > 0.f )
    {
        float inner_radius = _point->half_width - _state->penumbra;
        if( inner_radius < 0.f )
        {
            inner_radius = 0.f;
        }

        gp_result_t result = __line_render_soft_sector( _mesh, _state, _point, _cap_tangent, _source_total_length, _point->half_width, inner_radius, _sector, color, _vertex_iterator, _index_iterator );
        return result;
    }

    if( _state->outline_width > 0.f )
    {
        gp_color_t total_outline_color;
        gp_color_mul( &total_outline_color, &_mesh->color, &_point->outline_color );
        GP_CALL( __line_render_solid_sector, (_mesh, _state, _point, _cap_tangent, _source_total_length,
            _point->half_width + _state->outline_width, _sector, gp_color_argb( &total_outline_color ),
            _vertex_iterator, _index_iterator) );
    }

    GP_CALL( __line_render_solid_sector, (_mesh, _state, _point, _cap_tangent, _source_total_length, _point->half_width,
        _sector, color, _vertex_iterator, _index_iterator) );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static void __line_align_cross_section( const gp_cooked_span_t * _span, const gp_cooked_point_t * _point, const gp_state_t * _state, gp_vec2f_t * _left, gp_vec2f_t * _right )
{
    if( _span->source_closed == GP_FALSE || _state->stroke_alignment == GP_STROKE_ALIGNMENT_CENTER )
    {
        return;
    }

    gp_bool_t interior_right = _span->source_ccw;
    gp_bool_t keep_right = _state->stroke_alignment == GP_STROKE_ALIGNMENT_INNER
        ? interior_right
        : (interior_right == GP_TRUE ? GP_FALSE : GP_TRUE);
    if( keep_right == GP_TRUE )
    {
        _right->x = _point->position.x + (_right->x - _point->position.x) * 2.f;
        _right->y = _point->position.y + (_right->y - _point->position.y) * 2.f;
        *_left = _point->position;
    }
    else
    {
        _left->x = _point->position.x + (_left->x - _point->position.x) * 2.f;
        _left->y = _point->position.y + (_left->y - _point->position.y) * 2.f;
        *_right = _point->position;
    }
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_render_span( gp_cooked_span_t * _span, const gp_state_t * _state, const gp_mesh_t * _mesh, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator )
{
    if( _span->count < 2 )
    {
        return GP_SUCCESSFUL;
    }

    if( _span->closed == GP_FALSE && _state->line_cap == GP_LINE_CAP_SQUARE )
    {
        gp_vec2f_t first_direction;
        gp_vec2f_t last_direction;
        __line_direction( &first_direction, &_span->points[0].position, &_span->points[1].position );
        __line_direction( &last_direction, &_span->points[_span->count - 2].position, &_span->points[_span->count - 1].position );
        _span->points[0].position.x -= first_direction.x * _span->points[0].half_width;
        _span->points[0].position.y -= first_direction.y * _span->points[0].half_width;
        _span->points[_span->count - 1].position.x += last_direction.x * _span->points[_span->count - 1].half_width;
        _span->points[_span->count - 1].position.y += last_direction.y * _span->points[_span->count - 1].half_width;
    }

    gp_uint32_t vertex_stride = _state->penumbra > 0.f ? 4 : (_state->outline_width > 0.f ? 6 : 2);
    gp_uint32_t vertex_begin = *_vertex_iterator;
    float total_length = _span->points[_span->count - 1].length;
    gp_bool_t round_sections = _state->line_join == GP_LINE_JOIN_ROUND ? GP_TRUE : GP_FALSE;
    gp_uint32_t section_count = __line_body_section_count( _span, _state );

    gp_uint32_t section_index = 0;
    for( gp_uint32_t point_index = 0; point_index != _span->count; ++point_index )
    {
        gp_uint32_t point_section_count = __line_point_section_count( _span, point_index, _state );
        const gp_cooked_point_t * point = _span->points + point_index;
        for( gp_uint32_t point_section = 0; point_section != point_section_count; ++point_section, ++section_index )
        {
            gp_bool_t separated = GP_FALSE;
            gp_uint32_t segment_index = 0;
            if( round_sections == GP_TRUE )
            {
                gp_uint32_t last = _span->count - 1;
                gp_uint32_t join_index = _span->closed == GP_TRUE && point_index == last ? 0 : point_index;
                gp_round_sector_t sector;
                if( (_span->closed == GP_TRUE || (point_index != 0 && point_index != last)) &&
                    __line_make_join_sector( _span, join_index, _state, &sector ) == GP_TRUE )
                {
                    separated = GP_TRUE;
                    if( point_index == 0 )
                    {
                        segment_index = 0;
                    }
                    else if( point_index == last )
                    {
                        segment_index = last - 1;
                    }
                    else
                    {
                        segment_index = point_index - 1 + point_section;
                    }
                }
            }

            gp_vec2f_t left;
            gp_vec2f_t right;
            if( separated == GP_TRUE )
            {
                __line_segment_cross_section( _span, segment_index, point_index, 0.f, 0.f, _state, &left, &right );
            }
            else if( round_sections == GP_TRUE )
            {
                __line_smooth_cross_section( _span, point_index, 0.f, 0.f, _state, &left, &right );
            }
            else
            {
                __line_cross_section( _span, point_index, 0.f, 0.f, _state, &left, &right );
            }
            __line_align_cross_section( _span, point, _state, &left, &right );

            float u;
            if( _state->uv_mode == GP_UV_MODE_PATH && _span->source_total_length > 0.f )
            {
                float source_length = point->source_length;
                if( source_length > _span->source_total_length )
                {
                    source_length = GP_MATH_FMODF( source_length, _span->source_total_length );
                }
                u = source_length / _span->source_total_length;
            }
            else
            {
                u = total_length > 0.f ? point->length / total_length : 0.f;
            }
            gp_color_t total_color;
            gp_color_mul( &total_color, &_mesh->color, &point->color );
            gp_argb_t color = gp_color_argb( &total_color );
            gp_uint32_t vertex = vertex_begin + section_index * vertex_stride;

            if( _state->penumbra > 0.f )
            {
                gp_vec2f_t inner_left;
                gp_vec2f_t inner_right;
                if( separated == GP_TRUE )
                {
                    __line_segment_cross_section( _span, segment_index, point_index, 0.f, _state->penumbra, _state, &inner_left, &inner_right );
                }
                else if( round_sections == GP_TRUE )
                {
                    __line_smooth_cross_section( _span, point_index, 0.f, _state->penumbra, _state, &inner_left, &inner_right );
                }
                else
                {
                    __line_cross_section( _span, point_index, 0.f, _state->penumbra, _state, &inner_left, &inner_right );
                }
                __line_align_cross_section( _span, point, _state, &inner_left, &inner_right );
                float inner_width = __line_point_width( point, 0.f, _state->penumbra );
                float inner_uv = point->half_width > 0.f ? inner_width / point->half_width * 0.5f : 0.f;
                GP_CALL( gp_mesh_push_position, (_mesh, vertex + 0, left.x, left.y) );
                GP_CALL( gp_mesh_push_color, (_mesh, vertex + 0, color & 0x00ffffffU) );
                GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex + 0, u, 0.f) );
                GP_CALL( gp_mesh_push_position, (_mesh, vertex + 1, inner_left.x, inner_left.y) );
                GP_CALL( gp_mesh_push_color, (_mesh, vertex + 1, color) );
                GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex + 1, u, 0.5f - inner_uv) );
                GP_CALL( gp_mesh_push_position, (_mesh, vertex + 2, inner_right.x, inner_right.y) );
                GP_CALL( gp_mesh_push_color, (_mesh, vertex + 2, color) );
                GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex + 2, u, 0.5f + inner_uv) );
                GP_CALL( gp_mesh_push_position, (_mesh, vertex + 3, right.x, right.y) );
                GP_CALL( gp_mesh_push_color, (_mesh, vertex + 3, color & 0x00ffffffU) );
                GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex + 3, u, 1.f) );
            }
            else if( _state->outline_width > 0.f )
            {
                gp_vec2f_t outline_left;
                gp_vec2f_t outline_right;
                if( separated == GP_TRUE )
                {
                    __line_segment_cross_section( _span, segment_index, point_index, _state->outline_width, 0.f, _state, &outline_left, &outline_right );
                }
                else if( round_sections == GP_TRUE )
                {
                    __line_smooth_cross_section( _span, point_index, _state->outline_width, 0.f, _state, &outline_left, &outline_right );
                }
                else
                {
                    __line_cross_section( _span, point_index, _state->outline_width, 0.f, _state, &outline_left, &outline_right );
                }
                __line_align_cross_section( _span, point, _state, &outline_left, &outline_right );
            gp_color_t total_outline_color;
            gp_color_mul( &total_outline_color, &_mesh->color, &point->outline_color );
            gp_argb_t outline_color = gp_color_argb( &total_outline_color );
            GP_CALL( gp_mesh_push_position, (_mesh, vertex + 0, outline_left.x, outline_left.y) );
            GP_CALL( gp_mesh_push_color, (_mesh, vertex + 0, outline_color) );
            GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex + 0, u, 0.f) );
            GP_CALL( gp_mesh_push_position, (_mesh, vertex + 1, left.x, left.y) );
            GP_CALL( gp_mesh_push_color, (_mesh, vertex + 1, outline_color) );
            GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex + 1, u, 0.f) );
            GP_CALL( gp_mesh_push_position, (_mesh, vertex + 2, left.x, left.y) );
            GP_CALL( gp_mesh_push_color, (_mesh, vertex + 2, color) );
            GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex + 2, u, 0.f) );
            GP_CALL( gp_mesh_push_position, (_mesh, vertex + 3, right.x, right.y) );
            GP_CALL( gp_mesh_push_color, (_mesh, vertex + 3, color) );
            GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex + 3, u, 1.f) );
            GP_CALL( gp_mesh_push_position, (_mesh, vertex + 4, right.x, right.y) );
            GP_CALL( gp_mesh_push_color, (_mesh, vertex + 4, outline_color) );
            GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex + 4, u, 1.f) );
            GP_CALL( gp_mesh_push_position, (_mesh, vertex + 5, outline_right.x, outline_right.y) );
            GP_CALL( gp_mesh_push_color, (_mesh, vertex + 5, outline_color) );
            GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex + 5, u, 1.f) );
            }
            else
            {
                GP_CALL( gp_mesh_push_position, (_mesh, vertex + 0, left.x, left.y) );
                GP_CALL( gp_mesh_push_color, (_mesh, vertex + 0, color) );
                GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex + 0, u, 0.f) );
                GP_CALL( gp_mesh_push_position, (_mesh, vertex + 1, right.x, right.y) );
                GP_CALL( gp_mesh_push_color, (_mesh, vertex + 1, color) );
                GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex + 1, u, 1.f) );
            }
        }
    }

    gp_uint32_t current_section = 0;
    for( gp_uint32_t index = 0; index + 1 < _span->count; ++index )
    {
        gp_uint32_t current_point_section_count = __line_point_section_count( _span, index, _state );
        gp_uint32_t current = vertex_begin + (current_section + current_point_section_count - 1) * vertex_stride;
        current_section += current_point_section_count;
        gp_uint32_t next = vertex_begin + current_section * vertex_stride;
        if( _state->penumbra > 0.f )
        {
            GP_CALL( __line_push_quad, (_mesh, _index_iterator, current + 0, current + 1, next + 0, next + 1) );
            GP_CALL( __line_push_quad, (_mesh, _index_iterator, current + 1, current + 2, next + 1, next + 2) );
            GP_CALL( __line_push_quad, (_mesh, _index_iterator, current + 2, current + 3, next + 2, next + 3) );
        }
        else if( _state->outline_width > 0.f )
        {
            GP_CALL( __line_push_quad, (_mesh, _index_iterator, current + 0, current + 1, next + 0, next + 1) );
            GP_CALL( __line_push_quad, (_mesh, _index_iterator, current + 2, current + 3, next + 2, next + 3) );
            GP_CALL( __line_push_quad, (_mesh, _index_iterator, current + 4, current + 5, next + 4, next + 5) );
        }
        else
        {
            GP_CALL( __line_push_quad, (_mesh, _index_iterator, current + 0, current + 1, next + 0, next + 1) );
        }
    }

    *_vertex_iterator += section_count * vertex_stride;

    if( _state->line_join == GP_LINE_JOIN_ROUND )
    {
        gp_uint32_t begin = _span->closed == GP_TRUE ? 0 : 1;
        gp_uint32_t end = _span->count - 1;
        for( gp_uint32_t index = begin; index < end; ++index )
        {
            gp_round_sector_t sector;
            if( __line_make_join_sector( _span, index, _state, &sector ) == GP_TRUE )
            {
                GP_CALL( __line_render_round_sector, (_span->points + index, &sector, GP_NULLPTR,
                    _span->source_total_length,
                    _state, _mesh, _vertex_iterator, _index_iterator) );
            }
        }
    }

    if( _span->closed == GP_FALSE && _state->line_cap == GP_LINE_CAP_ROUND )
    {
        gp_round_sector_t start_sector;
        gp_round_sector_t end_sector;
        __line_make_cap_sector( _span, GP_TRUE, _state, &start_sector );
        __line_make_cap_sector( _span, GP_FALSE, _state, &end_sector );
        gp_vec2f_t start_tangent;
        gp_vec2f_t end_tangent;
        __line_direction( &start_tangent, &_span->points[0].position, &_span->points[1].position );
        __line_direction( &end_tangent, &_span->points[_span->count - 2].position,
            &_span->points[_span->count - 1].position );
        GP_CALL( __line_render_round_sector, (_span->points + 0, &start_sector, &start_tangent,
            _span->source_total_length,
            _state, _mesh, _vertex_iterator, _index_iterator) );
        GP_CALL( __line_render_round_sector, (_span->points + _span->count - 1, &end_sector, &end_tangent,
            _span->source_total_length,
            _state, _mesh, _vertex_iterator, _index_iterator) );
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_marker_vertex( const gp_mesh_t * _mesh, const gp_state_t * _state, gp_uint32_t _vertex,
    float _x, float _y, gp_argb_t _color, float _u, float _v )
{
    GP_CALL( gp_mesh_push_position, (_mesh, _vertex, _x, _y) );
    GP_CALL( gp_mesh_push_color, (_mesh, _vertex, _color) );
    GP_CALL( gp_mesh_push_uv, (_state, _mesh, _vertex, _u, _v) );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_render_marker_polygon( const gp_vec2f_t * _inner, const gp_vec2f_t * _outer,
    gp_uint32_t _count, gp_argb_t _core_color, gp_argb_t _outer_color,
    const gp_state_t * _state, const gp_mesh_t * _mesh, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator )
{
    gp_uint32_t vertex = *_vertex_iterator;
    const gp_vec2f_t * uv_points = _outer == GP_NULLPTR ? _inner : _outer;
    float minimum_x = uv_points[0].x;
    float minimum_y = uv_points[0].y;
    float maximum_x = uv_points[0].x;
    float maximum_y = uv_points[0].y;
    for( gp_uint32_t index = 1; index != _count; ++index )
    {
        minimum_x = uv_points[index].x < minimum_x ? uv_points[index].x : minimum_x;
        minimum_y = uv_points[index].y < minimum_y ? uv_points[index].y : minimum_y;
        maximum_x = uv_points[index].x > maximum_x ? uv_points[index].x : maximum_x;
        maximum_y = uv_points[index].y > maximum_y ? uv_points[index].y : maximum_y;
    }
    float uv_width = maximum_x - minimum_x;
    float uv_height = maximum_y - minimum_y;
    uv_width = uv_width > 0.00001f ? uv_width : 1.f;
    uv_height = uv_height > 0.00001f ? uv_height : 1.f;

    if( _outer == GP_NULLPTR )
    {
        for( gp_uint32_t index = 0; index != _count; ++index )
        {
            GP_CALL( __line_marker_vertex, (_mesh, _state, vertex + index, _inner[index].x, _inner[index].y,
                _core_color, (_inner[index].x - minimum_x) / uv_width, (_inner[index].y - minimum_y) / uv_height) );
        }
        for( gp_uint32_t index = 1; index + 1 < _count; ++index )
        {
            GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 0, vertex) );
            GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 1, vertex + index) );
            GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 2, vertex + index + 1) );
            *_index_iterator += 3;
        }
        *_vertex_iterator += _count;
        return GP_SUCCESSFUL;
    }

    gp_uint32_t inner_vertex = vertex + _count;
    for( gp_uint32_t index = 0; index != _count; ++index )
    {
        GP_CALL( __line_marker_vertex, (_mesh, _state, vertex + index, _outer[index].x, _outer[index].y,
            _outer_color, (_outer[index].x - minimum_x) / uv_width, (_outer[index].y - minimum_y) / uv_height) );
        GP_CALL( __line_marker_vertex, (_mesh, _state, inner_vertex + index, _inner[index].x, _inner[index].y,
            _core_color, (_inner[index].x - minimum_x) / uv_width, (_inner[index].y - minimum_y) / uv_height) );

        gp_uint32_t next = (index + 1) % _count;
        GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 0, vertex + index) );
        GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 1, inner_vertex + index) );
        GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 2, vertex + next) );
        GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 3, vertex + next) );
        GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 4, inner_vertex + index) );
        GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 5, inner_vertex + next) );
        *_index_iterator += 6;
    }
    for( gp_uint32_t index = 1; index + 1 < _count; ++index )
    {
        GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 0, inner_vertex) );
        GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 1, inner_vertex + index) );
        GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 2, inner_vertex + index + 1) );
        *_index_iterator += 3;
    }
    *_vertex_iterator += _count * 2;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static void __line_marker_transform( gp_vec2f_t * _output, const gp_vec2f_t * _local, gp_uint32_t _count,
    const gp_vec2f_t * _center, const gp_vec2f_t * _direction, const gp_vec2f_t * _normal )
{
    for( gp_uint32_t index = 0; index != _count; ++index )
    {
        _output[index].x = _center->x + _direction->x * _local[index].x + _normal->x * _local[index].y;
        _output[index].y = _center->y + _direction->y * _local[index].x + _normal->y * _local[index].y;
    }
}
//////////////////////////////////////////////////////////////////////////
static void __line_marker_make_closed( gp_path_marker_type_t _type, gp_uint32_t _segments, float _length, float _width, gp_vec2f_t * _points )
{
    float half_length = _length * 0.5f;
    float half_width = _width * 0.5f;
    if( _type == GP_PATH_MARKER_FILLED_ARROW || _type == GP_PATH_MARKER_TRIANGLE )
    {
        _points[0] = (gp_vec2f_t){half_length, 0.f};
        _points[1] = (gp_vec2f_t){-half_length, half_width};
        _points[2] = (gp_vec2f_t){-half_length, -half_width};
    }
    else if( _type == GP_PATH_MARKER_DIAMOND )
    {
        _points[0] = (gp_vec2f_t){half_length, 0.f};
        _points[1] = (gp_vec2f_t){0.f, half_width};
        _points[2] = (gp_vec2f_t){-half_length, 0.f};
        _points[3] = (gp_vec2f_t){0.f, -half_width};
    }
    else
    {
        for( gp_uint32_t index = 0; index != _segments; ++index )
        {
            float angle = gp_constant_two_pi * (float)index / (float)_segments;
            _points[index].x = GP_MATH_COSF( angle ) * half_length;
            _points[index].y = GP_MATH_SINF( angle ) * half_width;
        }
    }
}
//////////////////////////////////////////////////////////////////////////
static void __line_marker_make_arm( gp_vec2f_t * _points, float _length, float _width, float _arm_width, float _side )
{
    gp_vec2f_t tip = {_length * 0.5f, 0.f};
    gp_vec2f_t base = {-_length * 0.5f, _side * _width * 0.5f};
    float dx = base.x - tip.x;
    float dy = base.y - tip.y;
    float inv_length = 1.f / GP_MATH_SQRTF( dx * dx + dy * dy );
    float nx = -dy * inv_length * _arm_width * 0.5f;
    float ny = dx * inv_length * _arm_width * 0.5f;
    _points[0] = (gp_vec2f_t){tip.x + nx, tip.y + ny};
    _points[1] = (gp_vec2f_t){base.x + nx, base.y + ny};
    _points[2] = (gp_vec2f_t){base.x - nx, base.y - ny};
    _points[3] = (gp_vec2f_t){tip.x - nx, tip.y - ny};
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_render_marker( const gp_path_marker_t * _marker, const gp_cooked_point_t * _endpoint, const gp_cooked_point_t * _neighbor, gp_bool_t _start, const gp_state_t * _state, const gp_mesh_t * _mesh, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator )
{
    if( _marker->type == GP_PATH_MARKER_NONE )
    {
        return GP_SUCCESSFUL;
    }

    gp_vec2f_t direction;
    if( _start == GP_TRUE )
    {
        __line_direction( &direction, &_neighbor->position, &_endpoint->position );
    }
    else
    {
        __line_direction( &direction, &_neighbor->position, &_endpoint->position );
    }
    gp_vec2f_t normal = {-direction.y, direction.x};
    GP_UNUSED( _start );
    gp_vec2f_t center = {
        _endpoint->position.x - direction.x * (_marker->inset + _marker->length * 0.5f),
        _endpoint->position.y - direction.y * (_marker->inset + _marker->length * 0.5f)
    };
    gp_color_t total_color;
    gp_color_mul( &total_color, &_mesh->color, &_endpoint->color );
    gp_argb_t color = gp_color_argb( &total_color );
    gp_bool_t layered = _state->penumbra > 0.f || _state->outline_width > 0.f ? GP_TRUE : GP_FALSE;
    float expansion = _state->penumbra > 0.f ? _state->penumbra : _state->outline_width;
    gp_argb_t outer_color;
    if( _state->penumbra > 0.f )
    {
        outer_color = color & 0x00ffffffU;
    }
    else
    {
        gp_color_t total_outline;
        gp_color_mul( &total_outline, &_mesh->color, &_endpoint->outline_color );
        outer_color = gp_color_argb( &total_outline );
    }

    gp_uint32_t count = _marker->type == GP_PATH_MARKER_CIRCLE ? (gp_uint32_t)_state->rect_quality :
        (_marker->type == GP_PATH_MARKER_FILLED_ARROW || _marker->type == GP_PATH_MARKER_TRIANGLE ? 3U : 4U);
    gp_vec2f_t inner_local[256];
    gp_vec2f_t outer_local[256];
    gp_vec2f_t inner[256];
    gp_vec2f_t outer[256];
    gp_uint32_t shape_count = _marker->type == GP_PATH_MARKER_OPEN_ARROW ? 2U : 1U;
    for( gp_uint32_t shape = 0; shape != shape_count; ++shape )
    {
        if( _marker->type == GP_PATH_MARKER_OPEN_ARROW )
        {
            float arm = (_marker->width < _marker->length ? _marker->width : _marker->length) * 0.12f;
            __line_marker_make_arm( inner_local, _marker->length, _marker->width, arm, shape == 0 ? 1.f : -1.f );
            if( layered == GP_TRUE )
            {
                __line_marker_make_arm( outer_local, _marker->length + expansion * 2.f,
                    _marker->width + expansion * 2.f, arm + expansion * 2.f, shape == 0 ? 1.f : -1.f );
            }
        }
        else
        {
            __line_marker_make_closed( _marker->type, count, _marker->length, _marker->width, inner_local );
            if( layered == GP_TRUE )
            {
                __line_marker_make_closed( _marker->type, count, _marker->length + expansion * 2.f,
                    _marker->width + expansion * 2.f, outer_local );
            }
        }

        __line_marker_transform( inner, inner_local, count, &center, &direction, &normal );
        if( layered == GP_TRUE )
        {
            __line_marker_transform( outer, outer_local, count, &center, &direction, &normal );
        }
        GP_CALL( __line_render_marker_polygon, (inner, layered == GP_TRUE ? outer : GP_NULLPTR, count,
            color, outer_color, _state, _mesh, _vertex_iterator, _index_iterator) );
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_render_line( const gp_line_t * _line, const gp_state_t * _state, const gp_mesh_t * _mesh, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator )
{
    if( _line->edges == GP_NULLPTR )
    {
        return GP_SUCCESSFUL;
    }

    gp_cooked_path_t path;
    GP_CALL( __line_cook_path, (_line, &path) );
    if( (_state->flags & GP_STATE_FLAG_FILL) != 0 )
    {
        GP_CALL( __line_render_fill, (_line, &path, _state, _mesh, _vertex_iterator, _index_iterator) );
        __line_destroy_path( _line, &path );
        return GP_SUCCESSFUL;
    }

    gp_cooked_path_t trimmed;
    GP_CALL( __line_trim_path, (_line, &path, &trimmed) );
    gp_cooked_stroke_t stroke;
    GP_CALL( __line_cook_stroke, (_line, &trimmed, &stroke) );
    for( gp_uint32_t index = 0; index != stroke.count; ++index )
    {
        GP_CALL( __line_render_span, (stroke.spans + index, _state, _mesh, _vertex_iterator, _index_iterator) );
    }

    if( trimmed.count >= 2 && trimmed.closed == GP_FALSE )
    {
        GP_CALL( __line_render_marker, (&_state->start_marker, trimmed.points + 0, trimmed.points + 1, GP_TRUE, _state, _mesh, _vertex_iterator, _index_iterator) );
        GP_CALL( __line_render_marker, (&_state->end_marker, trimmed.points + trimmed.count - 1, trimmed.points + trimmed.count - 2, GP_FALSE, _state, _mesh, _vertex_iterator, _index_iterator) );
    }

    __line_destroy_stroke( _line, &stroke );
    __line_destroy_path( _line, &trimmed );
    __line_destroy_path( _line, &path );

    return GP_SUCCESSFUL;
}
