#ifndef GP_GRAPHICS_H_
#define GP_GRAPHICS_H_

#include "graphics/config.h"

/*************************************************************************
*
*************************************************************************/

typedef struct gp_canvas_t gp_canvas_t;

typedef enum gp_line_cap_e
{
    GP_LINE_CAP_BUTT,
    GP_LINE_CAP_SQUARE,
    GP_LINE_CAP_ROUND
} gp_line_cap_e;

typedef enum gp_line_join_e
{
    GP_LINE_JOIN_MITER,
    GP_LINE_JOIN_BEVEL,
    GP_LINE_JOIN_ROUND
} gp_line_join_e;

typedef enum gp_fill_rule_e
{
    GP_FILL_RULE_NON_ZERO,
    GP_FILL_RULE_EVEN_ODD
} gp_fill_rule_e;

typedef gp_line_cap_e gp_line_cap_t;
typedef gp_line_join_e gp_line_join_t;
typedef gp_fill_rule_e gp_fill_rule_t;

typedef struct gp_bounds_t
{
    float minimum_x;
    float minimum_y;
    float maximum_x;
    float maximum_y;
} gp_bounds_t;

#define GP_PATH_ATTRIBUTE_NONE ((gp_size_t)-1)

typedef struct gp_path_layout_t
{
    gp_size_t width_offset;
    gp_size_t color_offset;
} gp_path_layout_t;

typedef void * (*gp_malloc_t)(gp_size_t _size, void * _ud);
typedef void * (*gp_realloc_t)(void * _ptr, gp_size_t _size, void * _ud);
typedef void (*gp_free_t)(void * _ptr, void * _ud);

/*************************************************************************
*
*************************************************************************/

gp_result_t gp_canvas_create( gp_canvas_t ** _canvas, gp_malloc_t _malloc, gp_realloc_t _realloc, gp_free_t _free, void * _ud );
gp_result_t gp_canvas_destroy( gp_canvas_t * _canvas );
gp_result_t gp_canvas_clear( gp_canvas_t * _canvas );

/*************************************************************************
*
*************************************************************************/

float gp_get_default_thickness( void );
float gp_get_default_penumbra( void );
float gp_get_default_outline_width( void );
float gp_get_default_miter_limit( void );
gp_uint8_t gp_get_default_curve_quality( void );
gp_uint8_t gp_get_default_ellipse_quality( void );
gp_uint8_t gp_get_default_rect_quality( void );

/*************************************************************************
*
*************************************************************************/

gp_result_t gp_set_thickness( gp_canvas_t * _canvas, float _thickness );
gp_result_t gp_get_thickness( const gp_canvas_t * _canvas, float * _thickness );

gp_result_t gp_set_penumbra( gp_canvas_t * _canvas, float _penumbra );
gp_result_t gp_get_penumbra( const gp_canvas_t * _canvas, float * _penumbra );

gp_result_t gp_set_inner_radius( gp_canvas_t * _canvas, float _inner_radius );
gp_result_t gp_get_inner_radius( const gp_canvas_t * _canvas, float * _inner_radius );

gp_result_t gp_set_outline_width( gp_canvas_t * _canvas, float _width );
gp_result_t gp_get_outline_width( const gp_canvas_t * _canvas, float * _width );

gp_result_t gp_set_outline_color( gp_canvas_t * _canvas, float _r, float _g, float _b, float _a );
gp_result_t gp_get_outline_color( const gp_canvas_t * _canvas, gp_color_t * _color );

gp_result_t gp_set_line_cap( gp_canvas_t * _canvas, gp_line_cap_t _cap );
gp_result_t gp_get_line_cap( const gp_canvas_t * _canvas, gp_line_cap_t * _cap );

gp_result_t gp_set_line_join( gp_canvas_t * _canvas, gp_line_join_t _join );
gp_result_t gp_get_line_join( const gp_canvas_t * _canvas, gp_line_join_t * _join );

gp_result_t gp_set_miter_limit( gp_canvas_t * _canvas, float _limit );
gp_result_t gp_get_miter_limit( const gp_canvas_t * _canvas, float * _limit );

gp_result_t gp_set_fill_rule( gp_canvas_t * _canvas, gp_fill_rule_t _rule );
gp_result_t gp_get_fill_rule( const gp_canvas_t * _canvas, gp_fill_rule_t * _rule );

gp_result_t gp_set_line_dash( gp_canvas_t * _canvas, const float * _pattern, gp_uint8_t _count, float _offset );
gp_result_t gp_clear_line_dash( gp_canvas_t * _canvas );

gp_result_t gp_set_color( gp_canvas_t * _canvas, float _r, float _g, float _b, float _a );
gp_result_t gp_get_color( const gp_canvas_t * _canvas, gp_color_t * _color );

gp_result_t gp_set_uv_offset( gp_canvas_t * _canvas, float _ou, float _ov, float _su, float _sv );
gp_result_t gp_get_uv_offset( const gp_canvas_t * _canvas, float * _ou, float * _ov, float * _su, float * _sv );

gp_result_t gp_set_curve_quality( gp_canvas_t * _canvas, gp_uint8_t _quality );
gp_result_t gp_get_curve_quality( const gp_canvas_t * _canvas, gp_uint8_t * _quality );

