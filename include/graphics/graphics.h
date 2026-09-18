#ifndef GP_GRAPHICS_H_
#define GP_GRAPHICS_H_

#include "graphics/config.h"

/*************************************************************************
*
*************************************************************************/

typedef struct gp_graphics_t gp_graphics_t;
typedef struct gp_canvas_t gp_canvas_t;
typedef struct gp_path_t gp_path_t;

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

typedef enum gp_path_sample_unit_e
{
    GP_PATH_SAMPLE_NORMALIZED,
    GP_PATH_SAMPLE_DISTANCE
} gp_path_sample_unit_e;

typedef enum gp_path_marker_type_e
{
    GP_PATH_MARKER_NONE,
    GP_PATH_MARKER_FILLED_ARROW,
    GP_PATH_MARKER_OPEN_ARROW,
    GP_PATH_MARKER_TRIANGLE,
    GP_PATH_MARKER_CIRCLE,
    GP_PATH_MARKER_DIAMOND
} gp_path_marker_type_e;

typedef enum gp_stroke_alignment_e
{
    GP_STROKE_ALIGNMENT_CENTER,
    GP_STROKE_ALIGNMENT_INNER,
    GP_STROKE_ALIGNMENT_OUTER
} gp_stroke_alignment_e;

typedef enum gp_uv_mode_e
{
    GP_UV_MODE_BOUNDS,
    GP_UV_MODE_PATH
} gp_uv_mode_e;

typedef enum gp_fill_spread_e
{
    GP_FILL_SPREAD_CLAMP,
    GP_FILL_SPREAD_REPEAT,
    GP_FILL_SPREAD_MIRROR
} gp_fill_spread_e;

typedef enum gp_stroke_scale_e
{
    GP_STROKE_SCALE_WORLD,
    GP_STROKE_SCALE_SCREEN
} gp_stroke_scale_e;

typedef enum gp_nine_slice_mode_e
{
    GP_NINE_SLICE_STRETCH,
    GP_NINE_SLICE_TILE
} gp_nine_slice_mode_e;

typedef gp_line_cap_e gp_line_cap_t;
typedef gp_line_join_e gp_line_join_t;
typedef gp_fill_rule_e gp_fill_rule_t;
typedef gp_path_sample_unit_e gp_path_sample_unit_t;
typedef gp_path_marker_type_e gp_path_marker_type_t;
typedef gp_stroke_alignment_e gp_stroke_alignment_t;
typedef gp_uv_mode_e gp_uv_mode_t;
typedef gp_fill_spread_e gp_fill_spread_t;
typedef gp_stroke_scale_e gp_stroke_scale_t;
typedef gp_nine_slice_mode_e gp_nine_slice_mode_t;

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
    gp_size_t outline_color_offset;
} gp_path_layout_t;

#define GP_PATH_STYLE_WIDTH (1U << 0)
#define GP_PATH_STYLE_COLOR (1U << 1)
#define GP_PATH_STYLE_OUTLINE_COLOR (1U << 2)

typedef struct gp_path_style_t
{
    gp_uint32_t attributes;
    float width;
    gp_color_t color;
    gp_color_t outline_color;
} gp_path_style_t;

typedef struct gp_path_sample_t
{
    float x;
    float y;
    float tangent_x;
    float tangent_y;
    float distance;
    float ratio;
    gp_uint32_t contour_index;
} gp_path_sample_t;

typedef struct gp_path_marker_t
{
    gp_path_marker_type_t type;
    float length;
    float width;
    float inset;
} gp_path_marker_t;

typedef struct gp_render_batch_t
{
    gp_uint32_t vertex_offset;
    gp_uint32_t vertex_count;
    gp_uint32_t index_offset;
    gp_uint32_t index_count;
    gp_bounds_t bounds;
} gp_render_batch_t;

typedef struct gp_canvas_memory_stats_t
{
    gp_size_t render_order_count;
    gp_size_t render_order_capacity;
    gp_size_t recording_used;
    gp_size_t recording_capacity;
    gp_size_t recording_peak;
    gp_size_t scratch_used;
    gp_size_t scratch_capacity;
    gp_size_t scratch_peak;
} gp_canvas_memory_stats_t;

typedef void * (*gp_malloc_t)(gp_size_t _size, void * _ud);
typedef void * (*gp_realloc_t)(void * _ptr, gp_size_t _size, void * _ud);
typedef void (*gp_free_t)(void * _ptr, void * _ud);

#define GP_CAPACITY_DEFAULT ((gp_size_t)~0U)

