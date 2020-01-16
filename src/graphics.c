#include "graphics/graphics.h"

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
    _canvas->curve_quality = 20;
    _canvas->ellipse_quality = 32;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_canvas_create( gp_canvas_t ** _canvas, gp_malloc_t _malloc, gp_free_t _free, gp_realloc_t _realloc )
{
    gp_canvas_t * canvas = (gp_canvas_t *)(*_malloc)(sizeof( gp_canvas_t ));

    __canvas_default_setup( canvas );

    canvas->malloc = _malloc;
    canvas->free = _free;
    canvas->realloc = _realloc;

    *_canvas = canvas;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_canvas_destroy( gp_canvas_t * _canvas )
{
    (*_canvas->free)(_canvas);

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_canvas_clear( gp_canvas_t * _canvas )
{
    GP_LIST_DESTROY( _canvas, gp_line_t, _canvas->lines );
    _canvas->line_last = GP_NULLPTR;

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
    *_penumbra  = _canvas->line_penumbra;

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
    p->x = _x;
    p->y = _y;
    p->next = GP_NULLPTR;

    gp_line_t * l = GP_NEW( _canvas, gp_line_t );
    l->points = p;
    l->penumbra = _canvas->line_penumbra;
    l->next = GP_NULLPTR;

    GP_LIST_PUSHBACK( gp_line_t, _canvas->lines, l );
    _canvas->line_last = l;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_line_to( gp_canvas_t * _canvas, float _x, float _y )
{
    gp_line_t * l = _canvas->line_last;

    gp_point_t * p = GP_NEW( _canvas, gp_point_t );
    p->x = _x;
    p->y = _y;
    p->next = GP_NULLPTR;
    
    GP_LIST_PUSHBACK( gp_point_t, l->points, p );
    
    gp_line_edge_t * e = GP_NEW( _canvas, gp_line_edge_t );

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
    p->x = _x;
    p->y = _y;
    p->next = GP_NULLPTR;

    gp_line_t * line_last = _canvas->line_last;
    GP_LIST_PUSHBACK( gp_point_t, line_last->points, p );

    gp_line_edge_t * edge = GP_NEW( _canvas, gp_line_edge_t );
    edge->controls = 1;
    edge->p[0].x = _p0x;
    edge->p[1].y = _p0y;
    edge->quality = _canvas->curve_quality;
    edge->dt = 1.f / (float)(_canvas->curve_quality - 1);
    edge->line_width = _canvas->line_width;
    edge->line_color = _canvas->line_color;

    GP_LIST_PUSHBACK( gp_line_edge_t, line_last->edges, edge );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_bezier_curve_to( gp_canvas_t * _canvas, float _p0x, float _p0y, float _p1x, float _p1y, float _x, float _y )
{
    gp_point_t * p = GP_NEW( _canvas, gp_point_t );
    p->x = _x;
    p->y = _y;
    p->next = GP_NULLPTR;

    gp_line_t * line_last = _canvas->line_last;
    GP_LIST_PUSHBACK( gp_point_t, line_last->points, p );

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

    GP_LIST_PUSHBACK( gp_line_edge_t, line_last->edges, edge );

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
    uint32_t vertex_size = 0;
    uint32_t index_size = 0;

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
            vertex_size += point_count * 4;
            index_size += (point_count - 1) * 18;
        }
        else
        {
            vertex_size += point_count * 2;
            index_size += (point_count - 1) * 6;
        }
    }

    for( const gp_rect_t * r = _canvas->rects; r != GP_NULLPTR; r = r->next )
    {
        if( r->line_penumbra > 0.f )
        {
            vertex_size += 16;
            index_size += 72;
        }
        else
        {
            vertex_size += 8;
            index_size += 24;
        }

        if( r->fill == GP_TRUE )
        {
            vertex_size += 4;
            index_size += 6;
        }
    }

    for( const gp_rounded_rect_t * r = _canvas->rounded_rects; r != GP_NULLPTR; r = r->next )
    {
        if( r->line_penumbra > 0.f )
        {
            vertex_size += 32;
            index_size += 72;

            for( uint32_t index = 0; index != 4; ++index )
            {
                vertex_size += (r->quality + 1) * 4;
                index_size += r->quality * 6 * 3;
            }
        }
        else
        {
            vertex_size += 16;
            index_size += 24;

            for( uint32_t index = 0; index != 4; ++index )
            {
                vertex_size += (r->quality + 1) * 2;
                index_size += r->quality * 6;
            }
        }

        if( r->fill == GP_TRUE )
        {
            for( uint32_t index = 0; index != 4; ++index )
            {
                vertex_size += (r->quality + 1) + 1;
                index_size += r->quality * 3;
            }

            vertex_size += 12;
            index_size += 6 * 5;
        }
    }

    for( const gp_ellipse_t * e = _canvas->ellipses; e != GP_NULLPTR; e = e->next )
    {
        if( e->line_penumbra > 0.f )
        {
            vertex_size += e->quality * 4;
            index_size += e->quality * 18;
        }
        else
        {
            vertex_size += e->quality * 2;
            index_size += e->quality * 6;
        }

        if( e->fill == GP_TRUE )
        {
            vertex_size += 1;
            vertex_size += e->quality;

            index_size += e->quality * 3;
        }
    }

    _mesh->vertex_size = vertex_size;
    _mesh->index_size = index_size;

    return GP_SUCCESSFUL;
}