gp_result_t gp_set_ellipse_quality( gp_canvas_t * _canvas, gp_uint8_t _quality );
gp_result_t gp_get_ellipse_quality( const gp_canvas_t * _canvas, gp_uint8_t * _quality );

gp_result_t gp_set_rect_quality( gp_canvas_t * _canvas, gp_uint8_t _quality );
gp_result_t gp_get_rect_quality( const gp_canvas_t * _canvas, gp_uint8_t * _quality );

gp_result_t gp_begin_fill( gp_canvas_t * _canvas );
gp_result_t gp_end_fill( gp_canvas_t * _canvas );

gp_result_t gp_push_state( gp_canvas_t * _canvas );
gp_result_t gp_pop_state( gp_canvas_t * _canvas );

/*************************************************************************
*
*************************************************************************/

gp_result_t gp_point_move_to( gp_canvas_t * _canvas, float _x, float _y );
gp_result_t gp_point_line_to( gp_canvas_t * _canvas, float _x, float _y );
gp_result_t gp_point_quadratic_curve_to( gp_canvas_t * _canvas, float _p0x, float _p0y, float _x, float _y );
gp_result_t gp_point_bezier_curve_to( gp_canvas_t * _canvas, float _p0x, float _p0y, float _p1x, float _p1y, float _x, float _y );
gp_result_t gp_point_close( gp_canvas_t * _canvas );
gp_result_t gp_point_tangent_arc_to( gp_canvas_t * _canvas, float _x1, float _y1, float _x2, float _y2, float _radius );

gp_result_t gp_rect( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height );
gp_result_t gp_rounded_rect( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _radius );
gp_result_t gp_rounded_rect_varying( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _radius_top_left, float _radius_top_right, float _radius_bottom_right, float _radius_bottom_left );
gp_result_t gp_polyline( gp_canvas_t * _canvas, const void * _points, gp_size_t _point_count, gp_size_t _point_stride, const gp_path_layout_t * _layout, gp_bool_t _closed );
gp_result_t gp_rounded_polygon( gp_canvas_t * _canvas, const void * _points, gp_size_t _point_count, gp_size_t _point_stride, float _radius );
gp_result_t gp_rounded_polyline( gp_canvas_t * _canvas, const void * _points, gp_size_t _point_count, gp_size_t _point_stride, const gp_path_layout_t * _layout, float _radius, gp_bool_t _closed );
gp_result_t gp_polygon_offset( gp_canvas_t * _canvas, const void * _points, gp_size_t _point_count, gp_size_t _point_stride, float _offset, gp_line_join_t _join );
gp_result_t gp_circle( gp_canvas_t * _canvas, float _x, float _y, float _radius );
gp_result_t gp_ellipse( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height );
gp_result_t gp_arc( gp_canvas_t * _canvas, float _x, float _y, float _radius, float _angle_from, float _angle_to );
gp_result_t gp_arc_to( gp_canvas_t * _canvas, float _x, float _y, float _radius, float _angle_from, float _angle_to );
gp_result_t gp_ellipse_arc( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _angle_from, float _angle_to );
gp_result_t gp_ellipse_arc_to( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _angle_from, float _angle_to );
gp_result_t gp_ring( gp_canvas_t * _canvas, float _x, float _y, float _inner_radius, float _outer_radius, float _angle_from, float _angle_to );
gp_result_t gp_rounded_ring( gp_canvas_t * _canvas, float _x, float _y, float _inner_radius, float _outer_radius, float _angle_from, float _angle_to );
gp_result_t gp_capsule( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height );
gp_result_t gp_regular_polygon( gp_canvas_t * _canvas, float _x, float _y, float _radius, gp_uint32_t _vertex_count, float _rotation );
gp_result_t gp_star( gp_canvas_t * _canvas, float _x, float _y, float _inner_radius, float _outer_radius, gp_uint32_t _ray_count, float _rotation );
gp_result_t gp_arrow( gp_canvas_t * _canvas, float _from_x, float _from_y, float _to_x, float _to_y, float _head_length, float _head_width );
gp_result_t gp_arrowed_polyline( gp_canvas_t * _canvas, const void * _points, gp_size_t _point_count, gp_size_t _point_stride, const gp_path_layout_t * _layout, float _head_length, float _head_width );

gp_result_t gp_begin_compound_fill( gp_canvas_t * _canvas, gp_fill_rule_t _rule );
gp_result_t gp_end_compound_fill( gp_canvas_t * _canvas );

/*************************************************************************
*
*************************************************************************/

typedef struct gp_mesh_t
{
    gp_uint32_t vertex_count;
    gp_uint32_t index_count;

    gp_color_t color;

    void * positions_buffer;
    gp_size_t positions_offset;
    gp_size_t positions_stride;

    void * colors_buffer;
    gp_size_t colors_offset;
    gp_size_t colors_stride;

    void * uv_buffer;
    gp_size_t uv_offset;
    gp_size_t uv_stride;

    void * indices_buffer;
    gp_size_t indices_offset;
    gp_size_t indices_stride;
} gp_mesh_t;

gp_result_t gp_calculate_mesh_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh );
gp_result_t gp_render( const gp_canvas_t * _canvas, const gp_mesh_t * _mesh );
gp_result_t gp_calculate_bounds( const gp_canvas_t * _canvas, gp_bounds_t * _bounds );

#endif
