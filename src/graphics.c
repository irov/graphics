#include "graphics/graphics.h"

#include "struct.h"
#include "detail.h"

//////////////////////////////////////////////////////////////////////////
gp_result_t gp_canvas_create( gp_canvas_t ** _canvas, gp_malloc_t _malloc, gp_free_t _free, gp_realloc_t _realloc )
{
    gp_canvas_t * canvas = (gp_canvas_t *)(*_malloc)(sizeof( gp_canvas_t ));

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
    GP_UNUSED( _canvas );

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
gp_result_t gp_set_line_color( gp_canvas_t * _canvas, uint32_t _color )
{
    _canvas->line_color = _color;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_line_color( gp_canvas_t * _canvas, uint32_t * _color )
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