/*************************************************************************
*
*************************************************************************/

gp_result_t gp_graphics_create( gp_graphics_t ** _graphics, gp_malloc_t _malloc, gp_realloc_t _realloc, gp_free_t _free, void * _ud, gp_size_t _recording_memory_size, gp_size_t _render_order_capacity, gp_size_t _scratch_memory_size );
gp_result_t gp_graphics_destroy( gp_graphics_t * _graphics );

/*************************************************************************
*
*************************************************************************/

gp_result_t gp_canvas_create( gp_graphics_t * _graphics, gp_canvas_t ** _canvas );
gp_result_t gp_canvas_destroy( gp_canvas_t * _canvas );
gp_result_t gp_canvas_clear( gp_canvas_t * _canvas );
gp_result_t gp_canvas_get_memory_stats( const gp_canvas_t * _canvas, gp_canvas_memory_stats_t * _stats );

gp_uint32_t gp_get_api_version( void );

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
gp_result_t gp_calculate_curve_segment_count( float _length, float _maximum_error, gp_uint8_t * _segments );
gp_result_t gp_calculate_circle_segment_count( float _radius, float _maximum_error, gp_uint8_t * _segments );

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

gp_result_t gp_set_path_trim( gp_canvas_t * _canvas, float _from, float _to );
gp_result_t gp_set_path_markers( gp_canvas_t * _canvas, const gp_path_marker_t * _start, const gp_path_marker_t * _end );
gp_result_t gp_set_stroke_alignment( gp_canvas_t * _canvas, gp_stroke_alignment_t _alignment );
gp_result_t gp_get_stroke_alignment( const gp_canvas_t * _canvas, gp_stroke_alignment_t * _alignment );

gp_result_t gp_set_uv_mode( gp_canvas_t * _canvas, gp_uv_mode_t _mode );
gp_result_t gp_get_uv_mode( const gp_canvas_t * _canvas, gp_uv_mode_t * _mode );

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

gp_result_t gp_set_transform( gp_canvas_t * _canvas, float _translation_x, float _translation_y, float _axis_x_x, float _axis_x_y, float _axis_y_x, float _axis_y_y );
gp_result_t gp_get_transform( const gp_canvas_t * _canvas, float * _translation_x, float * _translation_y, float * _axis_x_x, float * _axis_x_y, float * _axis_y_x, float * _axis_y_y );
gp_result_t gp_reset_transform( gp_canvas_t * _canvas );
gp_result_t gp_set_stroke_scale( gp_canvas_t * _canvas, gp_stroke_scale_t _scale );
gp_result_t gp_get_stroke_scale( const gp_canvas_t * _canvas, gp_stroke_scale_t * _scale );

gp_result_t gp_set_fill_color( gp_canvas_t * _canvas, float _r, float _g, float _b, float _a );
gp_result_t gp_set_fill_linear( gp_canvas_t * _canvas, float _from_x, float _from_y, float _to_x, float _to_y, gp_fill_spread_t _spread );
gp_result_t gp_set_fill_radial( gp_canvas_t * _canvas, float _center_x, float _center_y, float _radius, gp_fill_spread_t _spread );
gp_result_t gp_set_fill_conic( gp_canvas_t * _canvas, float _center_x, float _center_y, float _start_angle, gp_fill_spread_t _spread );
gp_result_t gp_clear_fill_stops( gp_canvas_t * _canvas );
gp_result_t gp_add_fill_stop( gp_canvas_t * _canvas, float _offset, float _r, float _g, float _b, float _a );
gp_result_t gp_set_border( gp_canvas_t * _canvas, float _width, float _r, float _g, float _b, float _a );
gp_result_t gp_set_feather( gp_canvas_t * _canvas, float _feather );
gp_result_t gp_set_shadow( gp_canvas_t * _canvas, float _offset_x, float _offset_y, float _spread, float _blur, float _r, float _g, float _b, float _a );
gp_result_t gp_set_uv( gp_canvas_t * _canvas, float _u, float _v );
gp_result_t gp_get_uv( const gp_canvas_t * _canvas, float * _u, float * _v );

