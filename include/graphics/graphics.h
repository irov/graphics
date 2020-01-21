#ifndef GP_GRAPHICS_H_
#define GP_GRAPHICS_H_

#include "graphics/config.h"

typedef struct gp_canvas_t gp_canvas_t;

typedef void * (*gp_malloc_t)(size_t _size, void * _ud);
typedef void (*gp_free_t)(void * _ptr, void * _ud);

gp_result_t gp_canvas_create( gp_canvas_t ** _canvas, gp_malloc_t _malloc, gp_free_t _free, void * _ud );
gp_result_t gp_canvas_destroy( gp_canvas_t * _canvas );
gp_result_t gp_canvas_clear( gp_canvas_t * _canvas );

gp_result_t gp_set_line_width( gp_canvas_t * _canvas, float _width );
gp_result_t gp_get_line_width( gp_canvas_t * _canvas, float * _width );

gp_result_t gp_set_line_penumbra( gp_canvas_t * _canvas, float _penumbra );
gp_result_t gp_get_line_penumbra( gp_canvas_t * _canvas, float * _penumbra );

gp_result_t gp_set_line_color( gp_canvas_t * _canvas, gp_color_t _color );
gp_result_t gp_get_line_color( gp_canvas_t * _canvas, gp_color_t * _color );

gp_result_t gp_set_curve_quality( gp_canvas_t * _canvas, uint8_t _quality );
gp_result_t gp_get_curve_quality( gp_canvas_t * _canvas, uint8_t * _quality );

gp_result_t gp_set_ellipse_quality( gp_canvas_t * _canvas, uint8_t _quality );
gp_result_t gp_get_ellipse_quality( gp_canvas_t * _canvas, uint8_t * _quality );

gp_result_t gp_begin_fill( gp_canvas_t * _canvas );
gp_result_t gp_end_fill( gp_canvas_t * _canvas );

gp_result_t gp_move_to( gp_canvas_t * _canvas, float _x, float _y );
gp_result_t gp_line_to( gp_canvas_t * _canvas, float _x, float _y );
gp_result_t gp_quadratic_curve_to( gp_canvas_t * _canvas, float _p0x, float _p0y, float _x, float _y );
gp_result_t gp_bezier_curve_to( gp_canvas_t * _canvas, float _p0x, float _p0y, float _p1x, float _p1y, float _x, float _y );

gp_result_t gp_draw_rect( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height );
gp_result_t gp_draw_rounded_rect( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height, float _radius );
gp_result_t gp_draw_circle( gp_canvas_t * _canvas, float _x, float _y, float _radius );
gp_result_t gp_draw_ellipse( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height );

typedef struct gp_mesh_t
{
    uint32_t vertex_count;
    uint16_t index_count;

    gp_color_t color;

    void * positions_buffer;
    size_t positions_offset;
    size_t positions_stride;

    void * colors_buffer;
    size_t colors_offset;
    size_t colors_stride;

    void * indices_buffer;
    size_t indices_offset;
    size_t indices_stride;
} gp_mesh_t;

gp_result_t gp_calculate_mesh_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh );
gp_result_t gp_render( const gp_canvas_t * _canvas, const gp_mesh_t * _mesh );

#endif