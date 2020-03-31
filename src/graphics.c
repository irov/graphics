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
    _canvas->state_cook.next = GP_NULLPTR;
    _canvas->state_cook.prev = GP_NULLPTR;
    _canvas->state_cook.line_thickness = 3.f;
    _canvas->state_cook.penumbra = 1.f;
    _canvas->state_cook.color.r = 1.f;
    _canvas->state_cook.color.g = 1.f;
    _canvas->state_cook.color.b = 1.f;
    _canvas->state_cook.color.a = 1.f;
    _canvas->state_cook.fill = GP_FALSE;
    _canvas->state_cook.uv_ou = 0.f;
    _canvas->state_cook.uv_ov = 0.f;
    _canvas->state_cook.uv_su = 1.f;
    _canvas->state_cook.uv_sv = 1.f;
    _canvas->state_cook.curve_quality = 32;
    _canvas->state_cook.curve_quality_inv = 1.f / 32.f;
    _canvas->state_cook.ellipse_quality = 64;
    _canvas->state_cook.ellipse_quality_inv = 1.f / 64.f;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_canvas_create( gp_canvas_t ** _canvas, gp_malloc_t _malloc, gp_free_t _free, void * _ud )
{
    gp_canvas_t * canvas = (gp_canvas_t *)(*_malloc)(sizeof( gp_canvas_t ), _ud);

    canvas->state_invalidate = GP_TRUE;
    canvas->states = GP_NULLPTR;

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
    gp_canvas_clear( _canvas );

    GP_FREE( _canvas, _canvas );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_canvas_clear( gp_canvas_t * _canvas )
{
    GP_LIST_DESTROY( _canvas, gp_state_t, _canvas->states );

    GP_LIST_FOREACH( gp_line_t, _canvas->lines, l )
    {
        GP_LIST_DESTROY( _canvas, gp_line_point_t, l->points );
        GP_LIST_DESTROY( _canvas, gp_line_edge_t, l->edges );
    }

    GP_LIST_DESTROY( _canvas, gp_line_t, _canvas->lines );
    GP_LIST_DESTROY( _canvas, gp_rect_t, _canvas->rects );
    GP_LIST_DESTROY( _canvas, gp_rounded_rect_t, _canvas->rounded_rects );
    GP_LIST_DESTROY( _canvas, gp_ellipse_t, _canvas->ellipses );

    __canvas_default_setup( _canvas );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_line_thickness( gp_canvas_t * _canvas, float _thickness )
{
    if( _canvas->state_cook.line_thickness == _thickness )
    {
        return GP_SUCCESSFUL;
    }

    _canvas->state_cook.line_thickness = _thickness;
    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_line_thickness( const gp_canvas_t * _canvas, float * _thickness )
{
    *_thickness = _canvas->state_cook.line_thickness;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_penumbra( gp_canvas_t * _canvas, float _penumbra )
{
    if( _penumbra >= _canvas->state_cook.line_thickness * 0.5f )
    {
        return GP_FAILURE;
    }

    if( _canvas->state_cook.penumbra == _penumbra )
    {
        return GP_SUCCESSFUL;
    }

    _canvas->state_cook.penumbra = _penumbra;
    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_penumbra( const gp_canvas_t * _canvas, float * _penumbra )
{
    *_penumbra = _canvas->state_cook.penumbra;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_color( gp_canvas_t * _canvas, float _r, float _g, float _b, float _a )
{
    if( _canvas->state_cook.color.r == _r &&
        _canvas->state_cook.color.g == _g &&
        _canvas->state_cook.color.b == _b &&
        _canvas->state_cook.color.a == _a )
    {
        return GP_SUCCESSFUL;
    }

    _canvas->state_cook.color.r = _r;
    _canvas->state_cook.color.g = _g;
    _canvas->state_cook.color.b = _b;
    _canvas->state_cook.color.a = _a;
    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_color( const gp_canvas_t * _canvas, gp_color_t * _color )
{
    *_color = _canvas->state_cook.color;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_uv_offset( gp_canvas_t * _canvas, float _ou, float _ov, float _su, float _sv )
{
    if( _canvas->state_cook.uv_ou == _ou &&
        _canvas->state_cook.uv_ov == _ov &&
        _canvas->state_cook.uv_su == _su &&
        _canvas->state_cook.uv_sv == _sv )
    {
        return GP_SUCCESSFUL;
    }

    _canvas->state_cook.uv_ou = _ou;
    _canvas->state_cook.uv_ov = _ov;
    _canvas->state_cook.uv_su = _su;
    _canvas->state_cook.uv_sv = _sv;

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_uv_offset( const gp_canvas_t * _canvas, float * _ou, float * _ov, float * _su, float * _sv )
{
    *_ou = _canvas->state_cook.uv_ou;
    *_ov = _canvas->state_cook.uv_ov;
    *_su = _canvas->state_cook.uv_su;
    *_sv = _canvas->state_cook.uv_sv;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_curve_quality( gp_canvas_t * _canvas, gp_uint8_t _quality )
{
    if( _canvas->state_cook.curve_quality == _quality )
    {
        return GP_SUCCESSFUL;
    }

    _canvas->state_cook.curve_quality = _quality;
    _canvas->state_cook.curve_quality_inv = 1.f / (float)_quality;

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_curve_quality( const gp_canvas_t * _canvas, gp_uint8_t * _quality )
{
    *_quality = _canvas->state_cook.curve_quality;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_ellipse_quality( gp_canvas_t * _canvas, gp_uint8_t _quality )
{
    gp_uint8_t quality = (_quality + 3) / 4;

    if( _canvas->state_cook.ellipse_quality == quality )
    {
        return GP_SUCCESSFUL;
    }

    _canvas->state_cook.ellipse_quality = quality;
    _canvas->state_cook.ellipse_quality_inv = 1.f / (float)_canvas->state_cook.ellipse_quality;

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_ellipse_quality( const gp_canvas_t * _canvas, gp_uint8_t * _quality )
{
    *_quality = _canvas->state_cook.ellipse_quality;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_begin_fill( gp_canvas_t * _canvas )
{
    if( _canvas->state_cook.fill == GP_TRUE )
    {
        return GP_FAILURE;
    }

    _canvas->state_cook.fill = GP_TRUE;

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_end_fill( gp_canvas_t * _canvas )
{
    if( _canvas->state_cook.fill == GP_FALSE )
    {
        return GP_FAILURE;
    }

    _canvas->state_cook.fill = GP_FALSE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static const gp_state_t * __copy_state( gp_canvas_t * _canvas )
{
    if( _canvas->state_invalidate == GP_FALSE )
    {
        const gp_state_t * back = GP_LIST_BACK( _canvas->states );

        return back;
    }

    gp_state_t * s = GP_NEW( _canvas, gp_state_t );
    s->next = GP_NULLPTR;
    s->prev = GP_NULLPTR;

    *s = _canvas->state_cook;

    GP_LIST_PUSHBACK( gp_state_t, _canvas->states, s );

    _canvas->state_invalidate = GP_FALSE;

    return s;
}
//////////////////////////////////////////////////////////////////////////
#define GP_COPY_STATE(canvas) __copy_state( canvas )
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_move_to( gp_canvas_t * _canvas, float _x, float _y )
{
    gp_line_point_t * p = GP_NEW( _canvas, gp_line_point_t );
    p->next = GP_NULLPTR;
    p->prev = GP_NULLPTR;

    p->p.x = _x;
    p->p.y = _y;

    p->state = GP_COPY_STATE( _canvas );

    gp_line_t * l = GP_NEW( _canvas, gp_line_t );
    l->next = GP_NULLPTR;
    l->prev = GP_NULLPTR;

    l->points = GP_NULLPTR;
    GP_LIST_PUSHBACK( gp_line_point_t, l->points, p );
    l->edges = GP_NULLPTR;
    l->state = p->state;

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

    gp_line_point_t * p = GP_NEW( _canvas, gp_line_point_t );
    p->next = GP_NULLPTR;
    p->prev = GP_NULLPTR;

    p->p.x = _x;
    p->p.y = _y;

    p->state = GP_COPY_STATE( _canvas );

    GP_LIST_PUSHBACK( gp_line_point_t, l->points, p );

    gp_line_edge_t * e = GP_NEW( _canvas, gp_line_edge_t );
    e->next = GP_NULLPTR;
    e->prev = GP_NULLPTR;

    e->controls = 0;

    GP_LIST_PUSHBACK( gp_line_edge_t, l->edges, e );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_quadratic_curve_to( gp_canvas_t * _canvas, float _p0x, float _p0y, float _x, float _y )
{
    gp_line_point_t * p = GP_NEW( _canvas, gp_line_point_t );
    p->next = GP_NULLPTR;
    p->prev = GP_NULLPTR;

    p->p.x = _x;
    p->p.y = _y;

    p->state = GP_COPY_STATE( _canvas );

    gp_line_t * line_back = GP_LIST_BACK( _canvas->lines );
    GP_LIST_PUSHBACK( gp_line_point_t, line_back->points, p );

    gp_line_edge_t * e = GP_NEW( _canvas, gp_line_edge_t );
    e->next = GP_NULLPTR;
    e->prev = GP_NULLPTR;

    e->controls = 1;
    e->p[0].x = _p0x;
    e->p[0].y = _p0y;

    GP_LIST_PUSHBACK( gp_line_edge_t, line_back->edges, e );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_bezier_curve_to( gp_canvas_t * _canvas, float _p0x, float _p0y, float _p1x, float _p1y, float _x, float _y )
{
    gp_line_point_t * p = GP_NEW( _canvas, gp_line_point_t );
    p->next = GP_NULLPTR;
    p->prev = GP_NULLPTR;

    p->p.x = _x;
    p->p.y = _y;

    p->state = GP_COPY_STATE( _canvas );

    gp_line_t * line_back = GP_LIST_BACK( _canvas->lines );
    GP_LIST_PUSHBACK( gp_line_point_t, line_back->points, p );

    gp_line_edge_t * e = GP_NEW( _canvas, gp_line_edge_t );
    e->next = GP_NULLPTR;
    e->prev = GP_NULLPTR;

    e->controls = 2;
    e->p[0].x = _p0x;
    e->p[0].y = _p0y;
    e->p[1].x = _p1x;
    e->p[1].y = _p1y;

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

    r->state = GP_COPY_STATE( _canvas );

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

    rr->state = GP_COPY_STATE( _canvas );

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
    e->radius_width = _width;
    e->radius_height = _height;

    e->state = GP_COPY_STATE( _canvas );

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

    if( gp_render_ellipse( _canvas, _mesh, &vertex_iterator, &index_iterator ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    return GP_SUCCESSFUL;
}