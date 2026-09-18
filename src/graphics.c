#include "graphics/graphics.h"

#include "line.h"
#include "rect.h"
#include "rounded_rect.h"
#include "ellipse.h"
#include "compound.h"
#include "path.h"
#include "geometry.h"

#include "struct.h"
#include "detail.h"

#include <string.h>

static gp_result_t __push_render_order( gp_canvas_t * _canvas, gp_render_order_type_e _type, void * _primitive, const gp_state_t * _state );
static gp_result_t __render_order( const gp_render_order_t * _order, const gp_mesh_t * _mesh, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator );

//////////////////////////////////////////////////////////////////////////
gp_uint32_t gp_get_api_version( void )
{
    return GP_API_VERSION;
}
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
gp_result_t gp_calculate_curve_segment_count( float _length, float _maximum_error, gp_uint8_t * _segments )
{
#if defined(GP_DEBUG)
    if( _length <= 0.f || _maximum_error <= 0.f || _segments == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    float estimate = GP_MATH_SQRTF( _length / _maximum_error );
    gp_uint32_t segment_count = (gp_uint32_t)estimate;
    if( (float)segment_count < estimate )
    {
        ++segment_count;
    }

    if( segment_count < 1U )
    {
        segment_count = 1U;
    }

    if( segment_count > 255U )
    {
        segment_count = 255U;
    }

    *_segments = (gp_uint8_t)segment_count;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_calculate_circle_segment_count( float _radius, float _maximum_error, gp_uint8_t * _segments )
{
#if defined(GP_DEBUG)
    if( _radius <= 0.f || _maximum_error <= 0.f || _segments == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    float ratio = _maximum_error / _radius;
    if( ratio > 1.f )
    {
        ratio = 1.f;
    }

    float angle = GP_MATH_ACOSF( 1.f - ratio );
    float estimate = gp_constant_pi / angle;
    gp_uint32_t segment_count = (gp_uint32_t)estimate;
    if( (float)segment_count < estimate )
    {
        ++segment_count;
    }

    segment_count = (segment_count + 3U) / 4U * 4U;
    if( segment_count < 4U )
    {
        segment_count = 4U;
    }

    if( segment_count > 252U )
    {
        segment_count = 252U;
    }

    *_segments = (gp_uint8_t)segment_count;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static void __canvas_default_state_setup( gp_canvas_t * _canvas )
{
    _canvas->state_cook[0].flags = 0;
    _canvas->state_cook[0].line_cap = GP_LINE_CAP_BUTT;
    _canvas->state_cook[0].line_join = GP_LINE_JOIN_MITER;
    _canvas->state_cook[0].fill_rule = GP_FILL_RULE_NON_ZERO;
    _canvas->state_cook[0].stroke_alignment = GP_STROKE_ALIGNMENT_CENTER;
    _canvas->state_cook[0].uv_mode = GP_UV_MODE_BOUNDS;
    _canvas->state_cook[0].stroke_scale = GP_STROKE_SCALE_WORLD;
    _canvas->state_cook[0].translation_x = 0.f;
    _canvas->state_cook[0].translation_y = 0.f;
    _canvas->state_cook[0].axis_x_x = 1.f;
    _canvas->state_cook[0].axis_x_y = 0.f;
    _canvas->state_cook[0].axis_y_x = 0.f;
    _canvas->state_cook[0].axis_y_y = 1.f;
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
    _canvas->state_cook[0].fill.type = GP_FILL_TYPE_SOLID;
    _canvas->state_cook[0].fill.spread = GP_FILL_SPREAD_CLAMP;
    _canvas->state_cook[0].fill.from_x = 0.f;
    _canvas->state_cook[0].fill.from_y = 0.f;
    _canvas->state_cook[0].fill.to_x = 0.f;
    _canvas->state_cook[0].fill.to_y = 0.f;
    _canvas->state_cook[0].fill.color = _canvas->state_cook[0].color;
    _canvas->state_cook[0].fill.stop_count = 0;
    _canvas->state_cook[0].border_width = 0.f;
    _canvas->state_cook[0].border_color.r = 0.f;
    _canvas->state_cook[0].border_color.g = 0.f;
    _canvas->state_cook[0].border_color.b = 0.f;
    _canvas->state_cook[0].border_color.a = 0.f;
    _canvas->state_cook[0].feather = 0.f;
    _canvas->state_cook[0].shadow.offset_x = 0.f;
    _canvas->state_cook[0].shadow.offset_y = 0.f;
    _canvas->state_cook[0].shadow.spread = 0.f;
    _canvas->state_cook[0].shadow.blur = 0.f;
    _canvas->state_cook[0].shadow.color.r = 0.f;
    _canvas->state_cook[0].shadow.color.g = 0.f;
    _canvas->state_cook[0].shadow.color.b = 0.f;
    _canvas->state_cook[0].shadow.color.a = 0.f;
    _canvas->state_cook[0].uv_ou = 0.f;
    _canvas->state_cook[0].uv_ov = 0.f;
    _canvas->state_cook[0].uv_su = 1.f;
    _canvas->state_cook[0].uv_sv = 1.f;
    _canvas->state_cook[0].uv_u = 0.f;
    _canvas->state_cook[0].uv_v = 0.f;
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
    if( _canvas == GP_NULLPTR || _malloc == GP_NULLPTR || _realloc == GP_NULLPTR || _free == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    const gp_size_t alignment = sizeof( void * );
    const gp_size_t render_order_capacity = GP_CANVAS_RENDER_ORDER_CAPACITY;
    if( render_order_capacity > SIZE_MAX / sizeof( gp_render_order_t ) )
    {
        return GP_FAILURE;
    }

    const gp_size_t render_order_size = render_order_capacity * sizeof( gp_render_order_t );
    const gp_size_t recording_capacity = GP_CANVAS_RECORDING_MEMORY_SIZE;
    const gp_size_t scratch_capacity = GP_CANVAS_SCRATCH_MEMORY_SIZE;

    if( render_order_size > SIZE_MAX - recording_capacity )
    {
        return GP_FAILURE;
    }

    gp_size_t arena_size = render_order_size + recording_capacity;

    if( arena_size > SIZE_MAX - scratch_capacity )
    {
        return GP_FAILURE;
    }

    arena_size += scratch_capacity;

    if( arena_size > SIZE_MAX - sizeof( gp_canvas_t ) - alignment + 1U )
    {
        return GP_FAILURE;
    }

    const gp_size_t allocation_size = sizeof( gp_canvas_t ) + alignment - 1U + arena_size;
    gp_canvas_t * canvas = (gp_canvas_t *)(*_malloc)(allocation_size, _ud);
    if( canvas == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    gp_uint8_t * arena_memory = (gp_uint8_t *)(canvas + 1);
    gp_size_t arena_address = (gp_size_t)arena_memory;
    arena_memory += (alignment - arena_address % alignment) % alignment;

    canvas->render_orders = (gp_render_order_t *)arena_memory;
    canvas->render_order_count = 0;

    canvas->recording_arena.memory = arena_memory + render_order_size;
    canvas->recording_arena.capacity = recording_capacity;
    canvas->recording_arena.offset = 0;
    canvas->recording_arena.peak = 0;
    canvas->scratch_arena.memory = canvas->recording_arena.memory + recording_capacity;
    canvas->scratch_arena.capacity = scratch_capacity;
    canvas->scratch_arena.offset = 0;
    canvas->scratch_arena.peak = 0;

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
    canvas->active_rects = GP_NULLPTR;
    canvas->active_quads = GP_NULLPTR;

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
    if( _canvas == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    (*_canvas->free)( _canvas, _canvas->ud );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static void __canvas_reset_recording( gp_canvas_t * _canvas )
{
    _canvas->recording_arena.offset = 0;
    _canvas->scratch_arena.offset = 0;
    _canvas->states = GP_NULLPTR;
    _canvas->lines = GP_NULLPTR;
    _canvas->rects = GP_NULLPTR;
    _canvas->rounded_rects = GP_NULLPTR;
    _canvas->ellipses = GP_NULLPTR;
    _canvas->ellipse_arcs = GP_NULLPTR;
    _canvas->compound_fills = GP_NULLPTR;
    _canvas->render_order_count = 0;
    _canvas->state_invalidate = GP_TRUE;
    _canvas->active_compound_fill = GP_NULLPTR;
    _canvas->compound_previous_fill = GP_FALSE;
    _canvas->active_rects = GP_NULLPTR;
    _canvas->active_quads = GP_NULLPTR;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_canvas_clear( gp_canvas_t * _canvas )
{
    if( _canvas == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    __canvas_reset_recording( _canvas );
    _canvas->state_stack = 0;
    __canvas_default_state_setup( _canvas );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_canvas_get_memory_stats( const gp_canvas_t * _canvas, gp_canvas_memory_stats_t * _stats )
{
    if( _canvas == GP_NULLPTR || _stats == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    _stats->render_order_count = _canvas->render_order_count;
    _stats->render_order_capacity = GP_CANVAS_RENDER_ORDER_CAPACITY;
    _stats->recording_used = _canvas->recording_arena.offset;
    _stats->recording_capacity = _canvas->recording_arena.capacity;
    _stats->recording_peak = _canvas->recording_arena.peak;
    _stats->scratch_used = _canvas->scratch_arena.offset;
    _stats->scratch_capacity = _canvas->scratch_arena.capacity;
    _stats->scratch_peak = _canvas->scratch_arena.peak;

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
    if( state->fill.type == GP_FILL_TYPE_SOLID )
    {
        state->fill.color = state->color;
    }

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
    if( (state->flags & GP_STATE_FLAG_FILL) != 0 )
    {
        return GP_FAILURE;
    }
#endif

    state->flags |= GP_STATE_FLAG_FILL;

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_end_fill( gp_canvas_t * _canvas )
{
    gp_state_t * state = GP_GET_STATE( _canvas );

#if defined(GP_DEBUG)
    if( (state->flags & GP_STATE_FLAG_FILL) == 0 )
    {
        return GP_FAILURE;
    }
#endif

    state->flags &= ~GP_STATE_FLAG_FILL;

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
gp_result_t gp_set_transform( gp_canvas_t * _canvas, float _translation_x, float _translation_y, float _axis_x_x, float _axis_x_y, float _axis_y_x, float _axis_y_y )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    if( GP_MATH_ISFINITE( _translation_x ) == 0 || GP_MATH_ISFINITE( _translation_y ) == 0 )
    {
        return GP_FAILURE;
    }

    if( GP_MATH_ISFINITE( _axis_x_x ) == 0 || GP_MATH_ISFINITE( _axis_x_y ) == 0 || GP_MATH_ISFINITE( _axis_y_x ) == 0 || GP_MATH_ISFINITE( _axis_y_y ) == 0 )
    {
        return GP_FAILURE;
    }

    float determinant = _axis_x_x * _axis_y_y - _axis_x_y * _axis_y_x;

    if( GP_EQUAL_F( determinant, 0.000001f ) == GP_TRUE )
    {
        return GP_FAILURE;
    }
#endif

    gp_state_t * state = GP_GET_STATE( _canvas );
    state->translation_x = _translation_x;
    state->translation_y = _translation_y;
    state->axis_x_x = _axis_x_x;
    state->axis_x_y = _axis_x_y;
    state->axis_y_x = _axis_y_x;
    state->axis_y_y = _axis_y_y;

    if( _translation_x == 0.f && _translation_y == 0.f && _axis_x_x == 1.f && _axis_x_y == 0.f && _axis_y_x == 0.f && _axis_y_y == 1.f )
    {
        state->flags &= ~GP_STATE_FLAG_TRANSFORM;
    }
    else
    {
        state->flags |= GP_STATE_FLAG_TRANSFORM;
    }

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_transform( const gp_canvas_t * _canvas, float * _translation_x, float * _translation_y, float * _axis_x_x, float * _axis_x_y, float * _axis_y_x, float * _axis_y_y )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _translation_x == GP_NULLPTR || _translation_y == GP_NULLPTR || _axis_x_x == GP_NULLPTR || _axis_x_y == GP_NULLPTR || _axis_y_x == GP_NULLPTR || _axis_y_y == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    const gp_state_t * state = GP_GET_STATE( _canvas );
    *_translation_x = state->translation_x;
    *_translation_y = state->translation_y;
    *_axis_x_x = state->axis_x_x;
    *_axis_x_y = state->axis_x_y;
    *_axis_y_x = state->axis_y_x;
    *_axis_y_y = state->axis_y_y;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_reset_transform( gp_canvas_t * _canvas )
{
    gp_result_t result = gp_set_transform( _canvas, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f );

    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_set_stroke_scale( gp_canvas_t * _canvas, gp_stroke_scale_t _scale )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    if( _scale != GP_STROKE_SCALE_WORLD && _scale != GP_STROKE_SCALE_SCREEN )
    {
        return GP_FAILURE;
    }
#endif

    gp_state_t * state = GP_GET_STATE( _canvas );

    if( state->stroke_scale == _scale )
    {
        return GP_SUCCESSFUL;
    }

    state->stroke_scale = _scale;
    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_get_stroke_scale( const gp_canvas_t * _canvas, gp_stroke_scale_t * _scale )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _scale == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    const gp_state_t * state = GP_GET_STATE( _canvas );
    *_scale = state->stroke_scale;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __push_render_order( gp_canvas_t * _canvas, gp_render_order_type_e _type, void * _primitive, const gp_state_t * _state )
{
    if( _canvas->render_order_count == GP_CANVAS_RENDER_ORDER_CAPACITY )
    {
        return GP_FAILURE;
    }

    gp_uint32_t index = _canvas->render_order_count++;
    gp_render_order_t * order = _canvas->render_orders + index;
    order->type = _type;
    order->primitive = _primitive;
    order->state = _state;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __record_payload( gp_canvas_t * _canvas, const void * _payload, gp_size_t _payload_size, const void ** _recorded_payload )
{
    if( _payload_size == 0 )
    {
        *_recorded_payload = _payload;
        return GP_SUCCESSFUL;
    }

    if( _payload == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    void * recorded_payload = gp_canvas_recording_allocate( _canvas, _payload_size );
    if( recorded_payload == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    memcpy( recorded_payload, _payload, _payload_size );
    *_recorded_payload = recorded_payload;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_internal_geometry( gp_canvas_t * _canvas, gp_uint32_t _vertex_count, gp_uint32_t _index_count, gp_internal_geometry_callback_t _callback, const void * _payload, gp_size_t _payload_size )
{
    if( _canvas == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    if( _callback == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    if( _vertex_count == 0 )
    {
        return GP_FAILURE;
    }

    if( _index_count == 0 )
    {
        return GP_FAILURE;
    }

    if( _index_count % 3U != 0U )
    {
        return GP_FAILURE;
    }

    gp_internal_geometry_t * geometry = GP_NEW( _canvas, gp_internal_geometry_t );

    if( geometry == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    geometry->vertex_count = _vertex_count;
    geometry->index_count = _index_count;
    geometry->callback = _callback;
    GP_CALL( __record_payload, (_canvas, _payload, _payload_size, &geometry->payload) );
    geometry->state = GP_COPY_STATE( _canvas );

    if( geometry->state == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    GP_CALL( __push_render_order, (_canvas, GP_RENDER_ORDER_INTERNAL_GEOMETRY, geometry, geometry->state) );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_geometry( gp_canvas_t * _canvas, gp_uint32_t _vertex_count, const void * _positions_buffer, gp_size_t _positions_offset, gp_size_t _positions_stride, const void * _colors_buffer, gp_size_t _colors_offset, gp_size_t _colors_stride, const void * _uv_buffer, gp_size_t _uv_offset, gp_size_t _uv_stride, gp_uint32_t _index_count, const void * _indices_buffer, gp_size_t _indices_offset, gp_size_t _indices_stride, float _translation_x, float _translation_y, float _axis_x_x, float _axis_x_y, float _axis_y_x, float _axis_y_y )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    if( _positions_buffer == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    if( _indices_buffer == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    if( _vertex_count == 0 )
    {
        return GP_FAILURE;
    }

    if( _index_count == 0 )
    {
        return GP_FAILURE;
    }

    if( _index_count % 3U != 0U )
    {
        return GP_FAILURE;
    }

    if( _positions_stride < sizeof( float ) * 2U )
    {
        return GP_FAILURE;
    }

    if( _indices_stride < sizeof( gp_uint32_t ) )
    {
        return GP_FAILURE;
    }

    if( _colors_buffer != GP_NULLPTR )
    {
        if( _colors_stride < sizeof( float ) * 4U )
        {
            return GP_FAILURE;
        }
    }

    if( _uv_buffer != GP_NULLPTR )
    {
        if( _uv_stride < sizeof( float ) * 2U )
        {
            return GP_FAILURE;
        }
    }

    if( GP_MATH_ISFINITE( _translation_x ) == 0 )
    {
        return GP_FAILURE;
    }

    if( GP_MATH_ISFINITE( _translation_y ) == 0 )
    {
        return GP_FAILURE;
    }

    if( GP_MATH_ISFINITE( _axis_x_x ) == 0 )
    {
        return GP_FAILURE;
    }

    if( GP_MATH_ISFINITE( _axis_x_y ) == 0 )
    {
        return GP_FAILURE;
    }

    if( GP_MATH_ISFINITE( _axis_y_x ) == 0 )
    {
        return GP_FAILURE;
    }

    if( GP_MATH_ISFINITE( _axis_y_y ) == 0 )
    {
        return GP_FAILURE;
    }
#endif

    gp_geometry_t * geometry = GP_NEW( _canvas, gp_geometry_t );

    if( geometry == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    geometry->vertex_count = _vertex_count;
    geometry->positions_buffer = _positions_buffer;
    geometry->positions_offset = _positions_offset;
    geometry->positions_stride = _positions_stride;
    geometry->colors_buffer = _colors_buffer;
    geometry->colors_offset = _colors_offset;
    geometry->colors_stride = _colors_stride;
    geometry->uv_buffer = _uv_buffer;
    geometry->uv_offset = _uv_offset;
    geometry->uv_stride = _uv_stride;
    geometry->index_count = _index_count;
    geometry->indices_buffer = _indices_buffer;
    geometry->indices_offset = _indices_offset;
    geometry->indices_stride = _indices_stride;
    geometry->translation_x = _translation_x;
    geometry->translation_y = _translation_y;
    geometry->axis_x_x = _axis_x_x;
    geometry->axis_x_y = _axis_x_y;
    geometry->axis_y_x = _axis_y_x;
    geometry->axis_y_y = _axis_y_y;
    geometry->state = GP_COPY_STATE( _canvas );

    if( geometry->state == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    gp_result_t result = __push_render_order( _canvas, GP_RENDER_ORDER_GEOMETRY, geometry, geometry->state );

    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_textured_rects( gp_canvas_t * _canvas, gp_uint32_t _rect_count, const void * _bounds_buffer, gp_size_t _bounds_offset, gp_size_t _bounds_stride, const void * _uv_buffer, gp_size_t _uv_offset, gp_size_t _uv_stride, const void * _colors_buffer, gp_size_t _colors_offset, gp_size_t _colors_stride, float _translation_x, float _translation_y, float _axis_x_x, float _axis_x_y, float _axis_y_x, float _axis_y_y )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    if( _bounds_buffer == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    if( _uv_buffer == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    if( _rect_count == 0 )
    {
        return GP_FAILURE;
    }

    if( _rect_count > UINT32_MAX / 6U )
    {
        return GP_FAILURE;
    }

    if( _bounds_stride < sizeof( float ) * 4U )
    {
        return GP_FAILURE;
    }

    if( _uv_stride < sizeof( float ) * 4U )
    {
        return GP_FAILURE;
    }

    if( _colors_buffer != GP_NULLPTR )
    {
        if( _colors_stride < sizeof( float ) * 4U )
        {
            return GP_FAILURE;
        }
    }

    if( GP_MATH_ISFINITE( _translation_x ) == 0 )
    {
        return GP_FAILURE;
    }

    if( GP_MATH_ISFINITE( _translation_y ) == 0 )
    {
        return GP_FAILURE;
    }

    if( GP_MATH_ISFINITE( _axis_x_x ) == 0 )
    {
        return GP_FAILURE;
    }

    if( GP_MATH_ISFINITE( _axis_x_y ) == 0 )
    {
        return GP_FAILURE;
    }

    if( GP_MATH_ISFINITE( _axis_y_x ) == 0 )
    {
        return GP_FAILURE;
    }

    if( GP_MATH_ISFINITE( _axis_y_y ) == 0 )
    {
        return GP_FAILURE;
    }
#endif

    gp_textured_rects_t * rects = GP_NEW( _canvas, gp_textured_rects_t );

    if( rects == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    rects->rect_count = _rect_count;
    rects->bounds_buffer = _bounds_buffer;
    rects->bounds_offset = _bounds_offset;
    rects->bounds_stride = _bounds_stride;
    rects->uv_buffer = _uv_buffer;
    rects->uv_offset = _uv_offset;
    rects->uv_stride = _uv_stride;
    rects->colors_buffer = _colors_buffer;
    rects->colors_offset = _colors_offset;
    rects->colors_stride = _colors_stride;
    rects->translation_x = _translation_x;
    rects->translation_y = _translation_y;
    rects->axis_x_x = _axis_x_x;
    rects->axis_x_y = _axis_x_y;
    rects->axis_y_x = _axis_y_x;
    rects->axis_y_y = _axis_y_y;
    rects->state = GP_COPY_STATE( _canvas );

    if( rects->state == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    gp_result_t result = __push_render_order( _canvas, GP_RENDER_ORDER_TEXTURED_RECTS, rects, rects->state );

    return result;
}
//////////////////////////////////////////////////////////////////////////
static gp_state_t * __copy_path_state( gp_canvas_t * _canvas, const gp_path_t * _path, const gp_path_style_t * _style,
    float _length_offset, float _total_length, gp_bool_t _start_marker, gp_bool_t _end_marker )
{
    gp_state_t * state = GP_NEW( _canvas, gp_state_t );
    if( state == GP_NULLPTR )
    {
        return GP_NULLPTR;
    }

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

    gp_state_apply_stroke_scale( state );

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
        if( (canvas_state->flags & GP_STATE_FLAG_FILL) == 0 && contour->closed == GP_FALSE &&
            (canvas_state->trim_from > canvas_state->trim_to || canvas_state->stroke_alignment != GP_STROKE_ALIGNMENT_CENTER) )
        {
            return GP_FAILURE;
        }
#endif
        ++contour_count;
    }

#if defined(GP_DEBUG)
    if( (canvas_state->flags & GP_STATE_FLAG_FILL) == 0 && canvas_state->trim_from > canvas_state->trim_to &&
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
    if( (canvas_state->flags & GP_STATE_FLAG_FILL) != 0 && contour_count > 1 )
    {
        compound = GP_NEW( _canvas, gp_compound_fill_t );
        if( compound == GP_NULLPTR )
        {
            return GP_FAILURE;
        }

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
        if( line == GP_NULLPTR )
        {
            return GP_FAILURE;
        }

        line->next = GP_NULLPTR;
        line->prev = GP_NULLPTR;
        line->points = GP_NULLPTR;
        line->edges = GP_NULLPTR;
        line->state = GP_NULLPTR;
        line->closed = contour->closed;
        line->compound_fill = compound;
        line->realloc = gp_canvas_scratch_realloc_callback;
        line->free = gp_canvas_scratch_free_callback;
        line->ud = _canvas;

        for( const gp_path_anchor_t * anchor = contour->anchors; anchor != GP_NULLPTR; anchor = anchor->next )
        {
            gp_line_point_t * point = GP_NEW( _canvas, gp_line_point_t );
            if( point == GP_NULLPTR )
            {
                return GP_FAILURE;
            }

            point->next = GP_NULLPTR;
            point->prev = GP_NULLPTR;
            point->p = anchor->point;
            point->state = __copy_path_state( _canvas, _path, &anchor->style,
                length_offset, total_length, start_marker, end_marker );
            if( point->state == GP_NULLPTR )
            {
                return GP_FAILURE;
            }

            GP_LIST_PUSHBACK( gp_line_point_t, line->points, point );

            if( line->state == GP_NULLPTR )
            {
                line->state = point->state;
            }
        }

        for( const gp_path_edge_t * source_edge = contour->edges; source_edge != GP_NULLPTR; source_edge = source_edge->next )
        {
            gp_line_edge_t * edge = GP_NEW( _canvas, gp_line_edge_t );
            if( edge == GP_NULLPTR )
            {
                return GP_FAILURE;
            }

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
            GP_CALL( __push_render_order, (_canvas, GP_RENDER_ORDER_LINE, line, line->state) );
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
        GP_CALL( __push_render_order, (_canvas, GP_RENDER_ORDER_COMPOUND_FILL, compound, compound->first_line->state) );
    }

    _canvas->state_invalidate = GP_TRUE;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_rect( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height )
{
    const gp_state_t * state = GP_GET_STATE( _canvas );
    if( (state->flags & GP_STATE_FLAG_FILL) == 0 && state->dash_count != 0 )
    {
        float points[4][2] = {
            {_x, _y},
            {_x + _width, _y},
            {_x + _width, _y + _height},
            {_x, _y + _height}
        };

        gp_result_t result = gp_polyline( _canvas, points, 4, sizeof( points[0] ), GP_NULLPTR, GP_TRUE );
        return result;
    }

    gp_rect_t * r = GP_NEW( _canvas, gp_rect_t );
    if( r == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    r->next = GP_NULLPTR;
    r->prev = GP_NULLPTR;

    r->point.x = _x;
    r->point.y = _y;
    r->width = _width;
    r->height = _height;

    r->state = GP_COPY_STATE( _canvas );
    if( r->state == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    GP_LIST_PUSHBACK( gp_rect_t, _canvas->rects, r );
    GP_CALL( __push_render_order, (_canvas, GP_RENDER_ORDER_RECT, r, r->state) );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_rounded_rect( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _radius )
{
    const gp_state_t * state = GP_GET_STATE( _canvas );
    if( (state->flags & GP_STATE_FLAG_FILL) == 0 && state->dash_count != 0 )
    {
        gp_result_t result = gp_rounded_rect_varying( _canvas, _x, _y, _width, _height, _radius, _radius, _radius, _radius );
        return result;
    }

    gp_rounded_rect_t * rr = GP_NEW( _canvas, gp_rounded_rect_t );
    if( rr == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    rr->next = GP_NULLPTR;
    rr->prev = GP_NULLPTR;

    rr->point.x = _x;
    rr->point.y = _y;
    rr->width = _width;
    rr->height = _height;
    rr->radius = _radius;

    rr->state = GP_COPY_STATE( _canvas );
    if( rr->state == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    GP_LIST_PUSHBACK( gp_rounded_rect_t, _canvas->rounded_rects, rr );
    GP_CALL( __push_render_order, (_canvas, GP_RENDER_ORDER_ROUNDED_RECT, rr, rr->state) );

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
    if( (state->flags & GP_STATE_FLAG_FILL) == 0 )
    {
        gp_result_t result = gp_ellipse_arc( _canvas, _x, _y, _width, _height, 0.f, gp_constant_two_pi );
        return result;
    }

    gp_ellipse_t * e = GP_NEW( _canvas, gp_ellipse_t );
    if( e == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    e->next = GP_NULLPTR;
    e->prev = GP_NULLPTR;

    e->point.x = _x;
    e->point.y = _y;
    e->radius_width = _width;
    e->radius_height = _height;

    e->state = GP_COPY_STATE( _canvas );
    if( e->state == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    GP_LIST_PUSHBACK( gp_ellipse_t, _canvas->ellipses, e );
    GP_CALL( __push_render_order, (_canvas, GP_RENDER_ORDER_ELLIPSE, e, e->state) );

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
    if( (state->flags & GP_STATE_FLAG_FILL) != 0 )
    {
        gp_ellipse_arc_t * a = GP_NEW( _canvas, gp_ellipse_arc_t );
        if( a == GP_NULLPTR )
        {
            return GP_FAILURE;
        }

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
        if( a->state == GP_NULLPTR )
        {
            return GP_FAILURE;
        }

        GP_LIST_PUSHBACK( gp_ellipse_arc_t, _canvas->ellipse_arcs, a );
        GP_CALL( __push_render_order, (_canvas, GP_RENDER_ORDER_ELLIPSE_ARC, a, a->state) );

        return GP_SUCCESSFUL;
    }

    GP_UNUSED( _move_to );
    GP_UNUSED( _rounded_caps );
    gp_path_t * path;
    GP_CALL( gp_path_create_scratch_internal, (_canvas, &path) );
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
    if( (state->flags & GP_STATE_FLAG_FILL) == 0 )
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
    case GP_RENDER_ORDER_INTERNAL_GEOMETRY:
    {
        const gp_internal_geometry_t * geometry = (const gp_internal_geometry_t *)_order->primitive;
        _mesh->vertex_count = geometry->vertex_count;
        _mesh->index_count = geometry->index_count;
    } break;
    case GP_RENDER_ORDER_GEOMETRY:
    {
        const gp_geometry_t * geometry = (const gp_geometry_t *)_order->primitive;
        _mesh->vertex_count = geometry->vertex_count;
        _mesh->index_count = geometry->index_count;
    } break;
    case GP_RENDER_ORDER_TEXTURED_RECTS:
    {
        const gp_textured_rects_t * rects = (const gp_textured_rects_t *)_order->primitive;
        _mesh->vertex_count = rects->rect_count * 4U;
        _mesh->index_count = rects->rect_count * 6U;
    } break;
    default:
        return GP_FAILURE;
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __render_order_indices32( const gp_canvas_t * _canvas, const gp_render_order_t * _order, const gp_mesh_t * _size, gp_uint32_t ** _indices )
{
    gp_canvas_t * canvas = (gp_canvas_t *)_canvas;
    gp_uint32_t * indices = GP_REALLOC( canvas, GP_NULLPTR, gp_uint32_t, _size->index_count );
    if( indices == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

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
        GP_FREE( canvas, indices );
        return GP_FAILURE;
    }

    *_indices = indices;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_calculate_mesh_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh )
{
    gp_canvas_t * canvas = (gp_canvas_t *)_canvas;
    gp_canvas_scratch_reset( canvas, 0 );

    _mesh->vertex_count = 0;
    _mesh->index_count = 0;
    _mesh->batch_count = 0;

    gp_uint32_t batch_vertex_count = 0;
    gp_bool_t has_batch = GP_FALSE;
    for( gp_uint32_t order_index = 0; order_index != _canvas->render_order_count; ++order_index )
    {
        const gp_render_order_t * order = _canvas->render_orders + order_index;
        gp_mesh_t primitive_mesh;
        GP_CALL( __calculate_render_order_size, (order, &primitive_mesh) );
        if( primitive_mesh.vertex_count == 0 || primitive_mesh.index_count == 0 )
        {
            continue;
        }

        if( primitive_mesh.vertex_count <= UINT16_MAX )
        {
            if( has_batch == GP_FALSE || batch_vertex_count + primitive_mesh.vertex_count > UINT16_MAX )
            {
                ++_mesh->batch_count;
                batch_vertex_count = primitive_mesh.vertex_count;
                has_batch = GP_TRUE;
            }
            else
            {
                batch_vertex_count += primitive_mesh.vertex_count;
            }

            _mesh->vertex_count += primitive_mesh.vertex_count;
            _mesh->index_count += primitive_mesh.index_count;
            continue;
        }

        if( has_batch == GP_FALSE )
        {
            ++_mesh->batch_count;
            batch_vertex_count = 0;
            has_batch = GP_TRUE;
        }

        gp_uint32_t * indices;
        GP_CALL( __render_order_indices32, (_canvas, order, &primitive_mesh, &indices) );
        gp_uint32_t * vertex_map = GP_REALLOC( canvas, GP_NULLPTR, gp_uint32_t, primitive_mesh.vertex_count );
        if( vertex_map == GP_NULLPTR )
        {
            return GP_FAILURE;
        }

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

        GP_FREE( canvas, vertex_map );
        GP_FREE( canvas, indices );
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

    gp_canvas_scratch_reset( canvas, 0 );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_preflight( const gp_canvas_t * _canvas, gp_uint32_t * _vertex_count, gp_uint32_t * _index_count, gp_uint32_t * _batch_count, gp_size_t * _recording_size, gp_size_t * _scratch_size )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    if( _vertex_count == GP_NULLPTR && _index_count == GP_NULLPTR && _batch_count == GP_NULLPTR && _recording_size == GP_NULLPTR && _scratch_size == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    gp_canvas_t * canvas = (gp_canvas_t *)_canvas;
    gp_size_t previous_scratch_peak = canvas->scratch_arena.peak;
    canvas->scratch_arena.peak = canvas->scratch_arena.offset;

    gp_mesh_t mesh;
    gp_result_t result = gp_calculate_mesh_size( _canvas, &mesh );
    gp_size_t required_scratch_size = canvas->scratch_arena.peak;

    if( previous_scratch_peak > canvas->scratch_arena.peak )
    {
        canvas->scratch_arena.peak = previous_scratch_peak;
    }

    if( result == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    if( _vertex_count != GP_NULLPTR )
    {
        *_vertex_count = mesh.vertex_count;
    }

    if( _index_count != GP_NULLPTR )
    {
        *_index_count = mesh.index_count;
    }

    if( _batch_count != GP_NULLPTR )
    {
        *_batch_count = mesh.batch_count;
    }

    if( _recording_size != GP_NULLPTR )
    {
        *_recording_size = canvas->recording_arena.offset;
    }

    if( _scratch_size != GP_NULLPTR )
    {
        *_scratch_size = required_scratch_size;
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_render_batch_t * __mesh_get_batch( const gp_mesh_t * _mesh, gp_uint32_t _index )
{
    if( _mesh->batches_buffer == GP_NULLPTR )
    {
        return GP_NULLPTR;
    }

    gp_render_batch_t * batch = (gp_render_batch_t *)((gp_uint8_t *)_mesh->batches_buffer + _mesh->batches_offset + _mesh->batches_stride * _index);
    return batch;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __finalize_batch( const gp_mesh_t * _mesh, gp_uint32_t _batch_index, gp_render_batch_t * _batch )
{
    if( _mesh->positions_buffer != GP_NULLPTR && _batch->vertex_count != 0 )
    {
        gp_vec2f_t position;
        GP_CALL( gp_mesh_get_position, (_mesh, _batch->vertex_offset, &position) );
        _batch->bounds.minimum_x = position.x;
        _batch->bounds.minimum_y = position.y;
        _batch->bounds.maximum_x = position.x;
        _batch->bounds.maximum_y = position.y;
        for( gp_uint32_t vertex = 1; vertex != _batch->vertex_count; ++vertex )
        {
            GP_CALL( gp_mesh_get_position, (_mesh, _batch->vertex_offset + vertex, &position) );
            _batch->bounds.minimum_x = position.x < _batch->bounds.minimum_x ? position.x : _batch->bounds.minimum_x;
            _batch->bounds.minimum_y = position.y < _batch->bounds.minimum_y ? position.y : _batch->bounds.minimum_y;
            _batch->bounds.maximum_x = position.x > _batch->bounds.maximum_x ? position.x : _batch->bounds.maximum_x;
            _batch->bounds.maximum_y = position.y > _batch->bounds.maximum_y ? position.y : _batch->bounds.maximum_y;
        }
    }

    gp_render_batch_t * output = __mesh_get_batch( _mesh, _batch_index );
    if( output != GP_NULLPTR )
    {
        *output = *_batch;
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __render_order_raw( const gp_render_order_t * _order, const gp_mesh_t * _mesh, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator )
{
    switch( _order->type )
    {
    case GP_RENDER_ORDER_LINE:
    {
        const gp_line_t * line = (const gp_line_t *)_order->primitive;
        gp_result_t result = gp_render_line( line, line->state, _mesh, _vertex_iterator, _index_iterator );
        return result;
    }
    case GP_RENDER_ORDER_RECT:
    {
        const gp_rect_t * rect = (const gp_rect_t *)_order->primitive;
        gp_result_t result = gp_render_rect( rect, rect->state, _mesh, _vertex_iterator, _index_iterator );
        return result;
    }
    case GP_RENDER_ORDER_ROUNDED_RECT:
    {
        const gp_rounded_rect_t * rounded_rect = (const gp_rounded_rect_t *)_order->primitive;
        gp_result_t result = gp_render_rounded_rect( rounded_rect, rounded_rect->state, _mesh, _vertex_iterator, _index_iterator );
        return result;
    }
    case GP_RENDER_ORDER_ELLIPSE:
    {
        const gp_ellipse_t * ellipse = (const gp_ellipse_t *)_order->primitive;
        gp_result_t result = gp_render_ellipse( ellipse, ellipse->state, _mesh, _vertex_iterator, _index_iterator );
        return result;
    }
    case GP_RENDER_ORDER_ELLIPSE_ARC:
    {
        const gp_ellipse_arc_t * ellipse_arc = (const gp_ellipse_arc_t *)_order->primitive;
        gp_result_t result = gp_render_ellipse_arc( ellipse_arc, ellipse_arc->state, _mesh, _vertex_iterator, _index_iterator );
        return result;
    }
    case GP_RENDER_ORDER_COMPOUND_FILL:
    {
        const gp_compound_fill_t * compound = (const gp_compound_fill_t *)_order->primitive;
        gp_result_t result = gp_render_compound_fill( compound, _mesh, _vertex_iterator, _index_iterator );
        return result;
    }
    case GP_RENDER_ORDER_INTERNAL_GEOMETRY:
    {
        const gp_internal_geometry_t * geometry = (const gp_internal_geometry_t *)_order->primitive;
        gp_mesh_writer_t writer;
        writer.mesh = _mesh;
        writer.vertex_offset = *_vertex_iterator;
        writer.vertex_count = geometry->vertex_count;
        writer.index_offset = *_index_iterator;
        writer.index_count = geometry->index_count;

        GP_CALL( geometry->callback, (&writer, geometry->payload) );
        *_vertex_iterator += geometry->vertex_count;
        *_index_iterator += geometry->index_count;

        return GP_SUCCESSFUL;
    }
    case GP_RENDER_ORDER_GEOMETRY:
    {
        const gp_geometry_t * geometry = (const gp_geometry_t *)_order->primitive;
        gp_mesh_writer_t writer;
        writer.mesh = _mesh;
        writer.vertex_offset = *_vertex_iterator;
        writer.vertex_count = geometry->vertex_count;
        writer.index_offset = *_index_iterator;
        writer.index_count = geometry->index_count;
        GP_CALL( gp_geometry_write, (&writer, geometry) );
        *_vertex_iterator += geometry->vertex_count;
        *_index_iterator += geometry->index_count;

        return GP_SUCCESSFUL;
    }
    case GP_RENDER_ORDER_TEXTURED_RECTS:
    {
        const gp_textured_rects_t * rects = (const gp_textured_rects_t *)_order->primitive;
        gp_mesh_writer_t writer;
        writer.mesh = _mesh;
        writer.vertex_offset = *_vertex_iterator;
        writer.vertex_count = rects->rect_count * 4U;
        writer.index_offset = *_index_iterator;
        writer.index_count = rects->rect_count * 6U;
        GP_CALL( gp_textured_rects_write, (&writer, rects) );
        *_vertex_iterator += writer.vertex_count;
        *_index_iterator += writer.index_count;

        return GP_SUCCESSFUL;
    }
    default:
        return GP_FAILURE;
    }
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __render_order( const gp_render_order_t * _order, const gp_mesh_t * _mesh, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator )
{
    gp_uint32_t first_vertex = *_vertex_iterator;
    gp_result_t result = __render_order_raw( _order, _mesh, _vertex_iterator, _index_iterator );

    if( result == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    if( _mesh->positions_buffer == GP_NULLPTR || _order->state == GP_NULLPTR )
    {
        return GP_SUCCESSFUL;
    }

    const gp_state_t * state = _order->state;

    if( (state->flags & GP_STATE_FLAG_TRANSFORM) == 0 )
    {
        return GP_SUCCESSFUL;
    }

    for( gp_uint32_t vertex = first_vertex; vertex != *_vertex_iterator; ++vertex )
    {
        gp_vec2f_t position;
        GP_CALL( gp_mesh_get_position, (_mesh, vertex, &position) );

        float x = state->translation_x + state->axis_x_x * position.x + state->axis_y_x * position.y;
        float y = state->translation_y + state->axis_x_y * position.x + state->axis_y_y * position.y;
        GP_CALL( gp_mesh_push_position, (_mesh, vertex, x, y) );
    }

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


    if( _mesh->batches_buffer != GP_NULLPTR && (_mesh->batches_offset == ~0U || _mesh->batches_stride == ~0U) )
    {
        return GP_FAILURE;
    }
#endif

    gp_canvas_t * canvas = (gp_canvas_t *)_canvas;
    const gp_size_t scratch_mark = gp_canvas_scratch_mark( canvas );

    gp_uint32_t batch_index = 0;
    gp_uint32_t global_vertex_iterator = 0;
    gp_uint32_t global_index_iterator = 0;
    gp_render_batch_t batch;
    gp_bool_t has_batch = GP_FALSE;

    for( gp_uint32_t order_index = 0; order_index != _canvas->render_order_count; ++order_index )
    {
        const gp_render_order_t * order = _canvas->render_orders + order_index;
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
                GP_REALLOC( canvas, GP_NULLPTR, gp_vec2f_t, primitive_size.vertex_count );
            gp_argb_t * colors = _mesh->colors_buffer == GP_NULLPTR ? GP_NULLPTR :
                GP_REALLOC( canvas, GP_NULLPTR, gp_argb_t, primitive_size.vertex_count );
            gp_vec2f_t * uv0 = _mesh->uv_buffer == GP_NULLPTR ? GP_NULLPTR :
                GP_REALLOC( canvas, GP_NULLPTR, gp_vec2f_t, primitive_size.vertex_count );
            gp_uint32_t * indices = GP_REALLOC( canvas, GP_NULLPTR, gp_uint32_t, primitive_size.index_count );
            if( (_mesh->positions_buffer != GP_NULLPTR && positions == GP_NULLPTR) ||
                (_mesh->colors_buffer != GP_NULLPTR && colors == GP_NULLPTR) ||
                (_mesh->uv_buffer != GP_NULLPTR && uv0 == GP_NULLPTR) || indices == GP_NULLPTR )
            {
                gp_canvas_scratch_reset( canvas, scratch_mark );
                return GP_FAILURE;
            }

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
                if( positions != GP_NULLPTR ) GP_FREE( canvas, positions );
                if( colors != GP_NULLPTR ) GP_FREE( canvas, colors );
                if( uv0 != GP_NULLPTR ) GP_FREE( canvas, uv0 );
                GP_FREE( canvas, indices );
                return GP_FAILURE;
            }

            gp_uint32_t * vertex_map = GP_REALLOC( canvas, GP_NULLPTR, gp_uint32_t, primitive_size.vertex_count );
            if( vertex_map == GP_NULLPTR )
            {
                gp_canvas_scratch_reset( canvas, scratch_mark );
                return GP_FAILURE;
            }

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
                    GP_CALL( __finalize_batch, (_mesh, batch_index++, &batch) );
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

            GP_FREE( canvas, vertex_map );
            if( positions != GP_NULLPTR ) GP_FREE( canvas, positions );
            if( colors != GP_NULLPTR ) GP_FREE( canvas, colors );
            if( uv0 != GP_NULLPTR ) GP_FREE( canvas, uv0 );
            GP_FREE( canvas, indices );
            continue;
        }

        if( has_batch == GP_FALSE || batch.vertex_count + primitive_size.vertex_count > UINT16_MAX )
        {
            if( has_batch == GP_TRUE )
            {
                GP_CALL( __finalize_batch, (_mesh, batch_index++, &batch) );
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
        GP_CALL( __finalize_batch, (_mesh, batch_index++, &batch) );
    }

#if defined(GP_DEBUG)
    if( batch_index != _mesh->batch_count )
    {
        return GP_FAILURE;
    }
#endif

    gp_canvas_scratch_reset( canvas, scratch_mark );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_flush( gp_canvas_t * _canvas, const gp_mesh_t * _mesh )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    if( _mesh == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    if( _canvas->active_compound_fill != GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    if( _canvas->active_rects != GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    if( _canvas->active_quads != GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    gp_result_t result = gp_render( _canvas, _mesh );

    if( result == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    __canvas_reset_recording( _canvas );

    return GP_SUCCESSFUL;
}
