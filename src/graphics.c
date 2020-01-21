#include "graphics/graphics.h"

#include "struct.h"
#include "detail.h"

#include <math.h>

//////////////////////////////////////////////////////////////////////////
static void __canvas_default_setup( gp_canvas_t * _canvas )
{
    _canvas->line_width = 2.f;
    _canvas->line_penumbra = 1.f;
    _canvas->line_color.r = 1.f;
    _canvas->line_color.g = 1.f;
    _canvas->line_color.b = 1.f;
    _canvas->line_color.a = 1.f;
    _canvas->fill_color.r = 1.f;
    _canvas->fill_color.g = 1.f;
    _canvas->fill_color.b = 1.f;
    _canvas->fill_color.a = 1.f;
    _canvas->fill = GP_FALSE;
    _canvas->curve_quality = 20;
    _canvas->ellipse_quality = 32;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_canvas_create( gp_canvas_t ** _canvas, gp_malloc_t _malloc, gp_free_t _free, void * _ud )
{
    gp_canvas_t * canvas = (gp_canvas_t *)(*_malloc)(sizeof( gp_canvas_t ), _ud);

    canvas->lines = GP_NULLPTR;
    canvas->rects = GP_NULLPTR;
    canvas->rounded_rects = GP_NULLPTR;
    canvas->ellipses = GP_NULLPTR;

    __canvas_default_setup( canvas );

    canvas->malloc = _malloc;
    canvas->free = _free;
    canvas->ud = _ud;

    *_canvas = canvas;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_canvas_destroy( gp_canvas_t * _canvas )
{
    GP_FREE( _canvas, _canvas );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_canvas_clear( gp_canvas_t * _canvas )
{
    GP_LIST_DESTROY( _canvas, gp_line_t, _canvas->lines );
    GP_LIST_DESTROY( _canvas, gp_rect_t, _canvas->rects );
    GP_LIST_DESTROY( _canvas, gp_rounded_rect_t, _canvas->rounded_rects );
    GP_LIST_DESTROY( _canvas, gp_ellipse_t, _canvas->ellipses );

    __canvas_default_setup( _canvas );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_line_width( gp_canvas_t * _canvas, float _width )
{
    _canvas->line_width = _width;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_line_width( gp_canvas_t * _canvas, float * _width )
{
    *_width = _canvas->line_width;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_line_penumbra( gp_canvas_t * _canvas, float _penumbra )
{
    _canvas->line_penumbra = _penumbra;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_line_penumbra( gp_canvas_t * _canvas, float * _penumbra )
{
    *_penumbra = _canvas->line_penumbra;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_line_color( gp_canvas_t * _canvas, gp_color_t _color )
{
    _canvas->line_color = _color;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_line_color( gp_canvas_t * _canvas, gp_color_t * _color )
{
    *_color = _canvas->line_color;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_curve_quality( gp_canvas_t * _canvas, uint8_t _quality )
{
    _canvas->curve_quality = _quality;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_curve_quality( gp_canvas_t * _canvas, uint8_t * _quality )
{
    *_quality = _canvas->curve_quality;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_ellipse_quality( gp_canvas_t * _canvas, uint8_t _quality )
{
    _canvas->ellipse_quality = _quality;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_ellipse_quality( gp_canvas_t * _canvas, uint8_t * _quality )
{
    *_quality = _canvas->ellipse_quality;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_begin_fill( gp_canvas_t * _canvas )
{
    _canvas->fill = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_end_fill( gp_canvas_t * _canvas )
{
    _canvas->fill = GP_FALSE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_move_to( gp_canvas_t * _canvas, float _x, float _y )
{
    gp_point_t * p = GP_NEW( _canvas, gp_point_t );
    p->p.x = _x;
    p->p.y = _y;
    p->next = GP_NULLPTR;
    p->prev = GP_NULLPTR;

    gp_line_t * l = GP_NEW( _canvas, gp_line_t );
    l->points = GP_NULLPTR;
    GP_LIST_PUSHBACK( gp_point_t, l->points, p );
    l->edges = GP_NULLPTR;
    l->penumbra = _canvas->line_penumbra;
    l->next = GP_NULLPTR;
    l->prev = GP_NULLPTR;

    GP_LIST_PUSHBACK( gp_line_t, _canvas->lines, l );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_line_to( gp_canvas_t * _canvas, float _x, float _y )
{
    if( _canvas->lines == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    gp_line_t * l = GP_LIST_BACK( _canvas->lines );

    gp_point_t * p = GP_NEW( _canvas, gp_point_t );
    p->p.x = _x;
    p->p.y = _y;
    p->next = GP_NULLPTR;
    p->prev = GP_NULLPTR;

    GP_LIST_PUSHBACK( gp_point_t, l->points, p );

    gp_line_edge_t * e = GP_NEW( _canvas, gp_line_edge_t );

    e->controls = 0;
    e->quality = 2;
    e->dt = 1.f;
    e->line_width = _canvas->line_width;
    e->line_color = _canvas->line_color;
    e->next = GP_NULLPTR;
    e->prev = GP_NULLPTR;

    GP_LIST_PUSHBACK( gp_line_edge_t, l->edges, e );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_quadratic_curve_to( gp_canvas_t * _canvas, float _p0x, float _p0y, float _x, float _y )
{
    gp_point_t * p = GP_NEW( _canvas, gp_point_t );
    p->p.x = _x;
    p->p.y = _y;
    p->next = GP_NULLPTR;
    p->prev = GP_NULLPTR;

    gp_line_t * line_back = GP_LIST_BACK( _canvas->lines );
    GP_LIST_PUSHBACK( gp_point_t, line_back->points, p );

    gp_line_edge_t * edge = GP_NEW( _canvas, gp_line_edge_t );
    edge->controls = 1;
    edge->p[0].x = _p0x;
    edge->p[1].y = _p0y;
    edge->quality = _canvas->curve_quality;
    edge->dt = 1.f / (float)(_canvas->curve_quality - 1);
    edge->line_width = _canvas->line_width;
    edge->line_color = _canvas->line_color;

    GP_LIST_PUSHBACK( gp_line_edge_t, line_back->edges, edge );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_bezier_curve_to( gp_canvas_t * _canvas, float _p0x, float _p0y, float _p1x, float _p1y, float _x, float _y )
{
    gp_point_t * p = GP_NEW( _canvas, gp_point_t );
    p->p.x = _x;
    p->p.y = _y;
    p->next = GP_NULLPTR;
    p->prev = GP_NULLPTR;

    gp_line_t * line_back = GP_LIST_BACK( _canvas->lines );
    GP_LIST_PUSHBACK( gp_point_t, line_back->points, p );

    gp_line_edge_t * edge = GP_NEW( _canvas, gp_line_edge_t );
    edge->controls = 2;
    edge->p[0].x = _p0x;
    edge->p[0].y = _p0y;
    edge->p[1].x = _p1x;
    edge->p[1].y = _p1y;
    edge->quality = _canvas->curve_quality;
    edge->dt = 1.f / (float)(_canvas->curve_quality - 1);
    edge->line_width = _canvas->line_width;
    edge->line_color = _canvas->line_color;

    GP_LIST_PUSHBACK( gp_line_edge_t, line_back->edges, edge );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_draw_rect( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height )
{
    gp_rect_t * rect = GP_NEW( _canvas, gp_rect_t );
    rect->point.x = _x;
    rect->point.y = _y;
    rect->width = _width;
    rect->height = _height;
    rect->line_width = _canvas->line_width;
    rect->line_penumbra = _canvas->line_penumbra;
    rect->line_color = _canvas->line_color;
    rect->fill_color = _canvas->fill_color;
    rect->fill = _canvas->fill;

    GP_LIST_PUSHBACK( gp_rect_t, _canvas->rects, rect );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_draw_rounded_rect( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _radius )
{
    gp_rounded_rect_t * rounded_rect = GP_NEW( _canvas, gp_rounded_rect_t );

    rounded_rect->point.x = _x;
    rounded_rect->point.y = _y;
    rounded_rect->width = _width;
    rounded_rect->height = _height;
    rounded_rect->radius = _radius;
    rounded_rect->quality = (_canvas->ellipse_quality + 3) / 4;
    rounded_rect->line_width = _canvas->line_width;
    rounded_rect->line_penumbra = _canvas->line_penumbra;
    rounded_rect->line_color = _canvas->line_color;
    rounded_rect->fill_color = _canvas->fill_color;
    rounded_rect->fill = _canvas->fill;

    GP_LIST_PUSHBACK( gp_rounded_rect_t, _canvas->rounded_rects, rounded_rect );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_draw_circle( gp_canvas_t * _canvas, float _x, float _y, float _radius )
{
    gp_result_t result = gp_draw_ellipse( _canvas, _x, _y, _radius, _radius );

    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_draw_ellipse( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height )
{
    gp_ellipse_t * ellipse = GP_NEW( _canvas, gp_ellipse_t );

    ellipse->point.x = _x;
    ellipse->point.y = _y;
    ellipse->width = _width;
    ellipse->height = _height;
    ellipse->quality = _canvas->ellipse_quality;
    ellipse->line_width = _canvas->line_width;
    ellipse->line_penumbra = _canvas->line_penumbra;
    ellipse->line_color = _canvas->line_color;
    ellipse->fill_color = _canvas->fill_color;
    ellipse->fill = _canvas->fill;

    GP_LIST_PUSHBACK( gp_ellipse_t, _canvas->ellipses, ellipse );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_calculate_mesh_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh )
{
    uint32_t vertex_count = 0;
    uint16_t index_count = 0;

    for( const gp_line_t * l = _canvas->lines; l != GP_NULLPTR; l = l->next )
    {
        if( l->edges == GP_NULLPTR )
        {
            continue;
        }

        uint16_t point_count = 0;

        for( const gp_line_edge_t * e = l->edges; e != GP_NULLPTR; e = e->next )
        {
            point_count += e->quality - 1;
        }

        point_count += 1;

        if( l->penumbra > 0.f )
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

    for( const gp_rect_t * r = _canvas->rects; r != GP_NULLPTR; r = r->next )
    {
        if( r->line_penumbra > 0.f )
        {
            vertex_count += 16;
            index_count += 72;
        }
        else
        {
            vertex_count += 8;
            index_count += 24;
        }

        if( r->fill == GP_TRUE )
        {
            vertex_count += 4;
            index_count += 6;
        }
    }

    for( const gp_rounded_rect_t * r = _canvas->rounded_rects; r != GP_NULLPTR; r = r->next )
    {
        if( r->line_penumbra > 0.f )
        {
            vertex_count += 32;
            index_count += 72;

            for( uint32_t index = 0; index != 4; ++index )
            {
                vertex_count += (r->quality + 1) * 4;
                index_count += r->quality * 6 * 3;
            }
        }
        else
        {
            vertex_count += 16;
            index_count += 24;

            for( uint32_t index = 0; index != 4; ++index )
            {
                vertex_count += (r->quality + 1) * 2;
                index_count += r->quality * 6;
            }
        }

        if( r->fill == GP_TRUE )
        {
            for( uint32_t index = 0; index != 4; ++index )
            {
                vertex_count += (r->quality + 1) + 1;
                index_count += r->quality * 3;
            }

            vertex_count += 12;
            index_count += 6 * 5;
        }
    }

    for( const gp_ellipse_t * e = _canvas->ellipses; e != GP_NULLPTR; e = e->next )
    {
        if( e->line_penumbra > 0.f )
        {
            vertex_count += e->quality * 4;
            index_count += e->quality * 18;
        }
        else
        {
            vertex_count += e->quality * 2;
            index_count += e->quality * 6;
        }

        if( e->fill == GP_TRUE )
        {
            vertex_count += 1;
            vertex_count += e->quality;

            index_count += e->quality * 3;
        }
    }

    _mesh->vertex_count = vertex_count;
    _mesh->index_count = index_count;

    _mesh->color.r = 1.f;
    _mesh->color.g = 1.f;
    _mesh->color.b = 1.f;
    _mesh->color.a = 1.f;
    
    _mesh->positions_buffer = GP_NULLPTR;
    _mesh->colors_buffer = GP_NULLPTR;
    _mesh->indices_buffer = GP_NULLPTR;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static float __integral_powf( float _value, uint32_t _count )
{
    if( _count == 0 )
    {
        return 1.f;
    }

    float f = _value;

    for( uint32_t i = 1; i != _count; ++i )
    {
        f *= _value;
    }

    return f;
}
//////////////////////////////////////////////////////////////////////////
static float __factorialf( uint32_t _value )
{
    if( _value == 0 )
    {
        return 1.f;
    }

    float f = 1.f;
    float d = 0.f;

    for( uint32_t i = 0; i != _value; ++i )
    {
        d += 1.f;

        f *= d;
    }

    return f;
}
//////////////////////////////////////////////////////////////////////////
static void __calculate_bezier_position( gp_vec2f_t * _out, const gp_vec2f_t * _p0, const gp_vec2f_t * _p1, const gp_vec2f_t * _v, uint32_t _n, float _dt )
{
    uint32_t n = _n + 1;

    float t0 = __integral_powf( 1.f - _dt, n );
    float tn = __integral_powf( _dt, n );

    _out->x = t0 * _p0->x + tn * _p1->x;
    _out->y = t0 * _p0->y + tn * _p1->y;

    float f_count = __factorialf( n );

    for( uint32_t i = 1; i != n; ++i )
    {
        float c = f_count / (__factorialf( i ) * __factorialf( n - i ));
        float t = __integral_powf( _dt, i ) * __integral_powf( 1.f - _dt, n - i );

        const gp_vec2f_t * v = _v + i - 1;

        _out->x += c * t * v->x;
        _out->y += c * t * v->y;
    }
}
//////////////////////////////////////////////////////////////////////////
static uint16_t * __mesh_index( const gp_mesh_t * _mesh, uint16_t _iterator )
{
    return (uint16_t *)((uint8_t *)_mesh->indices_buffer + _mesh->indices_offset + _mesh->indices_stride * _iterator);
}
//////////////////////////////////////////////////////////////////////////
static gp_vec2f_t * __mesh_position( const gp_mesh_t * _mesh, uint16_t _iterator )
{
    return (gp_vec2f_t *)((uint8_t *)_mesh->positions_buffer + _mesh->positions_offset + _mesh->positions_stride * _iterator);
}
//////////////////////////////////////////////////////////////////////////
static uint32_t * __mesh_color( const gp_mesh_t * _mesh, uint16_t _iterator )
{
    return (uint32_t *)((uint8_t *)_mesh->colors_buffer + _mesh->colors_offset + _mesh->colors_stride * _iterator);
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

    return sqrtf( l );
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
gp_result_t gp_render( const gp_canvas_t * _canvas, const gp_mesh_t * _mesh )
{
    uint16_t vertex_iterator = 0;
    uint16_t index_iterator = 0;

    for( const gp_line_t * l = _canvas->lines; l != GP_NULLPTR; l = l->next )
    {
        if( l->edges == GP_NULLPTR )
        {
            continue;
        }

        gp_line_points_t points[GP_LINE_POINTS_MAX];
        uint16_t points_size = 0;

        const gp_point_t * point_iterator = l->points;

        for( const gp_line_edge_t * e = l->edges; e != GP_NULLPTR; e = e->next )
        {
            const gp_point_t * p0 = point_iterator;
            point_iterator = point_iterator->next;
            const gp_point_t * p1 = point_iterator;

            gp_color_t line_color;
            gp_color_mul( &line_color, &_mesh->color, &e->line_color );
            uint32_t argb = gp_color_argb( &line_color );

            switch( e->controls )
            {
            case 0:
                {
                    gp_line_points_t * p = points + points_size;
                    p->p = p0->p;
                    p->width = e->line_width;
                    p->argb = argb;

                    ++points_size;
                }break;
            case 1:
                {
                    float t = 0.f;

                    for( uint8_t index = 0; index != e->quality - 1; ++index )
                    {
                        gp_vec2f_t bp;
                        __calculate_bezier_position( &bp, &p0->p, &p1->p, e->p, 1, t );

                        t += e->dt;

                        gp_line_points_t * p = points + points_size;
                        p->p = bp;
                        p->width = e->line_width;
                        p->argb = argb;

                        ++points_size;
                    }
                }break;
            case 2:
                {
                    float t = 0.f;

                    for( uint8_t index = 0; index != e->quality - 1; ++index )
                    {
                        gp_vec2f_t bp;
                        __calculate_bezier_position( &bp, &p0->p, &p1->p, e->p, 2, t );

                        t += e->dt;

                        gp_line_points_t * p = points + points_size;
                        p->p = bp;
                        p->width = e->line_width;
                        p->argb = argb;

                        ++points_size;
                    }
                }break;
            }
        }

        {
            const gp_point_t * p1 = point_iterator;

            gp_line_points_t * p = points + points_size;
            p->p = p1->p;

            gp_line_edge_t * edge_back = GP_LIST_BACK( l->edges );
            p->width = edge_back->line_width;

            gp_color_t line_color;
            gp_color_mul( &line_color, &_mesh->color, &edge_back->line_color );
            uint32_t argb = gp_color_argb( &line_color );

            p->argb = argb;

            ++points_size;
        }

        float penumbra = l->penumbra;

        if( penumbra > 0.f )
        {
            for( uint16_t index = 0; index != points_size - 1; ++index )
            {
                *__mesh_index( _mesh, index_iterator + 0 ) = vertex_iterator + index * 4 + 0;
                *__mesh_index( _mesh, index_iterator + 0 ) = vertex_iterator + index * 4 + 0;
                *__mesh_index( _mesh, index_iterator + 1 ) = vertex_iterator + index * 4 + 1;
                *__mesh_index( _mesh, index_iterator + 2 ) = vertex_iterator + index * 4 + 4;
                *__mesh_index( _mesh, index_iterator + 3 ) = vertex_iterator + index * 4 + 4;
                *__mesh_index( _mesh, index_iterator + 4 ) = vertex_iterator + index * 4 + 1;
                *__mesh_index( _mesh, index_iterator + 5 ) = vertex_iterator + index * 4 + 5;

                index_iterator += 6;

                *__mesh_index( _mesh, index_iterator + 0 ) = vertex_iterator + index * 4 + 1;
                *__mesh_index( _mesh, index_iterator + 1 ) = vertex_iterator + index * 4 + 2;
                *__mesh_index( _mesh, index_iterator + 2 ) = vertex_iterator + index * 4 + 5;
                *__mesh_index( _mesh, index_iterator + 3 ) = vertex_iterator + index * 4 + 5;
                *__mesh_index( _mesh, index_iterator + 4 ) = vertex_iterator + index * 4 + 2;
                *__mesh_index( _mesh, index_iterator + 5 ) = vertex_iterator + index * 4 + 6;

                index_iterator += 6;

                *__mesh_index( _mesh, index_iterator + 0 ) = vertex_iterator + index * 4 + 2;
                *__mesh_index( _mesh, index_iterator + 1 ) = vertex_iterator + index * 4 + 3;
                *__mesh_index( _mesh, index_iterator + 2 ) = vertex_iterator + index * 4 + 6;
                *__mesh_index( _mesh, index_iterator + 3 ) = vertex_iterator + index * 4 + 6;
                *__mesh_index( _mesh, index_iterator + 4 ) = vertex_iterator + index * 4 + 3;
                *__mesh_index( _mesh, index_iterator + 5 ) = vertex_iterator + index * 4 + 7;

                index_iterator += 6;
            }
        }
        else
        {
            for( uint16_t index = 0; index != points_size - 1; ++index )
            {
                *__mesh_index( _mesh, index_iterator + 0 ) = vertex_iterator + index * 2 + 0;
                *__mesh_index( _mesh, index_iterator + 1 ) = vertex_iterator + index * 2 + 1;
                *__mesh_index( _mesh, index_iterator + 2 ) = vertex_iterator + index * 2 + 2;
                *__mesh_index( _mesh, index_iterator + 3 ) = vertex_iterator + index * 2 + 2;
                *__mesh_index( _mesh, index_iterator + 4 ) = vertex_iterator + index * 2 + 1;
                *__mesh_index( _mesh, index_iterator + 5 ) = vertex_iterator + index * 2 + 3;

                index_iterator += 6;
            }
        }

        {
            gp_line_points_t * point0 = points;
            gp_line_points_t * point1 = points + points_size - 1;

            float width = point0->width;
            uint32_t argb = point0->argb;

            const gp_vec2f_t * p0 = &point0->p;
            const gp_vec2f_t * p1 = &point1->p;

            gp_vec2f_t perp;
            __make_line_perp( &perp, p0, p1 );

            float line_width = width * 0.5f;

            if( penumbra > 0.f )
            {
                float line_width_soft = (width - penumbra * 2.f) * 0.5f;

                __mesh_position( _mesh, vertex_iterator + 0 )->x = p0->x - perp.x * line_width;
                __mesh_position( _mesh, vertex_iterator + 0 )->y = p0->y - perp.y * line_width;
                *__mesh_color( _mesh, vertex_iterator + 0 ) = argb & 0x00ffffff;

                __mesh_position( _mesh, vertex_iterator + 1 )->x = p0->x - perp.x * line_width_soft;
                __mesh_position( _mesh, vertex_iterator + 1 )->y = p0->y - perp.y * line_width_soft;
                *__mesh_color( _mesh, vertex_iterator + 1 ) = argb;

                __mesh_position( _mesh, vertex_iterator + 2 )->x = p0->x + perp.x * line_width_soft;
                __mesh_position( _mesh, vertex_iterator + 2 )->y = p0->y + perp.y * line_width_soft;
                *__mesh_color( _mesh, vertex_iterator + 2 ) = argb;

                __mesh_position( _mesh, vertex_iterator + 3 )->x = p0->x + perp.x * line_width;
                __mesh_position( _mesh, vertex_iterator + 3 )->y = p0->y + perp.y * line_width;
                *__mesh_color( _mesh, vertex_iterator + 3 ) = argb & 0x00ffffff;

                vertex_iterator += 4;
            }
            else
            {
                __mesh_position( _mesh, vertex_iterator + 0 )->x = p0->x - perp.x * line_width;
                __mesh_position( _mesh, vertex_iterator + 0 )->y = p0->y - perp.y * line_width;
                *__mesh_color( _mesh, vertex_iterator + 0 ) = argb;

                __mesh_position( _mesh, vertex_iterator + 1 )->x = p0->x + perp.x * line_width;
                __mesh_position( _mesh, vertex_iterator + 1 )->y = p0->y + perp.y * line_width;
                *__mesh_color( _mesh, vertex_iterator + 1 ) = argb;

                vertex_iterator += 2;
            }
        }

        for( uint16_t index = 1; index != points_size - 1; ++index )
        {
            float width = points[index + 0].width;
            uint32_t argb = points[index + 0].argb;

            const gp_vec2f_t * p0 = &points[index - 1].p;
            const gp_vec2f_t * p1 = &points[index + 0].p;
            const gp_vec2f_t * p2 = &points[index + 1].p;

            float line_width = width * 0.5f;

            gp_vec2f_t perp01;
            __make_line_perp( &perp01, p0, p1 );

            gp_vec2f_t perp12;
            __make_line_perp( &perp12, p1, p2 );

            gp_vec2f_t linep00;
            linep00.x = p0->x - perp01.x * line_width;
            linep00.y = p0->y - perp01.y * line_width;

            gp_vec2f_t linep01;
            linep01.x = p1->x - perp01.x * line_width;
            linep01.y = p1->y - perp01.y * line_width;

            gp_linef_t line01l;
            __make_line_from_two_point_v2( &line01l, &linep00, &linep01 );

            gp_vec2f_t linep10;
            linep00.x = p1->x - perp12.x * line_width;
            linep00.y = p1->y - perp12.y * line_width;

            gp_vec2f_t linep11;
            linep01.x = p2->x - perp12.x * line_width;
            linep01.y = p2->y - perp12.y * line_width;

            gp_linef_t line12l;
            __make_line_from_two_point_v2( &line12l, &linep10, &linep11 );

            gp_vec2f_t pl;
            if( __intersect_line( &line01l, &line12l, &pl ) == GP_FALSE )
            {
                pl.x = p1->x - perp01.x * line_width;
                pl.y = p1->y - perp01.y * line_width;
            }

            gp_vec2f_t linep20;
            linep20.x = p0->x + perp01.x * line_width;
            linep20.y = p0->y + perp01.y * line_width;

            gp_vec2f_t linep21;
            linep21.x = p1->x + perp01.x * line_width;
            linep21.y = p1->y + perp01.y * line_width;

            gp_linef_t line01r;
            __make_line_from_two_point_v2( &line01r, &linep20, &linep21 );

            gp_vec2f_t linep30;
            linep20.x = p1->x + perp12.x * line_width;
            linep20.y = p1->y + perp12.y * line_width;

            gp_vec2f_t linep31;
            linep21.x = p2->x + perp12.x * line_width;
            linep21.y = p2->y + perp12.y * line_width;

            gp_linef_t line12r;
            __make_line_from_two_point_v2( &line12r, &linep30, &linep31 );

            gp_vec2f_t pr;
            if( __intersect_line( &line01r, &line12r, &pr ) == GP_FALSE )
            {
                pr.x = p1->x + perp01.x * line_width;
                pr.y = p1->y + perp01.y * line_width;
            }

            if( penumbra > 0.f )
            {
                float line_width_soft = (width - penumbra * 2.f) * 0.5f;

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

                __mesh_position( _mesh, vertex_iterator + 0 )->x = pl.x;
                __mesh_position( _mesh, vertex_iterator + 0 )->y = pl.y;
                *__mesh_color( _mesh, vertex_iterator + 0 ) = argb & 0x00ffffff;

                __mesh_position( _mesh, vertex_iterator + 1 )->x = pl_soft.x;
                __mesh_position( _mesh, vertex_iterator + 1 )->y = pl_soft.y;
                *__mesh_color( _mesh, vertex_iterator + 1 ) = argb;

                __mesh_position( _mesh, vertex_iterator + 2 )->x = pr_soft.x;
                __mesh_position( _mesh, vertex_iterator + 2 )->y = pr_soft.y;
                *__mesh_color( _mesh, vertex_iterator + 2 ) = argb;

                __mesh_position( _mesh, vertex_iterator + 3 )->x = pr.x;
                __mesh_position( _mesh, vertex_iterator + 3 )->y = pr.y;
                *__mesh_color( _mesh, vertex_iterator + 3 ) = argb & 0x00ffffff;

                vertex_iterator += 4;
            }
            else
            {
                __mesh_position( _mesh, vertex_iterator + 0 )->x = pl.x;
                __mesh_position( _mesh, vertex_iterator + 0 )->y = pl.y;
                *__mesh_color( _mesh, vertex_iterator + 0 ) = argb;

                __mesh_position( _mesh, vertex_iterator + 1 )->x = pr.x;
                __mesh_position( _mesh, vertex_iterator + 1 )->y = pr.y;
                *__mesh_color( _mesh, vertex_iterator + 1 ) = argb;

                vertex_iterator += 2;
            }
        }

        {
            float width = points[points_size - 2].width;
            uint32_t argb = points[points_size - 2].argb;

            const gp_vec2f_t * p0 = &points[points_size - 2].p;
            const gp_vec2f_t * p1 = &points[points_size - 1].p;

            gp_vec2f_t perp;
            __make_line_perp( &perp, p0, p1 );

            float line_width = width * 0.5f;

            if( penumbra > 0.f )
            {
                float line_width_soft = (width - penumbra * 2.f) * 0.5f;

                __mesh_position( _mesh, vertex_iterator + 0 )->x = p1->x - perp.x * line_width;
                __mesh_position( _mesh, vertex_iterator + 0 )->y = p1->y - perp.y * line_width;
                *__mesh_color( _mesh, vertex_iterator + 0 ) = argb & 0x00ffffff;

                __mesh_position( _mesh, vertex_iterator + 1 )->x = p1->x - perp.x * line_width_soft;
                __mesh_position( _mesh, vertex_iterator + 1 )->y = p1->y - perp.y * line_width_soft;
                *__mesh_color( _mesh, vertex_iterator + 1 ) = argb;

                __mesh_position( _mesh, vertex_iterator + 2 )->x = p1->x + perp.x * line_width_soft;
                __mesh_position( _mesh, vertex_iterator + 2 )->y = p1->y + perp.y * line_width_soft;
                *__mesh_color( _mesh, vertex_iterator + 2 ) = argb;

                __mesh_position( _mesh, vertex_iterator + 3 )->x = p1->x + perp.x * line_width;
                __mesh_position( _mesh, vertex_iterator + 3 )->y = p1->y + perp.y * line_width;
                *__mesh_color( _mesh, vertex_iterator + 3 ) = argb & 0x00ffffff;

                vertex_iterator += 4;
            }
            else
            {
                __mesh_position( _mesh, vertex_iterator + 0 )->x = p1->x - perp.x * line_width;
                __mesh_position( _mesh, vertex_iterator + 0 )->y = p1->y - perp.y * line_width;
                *__mesh_color( _mesh, vertex_iterator + 0 ) = argb;

                __mesh_position( _mesh, vertex_iterator + 1 )->x = p1->x + perp.x * line_width;
                __mesh_position( _mesh, vertex_iterator + 1 )->y = p1->y + perp.y * line_width;
                *__mesh_color( _mesh, vertex_iterator + 1 ) = argb;

                vertex_iterator += 2;
            }
        }
    }

    return GP_SUCCESSFUL;
}