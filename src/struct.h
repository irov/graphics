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

    float line_width;
    float penumbra;

    gp_color_t color;

    float uv_ou;
    float uv_ov;
    float uv_su;
    float uv_sv;

    gp_uint8_t curve_quality;
    gp_uint8_t ellipse_quality;

    float curve_quality_inv;
    float ellipse_quality_inv;
} gp_state_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_point_t
{
    gp_vec2f_t p;

    struct gp_point_t * next;
    struct gp_point_t * prev;
} gp_point_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_line_points_t
{
    gp_vec2f_t p;
    gp_uint32_t argb;
} gp_line_points_t;
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
    gp_point_t * points;
    gp_line_edge_t * edges;

    gp_state_t state;

    struct gp_line_t * next;
    struct gp_line_t * prev;
} gp_line_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_rect_t
{
    gp_vec2f_t point;
    float width;
    float height;

    gp_state_t state;

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

    gp_state_t state;

    struct gp_rounded_rect_t * next;
    struct gp_rounded_rect_t * prev;
} gp_rounded_rect_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_ellipse_t
{
    gp_vec2f_t point;
    float width;
    float height;

    gp_state_t state;

    struct gp_ellipse_t * next;
    struct gp_ellipse_t * prev;
} gp_ellipse_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_canvas_t
{
    gp_state_t state;

    gp_line_t * lines;
    gp_rect_t * rects;
    gp_rounded_rect_t * rounded_rects;
    gp_ellipse_t * ellipses;

    gp_malloc_t malloc;
    gp_free_t free;

    void * ud;
} gp_canvas_t;

#endif