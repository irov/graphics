#ifndef GP_STRUCT_H_
#define GP_STRUCT_H_

#include "graphics/config.h"

//////////////////////////////////////////////////////////////////////////
typedef struct gp_vec2f_t
{
    float x;
    float y;
} gp_vec2f_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_linef_t
{
    float a;
    float b;
    float c;
} gp_linef_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_state_t
{
    gp_bool_t fill;

    gp_line_cap_t line_cap;
    gp_line_join_t line_join;
    gp_fill_rule_t fill_rule;
    gp_stroke_alignment_t stroke_alignment;
    gp_uv_mode_t uv_mode;

    float thickness;
    float outline_width;
    float penumbra;
    float inner_radius;
    float miter_limit;
    float dash_offset;
    float trim_from;
    float trim_to;
    float path_length_offset;
    float path_total_length;

    float dash_pattern[GP_DASH_PATTERN_MAX];
    gp_uint8_t dash_count;

    gp_color_t color;
    gp_color_t outline_color;

    gp_path_marker_t start_marker;
    gp_path_marker_t end_marker;

    float uv_ou;
    float uv_ov;
    float uv_su;
    float uv_sv;

    gp_uint8_t curve_quality;
    gp_uint8_t ellipse_quality;
    gp_uint8_t rect_quality;

    float curve_quality_inv;
    float ellipse_quality_inv;
    float rect_quality_inv;

    struct gp_state_t * next;
    struct gp_state_t * prev;
} gp_state_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_line_point_t
{
    gp_vec2f_t p;

    const gp_state_t * state;

    struct gp_line_point_t * next;
    struct gp_line_point_t * prev;
} gp_line_point_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_line_edge_t
{
    gp_uint32_t controls;
    gp_vec2f_t p[3];

    struct gp_line_edge_t * next;
    struct gp_line_edge_t * prev;
} gp_line_edge_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_line_t
{
    gp_line_point_t * points;
    gp_line_edge_t * edges;

    const gp_state_t * state;
    gp_bool_t closed;
    struct gp_compound_fill_t * compound_fill;
    gp_realloc_t realloc;
    gp_free_t free;
    void * ud;

    struct gp_line_t * next;
    struct gp_line_t * prev;
} gp_line_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_compound_fill_t
{
    gp_line_t * first_line;
    gp_line_t * last_line;
    gp_fill_rule_t fill_rule;

    struct gp_compound_fill_t * next;
    struct gp_compound_fill_t * prev;
} gp_compound_fill_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_rect_t
{
    gp_vec2f_t point;
    float width;
    float height;

    const gp_state_t * state;

    struct gp_rect_t * next;
    struct gp_rect_t * prev;
} gp_rect_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_rounded_rect_t
{
    gp_vec2f_t point;
    float width;
    float height;
    float radius;

    const gp_state_t * state;

    struct gp_rounded_rect_t * next;
    struct gp_rounded_rect_t * prev;
} gp_rounded_rect_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_ellipse_t
{
    gp_vec2f_t point;
    float radius_width;
    float radius_height;

    const gp_state_t * state;

    struct gp_ellipse_t * next;
    struct gp_ellipse_t * prev;
} gp_ellipse_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_ellipse_arc_t
{
    gp_vec2f_t point;
    float radius_width;
    float radius_height;
    float angle_from;
    float angle_to;
    gp_bool_t rounded_caps;

    const gp_state_t * state;

    struct gp_ellipse_arc_t * next;
    struct gp_ellipse_arc_t * prev;
} gp_ellipse_arc_t;
//////////////////////////////////////////////////////////////////////////
typedef enum gp_render_order_type_e
{
    GP_RENDER_ORDER_LINE,
    GP_RENDER_ORDER_RECT,
    GP_RENDER_ORDER_ROUNDED_RECT,
    GP_RENDER_ORDER_ELLIPSE,
    GP_RENDER_ORDER_ELLIPSE_ARC,
    GP_RENDER_ORDER_COMPOUND_FILL
} gp_render_order_type_e;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_render_order_t
{
    gp_render_order_type_e type;
    void * primitive;

    struct gp_render_order_t * next;
    struct gp_render_order_t * prev;
} gp_render_order_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_canvas_t
{
    gp_state_t state_cook[GP_STATE_STACK_MAX];
    gp_uint32_t state_stack;
    gp_bool_t state_invalidate;

    gp_state_t * states;

    gp_line_t * lines;
    gp_rect_t * rects;
    gp_rounded_rect_t * rounded_rects;
    gp_ellipse_t * ellipses;
    gp_ellipse_arc_t * ellipse_arcs;
    gp_compound_fill_t * compound_fills;
    gp_compound_fill_t * active_compound_fill;
    gp_bool_t compound_previous_fill;
    gp_render_order_t * render_order;

    gp_malloc_t malloc;
    gp_realloc_t realloc;
    gp_free_t free;

    void * ud;
} gp_canvas_t;

#endif
