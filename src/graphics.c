#include "graphics/graphics.h"

#include "line.h"
#include "rect.h"
#include "rounded_rect.h"
#include "ellipse.h"
#include "compound.h"
#include "path.h"

#include "struct.h"
#include "detail.h"

static gp_result_t __push_render_order( gp_canvas_t * _canvas, gp_render_order_type_e _type, void * _primitive );
static gp_result_t __render_order( const gp_render_order_t * _order, const gp_mesh_t * _mesh, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator );

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
    _canvas->state_cook[0].stroke_alignment = GP_STROKE_ALIGNMENT_CENTER;
    _canvas->state_cook[0].uv_mode = GP_UV_MODE_BOUNDS;
    _canvas->state_cook[0].thickness = gp_get_default_thickness();
    _canvas->state_cook[0].outline_width = 0.f;
    _canvas->state_cook[0].penumbra = gp_get_default_penumbra();
    _canvas->state_cook[0].inner_radius = 0.f;
    _canvas->state_cook[0].miter_limit = gp_get_default_miter_limit();
    _canvas->state_cook[0].dash_offset = 0.f;
    _canvas->state_cook[0].trim_from = 0.f;
    _canvas->state_cook[0].trim_to = 1.f;
    _canvas->state_cook[0].path_length_offset = 0.f;
    _canvas->state_cook[0].path_total_length = 0.f;
    _canvas->state_cook[0].dash_count = 0;
    _canvas->state_cook[0].start_marker.type = GP_PATH_MARKER_NONE;
    _canvas->state_cook[0].start_marker.length = 0.f;
    _canvas->state_cook[0].start_marker.width = 0.f;
    _canvas->state_cook[0].start_marker.inset = 0.f;
    _canvas->state_cook[0].end_marker = _canvas->state_cook[0].start_marker;
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
#if defined(GP_DEBUG)
    if( GP_MATH_ISFINITE( _thickness ) == 0 || _thickness <= 0.f )
    {
        return GP_FAILURE;
    }
#endif

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
    if( GP_MATH_ISFINITE( _penumbra ) == 0 || _penumbra < 0.f || _penumbra > state->thickness * 0.5f )
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
    if( GP_MATH_ISFINITE( _inner_radius ) == 0 || _inner_radius < 0.f || _inner_radius > 1.f )
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
#if defined(GP_DEBUG)
    if( GP_MATH_ISFINITE( _width ) == 0 || _width < 0.f )
    {
        return GP_FAILURE;
    }
#endif

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
    if( GP_MATH_ISFINITE( _r ) == 0 || GP_MATH_ISFINITE( _g ) == 0 ||
        GP_MATH_ISFINITE( _b ) == 0 || GP_MATH_ISFINITE( _a ) == 0 ||
        _r > 1.f || _r < 0.f ||
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
    if( GP_MATH_ISFINITE( _limit ) == 0 || _limit < 1.f )
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
        if( GP_MATH_ISFINITE( _pattern[index] ) == 0 || _pattern[index] <= 0.f )
        {
            return GP_FAILURE;
        }
    }

    if( GP_MATH_ISFINITE( _offset ) == 0 )
    {
        return GP_FAILURE;
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
gp_result_t gp_set_path_trim( gp_canvas_t * _canvas, float _from, float _to )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || GP_MATH_ISFINITE( _from ) == 0 || GP_MATH_ISFINITE( _to ) == 0 ||
        _from < 0.f || _from > 1.f || _to < 0.f || _to > 1.f )
    {
        return GP_FAILURE;
    }
#endif

    gp_state_t * state = GP_GET_STATE( _canvas );
    state->trim_from = _from;
    state->trim_to = _to;
    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __validate_marker( const gp_path_marker_t * _marker )
{
#if defined(GP_DEBUG)
    if( _marker == GP_NULLPTR )
    {
        return GP_SUCCESSFUL;
    }

    if( _marker->type < GP_PATH_MARKER_NONE || _marker->type > GP_PATH_MARKER_DIAMOND ||
        GP_MATH_ISFINITE( _marker->length ) == 0 || GP_MATH_ISFINITE( _marker->width ) == 0 ||
        GP_MATH_ISFINITE( _marker->inset ) == 0 ||
        _marker->length < 0.f || _marker->width < 0.f || _marker->inset < 0.f )
    {
        return GP_FAILURE;
    }

    if( _marker->type != GP_PATH_MARKER_NONE && (_marker->length <= 0.f || _marker->width <= 0.f) )
    {
        return GP_FAILURE;
    }
#else
    GP_UNUSED( _marker );
#endif

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_path_markers( gp_canvas_t * _canvas, const gp_path_marker_t * _start, const gp_path_marker_t * _end )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif
    GP_CALL( __validate_marker, (_start) );
    GP_CALL( __validate_marker, (_end) );

    gp_path_marker_t none;
    none.type = GP_PATH_MARKER_NONE;
    none.length = 0.f;
    none.width = 0.f;
    none.inset = 0.f;

    gp_state_t * state = GP_GET_STATE( _canvas );
    state->start_marker = _start == GP_NULLPTR ? none : *_start;
    state->end_marker = _end == GP_NULLPTR ? none : *_end;
    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_stroke_alignment( gp_canvas_t * _canvas, gp_stroke_alignment_t _alignment )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _alignment < GP_STROKE_ALIGNMENT_CENTER || _alignment > GP_STROKE_ALIGNMENT_OUTER )
    {
        return GP_FAILURE;
    }