gp_result_t gp_box( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _radius_top_left, float _radius_top_right, float _radius_bottom_right, float _radius_bottom_left );
gp_result_t gp_begin_rects( gp_canvas_t * _canvas, gp_size_t _rect_count );
gp_result_t gp_add_rect( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height );
gp_result_t gp_end_rects( gp_canvas_t * _canvas );
gp_result_t gp_begin_quads( gp_canvas_t * _canvas, gp_size_t _quad_count );
gp_result_t gp_add_quad_vertex( gp_canvas_t * _canvas, float _x, float _y );
gp_result_t gp_end_quads( gp_canvas_t * _canvas );
gp_result_t gp_multicolor_rect( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _top_left_r, float _top_left_g, float _top_left_b, float _top_left_a, float _top_right_r, float _top_right_g, float _top_right_b, float _top_right_a, float _bottom_right_r, float _bottom_right_g, float _bottom_right_b, float _bottom_right_a, float _bottom_left_r, float _bottom_left_g, float _bottom_left_b, float _bottom_left_a );

/* Source buffers are borrowed and must remain valid until gp_render or gp_flush returns. */
gp_result_t gp_geometry( gp_canvas_t * _canvas, gp_uint32_t _vertex_count, const void * _positions_buffer, gp_size_t _positions_offset, gp_size_t _positions_stride, const void * _colors_buffer, gp_size_t _colors_offset, gp_size_t _colors_stride, const void * _uv_buffer, gp_size_t _uv_offset, gp_size_t _uv_stride, gp_uint32_t _index_count, const void * _indices_buffer, gp_size_t _indices_offset, gp_size_t _indices_stride, float _translation_x, float _translation_y, float _axis_x_x, float _axis_x_y, float _axis_y_x, float _axis_y_y );
gp_result_t gp_textured_rects( gp_canvas_t * _canvas, gp_uint32_t _rect_count, const void * _bounds_buffer, gp_size_t _bounds_offset, gp_size_t _bounds_stride, const void * _uv_buffer, gp_size_t _uv_offset, gp_size_t _uv_stride, const void * _colors_buffer, gp_size_t _colors_offset, gp_size_t _colors_stride, float _translation_x, float _translation_y, float _axis_x_x, float _axis_x_y, float _axis_y_x, float _axis_y_y );
gp_result_t gp_nine_slice( gp_canvas_t * _canvas, float _destination_x, float _destination_y, float _destination_width, float _destination_height, float _source_width, float _source_height, float _uv_x, float _uv_y, float _uv_width, float _uv_height, float _inset_left, float _inset_top, float _inset_right, float _inset_bottom, gp_nine_slice_mode_t _edge_mode, gp_nine_slice_mode_t _center_mode );
gp_result_t gp_grid( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _origin_x, float _origin_y, float _axis_x_x, float _axis_x_y, float _axis_y_x, float _axis_y_y, float _minor_step, gp_uint32_t _major_interval, float _minor_thickness, float _major_thickness, float _minor_r, float _minor_g, float _minor_b, float _minor_a, float _major_r, float _major_g, float _major_b, float _major_a );
gp_result_t gp_grid_bounded( gp_canvas_t * _canvas, float _viewport_x, float _viewport_y, float _viewport_width, float _viewport_height, float _range_x, float _range_y, float _range_width, float _range_height, float _origin_x, float _origin_y, float _axis_x_x, float _axis_x_y, float _axis_y_x, float _axis_y_y, float _minor_step, gp_uint32_t _major_interval, float _minor_thickness, float _major_thickness, float _minor_r, float _minor_g, float _minor_b, float _minor_a, float _major_r, float _major_g, float _major_b, float _major_a );

/*************************************************************************
*
*************************************************************************/

gp_result_t gp_path_create( gp_canvas_t * _canvas, gp_path_t ** _path );
gp_result_t gp_path_destroy( gp_path_t * _path );
gp_result_t gp_path_clear( gp_path_t * _path );
gp_result_t gp_path_set_style( gp_path_t * _path, const gp_path_style_t * _style );
gp_result_t gp_path_move_to( gp_path_t * _path, float _x, float _y );
gp_result_t gp_path_line_to( gp_path_t * _path, float _x, float _y );
gp_result_t gp_path_quadratic_curve_to( gp_path_t * _path, float _p0x, float _p0y, float _x, float _y );
gp_result_t gp_path_bezier_curve_to( gp_path_t * _path, float _p0x, float _p0y, float _p1x, float _p1y, float _x, float _y );
gp_result_t gp_path_tangent_arc_to( gp_path_t * _path, float _x1, float _y1, float _x2, float _y2, float _radius );
gp_result_t gp_path_ellipse_arc( gp_path_t * _path, float _x, float _y, float _width, float _height, float _angle_from, float _angle_to );
gp_result_t gp_path_close( gp_path_t * _path );
gp_result_t gp_path_polyline( gp_path_t * _path, const void * _points, gp_size_t _point_count, gp_size_t _point_stride, const gp_path_layout_t * _layout, gp_bool_t _closed );
gp_result_t gp_path_catmull_rom( gp_path_t * _path, const void * _points, gp_size_t _point_count, gp_size_t _point_stride, const gp_path_layout_t * _layout, gp_bool_t _closed );
gp_result_t gp_path_get_length( const gp_path_t * _path, float * _length );
gp_result_t gp_path_get_bounds( const gp_path_t * _path, gp_bounds_t * _bounds );
gp_result_t gp_path_sample( const gp_path_t * _path, float _value, gp_path_sample_unit_t _unit, gp_path_sample_t * _sample );

