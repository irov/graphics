#ifndef GP_STRUCT_H_
#define GP_STRUCT_H_

#include "graphics/config.h"

typedef struct gp_vec2f_t
{
    float x;
    float y;
} gp_vec2f_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_point_t
{
    float x;
    float y;

    struct gp_point_t * next;
} gp_point_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_line_points_t
{
    const gp_line_points_t * p;
    float width;
    gp_color_t color;

    struct gp_line_points_t * next;
} gp_line_points_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_line_edge_t
{
    uint32_t controls;
    gp_vec2f_t p[3];

    uint8_t quality;
    float dt;

    float line_width;
    gp_color_t line_color;

    struct gp_line_edge_t * next;
} gp_line_edge_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_line_t
{
    gp_point_t * points;
    gp_line_edge_t * edges;

    float penumbra;

    struct gp_line_t * next;
} gp_line_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_rect_t
{
    gp_vec2f_t point;
    float width;
    float height;

    float line_width;
    float line_penumbra;
    gp_color_t line_color;

    gp_color_t fill_color;
    gp_bool_t fill;

    struct gp_rect_t * next;
} gp_rect_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_rounded_rect_t
{
    gp_vec2f_t point;
    float width;
    float height;
    float radius;
    uint8_t quality;

    float line_width;
    float line_penumbra;
    gp_color_t line_color;

    gp_color_t fill_color;
    gp_bool_t fill;

    struct gp_rounded_rect_t * next;
} gp_rounded_rect_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_ellipse_t
{
    gp_vec2f_t point;
    float width;
    float height;

    uint8_t quality;

    float line_width;
    float line_penumbra;
    gp_color_t line_color;

    gp_color_t fill_color;
    gp_bool_t fill;

    struct gp_ellipse_t * next;
} gp_ellipse_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_canvas_t
{
    float line_width;
    float line_penumbra;
    gp_color_t line_color;
    gp_color_t fill_color;
    gp_bool_t fill;

    uint8_t curve_quality;
    uint8_t ellipse_quality;

    gp_line_t * lines;
    gp_line_t * line_last;

    gp_rect_t * rects;
    gp_rounded_rect_t * rounded_rects;
    gp_ellipse_t * ellipses;

    gp_malloc_t malloc;
    gp_free_t free;
    gp_realloc_t realloc;
} gp_canvas_t;

#endif