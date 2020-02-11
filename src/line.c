#include "line.h"

#include "struct.h"
#include "detail.h"

//////////////////////////////////////////////////////////////////////////
static gp_result_t __calculate_mesh_line_size( const gp_canvas_t * _canvas, gp_uint16_t * _vertex_count, gp_uint16_t * _index_count )
{
    gp_uint16_t vertex_count = 0;
    gp_uint16_t index_count = 0;

    for( const gp_line_t * l = _canvas->lines; l != GP_NULLPTR; l = l->next )
    {
        if( l->edges == GP_NULLPTR )
        {
            continue;
        }

        uint8_t curve_quality = l->state.curve_quality;

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

        if( l->state.penumbra > 0.f )
        {
            vertex_count += point_count * 4;
            index_count += (point_count - 1) * 18;
        }
        else
        {
            vertex_count += point_count * 2;
            index_count += (point_count - 1) * 6;
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

    if( __calculate_mesh_line_size( _canvas, &vertex_count, &index_count ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    _mesh->vertex_count += vertex_count;
    _mesh->index_count += index_count;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static float __vec2f_sqrlength( const gp_vec2f_t * _p )
{
    return _p->x * _p->x + _p->y * _p->y;
}
//////////////////////////////////////////////////////////////////////////
static float __vec2f_length( const gp_vec2f_t * _p )
{
    float l = __vec2f_sqrlength( _p );

    return GP_MATH_SQRTF( l );
}
//////////////////////////////////////////////////////////////////////////
static void __vec2f_normalize( gp_vec2f_t * _o, const gp_vec2f_t * _p )
{
    float l = __vec2f_length( _p );

    float l_inv = 1.f / l;

    _o->x = _p->x * l_inv;
    _o->y = _p->y * l_inv;
}
//////////////////////////////////////////////////////////////////////////
static void __vec2f_perp( gp_vec2f_t * _out, const gp_vec2f_t * _in )
{
    _out->x = -_in->y;
    _out->y = _in->x;
}
//////////////////////////////////////////////////////////////////////////
static void __make_line_perp( gp_vec2f_t * _perp, const gp_vec2f_t * _from, const gp_vec2f_t * _to )
{
    gp_vec2f_t dir;
    dir.x = _to->x - _from->x;
    dir.y = _to->y - _from->y;

    gp_vec2f_t dir_norm;
    __vec2f_normalize( &dir_norm, &dir );

    __vec2f_perp( _perp, &dir_norm );
}
//////////////////////////////////////////////////////////////////////////
static float __integral_powf( float _value, gp_uint32_t _count )
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
static float __factorialf( gp_uint32_t _value )
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
static void __calculate_bezier_position( gp_vec2f_t * _out, const gp_vec2f_t * _p0, const gp_vec2f_t * _p1, const gp_vec2f_t * _v, gp_uint32_t _n, float _dt )
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
static void __make_line_from_two_point_v2( gp_linef_t * _line, const gp_vec2f_t * _a, const gp_vec2f_t * _b )
{
    _line->a = _a->y - _b->y;
    _line->b = _b->x - _a->x;
    _line->c = _a->x * _b->y - _b->x * _a->y;
}
//////////////////////////////////////////////////////////////////////////
static float __cross2( float _a, float _b, float _c, float _d )
{
    return _a * _d - _c * _b;
}
//////////////////////////////////////////////////////////////////////////
static gp_bool_t __intersect_line( const gp_linef_t * _l1, const gp_linef_t * _l2, gp_vec2f_t * _p )
{
    float zn = __cross2( _l1->a, _l1->b, _l2->a, _l2->b );

    if( GP_EQUAL_F( zn, 0.00001f ) == GP_TRUE )
    {
        return GP_FALSE;
    }

    float inv_zn = 1.f / zn;

    _p->x = -__cross2( _l1->c, _l1->b, _l2->c, _l2->b ) * inv_zn;
    _p->y = -__cross2( _l1->a, _l1->c, _l2->a, _l2->c ) * inv_zn;

    return GP_TRUE;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_render_line( const gp_canvas_t * _canvas, const gp_mesh_t * _mesh, gp_uint16_t * _vertex_iterator, gp_uint16_t * _index_iterator )
{
    gp_uint16_t vertex_iterator = *_vertex_iterator;
    gp_uint16_t index_iterator = *_index_iterator;

    for( const gp_line_t * l = _canvas->lines; l != GP_NULLPTR; l = l->next )
    {
        if( l->edges == GP_NULLPTR )
        {
            continue;
        }

        float line_width = l->state.line_width;
        float half_line_width = line_width * 0.5f;
        uint8_t curve_quality = l->state.curve_quality;
        float curve_quality_inv = l->state.curve_quality_inv;

        gp_line_points_t points[GP_LINE_POINTS_MAX];
        gp_uint16_t points_size = 0;

        const gp_point_t * point_iterator = l->points;

        for( const gp_line_edge_t * e = l->edges; e != GP_NULLPTR; e = e->next )
        {
            const gp_point_t * p0 = point_iterator;
            point_iterator = point_iterator->next;
            const gp_point_t * p1 = point_iterator;

            gp_color_t line_color;
            gp_color_mul( &line_color, &_mesh->color, &l->state.color );
            gp_uint32_t argb = gp_color_argb( &line_color );

            switch( e->controls )
            {
            case 0:
                {
                    gp_line_points_t * p = points + points_size;
                    p->p = p0->p;
                    p->argb = argb;

                    ++points_size;
                }break;
            case 1:
                {
                    float t = 0.f;

                    for( gp_uint8_t index = 0; index != curve_quality; ++index )
                    {
                        gp_vec2f_t bp;
                        __calculate_bezier_position( &bp, &p0->p, &p1->p, e->p, 1, t );

                        t += curve_quality_inv;

                        gp_line_points_t * p = points + points_size;
                        p->p = bp;
                        p->argb = argb;

                        ++points_size;
                    }
                }break;
            case 2:
                {
                    float t = 0.f;

                    for( gp_uint8_t index = 0; index != curve_quality; ++index )
                    {
                        gp_vec2f_t bp;
                        __calculate_bezier_position( &bp, &p0->p, &p1->p, e->p, 2, t );

                        t += curve_quality_inv;

                        gp_line_points_t * p = points + points_size;
                        p->p = bp;
                        p->argb = argb;

                        ++points_size;
                    }
                }break;
            default:
                return GP_FAILURE;
            }
        }

        {
            const gp_point_t * p1 = point_iterator;

            gp_line_points_t * p = points + points_size;
            p->p = p1->p;

            gp_color_t line_color;
            gp_color_mul( &line_color, &_mesh->color, &l->state.color );
            gp_uint32_t argb = gp_color_argb( &line_color );

            p->argb = argb;

            ++points_size;
        }

        float penumbra = l->state.penumbra;

        if( penumbra > 0.f )
        {
            for( gp_uint16_t index = 0; index != points_size - 1; ++index )
            {
                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index * 4 + 0 );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index * 4 + 1 );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + index * 4 + 4 );
                gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + index * 4 + 4 );
                gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index * 4 + 1 );
                gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + index * 4 + 5 );

                index_iterator += 6;

                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index * 4 + 1 );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index * 4 + 2 );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + index * 4 + 5 );
                gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + index * 4 + 5 );
                gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index * 4 + 2 );
                gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + index * 4 + 6 );

                index_iterator += 6;

                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index * 4 + 2 );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index * 4 + 3 );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + index * 4 + 6 );
                gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + index * 4 + 6 );
                gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index * 4 + 3 );
                gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + index * 4 + 7 );

                index_iterator += 6;
            }
        }
        else
        {
            for( gp_uint16_t index = 0; index != points_size - 1; ++index )
            {
                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index * 2 + 0 );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index * 2 + 1 );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + index * 2 + 2 );
                gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + index * 2 + 2 );
                gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index * 2 + 1 );
                gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + index * 2 + 3 );

                index_iterator += 6;
            }
        }

        {
            gp_line_points_t * point0 = points + 0;
            gp_line_points_t * point1 = points + 1;

            gp_uint32_t argb = point0->argb;

            const gp_vec2f_t * p0 = &point0->p;
            const gp_vec2f_t * p1 = &point1->p;

            gp_vec2f_t perp;
            __make_line_perp( &perp, p0, p1 );

            if( penumbra > 0.f )
            {
                float line_width_soft = half_line_width - penumbra;
                float uv_soft_offset = line_width_soft / half_line_width * 0.5f;

                gp_mesh_position( _mesh, vertex_iterator + 0, p0->x - perp.x * half_line_width, p0->y - perp.y * half_line_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );
                gp_mesh_uv( _mesh, vertex_iterator + 0, 0.f, 0.f );

                gp_mesh_position( _mesh, vertex_iterator + 1, p0->x - perp.x * line_width_soft, p0->y - perp.y * line_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );
                gp_mesh_uv( _mesh, vertex_iterator + 1, 0.f, 0.5f - uv_soft_offset );

                gp_mesh_position( _mesh, vertex_iterator + 2, p0->x + perp.x * line_width_soft, p0->y + perp.y * line_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 2, argb );
                gp_mesh_uv( _mesh, vertex_iterator + 2, 0.f, 0.5f + uv_soft_offset );

                gp_mesh_position( _mesh, vertex_iterator + 3, p0->x + perp.x * half_line_width, p0->y + perp.y * half_line_width );
                gp_mesh_color( _mesh, vertex_iterator + 3, argb & 0x00ffffff );
                gp_mesh_uv( _mesh, vertex_iterator + 2, 0.f, 1.f );

                vertex_iterator += 4;
            }
            else
            {
                gp_mesh_position( _mesh, vertex_iterator + 0, p0->x - perp.x * half_line_width, p0->y - perp.y * half_line_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );
                gp_mesh_uv( _mesh, vertex_iterator + 0, 0.f, 0.f );

                gp_mesh_position( _mesh, vertex_iterator + 1, p0->x + perp.x * half_line_width, p0->y + perp.y * half_line_width );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );
                gp_mesh_uv( _mesh, vertex_iterator + 1, 0.f, 1.f );

                vertex_iterator += 2;
            }
        }

        for( gp_uint16_t index = 1; index != points_size - 1; ++index )
        {
            gp_uint32_t argb = points[index + 0].argb;

            const gp_vec2f_t * p0 = &points[index - 1].p;
            const gp_vec2f_t * p1 = &points[index + 0].p;
            const gp_vec2f_t * p2 = &points[index + 1].p;

            gp_vec2f_t perp01;
            __make_line_perp( &perp01, p0, p1 );

            gp_vec2f_t perp12;
            __make_line_perp( &perp12, p1, p2 );

            gp_vec2f_t linep00;
            linep00.x = p0->x - perp01.x * half_line_width;
            linep00.y = p0->y - perp01.y * half_line_width;

            gp_vec2f_t linep01;
            linep01.x = p1->x - perp01.x * half_line_width;
            linep01.y = p1->y - perp01.y * half_line_width;

            gp_linef_t line01l;
            __make_line_from_two_point_v2( &line01l, &linep00, &linep01 );

            gp_vec2f_t linep10;
            linep10.x = p1->x - perp12.x * half_line_width;
            linep10.y = p1->y - perp12.y * half_line_width;

            gp_vec2f_t linep11;
            linep11.x = p2->x - perp12.x * half_line_width;
            linep11.y = p2->y - perp12.y * half_line_width;

            gp_linef_t line12l;
            __make_line_from_two_point_v2( &line12l, &linep10, &linep11 );

            gp_vec2f_t pl;
            if( __intersect_line( &line01l, &line12l, &pl ) == GP_FALSE )
            {
                pl.x = p1->x - perp01.x * half_line_width;
                pl.y = p1->y - perp01.y * half_line_width;
            }

            gp_vec2f_t linep20;
            linep20.x = p0->x + perp01.x * half_line_width;
            linep20.y = p0->y + perp01.y * half_line_width;

            gp_vec2f_t linep21;
            linep21.x = p1->x + perp01.x * half_line_width;
            linep21.y = p1->y + perp01.y * half_line_width;

            gp_linef_t line01r;
            __make_line_from_two_point_v2( &line01r, &linep20, &linep21 );

            gp_vec2f_t linep30;
            linep30.x = p1->x + perp12.x * half_line_width;
            linep30.y = p1->y + perp12.y * half_line_width;

            gp_vec2f_t linep31;
            linep31.x = p2->x + perp12.x * half_line_width;
            linep31.y = p2->y + perp12.y * half_line_width;

            gp_linef_t line12r;
            __make_line_from_two_point_v2( &line12r, &linep30, &linep31 );

            gp_vec2f_t pr;
            if( __intersect_line( &line01r, &line12r, &pr ) == GP_FALSE )
            {
                pr.x = p1->x + perp01.x * half_line_width;
                pr.y = p1->y + perp01.y * half_line_width;
            }

            if( penumbra > 0.f )
            {
                float line_width_soft = half_line_width - penumbra;
                float uv_soft_offset = line_width_soft / half_line_width * 0.5f;

                gp_vec2f_t linep40;
                linep40.x = p0->x - perp01.x * line_width_soft;
                linep40.y = p0->y - perp01.y * line_width_soft;

                gp_vec2f_t linep41;
                linep41.x = p1->x - perp01.x * line_width_soft;
                linep41.y = p1->y - perp01.y * line_width_soft;

                gp_linef_t line01l_soft;
                __make_line_from_two_point_v2( &line01l_soft, &linep40, &linep41 );

                gp_vec2f_t linep50;
                linep50.x = p1->x - perp12.x * line_width_soft;
                linep50.y = p1->y - perp12.y * line_width_soft;

                gp_vec2f_t linep51;
                linep51.x = p2->x - perp12.x * line_width_soft;
                linep51.y = p2->y - perp12.y * line_width_soft;

                gp_linef_t line12l_soft;
                __make_line_from_two_point_v2( &line12l_soft, &linep50, &linep51 );

                gp_vec2f_t pl_soft;
                if( __intersect_line( &line01l_soft, &line12l_soft, &pl_soft ) == GP_FALSE )
                {
                    pl_soft.x = p1->x - perp01.x * line_width_soft;
                    pl_soft.y = p1->y - perp01.y * line_width_soft;
                }

                gp_vec2f_t linep60;
                linep60.x = p0->x + perp01.x * line_width_soft;
                linep60.y = p0->y + perp01.y * line_width_soft;

                gp_vec2f_t linep61;
                linep61.x = p1->x + perp01.x * line_width_soft;
                linep61.y = p1->y + perp01.y * line_width_soft;

                gp_linef_t line01r_soft;
                __make_line_from_two_point_v2( &line01r_soft, &linep60, &linep61 );

                gp_vec2f_t linep70;
                linep70.x = p1->x + perp12.x * line_width_soft;
                linep70.y = p1->y + perp12.y * line_width_soft;

                gp_vec2f_t linep71;
                linep71.x = p2->x + perp12.x * line_width_soft;
                linep71.y = p2->y + perp12.y * line_width_soft;

                gp_linef_t line12r_soft;
                __make_line_from_two_point_v2( &line12r_soft, &linep70, &linep71 );

                gp_vec2f_t pr_soft;
                if( __intersect_line( &line01r_soft, &line12r_soft, &pr_soft ) == GP_FALSE )
                {
                    pr_soft.x = p1->x + perp01.x * line_width_soft;
                    pr_soft.y = p1->y + perp01.y * line_width_soft;
                }

                gp_mesh_position( _mesh, vertex_iterator + 0, pl.x, pl.y );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );
                gp_mesh_uv( _mesh, vertex_iterator + 0, (float)index / (float)points_size, 0.f );

                gp_mesh_position( _mesh, vertex_iterator + 1, pl_soft.x, pl_soft.y );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );
                gp_mesh_uv( _mesh, vertex_iterator + 1, (float)index / (float)points_size, 0.5f - uv_soft_offset );

                gp_mesh_position( _mesh, vertex_iterator + 2, pr_soft.x, pr_soft.y );
                gp_mesh_color( _mesh, vertex_iterator + 2, argb );
                gp_mesh_uv( _mesh, vertex_iterator + 2, (float)index / (float)points_size, 0.5f + uv_soft_offset );

                gp_mesh_position( _mesh, vertex_iterator + 3, pr.x, pr.y );
                gp_mesh_color( _mesh, vertex_iterator + 3, argb & 0x00ffffff );
                gp_mesh_uv( _mesh, vertex_iterator + 3, (float)index / (float)points_size, 1.f );

                vertex_iterator += 4;
            }
            else
            {
                gp_mesh_position( _mesh, vertex_iterator + 0, pl.x, pl.y );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );
                gp_mesh_uv( _mesh, vertex_iterator + 0, (float)index / (float)points_size, 0.f );

                gp_mesh_position( _mesh, vertex_iterator + 1, pr.x, pr.y );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );
                gp_mesh_uv( _mesh, vertex_iterator + 1, (float)index / (float)points_size, 1.f );

                vertex_iterator += 2;
            }
        }

        {
            gp_uint32_t argb = points[points_size - 2].argb;

            const gp_vec2f_t * p0 = &points[points_size - 2].p;
            const gp_vec2f_t * p1 = &points[points_size - 1].p;

            gp_vec2f_t perp;
            __make_line_perp( &perp, p0, p1 );

            if( penumbra > 0.f )
            {
                float line_width_soft = half_line_width - penumbra;
                float uv_soft_offset = line_width_soft / half_line_width * 0.5f;

                gp_mesh_position( _mesh, vertex_iterator + 0, p1->x - perp.x * half_line_width, p1->y - perp.y * half_line_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );
                gp_mesh_uv( _mesh, vertex_iterator + 0, 1.f, 0.f );

                gp_mesh_position( _mesh, vertex_iterator + 1, p1->x - perp.x * line_width_soft, p1->y - perp.y * line_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );
                gp_mesh_uv( _mesh, vertex_iterator + 1, 1.f, 0.5f - uv_soft_offset );

                gp_mesh_position( _mesh, vertex_iterator + 2, p1->x + perp.x * line_width_soft, p1->y + perp.y * line_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 2, argb );
                gp_mesh_uv( _mesh, vertex_iterator + 2, 1.f, 0.5f + uv_soft_offset );

                gp_mesh_position( _mesh, vertex_iterator + 3, p1->x + perp.x * half_line_width, p1->y + perp.y * half_line_width );
                gp_mesh_color( _mesh, vertex_iterator + 3, argb & 0x00ffffff );
                gp_mesh_uv( _mesh, vertex_iterator + 3, 1.f, 1.f );

                vertex_iterator += 4;
            }
            else
            {
                gp_mesh_position( _mesh, vertex_iterator + 0, p1->x - perp.x * half_line_width, p1->y - perp.y * half_line_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );
                gp_mesh_uv( _mesh, vertex_iterator + 0, 1.f, 0.f );

                gp_mesh_position( _mesh, vertex_iterator + 1, p1->x + perp.x * half_line_width, p1->y + perp.y * half_line_width );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );
                gp_mesh_uv( _mesh, vertex_iterator + 1, 1.f, 1.f );

                vertex_iterator += 2;
            }
        }
    }

#ifdef GP_DEBUG
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