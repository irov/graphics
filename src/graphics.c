#include "graphics/graphics.h"

#include "line.h"
#include "rect.h"
#include "rounded_rect.h"
#include "ellipse.h"

#include "struct.h"
#include "detail.h"

//////////////////////////////////////////////////////////////////////////
float gp_get_default_thickness( void )
{
    return 3.f;
}
//////////////////////////////////////////////////////////////////////////
float gp_get_default_penumbra( void )
{
    return 1.f;
}
//////////////////////////////////////////////////////////////////////////
gp_uint8_t gp_get_default_curve_quality( void )
{
    return 32;
}
//////////////////////////////////////////////////////////////////////////
gp_uint8_t gp_get_default_ellipse_quality( void )
{
    return 64;
}
//////////////////////////////////////////////////////////////////////////
gp_uint8_t gp_get_default_rect_quality( void )
{
    return 16;
}
//////////////////////////////////////////////////////////////////////////
static void __canvas_default_state_setup( gp_canvas_t * _canvas )
{
    _canvas->state_cook[0].next = GP_NULLPTR;
    _canvas->state_cook[0].prev = GP_NULLPTR;
    _canvas->state_cook[0].thickness = gp_get_default_thickness();
    _canvas->state_cook[0].penumbra = gp_get_default_penumbra();
    _canvas->state_cook[0].color.r = 1.f;
    _canvas->state_cook[0].color.g = 1.f;
    _canvas->state_cook[0].color.b = 1.f;
    _canvas->state_cook[0].color.a = 1.f;
    _canvas->state_cook[0].fill = GP_FALSE;
    _canvas->state_cook[0].uv_ou = 0.f;
    _canvas->state_cook[0].uv_ov = 0.f;
    _canvas->state_cook[0].uv_su = 1.f;
    _canvas->state_cook[0].uv_sv = 1.f;
    _canvas->state_cook[0].curve_quality = gp_get_default_curve_quality();
    _canvas->state_cook[0].curve_quality_inv = 1.f / (float)_canvas->state_cook[0].curve_quality;
    _canvas->state_cook[0].ellipse_quality = gp_get_default_ellipse_quality();
    _canvas->state_cook[0].ellipse_quality_inv = 1.f / (float)_canvas->state_cook[0].ellipse_quality;
    _canvas->state_cook[0].rect_quality = gp_get_default_rect_quality();
    _canvas->state_cook[0].rect_quality_inv = 1.f / (float)_canvas->state_cook[0].rect_quality;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_canvas_create( gp_canvas_t ** _canvas, gp_malloc_t _malloc, gp_realloc_t _realloc, gp_free_t _free, void * _ud )
{
    gp_canvas_t * canvas = (gp_canvas_t *)(*_malloc)(sizeof( gp_canvas_t ), _ud);

    canvas->state_stack = 0;
    canvas->state_invalidate = GP_TRUE;
    canvas->states = GP_NULLPTR;

    canvas->lines = GP_NULLPTR;
    canvas->rects = GP_NULLPTR;
    canvas->rounded_rects = GP_NULLPTR;
    canvas->ellipses = GP_NULLPTR;

    __canvas_default_state_setup( canvas );

    canvas->malloc = _malloc;
    canvas->realloc = _realloc;
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

    _canvas->state_stack = 0;
    _canvas->state_invalidate = GP_TRUE;

    __canvas_default_state_setup( _canvas );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_thickness( gp_canvas_t * _canvas, float _thickness )
{
    gp_state_t * state = GP_GET_STATE( _canvas );

    if( state->thickness == _thickness )
    {
        return GP_SUCCESSFUL;
    }

    state->thickness = _thickness;

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_thickness( const gp_canvas_t * _canvas, float * _thickness )
{
    const gp_state_t * state = GP_GET_STATE( _canvas );

    *_thickness = state->thickness;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_penumbra( gp_canvas_t * _canvas, float _penumbra )
{
    gp_state_t * state = GP_GET_STATE( _canvas );

#if defined(GP_DEBUG)
    if( _penumbra >= state->thickness * 0.5f )
    {
        return GP_FAILURE;
    }
#endif

    if( state->penumbra == _penumbra )
    {
        return GP_SUCCESSFUL;
    }

    state->penumbra = _penumbra;

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_penumbra( const gp_canvas_t * _canvas, float * _penumbra )
{
    const gp_state_t * state = GP_GET_STATE( _canvas );

    *_penumbra = state->penumbra;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_color( gp_canvas_t * _canvas, float _r, float _g, float _b, float _a )
{
#if defined(GP_DEBUG)
    if( _r > 1.f || _r < 0.f ||
        _g > 1.f || _g < 0.f ||
        _b > 1.f || _b < 0.f ||
        _a > 1.f || _a < 0.f )
    {
        return GP_FAILURE;
    }
#endif

    gp_state_t * state = GP_GET_STATE( _canvas );

    if( state->color.r == _r &&
        state->color.g == _g &&
        state->color.b == _b &&
        state->color.a == _a )
    {
        return GP_SUCCESSFUL;
    }

    state->color.r = _r;
    state->color.g = _g;
    state->color.b = _b;
    state->color.a = _a;

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_color( const gp_canvas_t * _canvas, gp_color_t * _color )
{
    const gp_state_t * state = GP_GET_STATE( _canvas );

    *_color = state->color;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_uv_offset( gp_canvas_t * _canvas, float _ou, float _ov, float _su, float _sv )
{
    gp_state_t * state = GP_GET_STATE( _canvas );

    if( state->uv_ou == _ou &&
        state->uv_ov == _ov &&
        state->uv_su == _su &&
        state->uv_sv == _sv )
    {
        return GP_SUCCESSFUL;
    }

    state->uv_ou = _ou;
    state->uv_ov = _ov;
    state->uv_su = _su;
    state->uv_sv = _sv;

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_uv_offset( const gp_canvas_t * _canvas, float * _ou, float * _ov, float * _su, float * _sv )
{
    const gp_state_t * state = GP_GET_STATE( _canvas );

    *_ou = state->uv_ou;
    *_ov = state->uv_ov;
    *_su = state->uv_su;
    *_sv = state->uv_sv;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_curve_quality( gp_canvas_t * _canvas, gp_uint8_t _quality )
{
#if defined(GP_DEBUG)
    if( _quality == 0 )
    {
        return GP_FAILURE;
    }
#endif

    gp_state_t * state = GP_GET_STATE( _canvas );

    if( state->curve_quality == _quality )
    {
        return GP_SUCCESSFUL;
    }

    state->curve_quality = _quality;
    state->curve_quality_inv = 1.f / (float)_quality;

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_curve_quality( const gp_canvas_t * _canvas, gp_uint8_t * _quality )
{
    const gp_state_t * state = GP_GET_STATE( _canvas );

    *_quality = state->curve_quality;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_ellipse_quality( gp_canvas_t * _canvas, gp_uint8_t _quality )
{
#if defined(GP_DEBUG)
    if( _quality < 4 )
    {
        return GP_FAILURE;
    }
#endif

    gp_state_t * state = GP_GET_STATE( _canvas );

    gp_uint8_t quality = (_quality + 3) / 4 * 4;

    if( state->ellipse_quality == quality )
    {
        return GP_SUCCESSFUL;
    }

    state->ellipse_quality = quality;
    state->ellipse_quality_inv = 1.f / (float)state->ellipse_quality;

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_ellipse_quality( const gp_canvas_t * _canvas, gp_uint8_t * _quality )
{
    const gp_state_t * state = GP_GET_STATE( _canvas );

    *_quality = state->ellipse_quality;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_rect_quality( gp_canvas_t * _canvas, gp_uint8_t _quality )
{
#if defined(GP_DEBUG)
    if( _quality < 2 )
    {
        return GP_FAILURE;
    }
#endif

    gp_state_t * state = GP_GET_STATE( _canvas );

    if( state->rect_quality == _quality )
    {
        return GP_SUCCESSFUL;
    }

    state->rect_quality = _quality;
    state->rect_quality_inv = 1.f / (float)state->rect_quality;

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_rect_quality( const gp_canvas_t * _canvas, gp_uint8_t * _quality )
{
    const gp_state_t * state = GP_GET_STATE( _canvas );

    *_quality = state->rect_quality;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_begin_fill( gp_canvas_t * _canvas )
{
    gp_state_t * state = GP_GET_STATE( _canvas );

#if defined(GP_DEBUG)
    if( state->fill == GP_TRUE )
    {
        return GP_FAILURE;
    }
#endif    

    state->fill = GP_TRUE;

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_end_fill( gp_canvas_t * _canvas )
{
    gp_state_t * state = GP_GET_STATE( _canvas );

#if defined(GP_DEBUG)
    if( state->fill == GP_FALSE )
    {
        return GP_FAILURE;
    }
#endif

    state->fill = GP_FALSE;

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_push_state( gp_canvas_t * _canvas )
{
#if defined(GP_DEBUG)
    if( _canvas->state_stack + 1 == GP_STATE_STACK_MAX )
    {
        return GP_FAILURE;
    }
#endif

    ++_canvas->state_stack;

    _canvas->state_cook[_canvas->state_stack] = _canvas->state_cook[_canvas->state_stack - 1];

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_pop_state( gp_canvas_t * _canvas )
{
#if defined(GP_DEBUG)
    if( _canvas->state_stack == 0 )
    {
        return GP_FAILURE;
    }
#endif

    --_canvas->state_stack;

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

    *s = *GP_GET_STATE( _canvas );

    GP_LIST_PUSHBACK( gp_state_t, _canvas->states, s );

    _canvas->state_invalidate = GP_FALSE;

    return s;
}
//////////////////////////////////////////////////////////////////////////
#define GP_COPY_STATE(canvas) __copy_state( canvas )
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_move_to( gp_canvas_t * _canvas, float _x, float _y )
{
#if defined(GP_DEBUG)
    if( _canvas->lines != GP_NULLPTR )
    {
        gp_line_t * l = GP_LIST_BACK( _canvas->lines );

        if( l->edges == GP_NULLPTR )
        {
            return GP_FAILURE;
        }
    }
#endif

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
#if defined(GP_DEBUG)
    if( _canvas->lines == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

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
gp_result_t gp_rect( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height )
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
gp_result_t gp_rounded_rect( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _radius )
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
gp_result_t gp_circle( gp_canvas_t * _canvas, float _x, float _y, float _radius )
{
    gp_result_t result = gp_ellipse( _canvas, _x, _y, _radius, _radius );

    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_ellipse( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height )
{
    gp_ellipse_t * e = GP_NEW( _canvas, gp_ellipse_t );
    e->next = GP_NULLPTR;
    e->prev = GP_NULLPTR;

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

    GP_DEBUG_CALL( gp_calculate_mesh_line_size, (_canvas, _mesh) );
    GP_DEBUG_CALL( gp_calculate_mesh_rect_size, (_canvas, _mesh) );
    GP_DEBUG_CALL( gp_calculate_mesh_rounded_rect_size, (_canvas, _mesh) );
    GP_DEBUG_CALL( gp_calculate_mesh_ellipse_size, (_canvas, _mesh) );

    _mesh->color.r = 1.f;
    _mesh->color.g = 1.f;
    _mesh->color.b = 1.f;
    _mesh->color.a = 1.f;

    _mesh->positions_buffer = GP_NULLPTR;
    _mesh->positions_offset = ~0U;
    _mesh->positions_stride = ~0U;

    _mesh->colors_buffer = GP_NULLPTR;
    _mesh->colors_offset = ~0U;
    _mesh->colors_stride = ~0U;

    _mesh->uv_buffer = GP_NULLPTR;
    _mesh->uv_offset = ~0U;
    _mesh->uv_stride = ~0U;

    _mesh->indices_buffer = GP_NULLPTR;
    _mesh->indices_offset = ~0U;
    _mesh->indices_stride = ~0U;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_render( const gp_canvas_t * _canvas, const gp_mesh_t * _mesh )
{
#if defined(GP_DEBUG)
    if( _mesh->positions_buffer != GP_NULLPTR && (_mesh->positions_offset == ~0U || _mesh->positions_stride == ~0U) )
    {
        return GP_FAILURE;
    }

    if( _mesh->colors_buffer != GP_NULLPTR && (_mesh->colors_offset == ~0U || _mesh->colors_stride == ~0U) )
    {
        return GP_FAILURE;
    }

    if( _mesh->uv_buffer != GP_NULLPTR && (_mesh->uv_offset == ~0U || _mesh->uv_stride == ~0U) )
    {
        return GP_FAILURE;
    }

    if( _mesh->indices_buffer != GP_NULLPTR && (_mesh->indices_offset == ~0U || _mesh->indices_stride == ~0U) )
    {
        return GP_FAILURE;
    }
#endif

    gp_uint16_t vertex_iterator = 0;
    gp_uint16_t index_iterator = 0;

    GP_DEBUG_CALL( gp_render_line, (_canvas, _mesh, &vertex_iterator, &index_iterator) );
    GP_DEBUG_CALL( gp_render_rect, (_canvas, _mesh, &vertex_iterator, &index_iterator) );
    GP_DEBUG_CALL( gp_render_rounded_rect, (_canvas, _mesh, &vertex_iterator, &index_iterator) );
    GP_DEBUG_CALL( gp_render_ellipse, (_canvas, _mesh, &vertex_iterator, &index_iterator) );

    return GP_SUCCESSFUL;
}