#include "graphics/graphics.h"

#include <stdlib.h>
#include <stdio.h>

//////////////////////////////////////////////////////////////////////////
static void * gp_malloc( gp_size_t _size, void * _ud )
{
    void * p = malloc( _size + sizeof( gp_size_t ) );

    *(gp_size_t *)p = _size;

    *(gp_size_t *)_ud += _size;

    return (gp_size_t *)p + 1;
}
//////////////////////////////////////////////////////////////////////////
static void gp_free( void * _ptr, void * _ud )
{
    gp_size_t * p = (gp_size_t *)_ptr - 1;

    *(gp_size_t *)_ud -= *p;

    free( p );
}
//////////////////////////////////////////////////////////////////////////
typedef struct gl_vertex_t
{
    float x;
    float y;
    gp_uint32_t c;
    float u;
    float v;
} gl_vertex_t;
//////////////////////////////////////////////////////////////////////////
typedef gp_uint16_t gl_index_t;
//////////////////////////////////////////////////////////////////////////
#define GP_CALL(M, Args) if( M Args == GP_FAILURE){ return GP_FAILURE;}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __draw_figure( gp_canvas_t * _canvas, float _dx, float _dy )
{
    GP_CALL( gp_set_color, (_canvas, 1.f, 0.5f, 0.25f, 1.f) );
    GP_CALL( gp_move_to, (_canvas, _dx + 0.f, _dy + 0.f) );
    GP_CALL( gp_set_color, (_canvas, 0.5f, 1.0f, 0.25f, 1.f) );
    GP_CALL( gp_quadratic_curve_to, (_canvas, _dx + 50.f, _dy + 100.f, _dx + 100.f, _dy + 100.f) );
    GP_CALL( gp_set_color, (_canvas, 0.5f, 0.5f, 1.f, 1.f) );
    GP_CALL( gp_bezier_curve_to, (_canvas, _dx + 125.f, _dy + 175.f, _dx + 175.f, _dy + 150.f, _dx + 200.f, _dy) );
    GP_CALL( gp_set_color, (_canvas, 0.25f, 1.f, 1.f, 1.f) );
    GP_CALL( gp_line_to, (_canvas, _dx + 250.f, _dy + 100.f) );

    GP_CALL( gp_set_color, (_canvas, 0.2f, 0.6f, 0.9f, 1.f) );
    GP_CALL( gp_draw_rect, (_canvas, _dx + 275.f, _dy + 0.f, 100.f, 50.f) );

    GP_CALL( gp_set_color, (_canvas, 0.7f, 0.3f, 0.4f, 1.f) );
    GP_CALL( gp_draw_rounded_rect, (_canvas, _dx + 275.f, _dy + 75.f, 100.f, 50.f, 10.f) );

    GP_CALL( gp_set_color, (_canvas, 0.1f, 0.8f, 0.2f, 1.f) );
    GP_CALL( gp_draw_circle, (_canvas, _dx + 100.f, _dy + 225.f, 50.f) );

    GP_CALL( gp_set_color, (_canvas, 0.9f, 0.1f, 0.7f, 1.f) );
    GP_CALL( gp_draw_ellipse, (_canvas, _dx + 250.f, _dy + 225.f, 50.f, 25.f) );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __test_figure( gp_canvas_t * _canvas )
{
    GP_CALL( gp_set_line_thickness, (_canvas, 20.f) );
    GP_CALL( gp_set_penumbra, (_canvas, 5.f) );

    GP_CALL( gp_set_uv_offset, (_canvas, 0.f, 0.f, 1.f, 1.f) );

    GP_CALL( gp_begin_fill, (_canvas) );

    GP_CALL( __draw_figure, (_canvas, 100.f, 100.f) );

    GP_CALL( gp_end_fill, (_canvas) );

    GP_CALL( __draw_figure, (_canvas, 100.f, 400.f) );

    GP_CALL( gp_set_penumbra, (_canvas, 2.f) );

    GP_CALL( gp_begin_fill, (_canvas) );

    GP_CALL( __draw_figure, (_canvas, 600.f, 100.f) );

    GP_CALL( gp_end_fill, (_canvas) );

    GP_CALL( __draw_figure, (_canvas, 600.f, 400.f) );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
int main( int argc, char ** argv )
{
    GP_UNUSED( argc );
    GP_UNUSED( argv );

    gp_size_t msz = 0;

    gp_canvas_t * canvas;
    if( gp_canvas_create( &canvas, &gp_malloc, &gp_free, &msz ) == GP_FAILURE )
    {
        return EXIT_FAILURE;
    }

    if( __test_figure( canvas ) == GP_FAILURE )
    {
        return EXIT_FAILURE;
    }

    gp_mesh_t mesh;
    if( gp_calculate_mesh_size( canvas, &mesh ) == GP_FAILURE )
    {
        return EXIT_FAILURE;
    }

    uint32_t max_vertex_count = 8196;
    uint32_t max_index_count = 32768;

    if( mesh.vertex_count >= max_vertex_count ||
        mesh.index_count >= max_index_count )
    {
        return EXIT_FAILURE;
    }

    void * vertices = malloc( max_vertex_count * sizeof( gl_vertex_t ) );
    void * indices = malloc( max_index_count * sizeof( gl_index_t ) );

    mesh.color.r = 1.f;
    mesh.color.g = 1.f;
    mesh.color.b = 1.f;
    mesh.color.a = 1.f;

    mesh.positions_buffer = vertices;
    mesh.positions_offset = offsetof( gl_vertex_t, x );
    mesh.positions_stride = sizeof( gl_vertex_t );

    mesh.colors_buffer = vertices;
    mesh.colors_offset = offsetof( gl_vertex_t, c );
    mesh.colors_stride = sizeof( gl_vertex_t );

    mesh.uv_buffer = vertices;
    mesh.uv_offset = offsetof( gl_vertex_t, u );
    mesh.uv_stride = sizeof( gl_vertex_t );

    mesh.indices_buffer = indices;
    mesh.indices_offset = 0;
    mesh.indices_stride = sizeof( gp_uint16_t );

    if( gp_render( canvas, &mesh ) == GP_FAILURE )
    {
        return EXIT_FAILURE;
    }

    free( vertices );
    free( indices );

    if( gp_canvas_destroy( canvas ) == GP_FAILURE )
    {
        return EXIT_FAILURE;
    }

    if( msz != 0 )
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}