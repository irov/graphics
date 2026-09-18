#ifndef GP_PATH_H_
#define GP_PATH_H_

#include "graphics/graphics.h"

#include "struct.h"

typedef struct gp_path_anchor_t
{
    gp_vec2f_t point;
    gp_path_style_t style;

    struct gp_path_anchor_t * next;
    struct gp_path_anchor_t * prev;
} gp_path_anchor_t;

typedef struct gp_path_edge_t
{
    gp_uint32_t controls;
    gp_vec2f_t point[3];

    struct gp_path_edge_t * next;
    struct gp_path_edge_t * prev;
} gp_path_edge_t;

typedef struct gp_path_contour_t
{
    gp_path_anchor_t * anchors;
    gp_path_edge_t * edges;
    gp_bool_t closed;

    struct gp_path_contour_t * next;
    struct gp_path_contour_t * prev;
} gp_path_contour_t;

struct gp_path_t
{
    gp_path_contour_t * contours;
    gp_path_contour_t * current_contour;
    gp_path_style_t current_style;

    gp_uint8_t curve_quality;
    gp_uint8_t ellipse_quality;

    gp_malloc_t malloc;
    gp_realloc_t realloc;
    gp_free_t free;
    void * ud;
};

gp_result_t gp_path_create_scratch_internal( gp_canvas_t * _canvas, gp_path_t ** _path );
float gp_path_contour_get_length_internal( const gp_path_t * _path, const gp_path_contour_t * _contour );

#endif