#endif

    GP_GET_STATE( _canvas )->stroke_alignment = _alignment;
    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_stroke_alignment( const gp_canvas_t * _canvas, gp_stroke_alignment_t * _alignment )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _alignment == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    *_alignment = GP_GET_STATE( _canvas )->stroke_alignment;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_uv_mode( gp_canvas_t * _canvas, gp_uv_mode_t _mode )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || (_mode != GP_UV_MODE_BOUNDS && _mode != GP_UV_MODE_PATH) )
    {
        return GP_FAILURE;
    }
#endif

    GP_GET_STATE( _canvas )->uv_mode = _mode;
    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_uv_mode( const gp_canvas_t * _canvas, gp_uv_mode_t * _mode )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _mode == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    *_mode = GP_GET_STATE( _canvas )->uv_mode;

    return GP_SUCCESSFUL;
}
gp_result_t gp_set_color( gp_canvas_t * _canvas, float _r, float _g, float _b, float _a )
{
#if defined(GP_DEBUG)
    if( GP_MATH_ISFINITE( _r ) == 0 || GP_MATH_ISFINITE( _g ) == 0 ||
        GP_MATH_ISFINITE( _b ) == 0 || GP_MATH_ISFINITE( _a ) == 0 ||
        _r > 1.f || _r < 0.f ||
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
#if defined(GP_DEBUG)
    if( GP_MATH_ISFINITE( _ou ) == 0 || GP_MATH_ISFINITE( _ov ) == 0 ||
        GP_MATH_ISFINITE( _su ) == 0 || GP_MATH_ISFINITE( _sv ) == 0 )
    {
        return GP_FAILURE;
    }
#endif

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
    if( _quality < 4 || _quality > 252 )
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
static gp_state_t * __copy_path_state( gp_canvas_t * _canvas, const gp_path_t * _path, const gp_path_style_t * _style,
    float _length_offset, float _total_length, gp_bool_t _start_marker, gp_bool_t _end_marker )
{
    gp_state_t * state = GP_NEW( _canvas, gp_state_t );
    *state = *GP_GET_STATE( _canvas );
    state->next = GP_NULLPTR;
    state->prev = GP_NULLPTR;
    state->curve_quality = _path->curve_quality;
    state->ellipse_quality = _path->ellipse_quality;
    state->curve_quality_inv = 1.f / (float)_path->curve_quality;
    state->ellipse_quality_inv = 1.f / (float)_path->ellipse_quality;
    state->path_length_offset = _length_offset;
    state->path_total_length = _total_length;

    if( _start_marker == GP_FALSE )
    {
        state->start_marker.type = GP_PATH_MARKER_NONE;
    }
    if( _end_marker == GP_FALSE )
    {
        state->end_marker.type = GP_PATH_MARKER_NONE;
    }

    if( (_style->attributes & GP_PATH_STYLE_WIDTH) != 0 )
    {
        state->thickness = _style->width;
    }
    if( (_style->attributes & GP_PATH_STYLE_COLOR) != 0 )
    {
        state->color = _style->color;
    }
    if( (_style->attributes & GP_PATH_STYLE_OUTLINE_COLOR) != 0 )
    {
        state->outline_color = _style->outline_color;
    }

    GP_LIST_PUSHBACK( gp_state_t, _canvas->states, state );

    return state;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_draw_path( gp_canvas_t * _canvas, const gp_path_t * _path )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _path == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    const gp_state_t * canvas_state = GP_GET_STATE( _canvas );
    gp_uint32_t contour_count = 0;
    float total_length = 0.f;
#if defined(GP_DEBUG)
    gp_bool_t all_closed = GP_TRUE;
#endif
    for( const gp_path_contour_t * contour = _path->contours; contour != GP_NULLPTR; contour = contour->next )
    {
        if( contour->edges == GP_NULLPTR )
        {
            continue;
        }
        total_length += gp_path_contour_get_length_internal( _path, contour );
#if defined(GP_DEBUG)
        if( contour->closed == GP_FALSE )
        {
            all_closed = GP_FALSE;
        }
        if( canvas_state->fill == GP_FALSE && contour->closed == GP_FALSE &&
            (canvas_state->trim_from > canvas_state->trim_to || canvas_state->stroke_alignment != GP_STROKE_ALIGNMENT_CENTER) )
        {
            return GP_FAILURE;
        }
#endif
        ++contour_count;
    }

#if defined(GP_DEBUG)
    if( canvas_state->fill == GP_FALSE && canvas_state->trim_from > canvas_state->trim_to &&
        (contour_count != 1 || all_closed == GP_FALSE) )
    {
        return GP_FAILURE;
    }
#endif

    if( contour_count == 0 )
    {
        return GP_SUCCESSFUL;
    }

    gp_compound_fill_t * compound = GP_NULLPTR;
    if( canvas_state->fill == GP_TRUE && contour_count > 1 )
    {
        compound = GP_NEW( _canvas, gp_compound_fill_t );
        compound->next = GP_NULLPTR;
        compound->prev = GP_NULLPTR;
        compound->first_line = GP_NULLPTR;
        compound->last_line = GP_NULLPTR;
        compound->fill_rule = canvas_state->fill_rule;
        GP_LIST_PUSHBACK( gp_compound_fill_t, _canvas->compound_fills, compound );
    }

    float length_offset = 0.f;
    float trim_from_distance = canvas_state->trim_from * total_length;
    float trim_to_distance = canvas_state->trim_to * total_length;
    for( const gp_path_contour_t * contour = _path->contours; contour != GP_NULLPTR; contour = contour->next )
    {
        if( contour->edges == GP_NULLPTR )
        {
            continue;
        }

        float contour_length = gp_path_contour_get_length_internal( _path, contour );
        float contour_end = length_offset + contour_length;
        gp_bool_t wraps = contour_count == 1 && contour->closed == GP_TRUE && canvas_state->trim_from > canvas_state->trim_to;
        gp_bool_t start_marker = wraps == GP_TRUE ||
            (trim_from_distance >= length_offset - 0.00001f && trim_from_distance <= contour_end + 0.00001f)
            ? GP_TRUE
            : GP_FALSE;
        gp_bool_t end_marker = wraps == GP_TRUE ||
            (trim_to_distance >= length_offset - 0.00001f && trim_to_distance <= contour_end + 0.00001f)
            ? GP_TRUE
            : GP_FALSE;

        gp_line_t * line = GP_NEW( _canvas, gp_line_t );
        line->next = GP_NULLPTR;
        line->prev = GP_NULLPTR;
        line->points = GP_NULLPTR;
        line->edges = GP_NULLPTR;
        line->state = GP_NULLPTR;
        line->closed = contour->closed;
        line->compound_fill = compound;
        line->realloc = _canvas->realloc;
        line->free = _canvas->free;
        line->ud = _canvas->ud;

        for( const gp_path_anchor_t * anchor = contour->anchors; anchor != GP_NULLPTR; anchor = anchor->next )
        {
            gp_line_point_t * point = GP_NEW( _canvas, gp_line_point_t );
            point->next = GP_NULLPTR;
            point->prev = GP_NULLPTR;
            point->p = anchor->point;
            point->state = __copy_path_state( _canvas, _path, &anchor->style,
                length_offset, total_length, start_marker, end_marker );
            GP_LIST_PUSHBACK( gp_line_point_t, line->points, point );

            if( line->state == GP_NULLPTR )
            {
                line->state = point->state;
            }
        }

        for( const gp_path_edge_t * source_edge = contour->edges; source_edge != GP_NULLPTR; source_edge = source_edge->next )
        {
            gp_line_edge_t * edge = GP_NEW( _canvas, gp_line_edge_t );
            edge->next = GP_NULLPTR;
            edge->prev = GP_NULLPTR;
            edge->controls = source_edge->controls;
            for( gp_uint32_t index = 0; index != source_edge->controls; ++index )
            {
                edge->p[index] = source_edge->point[index];
            }
            GP_LIST_PUSHBACK( gp_line_edge_t, line->edges, edge );
        }

        GP_LIST_PUSHBACK( gp_line_t, _canvas->lines, line );
        if( compound == GP_NULLPTR )
        {
            GP_CALL( __push_render_order, (_canvas, GP_RENDER_ORDER_LINE, line) );
        }
        else
        {
            if( compound->first_line == GP_NULLPTR )
            {
                compound->first_line = line;
            }
            compound->last_line = line;
        }

        length_offset = contour_end;
    }

    if( compound != GP_NULLPTR )
    {
        GP_CALL( __push_render_order, (_canvas, GP_RENDER_ORDER_COMPOUND_FILL, compound) );
    }

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_rect( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height )
{
    const gp_state_t * state = GP_GET_STATE( _canvas );
    if( state->fill == GP_FALSE && state->dash_count != 0 )
    {
        float points[4][2] = {
            {_x, _y},
            {_x + _width, _y},
            {_x + _width, _y + _height},
            {_x, _y + _height}
        };

        return gp_polyline( _canvas, points, 4, sizeof( points[0] ), GP_NULLPTR, GP_TRUE );
    }

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
    const gp_state_t * state = GP_GET_STATE( _canvas );
    if( state->fill == GP_FALSE && state->dash_count != 0 )
    {
        return gp_rounded_rect_varying( _canvas, _x, _y, _width, _height, _radius, _radius, _radius, _radius );
    }

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
    const gp_state_t * state = GP_GET_STATE( _canvas );
    if( state->fill == GP_FALSE )
    {
        return gp_ellipse_arc( _canvas, _x, _y, _width, _height, 0.f, gp_constant_two_pi );
    }

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
static gp_result_t gp_ellipse_arc_internal( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _angle_from, float _angle_to, gp_bool_t _move_to, gp_bool_t _rounded_caps )
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
    if( state->fill == GP_TRUE )
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
        a->rounded_caps = _rounded_caps;

        a->state = GP_COPY_STATE( _canvas );

        GP_LIST_PUSHBACK( gp_ellipse_arc_t, _canvas->ellipse_arcs, a );
        GP_CALL( __push_render_order, (_canvas, GP_RENDER_ORDER_ELLIPSE_ARC, a) );

        return GP_SUCCESSFUL;
    }

    GP_UNUSED( _move_to );
    GP_UNUSED( _rounded_caps );
    gp_path_t * path;
    GP_CALL( gp_path_create, (&path, _canvas->malloc, _canvas->realloc, _canvas->free, _canvas->ud, state->curve_quality, state->ellipse_quality) );
    GP_CALL( gp_path_ellipse_arc, (path, _x, _y, _width, _height, _angle_from, _angle_to) );
    gp_result_t result = gp_draw_path( _canvas, path );
    GP_CALL( gp_path_destroy, (path) );

    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_ellipse_arc( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _angle_from, float _angle_to )
{
    gp_result_t result = gp_ellipse_arc_internal( _canvas, _x, _y, _width, _height, _angle_from, _angle_to, GP_TRUE, GP_FALSE );

    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_rounded_ring( gp_canvas_t * _canvas, float _x, float _y, float _inner_radius, float _outer_radius, float _angle_from, float _angle_to )
{
#if defined(GP_DEBUG)
    if( _inner_radius < 0.f || _outer_radius <= 0.f || _inner_radius >= _outer_radius )
    {
        return GP_FAILURE;
    }
#endif

    GP_CALL( gp_push_state, (_canvas) );

    gp_state_t * state = GP_GET_STATE( _canvas );
    if( state->fill == GP_FALSE )
    {
        GP_CALL( gp_begin_fill, (_canvas) );
    }

    GP_CALL( gp_set_inner_radius, (_canvas, _inner_radius / _outer_radius) );
    GP_CALL( gp_ellipse_arc_internal, (_canvas, _x, _y, _outer_radius, _outer_radius, _angle_from, _angle_to, GP_TRUE, GP_TRUE) );
    GP_CALL( gp_pop_state, (_canvas) );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __calculate_render_order_size( const gp_render_order_t * _order, gp_mesh_t * _mesh )
{
    gp_canvas_t canvas = {0};
    _mesh->vertex_count = 0;
    _mesh->index_count = 0;

    switch( _order->type )
    {
    case GP_RENDER_ORDER_LINE:
    {
        gp_line_t line = *(const gp_line_t *)_order->primitive;
        line.next = GP_NULLPTR;
        canvas.lines = &line;
        GP_CALL( gp_calculate_mesh_line_size, (&canvas, _mesh) );
    } break;
    case GP_RENDER_ORDER_RECT:
    {
        gp_rect_t rect = *(const gp_rect_t *)_order->primitive;
        rect.next = GP_NULLPTR;
        canvas.rects = &rect;
        GP_CALL( gp_calculate_mesh_rect_size, (&canvas, _mesh) );
    } break;
    case GP_RENDER_ORDER_ROUNDED_RECT:
    {
        gp_rounded_rect_t rounded_rect = *(const gp_rounded_rect_t *)_order->primitive;
        rounded_rect.next = GP_NULLPTR;
        canvas.rounded_rects = &rounded_rect;
        GP_CALL( gp_calculate_mesh_rounded_rect_size, (&canvas, _mesh) );
    } break;
    case GP_RENDER_ORDER_ELLIPSE:
    {
        gp_ellipse_t ellipse = *(const gp_ellipse_t *)_order->primitive;
        ellipse.next = GP_NULLPTR;
        canvas.ellipses = &ellipse;
        GP_CALL( gp_calculate_mesh_ellipse_size, (&canvas, _mesh) );
    } break;
    case GP_RENDER_ORDER_ELLIPSE_ARC:
    {
        gp_ellipse_arc_t ellipse_arc = *(const gp_ellipse_arc_t *)_order->primitive;
        ellipse_arc.next = GP_NULLPTR;
        canvas.ellipse_arcs = &ellipse_arc;
        GP_CALL( gp_calculate_mesh_ellipse_size, (&canvas, _mesh) );
    } break;
    case GP_RENDER_ORDER_COMPOUND_FILL:
    {
        GP_CALL( gp_calculate_mesh_compound_size, ((const gp_compound_fill_t *)_order->primitive, _mesh) );
    } break;
    default:
        return GP_FAILURE;
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __render_order_indices32( const gp_canvas_t * _canvas, const gp_render_order_t * _order, const gp_mesh_t * _size, gp_uint32_t ** _indices )
{
    gp_uint32_t * indices = (gp_uint32_t *)(*_canvas->malloc)( sizeof( gp_uint32_t ) * _size->index_count, _canvas->ud );

    gp_mesh_t mesh = *_size;
    mesh.positions_buffer = GP_NULLPTR;
    mesh.colors_buffer = GP_NULLPTR;
    mesh.uv_buffer = GP_NULLPTR;
    mesh.indices_buffer = indices;
    mesh.indices_offset = 0;
    mesh.indices_stride = sizeof( gp_uint32_t );
    mesh.batches_buffer = GP_NULLPTR;

    gp_uint32_t vertex_iterator = 0;
    gp_uint32_t index_iterator = 0;
    gp_result_t result = __render_order( _order, &mesh, &vertex_iterator, &index_iterator );

#if defined(GP_DEBUG)
    if( result == GP_SUCCESSFUL && (vertex_iterator != _size->vertex_count || index_iterator != _size->index_count || index_iterator % 3U != 0U) )
    {
        result = GP_FAILURE;
    }
#endif

    if( result == GP_FAILURE )
    {
        (*_canvas->free)( indices, _canvas->ud );
        return GP_FAILURE;
    }

    *_indices = indices;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_calculate_mesh_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh )
{
    _mesh->vertex_count = 0;
    _mesh->index_count = 0;
    _mesh->batch_count = 0;

    gp_uint32_t batch_vertex_count = 0;
    for( const gp_render_order_t * order = _canvas->render_order; order != GP_NULLPTR; order = order->next )
    {
        gp_mesh_t primitive_mesh;
        GP_CALL( __calculate_render_order_size, (order, &primitive_mesh) );
        if( primitive_mesh.vertex_count == 0 || primitive_mesh.index_count == 0 )
        {
            continue;
        }

        if( primitive_mesh.vertex_count <= UINT16_MAX )
        {
            if( _mesh->batch_count == 0 || batch_vertex_count + primitive_mesh.vertex_count > UINT16_MAX )
            {
                ++_mesh->batch_count;
                batch_vertex_count = primitive_mesh.vertex_count;
            }
            else
            {
                batch_vertex_count += primitive_mesh.vertex_count;
            }

            _mesh->vertex_count += primitive_mesh.vertex_count;
            _mesh->index_count += primitive_mesh.index_count;
            continue;
        }

        if( _mesh->batch_count == 0 )
        {
            ++_mesh->batch_count;
            batch_vertex_count = 0;
        }

        gp_uint32_t * indices;
        GP_CALL( __render_order_indices32, (_canvas, order, &primitive_mesh, &indices) );
        gp_uint32_t * vertex_map = (gp_uint32_t *)(*_canvas->malloc)( sizeof( gp_uint32_t ) * primitive_mesh.vertex_count, _canvas->ud );
        for( gp_uint32_t vertex = 0; vertex != primitive_mesh.vertex_count; ++vertex )
        {
            vertex_map[vertex] = UINT32_MAX;
        }

        for( gp_uint32_t index = 0; index != primitive_mesh.index_count; index += 3 )
        {
            gp_uint32_t needed = 0;
            for( gp_uint32_t corner = 0; corner != 3; ++corner )
            {
                gp_uint32_t vertex = indices[index + corner];
                gp_bool_t repeated = GP_FALSE;
                for( gp_uint32_t previous = 0; previous != corner; ++previous )
                {
                    repeated |= indices[index + previous] == vertex ? GP_TRUE : GP_FALSE;
                }
                needed += vertex_map[vertex] == UINT32_MAX && repeated == GP_FALSE ? 1U : 0U;
            }

            if( batch_vertex_count + needed > UINT16_MAX )
            {
                ++_mesh->batch_count;
                batch_vertex_count = 0;
                for( gp_uint32_t vertex = 0; vertex != primitive_mesh.vertex_count; ++vertex )
                {
                    vertex_map[vertex] = UINT32_MAX;
                }
            }

            for( gp_uint32_t corner = 0; corner != 3; ++corner )
            {
                gp_uint32_t vertex = indices[index + corner];
                if( vertex_map[vertex] == UINT32_MAX )
                {
                    vertex_map[vertex] = batch_vertex_count++;
                    ++_mesh->vertex_count;
                }
            }
        }

        (*_canvas->free)( vertex_map, _canvas->ud );
        (*_canvas->free)( indices, _canvas->ud );
        _mesh->index_count += primitive_mesh.index_count;
    }

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

    _mesh->batches_buffer = GP_NULLPTR;
    _mesh->batches_offset = ~0U;
    _mesh->batches_stride = ~0U;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_render_batch_t * __mesh_get_batch( const gp_mesh_t * _mesh, gp_uint32_t _index )
{
    if( _mesh->batches_buffer == GP_NULLPTR )
    {
        return GP_NULLPTR;
    }

    return (gp_render_batch_t *)((gp_uint8_t *)_mesh->batches_buffer + _mesh->batches_offset + _mesh->batches_stride * _index);
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __render_order( const gp_render_order_t * _order, const gp_mesh_t * _mesh, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator )
{
    switch( _order->type )
    {
    case GP_RENDER_ORDER_LINE:
    {
        const gp_line_t * line = (const gp_line_t *)_order->primitive;
        return gp_render_line( line, line->state, _mesh, _vertex_iterator, _index_iterator );
    }
    case GP_RENDER_ORDER_RECT:
    {
        const gp_rect_t * rect = (const gp_rect_t *)_order->primitive;
        return gp_render_rect( rect, rect->state, _mesh, _vertex_iterator, _index_iterator );
    }
    case GP_RENDER_ORDER_ROUNDED_RECT:
    {
        const gp_rounded_rect_t * rounded_rect = (const gp_rounded_rect_t *)_order->primitive;
        return gp_render_rounded_rect( rounded_rect, rounded_rect->state, _mesh, _vertex_iterator, _index_iterator );
    }
    case GP_RENDER_ORDER_ELLIPSE:
    {
        const gp_ellipse_t * ellipse = (const gp_ellipse_t *)_order->primitive;
        return gp_render_ellipse( ellipse, ellipse->state, _mesh, _vertex_iterator, _index_iterator );
    }
    case GP_RENDER_ORDER_ELLIPSE_ARC:
    {
        const gp_ellipse_arc_t * ellipse_arc = (const gp_ellipse_arc_t *)_order->primitive;
        return gp_render_ellipse_arc( ellipse_arc, ellipse_arc->state, _mesh, _vertex_iterator, _index_iterator );
    }
    case GP_RENDER_ORDER_COMPOUND_FILL:
    {
        const gp_compound_fill_t * compound = (const gp_compound_fill_t *)_order->primitive;
        return gp_render_compound_fill( compound, _mesh, _vertex_iterator, _index_iterator );
    }
    default:
        return GP_FAILURE;
    }
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


    if( _mesh->batches_buffer != GP_NULLPTR && (_mesh->batches_offset == ~0U || _mesh->batches_stride == ~0U) )
    {
        return GP_FAILURE;
    }
#endif

    gp_uint32_t batch_index = 0;
    gp_uint32_t global_vertex_iterator = 0;
    gp_uint32_t global_index_iterator = 0;
    gp_render_batch_t batch;
    gp_bool_t has_batch = GP_FALSE;
    for( const gp_render_order_t * order = _canvas->render_order; order != GP_NULLPTR; order = order->next )
    {
        gp_mesh_t primitive_size;
        GP_CALL( __calculate_render_order_size, (order, &primitive_size) );
        if( primitive_size.vertex_count == 0 || primitive_size.index_count == 0 )
        {
            continue;
        }

        if( primitive_size.vertex_count > UINT16_MAX )
        {
            if( has_batch == GP_FALSE )
            {
                batch.vertex_offset = global_vertex_iterator;
                batch.vertex_count = 0;
                batch.index_offset = global_index_iterator;
                batch.index_count = 0;
                batch.bounds.minimum_x = 0.f;
                batch.bounds.minimum_y = 0.f;
                batch.bounds.maximum_x = 0.f;
                batch.bounds.maximum_y = 0.f;
                has_batch = GP_TRUE;
            }

            gp_vec2f_t * positions = _mesh->positions_buffer == GP_NULLPTR ? GP_NULLPTR :
                (gp_vec2f_t *)(*_canvas->malloc)( sizeof( gp_vec2f_t ) * primitive_size.vertex_count, _canvas->ud );
            gp_argb_t * colors = _mesh->colors_buffer == GP_NULLPTR ? GP_NULLPTR :
                (gp_argb_t *)(*_canvas->malloc)( sizeof( gp_argb_t ) * primitive_size.vertex_count, _canvas->ud );
            gp_vec2f_t * uv0 = _mesh->uv_buffer == GP_NULLPTR ? GP_NULLPTR :
                (gp_vec2f_t *)(*_canvas->malloc)( sizeof( gp_vec2f_t ) * primitive_size.vertex_count, _canvas->ud );
            gp_uint32_t * indices = (gp_uint32_t *)(*_canvas->malloc)( sizeof( gp_uint32_t ) * primitive_size.index_count, _canvas->ud );

            gp_mesh_t raw_mesh = *_mesh;
            raw_mesh.vertex_count = primitive_size.vertex_count;
            raw_mesh.index_count = primitive_size.index_count;
            raw_mesh.positions_buffer = positions;
            raw_mesh.positions_offset = 0;
            raw_mesh.positions_stride = sizeof( gp_vec2f_t );
            raw_mesh.colors_buffer = colors;
            raw_mesh.colors_offset = 0;
            raw_mesh.colors_stride = sizeof( gp_argb_t );
            raw_mesh.uv_buffer = uv0;
            raw_mesh.uv_offset = 0;
            raw_mesh.uv_stride = sizeof( gp_vec2f_t );
            raw_mesh.indices_buffer = indices;
            raw_mesh.indices_offset = 0;
            raw_mesh.indices_stride = sizeof( gp_uint32_t );
            raw_mesh.batches_buffer = GP_NULLPTR;

            gp_uint32_t raw_vertex_iterator = 0;
            gp_uint32_t raw_index_iterator = 0;
            gp_result_t raw_result = __render_order( order, &raw_mesh, &raw_vertex_iterator, &raw_index_iterator );
            if( raw_result == GP_FAILURE )
            {
                if( positions != GP_NULLPTR ) (*_canvas->free)( positions, _canvas->ud );
                if( colors != GP_NULLPTR ) (*_canvas->free)( colors, _canvas->ud );
                if( uv0 != GP_NULLPTR ) (*_canvas->free)( uv0, _canvas->ud );
                (*_canvas->free)( indices, _canvas->ud );
                return GP_FAILURE;
            }

            gp_uint32_t * vertex_map = (gp_uint32_t *)(*_canvas->malloc)( sizeof( gp_uint32_t ) * primitive_size.vertex_count, _canvas->ud );
            for( gp_uint32_t vertex = 0; vertex != primitive_size.vertex_count; ++vertex )
            {
                vertex_map[vertex] = UINT32_MAX;
            }

            gp_state_t passthrough = {0};
            passthrough.uv_ou = 0.f;
            passthrough.uv_ov = 0.f;
            passthrough.uv_su = 1.f;
            passthrough.uv_sv = 1.f;

            for( gp_uint32_t raw_index = 0; raw_index != primitive_size.index_count; raw_index += 3 )
            {
                gp_uint32_t needed = 0;
                for( gp_uint32_t corner = 0; corner != 3; ++corner )
                {
                    gp_uint32_t raw_vertex = indices[raw_index + corner];
                    gp_bool_t repeated = GP_FALSE;
                    for( gp_uint32_t previous = 0; previous != corner; ++previous )
                    {
                        repeated |= indices[raw_index + previous] == raw_vertex ? GP_TRUE : GP_FALSE;
                    }
                    needed += vertex_map[raw_vertex] == UINT32_MAX && repeated == GP_FALSE ? 1U : 0U;
                }

                if( batch.vertex_count + needed > UINT16_MAX )
                {
                    gp_render_batch_t * output = __mesh_get_batch( _mesh, batch_index++ );
                    if( output != GP_NULLPTR )
                    {
                        *output = batch;
                    }
                    global_vertex_iterator += batch.vertex_count;
                    global_index_iterator += batch.index_count;

                    batch.vertex_offset = global_vertex_iterator;
                    batch.vertex_count = 0;
                    batch.index_offset = global_index_iterator;
                    batch.index_count = 0;
                    batch.bounds.minimum_x = 0.f;
                    batch.bounds.minimum_y = 0.f;
                    batch.bounds.maximum_x = 0.f;
                    batch.bounds.maximum_y = 0.f;
                    for( gp_uint32_t vertex = 0; vertex != primitive_size.vertex_count; ++vertex )
                    {
                        vertex_map[vertex] = UINT32_MAX;
                    }
                }

                for( gp_uint32_t corner = 0; corner != 3; ++corner )
                {
                    gp_uint32_t raw_vertex = indices[raw_index + corner];
                    gp_uint32_t local_vertex = vertex_map[raw_vertex];
                    if( local_vertex == UINT32_MAX )
                    {
                        local_vertex = batch.vertex_count++;
                        vertex_map[raw_vertex] = local_vertex;
                        gp_uint32_t output_vertex = batch.vertex_offset + local_vertex;
                        if( positions != GP_NULLPTR ) GP_CALL( gp_mesh_push_position, (_mesh, output_vertex, positions[raw_vertex].x, positions[raw_vertex].y) );
                        if( colors != GP_NULLPTR ) GP_CALL( gp_mesh_push_color, (_mesh, output_vertex, colors[raw_vertex]) );
                        if( uv0 != GP_NULLPTR ) GP_CALL( gp_mesh_push_uv, (&passthrough, _mesh, output_vertex, uv0[raw_vertex].x, uv0[raw_vertex].y) );
                    }

                    GP_CALL( gp_mesh_push_index, (_mesh, batch.index_offset + batch.index_count, local_vertex) );
                    ++batch.index_count;
                }
            }

            (*_canvas->free)( vertex_map, _canvas->ud );
            if( positions != GP_NULLPTR ) (*_canvas->free)( positions, _canvas->ud );
            if( colors != GP_NULLPTR ) (*_canvas->free)( colors, _canvas->ud );
            if( uv0 != GP_NULLPTR ) (*_canvas->free)( uv0, _canvas->ud );
            (*_canvas->free)( indices, _canvas->ud );
            continue;
        }

        if( has_batch == GP_FALSE || batch.vertex_count + primitive_size.vertex_count > UINT16_MAX )
        {
            if( has_batch == GP_TRUE )
            {
                gp_render_batch_t * output = __mesh_get_batch( _mesh, batch_index++ );
                if( output != GP_NULLPTR )
                {
                    *output = batch;
                }
                global_vertex_iterator += batch.vertex_count;
                global_index_iterator += batch.index_count;
            }

            batch.vertex_offset = global_vertex_iterator;
            batch.vertex_count = 0;
            batch.index_offset = global_index_iterator;
            batch.index_count = 0;
            batch.bounds.minimum_x = 0.f;
            batch.bounds.minimum_y = 0.f;
            batch.bounds.maximum_x = 0.f;
            batch.bounds.maximum_y = 0.f;
            has_batch = GP_TRUE;
        }

        gp_mesh_t window = *_mesh;
        window.vertex_count = UINT16_MAX;
        window.index_count = _mesh->index_count - batch.index_offset;
        if( window.positions_buffer != GP_NULLPTR )
        {
            window.positions_offset += batch.vertex_offset * window.positions_stride;
        }
        if( window.colors_buffer != GP_NULLPTR )
        {
            window.colors_offset += batch.vertex_offset * window.colors_stride;
        }
        if( window.uv_buffer != GP_NULLPTR )
        {
            window.uv_offset += batch.vertex_offset * window.uv_stride;
        }
        if( window.indices_buffer != GP_NULLPTR )
        {
            window.indices_offset += batch.index_offset * window.indices_stride;
        }

        gp_uint32_t vertex_iterator = batch.vertex_count;
        gp_uint32_t index_iterator = batch.index_count;
        GP_CALL( __render_order, (order, &window, &vertex_iterator, &index_iterator) );
        batch.vertex_count = vertex_iterator;
        batch.index_count = index_iterator;
    }

    if( has_batch == GP_TRUE )
    {
        gp_render_batch_t * output = __mesh_get_batch( _mesh, batch_index++ );
        if( output != GP_NULLPTR )
        {
            *output = batch;
        }
    }

#if defined(GP_DEBUG)
    if( batch_index != _mesh->batch_count )
    {
        return GP_FAILURE;
    }
#endif

    if( _mesh->positions_buffer != GP_NULLPTR && _mesh->batches_buffer != GP_NULLPTR )
    {
        for( gp_uint32_t index = 0; index != _mesh->batch_count; ++index )
        {
            gp_render_batch_t * output = __mesh_get_batch( _mesh, index );
            gp_vec2f_t position;
            GP_CALL( gp_mesh_get_position, (_mesh, output->vertex_offset, &position) );
            output->bounds.minimum_x = position.x;
            output->bounds.minimum_y = position.y;
            output->bounds.maximum_x = position.x;
            output->bounds.maximum_y = position.y;
            for( gp_uint32_t vertex = 1; vertex != output->vertex_count; ++vertex )
            {
                GP_CALL( gp_mesh_get_position, (_mesh, output->vertex_offset + vertex, &position) );
                output->bounds.minimum_x = position.x < output->bounds.minimum_x ? position.x : output->bounds.minimum_x;
                output->bounds.minimum_y = position.y < output->bounds.minimum_y ? position.y : output->bounds.minimum_y;
                output->bounds.maximum_x = position.x > output->bounds.maximum_x ? position.x : output->bounds.maximum_x;
                output->bounds.maximum_y = position.y > output->bounds.maximum_y ? position.y : output->bounds.maximum_y;
            }
        }
    }

    return GP_SUCCESSFUL;
}
