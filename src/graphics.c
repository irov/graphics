#include "graphics/graphics.h"

#include "line.h"
#include "rect.h"
#include "rounded_rect.h"
#include "ellipse.h"
#include "compound.h"

#include "struct.h"
#include "detail.h"

static gp_result_t __push_render_order( gp_canvas_t * _canvas, gp_render_order_type_e _type, void * _primitive );

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
float gp_get_default_outline_width( void )
{
    return 0.f;
}
//////////////////////////////////////////////////////////////////////////
float gp_get_default_miter_limit( void )
{
    return 10.f;
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
    _canvas->state_cook[0].fill = GP_FALSE;
    _canvas->state_cook[0].line_cap = GP_LINE_CAP_BUTT;
    _canvas->state_cook[0].line_join = GP_LINE_JOIN_MITER;
    _canvas->state_cook[0].fill_rule = GP_FILL_RULE_NON_ZERO;
    _canvas->state_cook[0].thickness = gp_get_default_thickness();
    _canvas->state_cook[0].outline_width = 0.f;
    _canvas->state_cook[0].penumbra = gp_get_default_penumbra();
    _canvas->state_cook[0].inner_radius = 0.f;
    _canvas->state_cook[0].miter_limit = gp_get_default_miter_limit();
    _canvas->state_cook[0].dash_offset = 0.f;
    _canvas->state_cook[0].dash_count = 0;
    _canvas->state_cook[0].color.r = 1.f;
    _canvas->state_cook[0].color.g = 1.f;
    _canvas->state_cook[0].color.b = 1.f;
    _canvas->state_cook[0].color.a = 1.f;
    _canvas->state_cook[0].outline_color.r = 1.f;
    _canvas->state_cook[0].outline_color.g = 1.f;
    _canvas->state_cook[0].outline_color.b = 1.f;
    _canvas->state_cook[0].outline_color.a = 1.f;
    _canvas->state_cook[0].uv_ou = 0.f;
    _canvas->state_cook[0].uv_ov = 0.f;
    _canvas->state_cook[0].uv_su = 1.f;
    _canvas->state_cook[0].uv_sv = 1.f;
    _canvas->state_cook[0].curve_quality = gp_get_default_curve_quality();
    _canvas->state_cook[0].ellipse_quality = gp_get_default_ellipse_quality();
    _canvas->state_cook[0].rect_quality = gp_get_default_rect_quality();
    _canvas->state_cook[0].curve_quality_inv = 1.f / (float)_canvas->state_cook[0].curve_quality;
    _canvas->state_cook[0].ellipse_quality_inv = 1.f / (float)_canvas->state_cook[0].ellipse_quality;
    _canvas->state_cook[0].rect_quality_inv = 1.f / (float)_canvas->state_cook[0].rect_quality;
    _canvas->state_cook[0].next = GP_NULLPTR;
    _canvas->state_cook[0].prev = GP_NULLPTR;
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
    canvas->ellipse_arcs = GP_NULLPTR;
    canvas->compound_fills = GP_NULLPTR;
    canvas->active_compound_fill = GP_NULLPTR;
    canvas->compound_previous_fill = GP_FALSE;
    canvas->render_order = GP_NULLPTR;

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
    GP_LIST_DESTROY( _canvas, gp_ellipse_arc_t, _canvas->ellipse_arcs );
    GP_LIST_DESTROY( _canvas, gp_compound_fill_t, _canvas->compound_fills );
    GP_LIST_DESTROY( _canvas, gp_render_order_t, _canvas->render_order );

    _canvas->state_stack = 0;
    _canvas->state_invalidate = GP_TRUE;
    _canvas->active_compound_fill = GP_NULLPTR;
    _canvas->compound_previous_fill = GP_FALSE;

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
    if( _penumbra > state->thickness * 0.5f )
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
gp_result_t gp_set_inner_radius( gp_canvas_t * _canvas, float _inner_radius )
{
#if defined(GP_DEBUG)
    if( _inner_radius < 0.f || _inner_radius > 1.f )
    {
        return GP_FAILURE;
    }
#endif

    gp_state_t * state = GP_GET_STATE( _canvas );

    if( state->inner_radius == _inner_radius )
    {
        return GP_SUCCESSFUL;
    }

    state->inner_radius = _inner_radius;

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_inner_radius( const gp_canvas_t * _canvas, float * _inner_radius )
{
    const gp_state_t * state = GP_GET_STATE( _canvas );

    *_inner_radius = state->inner_radius;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_outline_width( gp_canvas_t * _canvas, float _width )
{
    gp_state_t * state = GP_GET_STATE( _canvas );

    if( state->outline_width == _width )
    {
        return GP_SUCCESSFUL;
    }

    state->outline_width = _width;

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_outline_width( const gp_canvas_t * _canvas, float * _width )
{
    const gp_state_t * state = GP_GET_STATE( _canvas );

    *_width = state->outline_width;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_outline_color( gp_canvas_t * _canvas, float _r, float _g, float _b, float _a )
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

    if( state->outline_color.r == _r &&
        state->outline_color.g == _g &&
        state->outline_color.b == _b &&
        state->outline_color.a == _a )
    {
        return GP_SUCCESSFUL;
    }

    state->outline_color.r = _r;
    state->outline_color.g = _g;
    state->outline_color.b = _b;
    state->outline_color.a = _a;

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_outline_color( const gp_canvas_t * _canvas, gp_color_t * _color )
{
    const gp_state_t * state = GP_GET_STATE( _canvas );

    *_color = state->outline_color;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_line_cap( gp_canvas_t * _canvas, gp_line_cap_t _cap )
{
#if defined(GP_DEBUG)
    if( _cap != GP_LINE_CAP_BUTT && _cap != GP_LINE_CAP_SQUARE && _cap != GP_LINE_CAP_ROUND )
    {
        return GP_FAILURE;
    }
#endif

    gp_state_t * state = GP_GET_STATE( _canvas );
    if( state->line_cap == _cap )
    {
        return GP_SUCCESSFUL;
    }

    state->line_cap = _cap;
    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_line_cap( const gp_canvas_t * _canvas, gp_line_cap_t * _cap )
{
    *_cap = GP_GET_STATE( _canvas )->line_cap;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_line_join( gp_canvas_t * _canvas, gp_line_join_t _join )
{
#if defined(GP_DEBUG)
    if( _join != GP_LINE_JOIN_MITER && _join != GP_LINE_JOIN_BEVEL && _join != GP_LINE_JOIN_ROUND )
    {
        return GP_FAILURE;
    }
#endif

    gp_state_t * state = GP_GET_STATE( _canvas );
    if( state->line_join == _join )
    {
        return GP_SUCCESSFUL;
    }

    state->line_join = _join;
    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_line_join( const gp_canvas_t * _canvas, gp_line_join_t * _join )
{
    *_join = GP_GET_STATE( _canvas )->line_join;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_miter_limit( gp_canvas_t * _canvas, float _limit )
{
#if defined(GP_DEBUG)
    if( _limit < 1.f )
    {
        return GP_FAILURE;
    }
#endif

    gp_state_t * state = GP_GET_STATE( _canvas );
    if( state->miter_limit == _limit )
    {
        return GP_SUCCESSFUL;
    }

    state->miter_limit = _limit;
    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_miter_limit( const gp_canvas_t * _canvas, float * _limit )
{
    *_limit = GP_GET_STATE( _canvas )->miter_limit;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_fill_rule( gp_canvas_t * _canvas, gp_fill_rule_t _rule )
{
#if defined(GP_DEBUG)
    if( _rule != GP_FILL_RULE_NON_ZERO && _rule != GP_FILL_RULE_EVEN_ODD )
    {
        return GP_FAILURE;
    }
#endif

    gp_state_t * state = GP_GET_STATE( _canvas );
    if( state->fill_rule == _rule )
    {
        return GP_SUCCESSFUL;
    }

    state->fill_rule = _rule;
    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_fill_rule( const gp_canvas_t * _canvas, gp_fill_rule_t * _rule )
{
    *_rule = GP_GET_STATE( _canvas )->fill_rule;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_line_dash( gp_canvas_t * _canvas, const float * _pattern, gp_uint8_t _count, float _offset )
{
#if defined(GP_DEBUG)
    if( _pattern == GP_NULLPTR || _count == 0 || _count > GP_DASH_PATTERN_MAX )
    {
        return GP_FAILURE;
    }

    for( gp_uint8_t index = 0; index != _count; ++index )
    {
        if( _pattern[index] <= 0.f )
        {
            return GP_FAILURE;
        }
    }
#endif

    gp_state_t * state = GP_GET_STATE( _canvas );
    for( gp_uint8_t index = 0; index != _count; ++index )
    {
        state->dash_pattern[index] = _pattern[index];
    }

    state->dash_count = _count;
    state->dash_offset = _offset;
    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_clear_line_dash( gp_canvas_t * _canvas )
{
    gp_state_t * state = GP_GET_STATE( _canvas );
    state->dash_count = 0;
    state->dash_offset = 0.f;
    _canvas->state_invalidate = GP_TRUE;

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
gp_result_t gp_begin_compound_fill( gp_canvas_t * _canvas, gp_fill_rule_t _rule )
{
#if defined(GP_DEBUG)
    if( _canvas->active_compound_fill != GP_NULLPTR ||
        (_rule != GP_FILL_RULE_NON_ZERO && _rule != GP_FILL_RULE_EVEN_ODD) )
    {
        return GP_FAILURE;
    }
#endif

    gp_compound_fill_t * compound = GP_NEW( _canvas, gp_compound_fill_t );
    compound->next = GP_NULLPTR;
    compound->prev = GP_NULLPTR;
    compound->first_line = GP_NULLPTR;
    compound->last_line = GP_NULLPTR;
    compound->fill_rule = _rule;

    GP_LIST_PUSHBACK( gp_compound_fill_t, _canvas->compound_fills, compound );

    gp_state_t * state = GP_GET_STATE( _canvas );
    _canvas->compound_previous_fill = state->fill;
    _canvas->active_compound_fill = compound;

    state->fill = GP_TRUE;
    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_end_compound_fill( gp_canvas_t * _canvas )
{
#if defined(GP_DEBUG)
    if( _canvas->active_compound_fill == GP_NULLPTR || _canvas->active_compound_fill->first_line == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    gp_compound_fill_t * compound = _canvas->active_compound_fill;
    _canvas->active_compound_fill = GP_NULLPTR;

    gp_state_t * state = GP_GET_STATE( _canvas );
    state->fill = _canvas->compound_previous_fill;
    _canvas->state_invalidate = GP_TRUE;

    GP_CALL( __push_render_order, (_canvas, GP_RENDER_ORDER_COMPOUND_FILL, compound) );

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

    _canvas->state_invalidate = GP_TRUE;

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
static gp_result_t __push_render_order( gp_canvas_t * _canvas, gp_render_order_type_e _type, void * _primitive )
{
    gp_render_order_t * order = GP_NEW( _canvas, gp_render_order_t );
    order->next = GP_NULLPTR;
    order->prev = GP_NULLPTR;
    order->type = _type;
    order->primitive = _primitive;

    GP_LIST_PUSHBACK( gp_render_order_t, _canvas->render_order, order );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_point_move_to( gp_canvas_t * _canvas, float _x, float _y )
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
    l->closed = GP_FALSE;
    l->compound_fill = _canvas->active_compound_fill;
    l->realloc = _canvas->realloc;
    l->free = _canvas->free;
    l->ud = _canvas->ud;

    GP_LIST_PUSHBACK( gp_line_t, _canvas->lines, l );

    if( _canvas->active_compound_fill == GP_NULLPTR )
    {
        GP_CALL( __push_render_order, (_canvas, GP_RENDER_ORDER_LINE, l) );
    }
    else
    {
        gp_compound_fill_t * compound = _canvas->active_compound_fill;
        if( compound->first_line == GP_NULLPTR )
        {
            compound->first_line = l;
        }

        compound->last_line = l;
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_point_line_to( gp_canvas_t * _canvas, float _x, float _y )
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
gp_result_t gp_point_quadratic_curve_to( gp_canvas_t * _canvas, float _p0x, float _p0y, float _x, float _y )
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
gp_result_t gp_point_bezier_curve_to( gp_canvas_t * _canvas, float _p0x, float _p0y, float _p1x, float _p1y, float _x, float _y )
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
gp_result_t gp_point_close( gp_canvas_t * _canvas )
{
#if defined(GP_DEBUG)
    if( _canvas->lines == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    gp_line_t * line = GP_LIST_BACK( _canvas->lines );

#if defined(GP_DEBUG)
    if( line->edges == GP_NULLPTR || line->closed == GP_TRUE )
    {
        return GP_FAILURE;
    }
#endif

    line->closed = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_point_tangent_arc_to( gp_canvas_t * _canvas, float _x1, float _y1, float _x2, float _y2, float _radius )
{
#if defined(GP_DEBUG)
    if( _canvas->lines == GP_NULLPTR || _radius < 0.f )
    {
        return GP_FAILURE;
    }
#endif

    gp_line_t * line = GP_LIST_BACK( _canvas->lines );
    const gp_line_point_t * current = GP_LIST_BACK( line->points );

    float incoming_x = current->p.x - _x1;
    float incoming_y = current->p.y - _y1;
    float outgoing_x = _x2 - _x1;
    float outgoing_y = _y2 - _y1;
    float incoming_length = GP_MATH_SQRTF( incoming_x * incoming_x + incoming_y * incoming_y );
    float outgoing_length = GP_MATH_SQRTF( outgoing_x * outgoing_x + outgoing_y * outgoing_y );

    if( incoming_length <= 0.0001f || outgoing_length <= 0.0001f || _radius <= 0.0001f )
    {
        return gp_point_line_to( _canvas, _x1, _y1 );
    }

    incoming_x /= incoming_length;
    incoming_y /= incoming_length;
    outgoing_x /= outgoing_length;
    outgoing_y /= outgoing_length;

    float dot = incoming_x * outgoing_x + incoming_y * outgoing_y;
    if( dot < -1.f )
    {
        dot = -1.f;
    }
    else if( dot > 1.f )
    {
        dot = 1.f;
    }

    float angle = GP_MATH_ACOSF( dot );
    float half_angle = angle * 0.5f;
    float sin_half_angle = GP_MATH_SINF( half_angle );
    float tan_half_angle = sin_half_angle / GP_MATH_COSF( half_angle );
    if( sin_half_angle <= 0.0001f || tan_half_angle <= 0.0001f )
    {
        return gp_point_line_to( _canvas, _x1, _y1 );
    }

    float tangent_distance = _radius / tan_half_angle;
    if( tangent_distance > incoming_length )
    {
        tangent_distance = incoming_length;
    }
    if( tangent_distance > outgoing_length )
    {
        tangent_distance = outgoing_length;
    }

    float effective_radius = tangent_distance * tan_half_angle;
    float tangent0_x = _x1 + incoming_x * tangent_distance;
    float tangent0_y = _y1 + incoming_y * tangent_distance;
    float tangent1_x = _x1 + outgoing_x * tangent_distance;
    float tangent1_y = _y1 + outgoing_y * tangent_distance;
    float bisector_x = incoming_x + outgoing_x;
    float bisector_y = incoming_y + outgoing_y;
    float bisector_length = GP_MATH_SQRTF( bisector_x * bisector_x + bisector_y * bisector_y );
    if( bisector_length <= 0.0001f )
    {
        return gp_point_line_to( _canvas, _x1, _y1 );
    }

    bisector_x /= bisector_length;
    bisector_y /= bisector_length;
    float center_distance = effective_radius / sin_half_angle;
    float center_x = _x1 + bisector_x * center_distance;
    float center_y = _y1 + bisector_y * center_distance;
    float angle_from = GP_MATH_ATAN2F( tangent0_y - center_y, tangent0_x - center_x );
    float angle_to = GP_MATH_ATAN2F( tangent1_y - center_y, tangent1_x - center_x );
    float turn = (-incoming_x) * outgoing_y - (-incoming_y) * outgoing_x;

    if( turn > 0.f )
    {
        while( angle_to < angle_from )
        {
            angle_to += gp_constant_two_pi;
        }
    }
    else
    {
        while( angle_to > angle_from )
        {
            angle_to -= gp_constant_two_pi;
        }
    }

    GP_CALL( gp_arc_to, (_canvas, center_x, center_y, effective_radius, angle_from, angle_to) );

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
    GP_CALL( __push_render_order, (_canvas, GP_RENDER_ORDER_RECT, r) );

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
    GP_CALL( __push_render_order, (_canvas, GP_RENDER_ORDER_ROUNDED_RECT, rr) );

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
    GP_CALL( __push_render_order, (_canvas, GP_RENDER_ORDER_ELLIPSE, e) );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_arc( gp_canvas_t * _canvas, float _x, float _y, float _radius, float _angle_from, float _angle_to )
{
    gp_result_t result = gp_ellipse_arc( _canvas, _x, _y, _radius, _radius, _angle_from, _angle_to );

    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_arc_to( gp_canvas_t * _canvas, float _x, float _y, float _radius, float _angle_from, float _angle_to )
{
    gp_result_t result = gp_ellipse_arc_to( _canvas, _x, _y, _radius, _radius, _angle_from, _angle_to );

    return result;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t gp_ellipse_arc_internal( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _angle_from, float _angle_to, gp_bool_t _move_to )
{
#if defined(GP_DEBUG)
    if( _width <= 0.f || _height <= 0.f )
    {
        return GP_FAILURE;
    }
#endif

    float angle_delta = _angle_to - _angle_from;
    if( GP_EQUAL_F( angle_delta, 0.000001f ) == GP_TRUE )
    {
        return GP_SUCCESSFUL;
    }

    const gp_state_t * state = GP_GET_STATE( _canvas );
    if( state->fill == GP_TRUE && _move_to == GP_TRUE )
    {
        gp_ellipse_arc_t * a = GP_NEW( _canvas, gp_ellipse_arc_t );
        a->next = GP_NULLPTR;
        a->prev = GP_NULLPTR;

        a->point.x = _x;
        a->point.y = _y;
        a->radius_width = _width;
        a->radius_height = _height;
        a->angle_from = _angle_from;
        a->angle_to = _angle_to;

        a->state = GP_COPY_STATE( _canvas );

        GP_LIST_PUSHBACK( gp_ellipse_arc_t, _canvas->ellipse_arcs, a );
        GP_CALL( __push_render_order, (_canvas, GP_RENDER_ORDER_ELLIPSE_ARC, a) );

        return GP_SUCCESSFUL;
    }

    float angle_delta_abs = angle_delta < 0.f ? -angle_delta : angle_delta;
    float segment_factor = angle_delta_abs / gp_constant_two_pi * (float)state->ellipse_quality;
    gp_uint32_t segment_count = (gp_uint32_t)segment_factor;
    if( (float)segment_count < segment_factor )
    {
        ++segment_count;
    }

    if( segment_count == 0 )
    {
        segment_count = 1;
    }

#if defined(GP_DISABLE_EXTRA_LINE_POINTS)
    if( segment_count >= GP_LINE_POINTS_MAX )
    {
        segment_count = GP_LINE_POINTS_MAX - 1;
    }
#endif

    float x = _x + GP_MATH_COSF( _angle_from ) * _width;
    float y = _y + GP_MATH_SINF( _angle_from ) * _height;
    if( _move_to == GP_TRUE )
    {
        GP_CALL( gp_point_move_to, (_canvas, x, y) );
    }
    else
    {
        GP_CALL( gp_point_line_to, (_canvas, x, y) );
    }

    for( gp_uint32_t index = 1; index != segment_count + 1; ++index )
    {
        float t = _angle_from + angle_delta * (float)index / (float)segment_count;
        x = _x + GP_MATH_COSF( t ) * _width;
        y = _y + GP_MATH_SINF( t ) * _height;
        GP_CALL( gp_point_line_to, (_canvas, x, y) );
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_ellipse_arc( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _angle_from, float _angle_to )
{
    gp_result_t result = gp_ellipse_arc_internal( _canvas, _x, _y, _width, _height, _angle_from, _angle_to, GP_TRUE );

    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_ellipse_arc_to( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _angle_from, float _angle_to )
{
    gp_result_t result = gp_ellipse_arc_internal( _canvas, _x, _y, _width, _height, _angle_from, _angle_to, GP_FALSE );

    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_calculate_mesh_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh )
{
    _mesh->vertex_count = 0;
    _mesh->index_count = 0;

    GP_CALL( gp_calculate_mesh_line_size, (_canvas, _mesh) );
    GP_CALL( gp_calculate_mesh_rect_size, (_canvas, _mesh) );
    GP_CALL( gp_calculate_mesh_rounded_rect_size, (_canvas, _mesh) );
    GP_CALL( gp_calculate_mesh_ellipse_size, (_canvas, _mesh) );
    GP_CALL( gp_calculate_mesh_compound_fill_size, (_canvas, _mesh) );

#if defined(GP_DEBUG)
    if( _mesh->vertex_count > UINT16_MAX )
    {
        return GP_FAILURE;
    }
#endif

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

    gp_uint32_t vertex_iterator = 0;
    gp_uint32_t index_iterator = 0;

    for( const gp_render_order_t * order = _canvas->render_order; order != GP_NULLPTR; order = order->next )
    {
        switch( order->type )
        {
        case GP_RENDER_ORDER_LINE:
        {
            const gp_line_t * line = (const gp_line_t *)order->primitive;
            GP_CALL( gp_render_line, (line, line->state, _mesh, &vertex_iterator, &index_iterator) );
        } break;
        case GP_RENDER_ORDER_RECT:
        {
            const gp_rect_t * rect = (const gp_rect_t *)order->primitive;
            GP_CALL( gp_render_rect, (rect, rect->state, _mesh, &vertex_iterator, &index_iterator) );
        } break;
        case GP_RENDER_ORDER_ROUNDED_RECT:
        {
            const gp_rounded_rect_t * rounded_rect = (const gp_rounded_rect_t *)order->primitive;
            GP_CALL( gp_render_rounded_rect, (rounded_rect, rounded_rect->state, _mesh, &vertex_iterator, &index_iterator) );
        } break;
        case GP_RENDER_ORDER_ELLIPSE:
        {
            const gp_ellipse_t * ellipse = (const gp_ellipse_t *)order->primitive;
            GP_CALL( gp_render_ellipse, (ellipse, ellipse->state, _mesh, &vertex_iterator, &index_iterator) );
        } break;
        case GP_RENDER_ORDER_ELLIPSE_ARC:
        {
            const gp_ellipse_arc_t * ellipse_arc = (const gp_ellipse_arc_t *)order->primitive;
            GP_CALL( gp_render_ellipse_arc, (ellipse_arc, ellipse_arc->state, _mesh, &vertex_iterator, &index_iterator) );
        } break;
        case GP_RENDER_ORDER_COMPOUND_FILL:
        {
            const gp_compound_fill_t * compound = (const gp_compound_fill_t *)order->primitive;
            GP_CALL( gp_render_compound_fill, (compound, _mesh, &vertex_iterator, &index_iterator) );
        } break;
        default:
            return GP_FAILURE;
        }
    }

    return GP_SUCCESSFUL;
}
