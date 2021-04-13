#include "line.h"

#include "struct.h"
#include "detail.h"

//////////////////////////////////////////////////////////////////////////
static gp_result_t __calculate_mesh_line_size( const gp_canvas_t * _canvas, gp_uint16_t * _vertex_count, gp_uint16_t * _index_count )
{
    gp_uint16_t vertex_count = 0;
    gp_uint16_t index_count = 0;

    GP_LIST_FOREACH( gp_line_t, _canvas->lines, l )
    {
        if( l->edges == GP_NULLPTR )
        {
            continue;
        }

        uint8_t curve_quality = l->state->curve_quality;

        gp_uint16_t point_count = 0;

        for( const gp_line_edge_t * e = l->edges; e != GP_NULLPTR; e = e->next )
        {
            switch( e->controls )
            {
            case 0:
                point_count += 1;
                break;
            case 1:
                point_count += curve_quality;
                break;
            case 2:
                point_count += curve_quality;
                break;
            default:
                return GP_FAILURE;
            }
        }

        point_count += 1;

        float penumbra = l->state->penumbra;
        float outline_width = l->state->outline_width;

        if( penumbra > 0.f )
        {
            vertex_count += point_count * 4;
            index_count += (point_count - 1) * 18;
        }
        else
        {
            if( outline_width > 0.f )
            {
                vertex_count += point_count * 6;
                index_count += (point_count - 1) * 18;
            }
            else
            {
                vertex_count += point_count * 2;
                index_count += (point_count - 1) * 6;
            }
        }
    }

    *_vertex_count = vertex_count;
    *_index_count = index_count;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_calculate_mesh_line_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh )
{
    gp_uint16_t vertex_count = 0;
    gp_uint16_t index_count = 0;

    GP_CALL( __calculate_mesh_line_size, (_canvas, &vertex_count, &index_count) );

    _mesh->vertex_count += vertex_count;
    _mesh->index_count += index_count;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static inline float __vec2f_sqrlength( const gp_vec2f_t * _p )
{
    return _p->x * _p->x + _p->y * _p->y;
}
//////////////////////////////////////////////////////////////////////////
static inline float __vec2f_length( const gp_vec2f_t * _p )
{
    float l = __vec2f_sqrlength( _p );

    float sqrt_l = GP_MATH_SQRTF( l );

    return sqrt_l;
}
//////////////////////////////////////////////////////////////////////////
static inline float __vec2f_distance( const gp_vec2f_t * _a, const gp_vec2f_t * _b )
{
    gp_vec2f_t d;
    d.x = _b->x - _a->x;
    d.y = _b->y - _a->y;

    float l = __vec2f_sqrlength( &d );

    float sqrt_l = GP_MATH_SQRTF( l );

    return sqrt_l;
}
//////////////////////////////////////////////////////////////////////////
static inline void __vec2f_normalize( gp_vec2f_t * _o, const gp_vec2f_t * _p )
{
    float l = __vec2f_length( _p );

    float l_inv = 1.f / l;

    _o->x = _p->x * l_inv;
    _o->y = _p->y * l_inv;
}
//////////////////////////////////////////////////////////////////////////
static inline void __vec2f_perp( gp_vec2f_t * _out, const gp_vec2f_t * _in )
{
    _out->x = -_in->y;
    _out->y = _in->x;
}
//////////////////////////////////////////////////////////////////////////
static inline void __make_line_perp( gp_vec2f_t * _perp, const gp_vec2f_t * _from, const gp_vec2f_t * _to )
{
    gp_vec2f_t dir;
    dir.x = _to->x - _from->x;
    dir.y = _to->y - _from->y;

    gp_vec2f_t dir_norm;
    __vec2f_normalize( &dir_norm, &dir );

    __vec2f_perp( _perp, &dir_norm );
}
//////////////////////////////////////////////////////////////////////////
static inline float __integral_powf( float _value, gp_uint32_t _count )
{
    if( _count == 0 )
    {
        return 1.f;
    }

    float f = _value;

    for( gp_uint32_t i = 1; i != _count; ++i )
    {
        f *= _value;
    }

    return f;
}
//////////////////////////////////////////////////////////////////////////
static inline float __factorialf( gp_uint32_t _value )
{
    if( _value == 0 )
    {
        return 1.f;
    }

    float f = 1.f;
    float d = 0.f;

    for( gp_uint32_t i = 0; i != _value; ++i )
    {
        d += 1.f;

        f *= d;
    }

    return f;
}
//////////////////////////////////////////////////////////////////////////
static inline void __calculate_bezier_position( gp_vec2f_t * _out, const gp_vec2f_t * _p0, const gp_vec2f_t * _p1, const gp_vec2f_t * _v, gp_uint32_t _n, float _dt )
{
    gp_uint32_t n = _n + 1;

    float t0 = __integral_powf( 1.f - _dt, n );
    float tn = __integral_powf( _dt, n );

    _out->x = t0 * _p0->x + tn * _p1->x;
    _out->y = t0 * _p0->y + tn * _p1->y;

    float f_count = __factorialf( n );

    for( gp_uint32_t i = 1; i != n; ++i )
    {
        float c = f_count / (__factorialf( i ) * __factorialf( n - i ));
        float t = __integral_powf( _dt, i ) * __integral_powf( 1.f - _dt, n - i );

        const gp_vec2f_t * v = _v + i - 1;

        _out->x += c * t * v->x;
        _out->y += c * t * v->y;
    }
}
//////////////////////////////////////////////////////////////////////////
static inline void __make_line_from_two_point_v2( gp_linef_t * _line, const gp_vec2f_t * _a, const gp_vec2f_t * _b )
{
    float a = _b->y - _a->y;
    float b = _a->x - _b->x;

    _line->a = a;
    _line->b = b;
    _line->c = a * _a->x + b * _a->y;
}
//////////////////////////////////////////////////////////////////////////
static inline float __cross2( float _a, float _b, float _c, float _d )
{
    return _a * _d - _c * _b;
}
//////////////////////////////////////////////////////////////////////////
static inline gp_bool_t __intersect_line( const gp_linef_t * _l1, const gp_linef_t * _l2, gp_vec2f_t * _p )
{
    float zn = __cross2( _l1->a, _l1->b, _l2->a, _l2->b );

    if( GP_EQUAL_F( zn, 0.00001f ) == GP_TRUE )
    {
        return GP_FALSE;
    }

    float inv_zn = 1.f / zn;

    _p->x = __cross2( _l1->c, _l1->b, _l2->c, _l2->b ) * inv_zn;
    _p->y = __cross2( _l1->a, _l1->c, _l2->a, _l2->c ) * inv_zn;

    return GP_TRUE;
}
//////////////////////////////////////////////////////////////////////////
static inline float __lerp( float _a, float _b, float _t )
{
    return _a * (1.f - _t) + _b * _t;
}
//////////////////////////////////////////////////////////////////////////
static inline void __lerp_color( gp_color_t * _c, const gp_color_t * _a, const gp_color_t * _b, float _t )
{
    _c->r = __lerp( _a->r, _b->r, _t );
    _c->g = __lerp( _a->g, _b->g, _t );
    _c->b = __lerp( _a->b, _b->b, _t );
    _c->a = __lerp( _a->a, _b->a, _t );
}
//////////////////////////////////////////////////////////////////////////
typedef struct gp_line_point_render_cache_t
{
    gp_points_t points[GP_LINE_POINTS_MAX];
    gp_uint16_t size;

#if defined(GP_DISABLE_EXTRA_LINE_POINTS)
    gp_points_t * extra_points;
    gp_uint16_t extra_capacity;
#endif
} gp_line_point_render_cache_t;
//////////////////////////////////////////////////////////////////////////
static gp_points_t * __get_render_point( gp_line_point_render_cache_t * _cache, gp_uint16_t _index )
{
#if defined(GP_DISABLE_EXTRA_LINE_POINTS)
    return _cache->points + _index;
#else
    if( _index < GP_LINE_POINTS_MAX )
    {
        return _cache->points + _index;
    }

    return _cache->extra_points + _index - GP_LINE_POINTS_MAX;
#endif
}
//////////////////////////////////////////////////////////////////////////
static inline gp_result_t __incref_render_points( const gp_canvas_t * _canvas, gp_line_point_render_cache_t * _cache )
{
    if( (++_cache->size) < GP_LINE_POINTS_MAX )
    {
        return GP_SUCCESSFUL;
    }

#if defined(GP_DISABLE_EXTRA_LINE_POINTS)
    GP_UNUSED( _canvas );

    return GP_FAILURE;
#else
    if( (_cache->size - GP_LINE_POINTS_MAX) < _cache->extra_capacity )
    {
        return GP_SUCCESSFUL;
    }

    _cache->extra_capacity += GP_LINE_POINTS_MAX;

    void * p = GP_REALLOC( _canvas, _cache->extra_points, gp_points_t, _cache->extra_capacity );

    _cache->extra_points = p;

    return GP_SUCCESSFUL;
#endif
}
//////////////////////////////////////////////////////////////////////////
#if defined( GP_DEBUG )
#define GP_POINTS_INCREF() if( __incref_render_points( _canvas, &points_cache) == GP_FAILURE) return GP_FAILURE
#else
#define GP_POINTS_INCREF() __incref_render_points( _canvas, &points_cache)
#endif
//////////////////////////////////////////////////////////////////////////
#define GP_GET_POINT(i) __get_render_point(&points_cache, i)
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_render_line( const gp_canvas_t * _canvas, const gp_mesh_t * _mesh, gp_uint16_t * _vertex_iterator, gp_uint16_t * _index_iterator )
{
    gp_uint16_t vertex_iterator = *_vertex_iterator;
    gp_uint16_t index_iterator = *_index_iterator;

    GP_LIST_FOREACH( gp_line_t, _canvas->lines, l )
    {
        if( l->edges == GP_NULLPTR )
        {
            continue;
        }

        float thickness = l->state->thickness;
        float outline_width = l->state->outline_width;
        float half_thickness = thickness * 0.5f;

        uint8_t curve_quality = l->state->curve_quality;
        float curve_quality_inv = l->state->curve_quality_inv;

        gp_line_point_render_cache_t points_cache;
        points_cache.size = 0;

#if defined(GP_DISABLE_EXTRA_LINE_POINTS)
        points_cache.extra_capacity = 0;
        points_cache.extra_points = GP_NULLPTR;
#endif

        {
            const gp_line_point_t * point_iterator = l->points;

            for( const gp_line_edge_t * e = l->edges; e != GP_NULLPTR; e = e->next )
            {
                const gp_line_point_t * p0 = point_iterator;
                point_iterator = point_iterator->next;

                gp_color_t point_color0;
                gp_color_mul( &point_color0, &_mesh->color, &p0->state->color );
                gp_argb_t argb0 = gp_color_argb( &point_color0 );

                gp_argb_t outline_argb0 = 0xffffffff;

                if( outline_width > 0.f )
                {
                    gp_color_t point_outline_color0;
                    gp_color_mul( &point_outline_color0, &_mesh->color, &p0->state->outline_color );
                    outline_argb0 = gp_color_argb( &point_outline_color0 );
                }

                switch( e->controls )
                {
                case 0:
                    {
                        gp_points_t * p = GP_GET_POINT( points_cache.size );
                        p->p = p0->p;
                        p->argb = argb0;
                        p->outline_argb = outline_argb0;

                        GP_POINTS_INCREF();
                    }break;
                case 1:
                    {
                        const gp_line_point_t * p1 = point_iterator;

                        float t = 0.f;

                        for( gp_uint8_t index = 0; index != curve_quality; ++index )
                        {
                            gp_color_t color01;
                            __lerp_color( &color01, &p0->state->color, &p1->state->color, t );

                            gp_color_t point_color1;
                            gp_color_mul( &point_color1, &_mesh->color, &color01 );
                            gp_argb_t argb01 = gp_color_argb( &point_color1 );

                            gp_argb_t outline_argb01 = 0xffffffff;

                            if( outline_width > 0.f )
                            {
                                gp_color_t outline_color01;
                                __lerp_color( &outline_color01, &p0->state->outline_color, &p1->state->outline_color, t );

                                gp_color_t point_outline_color1;
                                gp_color_mul( &point_outline_color1, &_mesh->color, &outline_color01 );
                                outline_argb01 = gp_color_argb( &point_outline_color1 );
                            }

                            gp_vec2f_t bp;
                            __calculate_bezier_position( &bp, &p0->p, &p1->p, e->p, 1, t );

                            t += curve_quality_inv;

                            gp_points_t * p = GP_GET_POINT( points_cache.size );
                            p->p = bp;
                            p->argb = argb01;
                            p->outline_argb = outline_argb01;

                            GP_POINTS_INCREF();
                        }
                    }break;
                case 2:
                    {
                        const gp_line_point_t * p1 = point_iterator;

                        float t = 0.f;

                        for( gp_uint8_t index = 0; index != curve_quality; ++index )
                        {
                            gp_color_t color01;
                            __lerp_color( &color01, &p0->state->color, &p1->state->color, t );

                            gp_color_t point_color1;
                            gp_color_mul( &point_color1, &_mesh->color, &color01 );
                            gp_argb_t argb01 = gp_color_argb( &point_color1 );

                            gp_argb_t outline_argb01 = 0xffffffff;

                            if( outline_width > 0.f )
                            {
                                gp_color_t outline_color01;
                                __lerp_color( &outline_color01, &p0->state->outline_color, &p1->state->outline_color, t );

                                gp_color_t point_outline_color1;
                                gp_color_mul( &point_outline_color1, &_mesh->color, &outline_color01 );
                                outline_argb01 = gp_color_argb( &point_outline_color1 );
                            }

                            gp_vec2f_t bp;
                            __calculate_bezier_position( &bp, &p0->p, &p1->p, e->p, 2, t );

                            t += curve_quality_inv;

                            gp_points_t * p = GP_GET_POINT( points_cache.size );
                            p->p = bp;
                            p->argb = argb01;
                            p->outline_argb = outline_argb01;

                            GP_POINTS_INCREF();
                        }
                    }break;
                default:
                    return GP_FAILURE;
                }
            }

            {
                const gp_line_point_t * p1 = point_iterator;

                gp_color_t point_color1;
                gp_color_mul( &point_color1, &_mesh->color, &p1->state->color );
                gp_argb_t argb1 = gp_color_argb( &point_color1 );

                gp_points_t * p = GP_GET_POINT( points_cache.size );
                p->p = p1->p;
                p->argb = argb1;

                GP_POINTS_INCREF();
            }
        }

        float penumbra = l->state->penumbra;

        if( penumbra > 0.f )
        {
            for( gp_uint16_t index = 0; index != points_cache.size - 1; ++index )
            {
                GP_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index * 4 + 0) );
                GP_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index * 4 + 1) );
                GP_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + index * 4 + 4) );
                GP_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + index * 4 + 4) );
                GP_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index * 4 + 1) );
                GP_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + index * 4 + 5) );

                index_iterator += 6;

                GP_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index * 4 + 1) );
                GP_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index * 4 + 2) );
                GP_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + index * 4 + 5) );
                GP_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + index * 4 + 5) );
                GP_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index * 4 + 2) );
                GP_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + index * 4 + 6) );

                index_iterator += 6;

                GP_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index * 4 + 2) );
                GP_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index * 4 + 3) );
                GP_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + index * 4 + 6) );
                GP_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + index * 4 + 6) );
                GP_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index * 4 + 3) );
                GP_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + index * 4 + 7) );

                index_iterator += 6;
            }
        }
        else
        {
            if( outline_width > 0.f )
            {
                for( gp_uint16_t index = 0; index != points_cache.size - 1; ++index )
                {
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index * 6 + 0) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index * 6 + 1) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + index * 6 + 6) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + index * 6 + 6) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index * 6 + 1) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + index * 6 + 7) );

                    index_iterator += 6;

                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index * 6 + 2) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index * 6 + 3) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + index * 6 + 8) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + index * 6 + 8) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index * 6 + 3) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + index * 6 + 9) );

                    index_iterator += 6;

                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index * 6 + 4) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index * 6 + 5) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + index * 6 + 10) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + index * 6 + 10) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index * 6 + 5) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + index * 6 + 11) );

                    index_iterator += 6;
                }
            }
            else
            {
                for( gp_uint16_t index = 0; index != points_cache.size - 1; ++index )
                {
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index * 2 + 0) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index * 2 + 1) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + index * 2 + 2) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + index * 2 + 2) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index * 2 + 1) );
                    GP_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + index * 2 + 3) );

                    index_iterator += 6;
                }
            }
        }

        float half_thickness_soft = 0.f;
        float uv_soft_offset = 0.f;

        if( penumbra > 0.f )
        {
            half_thickness_soft = half_thickness - penumbra;
            uv_soft_offset = half_thickness_soft / half_thickness * 0.5f;
        }

        float total_distance = 0.f;

        if( _mesh->uv_buffer != GP_NULLPTR )
        {
            for( gp_uint16_t index = 1; index != points_cache.size; ++index )
            {
                const gp_vec2f_t * p0 = &GP_GET_POINT( index - 1 )->p;
                const gp_vec2f_t * p1 = &GP_GET_POINT( index + 0 )->p;

                float d = __vec2f_distance( p0, p1 );

                total_distance += d;
            }
        }

        {
            const gp_vec2f_t * p0 = &GP_GET_POINT( 0 )->p;
            const gp_vec2f_t * p1 = &GP_GET_POINT( 1 )->p;

            gp_argb_t argb0 = GP_GET_POINT( 0 )->argb;
            gp_argb_t outline_argb0 = GP_GET_POINT( 0 )->outline_argb;

            gp_vec2f_t perp;
            __make_line_perp( &perp, p0, p1 );

            if( penumbra > 0.f )
            {
                GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p0->x - perp.x * half_thickness, p0->y - perp.y * half_thickness) );
                GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb0 & 0x00ffffff) );
                GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 0, 0.f, 0.f) );

                GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p0->x - perp.x * half_thickness_soft, p0->y - perp.y * half_thickness_soft) );
                GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb0) );
                GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 1, 0.f, 0.5f - uv_soft_offset) );

                GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p0->x + perp.x * half_thickness_soft, p0->y + perp.y * half_thickness_soft) );
                GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb0) );
                GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 2, 0.f, 0.5f + uv_soft_offset) );

                GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, p0->x + perp.x * half_thickness, p0->y + perp.y * half_thickness) );
                GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb0 & 0x00ffffff) );
                GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 3, 0.f, 1.f) );

                vertex_iterator += 4;
            }
            else
            {
                if( outline_width > 0.f )
                {
                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p0->x - perp.x * (half_thickness + outline_width), p0->y - perp.y * (half_thickness + outline_width)) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, outline_argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 0, 0.f, 0.f) );

                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p0->x - perp.x * half_thickness, p0->y - perp.y * half_thickness) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, outline_argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 1, 0.f, 0.f) );

                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p0->x - perp.x * half_thickness, p0->y - perp.y * half_thickness) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 2, 0.f, 0.f) );

                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, p0->x + perp.x * half_thickness, p0->y + perp.y * half_thickness) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 3, 0.f, 1.f) );

                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 4, p0->x + perp.x * half_thickness, p0->y + perp.y * half_thickness) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 4, outline_argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 4, 0.f, 1.f) );

                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 5, p0->x + perp.x * (half_thickness + outline_width), p0->y + perp.y * (half_thickness + outline_width)) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 5, outline_argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 5, 0.f, 1.f) );

                    vertex_iterator += 6;
                }
                else
                {
                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p0->x - perp.x * half_thickness, p0->y - perp.y * half_thickness) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 0, 0.f, 0.f) );

                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p0->x + perp.x * half_thickness, p0->y + perp.y * half_thickness) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 1, 0.f, 1.f) );

                    vertex_iterator += 2;
                }
            }
        }

        float dd = 0.f;

        for( gp_uint16_t index = 1; index != points_cache.size - 1; ++index )
        {
            const gp_vec2f_t * p0 = &GP_GET_POINT( index - 1 )->p;
            const gp_vec2f_t * p1 = &GP_GET_POINT( index + 0 )->p;
            const gp_vec2f_t * p2 = &GP_GET_POINT( index + 1 )->p;

            float u = 0.f;

            if( _mesh->uv_buffer != GP_NULLPTR )
            {
                float d = __vec2f_distance( p0, p1 );

                dd += d;

                u = dd / total_distance;
            }

            gp_argb_t argb0 = GP_GET_POINT( index + 0 )->argb;
            gp_argb_t outline_argb0 = GP_GET_POINT( index + 0 )->outline_argb;

            gp_vec2f_t perp01;
            __make_line_perp( &perp01, p0, p1 );

            gp_vec2f_t perp12;
            __make_line_perp( &perp12, p1, p2 );

            gp_vec2f_t linep00;
            linep00.x = p0->x - perp01.x * half_thickness;
            linep00.y = p0->y - perp01.y * half_thickness;

            gp_vec2f_t linep01;
            linep01.x = p1->x - perp01.x * half_thickness;
            linep01.y = p1->y - perp01.y * half_thickness;

            gp_linef_t line01l;
            __make_line_from_two_point_v2( &line01l, &linep00, &linep01 );

            gp_vec2f_t linep10;
            linep10.x = p1->x - perp12.x * half_thickness;
            linep10.y = p1->y - perp12.y * half_thickness;

            gp_vec2f_t linep11;
            linep11.x = p2->x - perp12.x * half_thickness;
            linep11.y = p2->y - perp12.y * half_thickness;

            gp_linef_t line12l;
            __make_line_from_two_point_v2( &line12l, &linep10, &linep11 );

            gp_vec2f_t pl;
            if( __intersect_line( &line01l, &line12l, &pl ) == GP_FALSE )
            {
                pl.x = p1->x - perp01.x * half_thickness;
                pl.y = p1->y - perp01.y * half_thickness;
            }

            gp_vec2f_t linep20;
            linep20.x = p0->x + perp01.x * half_thickness;
            linep20.y = p0->y + perp01.y * half_thickness;

            gp_vec2f_t linep21;
            linep21.x = p1->x + perp01.x * half_thickness;
            linep21.y = p1->y + perp01.y * half_thickness;

            gp_linef_t line01r;
            __make_line_from_two_point_v2( &line01r, &linep20, &linep21 );

            gp_vec2f_t linep30;
            linep30.x = p1->x + perp12.x * half_thickness;
            linep30.y = p1->y + perp12.y * half_thickness;

            gp_vec2f_t linep31;
            linep31.x = p2->x + perp12.x * half_thickness;
            linep31.y = p2->y + perp12.y * half_thickness;

            gp_linef_t line12r;
            __make_line_from_two_point_v2( &line12r, &linep30, &linep31 );

            gp_vec2f_t pr;
            if( __intersect_line( &line01r, &line12r, &pr ) == GP_FALSE )
            {
                pr.x = p1->x + perp01.x * half_thickness;
                pr.y = p1->y + perp01.y * half_thickness;
            }

            if( penumbra > 0.f )
            {
                gp_vec2f_t linep40;
                linep40.x = p0->x - perp01.x * half_thickness_soft;
                linep40.y = p0->y - perp01.y * half_thickness_soft;

                gp_vec2f_t linep41;
                linep41.x = p1->x - perp01.x * half_thickness_soft;
                linep41.y = p1->y - perp01.y * half_thickness_soft;

                gp_linef_t line01l_soft;
                __make_line_from_two_point_v2( &line01l_soft, &linep40, &linep41 );

                gp_vec2f_t linep50;
                linep50.x = p1->x - perp12.x * half_thickness_soft;
                linep50.y = p1->y - perp12.y * half_thickness_soft;

                gp_vec2f_t linep51;
                linep51.x = p2->x - perp12.x * half_thickness_soft;
                linep51.y = p2->y - perp12.y * half_thickness_soft;

                gp_linef_t line12l_soft;
                __make_line_from_two_point_v2( &line12l_soft, &linep50, &linep51 );

                gp_vec2f_t pl_soft;
                if( __intersect_line( &line01l_soft, &line12l_soft, &pl_soft ) == GP_FALSE )
                {
                    pl_soft.x = p1->x - perp01.x * half_thickness_soft;
                    pl_soft.y = p1->y - perp01.y * half_thickness_soft;
                }

                gp_vec2f_t linep60;
                linep60.x = p0->x + perp01.x * half_thickness_soft;
                linep60.y = p0->y + perp01.y * half_thickness_soft;

                gp_vec2f_t linep61;
                linep61.x = p1->x + perp01.x * half_thickness_soft;
                linep61.y = p1->y + perp01.y * half_thickness_soft;

                gp_linef_t line01r_soft;
                __make_line_from_two_point_v2( &line01r_soft, &linep60, &linep61 );

                gp_vec2f_t linep70;
                linep70.x = p1->x + perp12.x * half_thickness_soft;
                linep70.y = p1->y + perp12.y * half_thickness_soft;

                gp_vec2f_t linep71;
                linep71.x = p2->x + perp12.x * half_thickness_soft;
                linep71.y = p2->y + perp12.y * half_thickness_soft;

                gp_linef_t line12r_soft;
                __make_line_from_two_point_v2( &line12r_soft, &linep70, &linep71 );

                gp_vec2f_t pr_soft;
                if( __intersect_line( &line01r_soft, &line12r_soft, &pr_soft ) == GP_FALSE )
                {
                    pr_soft.x = p1->x + perp01.x * half_thickness_soft;
                    pr_soft.y = p1->y + perp01.y * half_thickness_soft;
                }

                GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, pl.x, pl.y) );
                GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb0 & 0x00ffffff) );
                GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 0, u, 0.f) );

                GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, pl_soft.x, pl_soft.y) );
                GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb0) );
                GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 1, u, 0.5f - uv_soft_offset) );

                GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, pr_soft.x, pr_soft.y) );
                GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb0) );
                GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 2, u, 0.5f + uv_soft_offset) );

                GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, pr.x, pr.y) );
                GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb0 & 0x00ffffff) );
                GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 3, u, 1.f) );

                vertex_iterator += 4;
            }
            else
            {
                if( outline_width > 0.f )
                {
                    gp_vec2f_t linep40;
                    linep40.x = p0->x - perp01.x * (half_thickness + outline_width);
                    linep40.y = p0->y - perp01.y * (half_thickness + outline_width);

                    gp_vec2f_t linep41;
                    linep41.x = p1->x - perp01.x * (half_thickness + outline_width);
                    linep41.y = p1->y - perp01.y * (half_thickness + outline_width);

                    gp_linef_t line01l_outline;
                    __make_line_from_two_point_v2( &line01l_outline, &linep40, &linep41 );

                    gp_vec2f_t linep50;
                    linep50.x = p1->x - perp12.x * (half_thickness + outline_width);
                    linep50.y = p1->y - perp12.y * (half_thickness + outline_width);

                    gp_vec2f_t linep51;
                    linep51.x = p2->x - perp12.x * (half_thickness + outline_width);
                    linep51.y = p2->y - perp12.y * (half_thickness + outline_width);

                    gp_linef_t line12l_outline;
                    __make_line_from_two_point_v2( &line12l_outline, &linep50, &linep51 );

                    gp_vec2f_t pl_outline;
                    if( __intersect_line( &line01l_outline, &line12l_outline, &pl_outline ) == GP_FALSE )
                    {
                        pl_outline.x = p1->x - perp01.x * (half_thickness + outline_width);
                        pl_outline.y = p1->y - perp01.y * (half_thickness + outline_width);
                    }

                    gp_vec2f_t linep60;
                    linep60.x = p0->x + perp01.x * (half_thickness + outline_width);
                    linep60.y = p0->y + perp01.y * (half_thickness + outline_width);

                    gp_vec2f_t linep61;
                    linep61.x = p1->x + perp01.x * (half_thickness + outline_width);
                    linep61.y = p1->y + perp01.y * (half_thickness + outline_width);

                    gp_linef_t line01r_outline;
                    __make_line_from_two_point_v2( &line01r_outline, &linep60, &linep61 );

                    gp_vec2f_t linep70;
                    linep70.x = p1->x + perp12.x * (half_thickness + outline_width);
                    linep70.y = p1->y + perp12.y * (half_thickness + outline_width);

                    gp_vec2f_t linep71;
                    linep71.x = p2->x + perp12.x * (half_thickness + outline_width);
                    linep71.y = p2->y + perp12.y * (half_thickness + outline_width);

                    gp_linef_t line12r_outline;
                    __make_line_from_two_point_v2( &line12r_outline, &linep70, &linep71 );

                    gp_vec2f_t pr_outline;
                    if( __intersect_line( &line01r_outline, &line12r_outline, &pr_outline ) == GP_FALSE )
                    {
                        pr_outline.x = p1->x + perp01.x * (half_thickness + outline_width);
                        pr_outline.y = p1->y + perp01.y * (half_thickness + outline_width);
                    }

                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, pl_outline.x, pl_outline.y) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, outline_argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 0, u, 0.f) );

                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, pl.x, pl.y) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, outline_argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 1, u, 0.f) );

                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, pl.x, pl.y) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 2, u, 0.f) );

                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, pr.x, pr.y) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 3, u, 1.f) );

                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 4, pr.x, pr.y) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 4, outline_argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 4, u, 1.f) );

                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 5, pr_outline.x, pr_outline.y) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 5, outline_argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 5, u, 1.f) );

                    vertex_iterator += 6;
                }
                else
                {
                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, pl.x, pl.y) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 0, u, 0.f) );

                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, pr.x, pr.y) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 1, u, 1.f) );

                    vertex_iterator += 2;
                }
            }
        }

        {
            const gp_vec2f_t * p0 = &GP_GET_POINT( points_cache.size - 2 )->p;
            const gp_vec2f_t * p1 = &GP_GET_POINT( points_cache.size - 1 )->p;

            gp_argb_t argb0 = GP_GET_POINT( points_cache.size - 2 )->argb;
            gp_argb_t outline_argb0 = GP_GET_POINT( points_cache.size - 2 )->outline_argb;

            gp_vec2f_t perp;
            __make_line_perp( &perp, p0, p1 );

            if( penumbra > 0.f )
            {
                GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p1->x - perp.x * half_thickness, p1->y - perp.y * half_thickness) );
                GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb0 & 0x00ffffff) );
                GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 0, 1.f, 0.f) );

                GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p1->x - perp.x * half_thickness_soft, p1->y - perp.y * half_thickness_soft) );
                GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb0) );
                GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 1, 1.f, 0.5f - uv_soft_offset) );

                GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p1->x + perp.x * half_thickness_soft, p1->y + perp.y * half_thickness_soft) );
                GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb0) );
                GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 2, 1.f, 0.5f + uv_soft_offset) );

                GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, p1->x + perp.x * half_thickness, p1->y + perp.y * half_thickness) );
                GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb0 & 0x00ffffff) );
                GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 3, 1.f, 1.f) );

                vertex_iterator += 4;
            }
            else
            {
                if( outline_width > 0.f )
                {
                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p1->x - perp.x * (half_thickness + outline_width), p1->y - perp.y * (half_thickness + outline_width)) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, outline_argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 0, 1.f, 0.f) );

                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p1->x - perp.x * half_thickness, p1->y - perp.y * half_thickness) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, outline_argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 1, 1.f, 0.f) );

                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p1->x - perp.x * half_thickness, p1->y - perp.y * half_thickness) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 2, 1.f, 0.f) );

                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, p1->x + perp.x * half_thickness, p1->y + perp.y * half_thickness) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 3, 1.f, 1.f) );

                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 4, p1->x + perp.x * half_thickness, p1->y + perp.y * half_thickness) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 4, outline_argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 4, 1.f, 1.f) );

                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 5, p1->x + perp.x * (half_thickness + outline_width), p1->y + perp.y * (half_thickness + outline_width)) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 5, outline_argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 5, 1.f, 1.f) );

                    vertex_iterator += 6;
                }
                else
                {
                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p1->x - perp.x * half_thickness, p1->y - perp.y * half_thickness) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 0, 1.f, 0.f) );

                    GP_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p1->x + perp.x * half_thickness, p1->y + perp.y * half_thickness) );
                    GP_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb0) );
                    GP_CALL( gp_mesh_uv, (_canvas, _mesh, vertex_iterator + 1, 1.f, 1.f) );

                    vertex_iterator += 2;
                }                
            }

#if defined(GP_DISABLE_EXTRA_LINE_POINTS)
            if( points_cache.extra_points != GP_NULLPTR )
            {
                GP_FREE( _canvas, points_cache.extra_points );
            }
#endif
        }
    }

#if defined(GP_DEBUG)
    gp_uint16_t test_vertex_count;
    gp_uint16_t test_index_count;

    if( __calculate_mesh_line_size( _canvas, &test_vertex_count, &test_index_count ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    if( test_vertex_count != vertex_iterator - *_vertex_iterator )
    {
        return GP_FAILURE;
    }

    if( test_index_count != index_iterator - *_index_iterator )
    {
        return GP_FAILURE;
    }
#endif

    * _vertex_iterator = vertex_iterator;
    *_index_iterator = index_iterator;

    return GP_SUCCESSFUL;
}