gp_result_t gp_draw_path( gp_canvas_t * _canvas, const gp_path_t * _path );

/*************************************************************************
*
*************************************************************************/

gp_result_t gp_rect( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height );
gp_result_t gp_quadratic_bezier( gp_canvas_t * _canvas, float _x0, float _y0, float _control_x, float _control_y, float _x1, float _y1 );
gp_result_t gp_cubic_bezier( gp_canvas_t * _canvas, float _x0, float _y0, float _control0_x, float _control0_y, float _control1_x, float _control1_y, float _x1, float _y1 );
gp_result_t gp_rounded_rect( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _radius );
gp_result_t gp_rounded_rect_varying( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _radius_top_left, float _radius_top_right, float _radius_bottom_right, float _radius_bottom_left );
gp_result_t gp_polyline( gp_canvas_t * _canvas, const void * _points, gp_size_t _point_count, gp_size_t _point_stride, const gp_path_layout_t * _layout, gp_bool_t _closed );
gp_result_t gp_rounded_polygon( gp_canvas_t * _canvas, const void * _points, gp_size_t _point_count, gp_size_t _point_stride, float _radius );
gp_result_t gp_rounded_polyline( gp_canvas_t * _canvas, const void * _points, gp_size_t _point_count, gp_size_t _point_stride, const gp_path_layout_t * _layout, float _radius, gp_bool_t _closed );
gp_result_t gp_polygon_offset( gp_canvas_t * _canvas, const void * _points, gp_size_t _point_count, gp_size_t _point_stride, float _offset, gp_line_join_t _join );
gp_result_t gp_superellipse( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _exponent, float _rotation );
gp_result_t gp_circle( gp_canvas_t * _canvas, float _x, float _y, float _radius );
gp_result_t gp_ellipse( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height );
gp_result_t gp_arc( gp_canvas_t * _canvas, float _x, float _y, float _radius, float _angle_from, float _angle_to );
gp_result_t gp_ellipse_arc( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _angle_from, float _angle_to );
gp_result_t gp_ring( gp_canvas_t * _canvas, float _x, float _y, float _inner_radius, float _outer_radius, float _angle_from, float _angle_to );
gp_result_t gp_rounded_ring( gp_canvas_t * _canvas, float _x, float _y, float _inner_radius, float _outer_radius, float _angle_from, float _angle_to );
gp_result_t gp_capsule( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height );
gp_result_t gp_regular_polygon( gp_canvas_t * _canvas, float _x, float _y, float _radius, gp_uint32_t _vertex_count, float _rotation );
gp_result_t gp_star( gp_canvas_t * _canvas, float _x, float _y, float _inner_radius, float _outer_radius, gp_uint32_t _ray_count, float _rotation );
/*************************************************************************
*
*************************************************************************/

typedef struct gp_mesh_t
{
    gp_uint32_t vertex_count;
    gp_uint32_t index_count;
    gp_uint32_t batch_count;

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

    void * batches_buffer;
    gp_size_t batches_offset;
    gp_size_t batches_stride;
} gp_mesh_t;

gp_result_t gp_calculate_mesh_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh );
gp_result_t gp_preflight( const gp_canvas_t * _canvas, gp_uint32_t * _vertex_count, gp_uint32_t * _index_count, gp_uint32_t * _batch_count, gp_size_t * _recording_size, gp_size_t * _scratch_size );
gp_result_t gp_render( const gp_canvas_t * _canvas, const gp_mesh_t * _mesh );
/* Renders recorded commands and resets only command storage. Canvas state is preserved. */
gp_result_t gp_flush( gp_canvas_t * _canvas, const gp_mesh_t * _mesh );
gp_result_t gp_calculate_bounds( const gp_canvas_t * _canvas, gp_bounds_t * _bounds );

#endif
