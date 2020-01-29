#include "graphics/graphics.h"

#include "line.h"
#include "rect.h"
#include "rounded_rect.h"
#include "ellipse.h"

#include "struct.h"
#include "detail.h"

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
    _canvas->curve_quality = 32;
    _canvas->ellipse_quality = 64;
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
gp_result_t gp_set_line_color( gp_canvas_t * _canvas, const gp_color_t * _color )
{
    _canvas->line_color = *_color;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_line_color( gp_canvas_t * _canvas, gp_color_t * _color )
{
    *_color = _canvas->line_color;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_curve_quality( gp_canvas_t * _canvas, gp_uint8_t _quality )
{
    _canvas->curve_quality = _quality;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_curve_quality( gp_canvas_t * _canvas, gp_uint8_t * _quality )
{
    *_quality = _canvas->curve_quality;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_ellipse_quality( gp_canvas_t * _canvas, gp_uint8_t _quality )
{
    _canvas->ellipse_quality = _quality;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_ellipse_quality( gp_canvas_t * _canvas, gp_uint8_t * _quality )
{
    *_quality = _canvas->ellipse_quality;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_fill_color( gp_canvas_t * _canvas, const gp_color_t * _color )
{
    _canvas->fill_color = *_color;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_fill_color( gp_canvas_t * _canvas, gp_color_t * _color )
{
    *_color = _canvas->fill_color;

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
    p->next = GP_NULLPTR;
    p->prev = GP_NULLPTR;

    p->p.x = _x;
    p->p.y = _y;

    gp_line_t * l = GP_NEW( _canvas, gp_line_t );
    l->next = GP_NULLPTR;
    l->prev = GP_NULLPTR;

    l->points = GP_NULLPTR;
    GP_LIST_PUSHBACK( gp_point_t, l->points, p );
    l->edges = GP_NULLPTR;
    l->penumbra = _canvas->line_penumbra;

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
    p->next = GP_NULLPTR;
    p->prev = GP_NULLPTR;

    p->p.x = _x;
    p->p.y = _y;

    GP_LIST_PUSHBACK( gp_point_t, l->points, p );

    gp_line_edge_t * e = GP_NEW( _canvas, gp_line_edge_t );
    e->next = GP_NULLPTR;
    e->prev = GP_NULLPTR;

    e->controls = 0;
    e->quality = 2;
    e->dt = 1.f;
    e->line_width = _canvas->line_width;
    e->line_color = _canvas->line_color;

    GP_LIST_PUSHBACK( gp_line_edge_t, l->edges, e );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_quadratic_curve_to( gp_canvas_t * _canvas, float _p0x, float _p0y, float _x, float _y )
{
    gp_point_t * p = GP_NEW( _canvas, gp_point_t );
    p->next = GP_NULLPTR;
    p->prev = GP_NULLPTR;

    p->p.x = _x;
    p->p.y = _y;

    gp_line_t * line_back = GP_LIST_BACK( _canvas->lines );
    GP_LIST_PUSHBACK( gp_point_t, line_back->points, p );

    gp_line_edge_t * e = GP_NEW( _canvas, gp_line_edge_t );
    e->next = GP_NULLPTR;
    e->prev = GP_NULLPTR;

    e->controls = 1;
    e->p[0].x = _p0x;
    e->p[0].y = _p0y;
    e->quality = _canvas->curve_quality;
    e->dt = 1.f / (float)(_canvas->curve_quality - 1);
    e->line_width = _canvas->line_width;
    e->line_color = _canvas->line_color;

    GP_LIST_PUSHBACK( gp_line_edge_t, line_back->edges, e );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_bezier_curve_to( gp_canvas_t * _canvas, float _p0x, float _p0y, float _p1x, float _p1y, float _x, float _y )
{
    gp_point_t * p = GP_NEW( _canvas, gp_point_t );
    p->next = GP_NULLPTR;
    p->prev = GP_NULLPTR;

    p->p.x = _x;
    p->p.y = _y;

    gp_line_t * line_back = GP_LIST_BACK( _canvas->lines );
    GP_LIST_PUSHBACK( gp_point_t, line_back->points, p );

    gp_line_edge_t * e = GP_NEW( _canvas, gp_line_edge_t );
    e->next = GP_NULLPTR;
    e->prev = GP_NULLPTR;

    e->controls = 2;
    e->p[0].x = _p0x;
    e->p[0].y = _p0y;
    e->p[1].x = _p1x;
    e->p[1].y = _p1y;
    e->quality = _canvas->curve_quality;
    e->dt = 1.f / (float)(_canvas->curve_quality - 1);
    e->line_width = _canvas->line_width;
    e->line_color = _canvas->line_color;

    GP_LIST_PUSHBACK( gp_line_edge_t, line_back->edges, e );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_draw_rect( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height )
{
    gp_rect_t * r = GP_NEW( _canvas, gp_rect_t );
    r->next = GP_NULLPTR;
    r->prev = GP_NULLPTR;

    r->point.x = _x;
    r->point.y = _y;
    r->width = _width;
    r->height = _height;
    r->line_width = _canvas->line_width;
    r->line_penumbra = _canvas->line_penumbra;
    r->line_color = _canvas->line_color;
    r->fill_color = _canvas->fill_color;
    r->fill = _canvas->fill;

    GP_LIST_PUSHBACK( gp_rect_t, _canvas->rects, r );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_draw_rounded_rect( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _radius )
{
    gp_rounded_rect_t * rr = GP_NEW( _canvas, gp_rounded_rect_t );
    rr->next = GP_NULLPTR;
    rr->prev = GP_NULLPTR;

    rr->point.x = _x;
    rr->point.y = _y;
    rr->width = _width;
    rr->height = _height;
    rr->radius = _radius;
    rr->quality = (_canvas->ellipse_quality + 3) / 4;
    rr->quality_inv = 1.f / (float)rr->quality;
    rr->line_width = _canvas->line_width;
    rr->line_penumbra = _canvas->line_penumbra;
    rr->line_color = _canvas->line_color;
    rr->fill_color = _canvas->fill_color;
    rr->fill = _canvas->fill;

    GP_LIST_PUSHBACK( gp_rounded_rect_t, _canvas->rounded_rects, rr );

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
    gp_ellipse_t * e = GP_NEW( _canvas, gp_ellipse_t );

    e->point.x = _x;
    e->point.y = _y;
    e->width = _width;
    e->height = _height;
    e->quality = _canvas->ellipse_quality;
    e->quality_inv = 1.f / (float)e->quality;
    e->line_width = _canvas->line_width;
    e->line_penumbra = _canvas->line_penumbra;
    e->line_color = _canvas->line_color;
    e->fill_color = _canvas->fill_color;
    e->fill = _canvas->fill;

    GP_LIST_PUSHBACK( gp_ellipse_t, _canvas->ellipses, e );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_calculate_mesh_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh )
{
    _mesh->vertex_count = 0;
    _mesh->index_count = 0;

    if( gp_calculate_mesh_line_size( _canvas, _mesh ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    if( gp_calculate_mesh_rect_size( _canvas, _mesh ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    if( gp_calculate_mesh_rounded_rect_size( _canvas, _mesh ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    if( gp_calculate_mesh_ellipse_size( _canvas, _mesh ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

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
gp_result_t gp_render( const gp_canvas_t * _canvas, const gp_mesh_t * _mesh )
{
    gp_uint16_t vertex_iterator = 0;
    gp_uint16_t index_iterator = 0;

    if( gp_render_line( _canvas, _mesh, &vertex_iterator, &index_iterator ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    if( gp_render_rect( _canvas, _mesh, &vertex_iterator, &index_iterator ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    if( gp_render_rounded_rect( _canvas, _mesh, &vertex_iterator, &index_iterator ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    for( const gp_ellipse_t * e = _canvas->ellipses; e != GP_NULLPTR; e = e->next )
    {
        gp_color_t line_color;
        gp_color_mul( &line_color, &_mesh->color, &e->line_color );
        gp_uint32_t argb = gp_color_argb( &line_color );

        gp_uint8_t quality = e->quality;
        float line_penumbra = e->line_penumbra;

        if( line_penumbra > 0.f )
        {
            gp_uint32_t ellipse_quality2 = quality * 4;

            for( gp_uint16_t index = 0; index != quality; ++index )
            {
                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 4 + 0) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 4 + 1) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 4 + 4) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 4 + 4) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 4 + 1) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + (index * 4 + 5) % ellipse_quality2 );

                index_iterator += 6;

                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 4 + 1) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 4 + 2) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 4 + 5) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 4 + 5) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 4 + 2) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + (index * 4 + 6) % ellipse_quality2 );

                index_iterator += 6;

                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 4 + 2) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 4 + 3) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 4 + 6) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 4 + 6) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 4 + 3) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + (index * 4 + 7) % ellipse_quality2 );

                index_iterator += 6;
            }
        }
        else
        {
            gp_uint32_t ellipse_quality2 = quality * 2;

            for( gp_uint16_t index = 0; index != quality; ++index )
            {
                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 2 + 0) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 2 + 1) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 2 + 2) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 2 + 2) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 2 + 1) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + (index * 2 + 3) % ellipse_quality2 );

                index_iterator += 6;
            }
        }

        float line_width = e->line_width;

        float line_half_width = line_width * 0.5f;

        float dt = gp_constant_two_pi * e->quality_inv;

        float t = 0.f;

        for( gp_uint32_t index = 0; index != quality; ++index, t += dt )
        {
            float ct = GP_MATH_COSF( t );
            float st = GP_MATH_SINF( t );

            float x0 = e->point.x + (e->width + line_half_width) * ct;
            float y0 = e->point.y + (e->height + line_half_width) * st;

            float x1 = e->point.x + (e->width - line_half_width) * ct;
            float y1 = e->point.y + (e->height - line_half_width) * st;

            if( line_penumbra > 0.f )
            {
                float line_width_soft = (line_width - line_penumbra * 2.f) * 0.5f;

                float x0_soft = e->point.x + (e->width + line_width_soft) * ct;
                float y0_soft = e->point.y + (e->height + line_width_soft) * st;

                float x1_soft = e->point.x + (e->width - line_width_soft) * ct;
                float y1_soft = e->point.y + (e->height - line_width_soft) * st;

                gp_mesh_position( _mesh, vertex_iterator + 0, x0, y0 );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                gp_mesh_position( _mesh, vertex_iterator + 1, x0_soft, y0_soft );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                gp_mesh_position( _mesh, vertex_iterator + 2, x1_soft, y1_soft );
                gp_mesh_color( _mesh, vertex_iterator + 2, argb );

                gp_mesh_position( _mesh, vertex_iterator + 3, x1, y1 );
                gp_mesh_color( _mesh, vertex_iterator + 3, argb & 0x00ffffff );

                vertex_iterator += 4;
            }
            else
            {
                gp_mesh_position( _mesh, vertex_iterator + 0, x0, y0 );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                gp_mesh_position( _mesh, vertex_iterator + 1, x1, y1 );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                vertex_iterator += 2;
            }
        }

        if( e->fill == GP_TRUE )
        {
            gp_color_t fill_color;
            gp_color_mul( &fill_color, &_mesh->color, &e->fill_color );
            gp_uint32_t fill_argb = gp_color_argb( &fill_color );

            for( gp_uint16_t index = 0; index != quality; ++index )
            {
                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index + 0) % quality + 1 );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index + 1) % quality + 1 );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator );

                index_iterator += 3;
            }

            gp_mesh_position( _mesh, vertex_iterator + 0, e->point.x, e->point.y );
            gp_mesh_color( _mesh, vertex_iterator + 0, fill_argb );

            vertex_iterator += 1;

            float tf = 0.f;

            for( gp_uint32_t index = 0; index != quality; ++index, tf += dt )
            {
                float ct = GP_MATH_COSF( tf );
                float st = GP_MATH_SINF( tf );

                float x = e->point.x + e->width * ct;
                float y = e->point.y + e->height * st;

                gp_mesh_position( _mesh, vertex_iterator + 0, x, y );
                gp_mesh_color( _mesh, vertex_iterator + 0, fill_argb );

                vertex_iterator += 1;
            }
        }
    }

    return GP_SUCCESSFUL;
}