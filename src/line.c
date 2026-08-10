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
    gp_bool_t join;
} gp_cooked_point_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_cooked_path_t
{
    gp_cooked_point_t * points;
    gp_uint32_t count;
    gp_uint32_t capacity;
    gp_bool_t closed;
} gp_cooked_path_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_cooked_span_t
{
    gp_cooked_point_t * points;
    gp_uint32_t count;
    gp_uint32_t capacity;
    gp_bool_t closed;
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
static float __line_lerp( float _from, float _to, float _t )
{
    return _from * (1.f - _t) + _to * _t;
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

    return GP_MATH_SQRTF( dx * dx + dy * dy );
}
//////////////////////////////////////////////////////////////////////////
static gp_bool_t __line_same_point( const gp_vec2f_t * _from, const gp_vec2f_t * _to )
{
    return GP_EQUAL_F( _from->x - _to->x, 0.00001f ) && GP_EQUAL_F( _from->y - _to->y, 0.00001f );
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
static void __line_reserve_points( const gp_line_t * _line, gp_cooked_point_t ** _points, gp_uint32_t _count, gp_uint32_t * _capacity, gp_uint32_t _required )
{
    if( _required <= *_capacity )
    {
        return;
    }

    gp_uint32_t capacity = *_capacity == 0 ? 16 : *_capacity;
    while( capacity < _required )
    {
        capacity *= 2;
    }

    gp_cooked_point_t * points = (gp_cooked_point_t *)(*_line->realloc)(GP_NULLPTR, sizeof( gp_cooked_point_t ) * capacity, _line->ud);
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
}
//////////////////////////////////////////////////////////////////////////
static void __line_push_point( const gp_line_t * _line, gp_cooked_path_t * _path, const gp_cooked_point_t * _point )
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

        return;
    }

    __line_reserve_points( _line, &_path->points, _path->count, &_path->capacity, _path->count + 1 );

    gp_cooked_point_t * point = _path->points + _path->count;
    *point = *_point;
    point->length = _path->count == 0
        ? 0.f
        : _path->points[_path->count - 1].length + __line_distance( &_path->points[_path->count - 1].position, &_point->position );
    ++_path->count;
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

    return GP_EQUAL_F( angle, 0.00001f ) == GP_TRUE ? GP_FALSE : GP_TRUE;
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

    return __line_tangents_join( &incoming, &outgoing );
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

        return __line_tangents_join( &incoming, &outgoing );
    }

    gp_vec2f_t outgoing;
    outgoing.x = first_point->p.x - _point->p.x;
    outgoing.y = first_point->p.y - _point->p.y;

    return __line_tangents_join( &incoming, &outgoing );
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
    _point->join = GP_FALSE;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_cook_path( const gp_line_t * _line, gp_cooked_path_t * const _path )
{
    _path->points = GP_NULLPTR;
    _path->count = 0;
    _path->capacity = 0;
    _path->closed = _line->closed;

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
            __line_push_point( _line, _path, &point );
        }

        from = to;
    }

    gp_cooked_point_t last;
    last.position = from->p;
    last.half_width = from->state->thickness * 0.5f;
    last.color = from->state->color;
    last.outline_color = from->state->outline_color;
    last.length = 0.f;
    last.join = __line_last_point_join( _line, from );
    __line_push_point( _line, _path, &last );

    if( _line->closed == GP_TRUE && _line->state->fill == GP_FALSE && _path->count >= 2 )
    {
        if( __line_same_point( &_path->points[0].position, &_path->points[_path->count - 1].position ) == GP_TRUE )
        {
            --_path->count;
        }

        gp_cooked_point_t closing = _path->points[0];
        __line_push_point( _line, _path, &closing );
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
static void __line_span_push( const gp_line_t * _line, gp_cooked_span_t * _span, const gp_cooked_point_t * _point )
{
    if( _span->count != 0 && __line_same_point( &_span->points[_span->count - 1].position, &_point->position ) == GP_TRUE )
    {
        gp_bool_t join = _span->points[_span->count - 1].join == GP_TRUE || _point->join == GP_TRUE
            ? GP_TRUE
            : GP_FALSE;
        _span->points[_span->count - 1] = *_point;
        _span->points[_span->count - 1].join = join;
        return;
    }

    __line_reserve_points( _line, &_span->points, _span->count, &_span->capacity, _span->count + 1 );
    _span->points[_span->count] = *_point;
    _span->points[_span->count].length = _span->count == 0
        ? 0.f
        : _span->points[_span->count - 1].length + __line_distance( &_span->points[_span->count - 1].position, &_point->position );
    ++_span->count;
}
//////////////////////////////////////////////////////////////////////////
static gp_cooked_span_t * __line_new_span( const gp_line_t * _line, gp_cooked_stroke_t * _stroke )
{
    if( _stroke->count == _stroke->capacity )
    {
        gp_uint32_t capacity = _stroke->capacity == 0 ? 8 : _stroke->capacity * 2;
        gp_cooked_span_t * spans = (gp_cooked_span_t *)(*_line->realloc)(GP_NULLPTR, sizeof( gp_cooked_span_t ) * capacity, _line->ud);
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

    return span;
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
static void __line_merge_closed_dash_seam( const gp_line_t * _line, gp_cooked_stroke_t * _stroke )
{
    if( _stroke->count < 2 )
    {
        return;
    }

    gp_cooked_span_t * first = _stroke->spans;
    gp_cooked_span_t * last = _stroke->spans + _stroke->count - 1;
    gp_uint32_t merged_count = last->count + first->count - 1;
    gp_cooked_point_t * merged = (gp_cooked_point_t *)(*_line->realloc)(GP_NULLPTR, sizeof( gp_cooked_point_t ) * merged_count, _line->ud);

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
    __line_recalculate_span_lengths( first );

    --_stroke->count;
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

    const gp_state_t * state = _line->state;
    if( state->dash_count == 0 )
    {
        gp_cooked_span_t * span = __line_new_span( _line, _stroke );
        for( gp_uint32_t index = 0; index != _path->count; ++index )
        {
            __line_span_push( _line, span, _path->points + index );
        }
        span->closed = _path->closed;

        return GP_SUCCESSFUL;
    }

    gp_uint32_t pattern_count = state->dash_count % 2 == 0 ? state->dash_count : state->dash_count * 2;
    float pattern_length = 0.f;
    for( gp_uint32_t index = 0; index != pattern_count; ++index )
    {
        pattern_length += state->dash_pattern[index % state->dash_count];
    }

    float phase = GP_MATH_FMODF( state->dash_offset, pattern_length );
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
                    active_span = __line_new_span( _line, _stroke );
                    __line_span_push( _line, active_span, &piece_from );
                }
                __line_span_push( _line, active_span, &piece_to );
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
            __line_merge_closed_dash_seam( _line, _stroke );
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

    return segment_count == 0 ? 1 : segment_count;
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
    return __line_make_join_sector( _span, _index, _state, &sector ) == GP_TRUE ? 2 : 1;
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
static gp_result_t __line_calculate_size( const gp_line_t * _line, gp_uint32_t * _vertex_count, gp_uint32_t * _index_count )
{
    if( _line->edges == GP_NULLPTR || _line->compound_fill != GP_NULLPTR )
    {
        return GP_SUCCESSFUL;
    }

    gp_cooked_path_t path;
    GP_CALL( __line_cook_path, (_line, &path) );

    if( _line->state->fill == GP_TRUE )
    {
        if( path.count >= 3 )
        {
            *_vertex_count += path.count;
            *_index_count += (path.count - 2) * 3;
        }

        __line_destroy_path( _line, &path );
        return GP_SUCCESSFUL;
    }

    gp_cooked_stroke_t stroke;
    GP_CALL( __line_cook_stroke, (_line, &path, &stroke) );
    for( gp_uint32_t index = 0; index != stroke.count; ++index )
    {
        __line_calculate_span_size( stroke.spans + index, _line->state, _vertex_count, _index_count );
    }

    __line_destroy_stroke( _line, &stroke );
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
    return (_b->x - _a->x) * (_c->y - _a->y) - (_b->y - _a->y) * (_c->x - _a->x);
}
//////////////////////////////////////////////////////////////////////////
static gp_bool_t __line_point_in_triangle( const gp_vec2f_t * _point, const gp_vec2f_t * _a, const gp_vec2f_t * _b, const gp_vec2f_t * _c )
{
    float c0 = __line_triangle_area( _a, _b, _point );
    float c1 = __line_triangle_area( _b, _c, _point );
    float c2 = __line_triangle_area( _c, _a, _point );
    gp_bool_t negative = c0 < -0.00001f || c1 < -0.00001f || c2 < -0.00001f ? GP_TRUE : GP_FALSE;
    gp_bool_t positive = c0 > 0.00001f || c1 > 0.00001f || c2 > 0.00001f ? GP_TRUE : GP_FALSE;

    return negative == GP_TRUE && positive == GP_TRUE ? GP_FALSE : GP_TRUE;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __line_render_fill_indices( const gp_line_t * _line, const gp_cooked_path_t * _path, const gp_mesh_t * _mesh, gp_uint32_t _vertex_begin, gp_uint32_t * _index_iterator )
{
    gp_uint32_t * order = (gp_uint32_t *)(*_line->realloc)(GP_NULLPTR, sizeof( gp_uint32_t ) * _path->count, _line->ud);
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
    for( gp_uint32_t index = 0; index != _path->count; ++index )
    {
        const gp_cooked_point_t * point = _path->points + index;
        gp_color_t total_color;
        gp_color_mul( &total_color, &_mesh->color, &point->color );
        GP_CALL( gp_mesh_push_position, (_mesh, vertex_begin + index, point->position.x, point->position.y) );
        GP_CALL( gp_mesh_push_color, (_mesh, vertex_begin + index, gp_color_argb( &total_color )) );
        GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex_begin + index, 0.f, 0.f) );
    }

    GP_CALL( __line_render_fill_indices, (_line, _path, _mesh, vertex_begin, _index_iterator) );
    *_vertex_iterator += _path->count;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static float __line_point_width( const gp_cooked_point_t * _point, float _addition, float _subtraction )
{
    float width = _point->half_width + _addition - _subtraction;

    return width > 0.f ? width : 0.f;
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
static gp_result_t __line_render_solid_sector( const gp_mesh_t * _mesh, const gp_state_t * _state, const gp_vec2f_t * _center, float _radius, const gp_round_sector_t * _sector, gp_argb_t _color, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator )
{
    gp_uint32_t vertex_begin = *_vertex_iterator;
    GP_CALL( gp_mesh_push_position, (_mesh, vertex_begin, _center->x, _center->y) );
    GP_CALL( gp_mesh_push_color, (_mesh, vertex_begin, _color) );
    GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex_begin, 0.5f, 0.5f) );

    for( gp_uint32_t index = 0; index != _sector->segment_count + 1; ++index )
    {
        float angle = _sector->angle_from + _sector->angle_delta * (float)index / (float)_sector->segment_count;
        float cosine = GP_MATH_COSF( angle );
        float sine = GP_MATH_SINF( angle );
        GP_CALL( gp_mesh_push_position, (_mesh, vertex_begin + index + 1, _center->x + cosine * _radius, _center->y + sine * _radius) );
        GP_CALL( gp_mesh_push_color, (_mesh, vertex_begin + index + 1, _color) );
        GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex_begin + index + 1, cosine * 0.5f + 0.5f, sine * 0.5f + 0.5f) );
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
static gp_result_t __line_render_soft_sector( const gp_mesh_t * _mesh, const gp_state_t * _state, const gp_vec2f_t * _center, float _radius, float _inner_radius, const gp_round_sector_t * _sector, gp_argb_t _color, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator )
{
    gp_uint32_t vertex_begin = *_vertex_iterator;
    gp_argb_t transparent = _color & 0x00ffffffU;
    GP_CALL( gp_mesh_push_position, (_mesh, vertex_begin, _center->x, _center->y) );
    GP_CALL( gp_mesh_push_color, (_mesh, vertex_begin, _color) );
    GP_CALL( gp_mesh_push_uv, (_state, _mesh, vertex_begin, 0.5f, 0.5f) );

    for( gp_uint32_t index = 0; index != _sector->segment_count + 1; ++index )
    {
        float angle = _sector->angle_from + _sector->angle_delta * (float)index / (float)_sector->segment_count;
        float cosine = GP_MATH_COSF( angle );
        float sine = GP_MATH_SINF( angle );
        gp_uint32_t inner = vertex_begin + 1 + index;
        gp_uint32_t outer = vertex_begin + 2 + _sector->segment_count + index;
        GP_CALL( gp_mesh_push_position, (_mesh, inner, _center->x + cosine * _inner_radius, _center->y + sine * _inner_radius) );
        GP_CALL( gp_mesh_push_color, (_mesh, inner, _color) );
        GP_CALL( gp_mesh_push_uv, (_state, _mesh, inner, cosine * 0.5f + 0.5f, sine * 0.5f + 0.5f) );
        GP_CALL( gp_mesh_push_position, (_mesh, outer, _center->x + cosine * _radius, _center->y + sine * _radius) );
        GP_CALL( gp_mesh_push_color, (_mesh, outer, transparent) );
        GP_CALL( gp_mesh_push_uv, (_state, _mesh, outer, cosine * 0.5f + 0.5f, sine * 0.5f + 0.5f) );
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
static gp_result_t __line_render_round_sector( const gp_cooked_point_t * _point, const gp_round_sector_t * _sector, const gp_state_t * _state, const gp_mesh_t * _mesh, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator )
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

        return __line_render_soft_sector( _mesh, _state, &_point->position, _point->half_width, inner_radius, _sector, color, _vertex_iterator, _index_iterator );
    }

    if( _state->outline_width > 0.f )
    {
        gp_color_t total_outline_color;
        gp_color_mul( &total_outline_color, &_mesh->color, &_point->outline_color );
        GP_CALL( __line_render_solid_sector, (_mesh, _state, &_point->position, _point->half_width + _state->outline_width, _sector, gp_color_argb( &total_outline_color ), _vertex_iterator, _index_iterator) );
    }

    GP_CALL( __line_render_solid_sector, (_mesh, _state, &_point->position, _point->half_width, _sector, color, _vertex_iterator, _index_iterator) );

    return GP_SUCCESSFUL;
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

            float u = total_length > 0.f ? point->length / total_length : 0.f;
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
                GP_CALL( __line_render_round_sector, (_span->points + index, &sector, _state, _mesh, _vertex_iterator, _index_iterator) );
            }
        }
    }

    if( _span->closed == GP_FALSE && _state->line_cap == GP_LINE_CAP_ROUND )
    {
        gp_round_sector_t start_sector;
        gp_round_sector_t end_sector;
        __line_make_cap_sector( _span, GP_TRUE, _state, &start_sector );
        __line_make_cap_sector( _span, GP_FALSE, _state, &end_sector );
        GP_CALL( __line_render_round_sector, (_span->points + 0, &start_sector, _state, _mesh, _vertex_iterator, _index_iterator) );
        GP_CALL( __line_render_round_sector, (_span->points + _span->count - 1, &end_sector, _state, _mesh, _vertex_iterator, _index_iterator) );
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
    if( _state->fill == GP_TRUE )
    {
        GP_CALL( __line_render_fill, (_line, &path, _state, _mesh, _vertex_iterator, _index_iterator) );
        __line_destroy_path( _line, &path );
        return GP_SUCCESSFUL;
    }

    gp_cooked_stroke_t stroke;
    GP_CALL( __line_cook_stroke, (_line, &path, &stroke) );
    for( gp_uint32_t index = 0; index != stroke.count; ++index )
    {
        GP_CALL( __line_render_span, (stroke.spans + index, _state, _mesh, _vertex_iterator, _index_iterator) );
    }

    __line_destroy_stroke( _line, &stroke );
    __line_destroy_path( _line, &path );

    return GP_SUCCESSFUL;
}
