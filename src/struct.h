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
    float width;
    uint32_t argb;
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
    struct gp_line_edge_t * prev;
} gp_line_edge_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_line_t
{
    gp_point_t * points;
    gp_line_edge_t * edges;

    float penumbra;

    struct gp_line_t * next;
    struct gp_line_t * prev;
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
    struct gp_rect_t * prev;
} gp_rect_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_rounded_rect_t
{
    gp_vec2f_t point;
    float width;
    float height;
    float radius;

    uint8_t quality;
    float quality_inv;

    float line_width;
    float line_penumbra;
    gp_color_t line_color;

    gp_color_t fill_color;
    gp_bool_t fill;

    struct gp_rounded_rect_t * next;
    struct gp_rounded_rect_t * prev;
} gp_rounded_rect_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_ellipse_t
{
    gp_vec2f_t point;
    float width;
    float height;

    uint8_t quality;
    float quality_inv;

    float line_width;
    float line_penumbra;
    gp_color_t line_color;

    gp_color_t fill_color;
    gp_bool_t fill;

    struct gp_ellipse_t * next;
    struct gp_ellipse_t * prev;
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
    gp_rect_t * rects;
    gp_rounded_rect_t * rounded_rects;
    gp_ellipse_t * ellipses;

    gp_malloc_t malloc;
    gp_free_t free;

    void * ud;
} gp_canvas_t;

#endif