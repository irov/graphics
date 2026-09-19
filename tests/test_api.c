#include "graphics/graphics.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////
typedef struct test_vertex_t
{
    float x;
    float y;
    gp_uint32_t color;
    float u0;
    float v0;
} test_vertex_t;
//////////////////////////////////////////////////////////////////////////
typedef struct test_path_point_t
{
    float x;
    float y;
    float width;
    gp_color_t color;
} test_path_point_t;
//////////////////////////////////////////////////////////////////////////
static gp_bool_t test_near( float _a, float _b, float _epsilon )
{
    return fabsf( _a - _b ) <= _epsilon ? GP_TRUE : GP_FALSE;
}
//////////////////////////////////////////////////////////////////////////
static void * test_malloc( gp_size_t _size, void * _ud )
{
    GP_UNUSED( _ud );

    return malloc( _size );
}
//////////////////////////////////////////////////////////////////////////
static void * test_realloc( void * _ptr, gp_size_t _size, void * _ud )
{
    GP_UNUSED( _ud );

    return realloc( _ptr, _size );
}
//////////////////////////////////////////////////////////////////////////
static void test_free( void * _ptr, void * _ud )
{
    GP_UNUSED( _ud );

    free( _ptr );
}
//////////////////////////////////////////////////////////////////////////
static gp_graphics_t * test_graphics;
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_path_create_quality( gp_canvas_t * _canvas, gp_path_t ** _path, gp_uint8_t _curve_quality, gp_uint8_t _ellipse_quality )
{
    gp_push_state( _canvas );
    gp_set_curve_quality( _canvas, _curve_quality );
    gp_set_ellipse_quality( _canvas, _ellipse_quality );

    gp_result_t result = gp_path_create( _canvas, _path );

    gp_pop_state( _canvas );

    return result;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_draw_destroy_path( gp_canvas_t * _canvas, gp_path_t * _path )
{
    gp_result_t result = gp_draw_path( _canvas, _path );
    gp_path_destroy( _path );

    return result;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_render( gp_canvas_t * _canvas, gp_mesh_t * _mesh, test_vertex_t ** _vertices, gp_uint16_t ** _indices )
{
    if( gp_calculate_mesh_size( _canvas, _mesh ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    *_vertices = (test_vertex_t *)malloc( sizeof( test_vertex_t ) * _mesh->vertex_count );
    *_indices = (gp_uint16_t *)malloc( sizeof( gp_uint16_t ) * _mesh->index_count );
    gp_render_batch_t * batches = (gp_render_batch_t *)malloc( sizeof( gp_render_batch_t ) * _mesh->batch_count );

    _mesh->positions_buffer = *_vertices;
    _mesh->positions_offset = offsetof( test_vertex_t, x );
    _mesh->positions_stride = sizeof( test_vertex_t );
    _mesh->colors_buffer = *_vertices;
    _mesh->colors_offset = offsetof( test_vertex_t, color );
    _mesh->colors_stride = sizeof( test_vertex_t );
    _mesh->uv_buffer = *_vertices;
    _mesh->uv_offset = offsetof( test_vertex_t, u0 );
    _mesh->uv_stride = sizeof( test_vertex_t );
    _mesh->indices_buffer = *_indices;
    _mesh->indices_offset = 0;
    _mesh->indices_stride = sizeof( gp_uint16_t );
    _mesh->batches_buffer = batches;
    _mesh->batches_offset = 0;
    _mesh->batches_stride = sizeof( gp_render_batch_t );

    if( gp_render( _canvas, _mesh ) == GP_FAILURE )
    {
        free( batches );
        return GP_FAILURE;
    }

    for( gp_uint32_t index = 0; index != _mesh->vertex_count; ++index )
    {
        if( isfinite( (*_vertices)[index].x ) == 0 || isfinite( (*_vertices)[index].y ) == 0 )
        {
            free( batches );
            return GP_FAILURE;
        }
    }

    gp_uint32_t expected_vertex_offset = 0;
    gp_uint32_t expected_index_offset = 0;
    for( gp_uint32_t batch_index = 0; batch_index != _mesh->batch_count; ++batch_index )
    {
        const gp_render_batch_t * batch = batches + batch_index;
        if( batch->vertex_offset != expected_vertex_offset || batch->index_offset != expected_index_offset ||
            batch->vertex_count > UINT16_MAX )
        {
            free( batches );
            return GP_FAILURE;
        }

        for( gp_uint32_t index = 0; index != batch->index_count; ++index )
        {
            if( (*_indices)[batch->index_offset + index] >= batch->vertex_count )
            {
                free( batches );
                return GP_FAILURE;
            }
        }

        expected_vertex_offset += batch->vertex_count;
        expected_index_offset += batch->index_count;
    }

    free( batches );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static float test_maximum_x( const test_vertex_t * _vertices, gp_uint32_t _vertex_count )
{
    float maximum_x = _vertices[0].x;
    for( gp_uint32_t index = 1; index != _vertex_count; ++index )
    {
        if( maximum_x < _vertices[index].x )
        {
            maximum_x = _vertices[index].x;
        }
    }

    return maximum_x;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_graphics_capacity( void )
{
    gp_graphics_t * graphics;
    if( gp_graphics_create( &graphics, &test_malloc, &test_realloc, &test_free, GP_NULLPTR,
        1024U * 1024U, 64U, 512U * 1024U ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_canvas_t * canvas;
    if( gp_canvas_create( graphics, &canvas ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_canvas_memory_stats_t stats;
    if( gp_canvas_get_memory_stats( canvas, &stats ) == GP_FAILURE ||
        stats.recording_capacity != 0 || stats.render_order_capacity != 0 ||
        stats.scratch_capacity != 0 )
    {
        return GP_FAILURE;
    }

    gp_canvas_destroy( canvas );
    gp_graphics_destroy( graphics );

    if( gp_canvas_create( test_graphics, &canvas ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    if( gp_canvas_get_memory_stats( canvas, &stats ) == GP_FAILURE ||
        stats.recording_capacity != 0 || stats.render_order_capacity != 0 ||
        stats.scratch_capacity != 0 )
    {
        return GP_FAILURE;
    }

    if( gp_graphics_create( &graphics, &test_malloc, &test_realloc, &test_free, GP_NULLPTR,
        GP_CAPACITY_DEFAULT, 0U, GP_CAPACITY_DEFAULT ) != GP_FAILURE ||
        gp_graphics_create( &graphics, GP_NULLPTR, &test_realloc, &test_free, GP_NULLPTR,
        GP_CAPACITY_DEFAULT, GP_CAPACITY_DEFAULT, GP_CAPACITY_DEFAULT ) != GP_FAILURE ||
        gp_canvas_create( GP_NULLPTR, &canvas ) != GP_FAILURE )
    {
        return GP_FAILURE;
    }

    return gp_canvas_destroy( canvas );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_multi_canvas( void )
{
    gp_canvas_t * first;
    gp_canvas_t * second;
    if( gp_canvas_create( test_graphics, &first ) == GP_FAILURE ||
        gp_canvas_create( test_graphics, &second ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_set_penumbra( first, 0.f );
    gp_set_penumbra( second, 0.f );

    gp_set_thickness( first, 7.f );

    float second_thickness;
    if( gp_get_thickness( second, &second_thickness ) == GP_FAILURE ||
        second_thickness != gp_get_default_thickness() )
    {
        return GP_FAILURE;
    }

    gp_begin_fill( first );
    gp_set_color( first, 1.f, 0.f, 0.f, 1.f );
    gp_rect( first, 0.f, 0.f, 10.f, 10.f );
    gp_begin_fill( second );
    gp_set_color( second, 0.f, 1.f, 0.f, 1.f );
    gp_rect( second, 0.f, 0.f, 10.f, 10.f );
    gp_rect( second, 20.f, 0.f, 10.f, 10.f );
    gp_rect( first, 40.f, 0.f, 10.f, 10.f );
    gp_rect( second, 25.f, 0.f, 5.f, 10.f );
    gp_end_fill( first );
    gp_end_fill( second );

    gp_mesh_t first_mesh;
    test_vertex_t * first_vertices;
    gp_uint16_t * first_indices;
    gp_mesh_t second_mesh;
    test_vertex_t * second_vertices;
    gp_uint16_t * second_indices;
    if( test_render( first, &first_mesh, &first_vertices, &first_indices ) == GP_FAILURE ||
        test_render( second, &second_mesh, &second_vertices, &second_indices ) == GP_FAILURE ||
        first_mesh.vertex_count != 8 || first_mesh.index_count != 12 ||
        second_mesh.vertex_count != 12 || second_mesh.index_count != 18 ||
        first_vertices[0].color == second_vertices[0].color ||
        test_near( test_maximum_x( first_vertices, first_mesh.vertex_count ), 53.5f, 0.0001f ) == GP_FALSE ||
        test_near( test_maximum_x( second_vertices, second_mesh.vertex_count ), 31.5f, 0.0001f ) == GP_FALSE )
    {
        return GP_FAILURE;
    }

    gp_canvas_memory_stats_t first_stats;
    gp_canvas_memory_stats_t second_stats;
    if( gp_canvas_get_memory_stats( first, &first_stats ) == GP_FAILURE ||
        gp_canvas_get_memory_stats( second, &second_stats ) == GP_FAILURE ||
        first_stats.render_order_count != 2 || second_stats.render_order_count != 3 ||
        first_stats.recording_used == 0 || first_stats.recording_used >= second_stats.recording_used )
    {
        return GP_FAILURE;
    }

    gp_canvas_clear( first );

    if( gp_canvas_get_memory_stats( first, &first_stats ) == GP_FAILURE ||
        gp_canvas_get_memory_stats( second, &second_stats ) == GP_FAILURE ||
        first_stats.render_order_count != 0 || first_stats.recording_used != 0 ||
        second_stats.render_order_count != 3 || second_stats.recording_used == 0 )
    {
        return GP_FAILURE;
    }

    free( first_vertices );
    free( first_indices );
    free( second_vertices );
    free( second_indices );

    if( gp_canvas_destroy( first ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    if( test_render( second, &second_mesh, &second_vertices, &second_indices ) == GP_FAILURE ||
        second_mesh.vertex_count != 12 || second_mesh.index_count != 18 ||
        test_near( test_maximum_x( second_vertices, second_mesh.vertex_count ), 31.5f, 0.0001f ) == GP_FALSE )
    {
        return GP_FAILURE;
    }

    free( second_vertices );
    free( second_indices );

    return gp_canvas_destroy( second );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_large_index_count( void )
{
    gp_canvas_t * canvas;
    if( gp_canvas_create( test_graphics, &canvas ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    if( gp_set_penumbra( canvas, 0.f ) == GP_FAILURE || gp_begin_fill( canvas ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    for( gp_uint32_t index = 0; index != 11000; ++index )
    {
        float x = (float)(index % 100) * 2.f;
        float y = (float)(index / 100) * 2.f;
        if( gp_rect( canvas, x, y, 1.f, 1.f ) == GP_FAILURE )
        {
            return GP_FAILURE;
        }
    }

    gp_mesh_t mesh;
    test_vertex_t * vertices;
    gp_uint16_t * indices;
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    if( mesh.vertex_count != 44000 || mesh.index_count != 66000 )
    {
        return GP_FAILURE;
    }

    for( gp_uint32_t index = 0; index != mesh.index_count; ++index )
    {
        if( indices[index] >= mesh.vertex_count )
        {
            return GP_FAILURE;
        }
    }

    free( vertices );
    free( indices );

    return gp_canvas_destroy( canvas );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_large_line_index_count( void )
{
    const gp_uint32_t point_count = 40000;
    float * points = (float *)malloc( sizeof( float ) * point_count * 2 );
    for( gp_uint32_t index = 0; index != point_count; ++index )
    {
        points[index * 2 + 0] = (float)index;
        points[index * 2 + 1] = 0.f;
    }

    gp_canvas_t * canvas;
    if( gp_canvas_create( test_graphics, &canvas ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_set_penumbra( canvas, 0.f );
    gp_polyline( canvas, points, point_count, sizeof( float ) * 2, GP_NULLPTR, GP_FALSE );
    free( points );

    gp_mesh_t mesh;
    test_vertex_t * vertices;
    gp_uint16_t * indices;
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE ||
        mesh.vertex_count < point_count * 2 || mesh.index_count != (point_count - 1) * 6 || mesh.batch_count < 2 )
    {
        return GP_FAILURE;
    }

    free( vertices );
    free( indices );

    return gp_canvas_destroy( canvas );
}
//////////////////////////////////////////////////////////////////////////
#if defined(GP_DEBUG)
static gp_result_t test_unique_vertex_limit( void )
{
    gp_canvas_t * canvas;
    if( gp_canvas_create( test_graphics, &canvas ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_set_penumbra( canvas, 0.f );
    gp_begin_fill( canvas );

    for( gp_uint32_t index = 0; index != 16384; ++index )
    {
        gp_rect( canvas, (float)index, 0.f, 1.f, 1.f );
    }

    gp_mesh_t mesh;
    gp_result_t result = gp_calculate_mesh_size( canvas, &mesh );
    gp_bool_t valid = result == GP_SUCCESSFUL && mesh.vertex_count == 65536 && mesh.index_count == 98304 && mesh.batch_count == 2
        ? GP_TRUE
        : GP_FALSE;
    gp_canvas_destroy( canvas );

    return valid == GP_TRUE ? GP_SUCCESSFUL : GP_FAILURE;
}
#endif
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_state_restore( void )
{
    gp_canvas_t * canvas;
    if( gp_canvas_create( test_graphics, &canvas ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_set_penumbra( canvas, 0.f );
    gp_begin_fill( canvas );
    gp_set_color( canvas, 1.f, 0.f, 0.f, 1.f );
    gp_rect( canvas, 0.f, 0.f, 10.f, 10.f );
    gp_push_state( canvas );
    gp_set_color( canvas, 0.f, 0.f, 1.f, 1.f );
    gp_rect( canvas, 20.f, 0.f, 10.f, 10.f );
    gp_pop_state( canvas );
    gp_rect( canvas, 40.f, 0.f, 10.f, 10.f );

    gp_mesh_t mesh;
    test_vertex_t * vertices;
    gp_uint16_t * indices;
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    if( vertices[0].color != 0xffff0000U || vertices[4].color != 0xff0000ffU || vertices[8].color != 0xffff0000U )
    {
        return GP_FAILURE;
    }

    free( vertices );
    free( indices );

    return gp_canvas_destroy( canvas );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_transform_state( void )
{
    gp_canvas_t * canvas;
    if( gp_canvas_create( test_graphics, &canvas ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    if( gp_set_penumbra( canvas, 0.f ) == GP_FAILURE ||
        gp_begin_fill( canvas ) == GP_FAILURE ||
        gp_set_transform( canvas, 10.f, 20.f, 2.f, 0.f, 0.f, 3.f ) == GP_FAILURE ||
        gp_push_state( canvas ) == GP_FAILURE ||
        gp_reset_transform( canvas ) == GP_FAILURE ||
        gp_rect( canvas, 1.f, 2.f, 3.f, 4.f ) == GP_FAILURE ||
        gp_pop_state( canvas ) == GP_FAILURE ||
        gp_rect( canvas, 1.f, 2.f, 3.f, 4.f ) == GP_FAILURE ||
        gp_reset_transform( canvas ) == GP_FAILURE ||
        gp_rect( canvas, 1.f, 2.f, 3.f, 4.f ) == GP_FAILURE )
    {
        gp_canvas_destroy( canvas );
        return GP_FAILURE;
    }

    float translation_x;
    float translation_y;
    float axis_x_x;
    float axis_x_y;
    float axis_y_x;
    float axis_y_y;
    if( gp_get_transform( canvas, &translation_x, &translation_y, &axis_x_x, &axis_x_y, &axis_y_x, &axis_y_y ) == GP_FAILURE ||
        translation_x != 0.f || translation_y != 0.f ||
        axis_x_x != 1.f || axis_x_y != 0.f || axis_y_x != 0.f || axis_y_y != 1.f )
    {
        gp_canvas_destroy( canvas );
        return GP_FAILURE;
    }

    gp_mesh_t mesh;
    test_vertex_t * vertices;
    gp_uint16_t * indices;
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE )
    {
        gp_canvas_destroy( canvas );
        return GP_FAILURE;
    }

    gp_bool_t valid = mesh.vertex_count == 12 ? GP_TRUE : GP_FALSE;
    for( gp_uint32_t vertex = 0; valid == GP_TRUE && vertex != 4; ++vertex )
    {
        const test_vertex_t * identity = vertices + vertex;
        const test_vertex_t * transformed = vertices + 4U + vertex;
        const test_vertex_t * reset = vertices + 8U + vertex;

        if( transformed->x != 10.f + 2.f * identity->x ||
            transformed->y != 20.f + 3.f * identity->y ||
            reset->x != identity->x || reset->y != identity->y )
        {
            valid = GP_FALSE;
        }
    }

    free( vertices );
    free( indices );
    gp_canvas_destroy( canvas );

    return valid == GP_TRUE ? GP_SUCCESSFUL : GP_FAILURE;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_close_path( void )
{
    gp_canvas_t * canvas;
    if( gp_canvas_create( test_graphics, &canvas ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_set_penumbra( canvas, 0.f );
    gp_path_t * path;
    gp_path_create( canvas, &path );
    gp_path_move_to( path, 0.f, 0.f );
    gp_path_line_to( path, 10.f, 0.f );
    gp_path_line_to( path, 0.f, 10.f );
    gp_path_close( path );
    test_draw_destroy_path( canvas, path );

    gp_mesh_t mesh;
    test_vertex_t * vertices;
    gp_uint16_t * indices;
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    if( mesh.vertex_count != 8 || mesh.index_count != 18 )
    {
        return GP_FAILURE;
    }

    free( vertices );
    free( indices );

    return gp_canvas_destroy( canvas );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_compound_fill( void )
{
    const float outer[][2] = {{0.f, 0.f}, {100.f, 0.f}, {100.f, 100.f}, {0.f, 100.f}};
    const float hole[][2] = {{30.f, 30.f}, {70.f, 30.f}, {70.f, 70.f}, {30.f, 70.f}};

    gp_canvas_t * canvas;
    if( gp_canvas_create( test_graphics, &canvas ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_set_penumbra( canvas, 0.f );
    gp_begin_fill( canvas );
    gp_set_fill_rule( canvas, GP_FILL_RULE_EVEN_ODD );
    gp_path_t * path;
    gp_path_create( canvas, &path );
    gp_path_polyline( path, outer, 4, sizeof( outer[0] ), GP_NULLPTR, GP_TRUE );
    gp_path_polyline( path, hole, 4, sizeof( hole[0] ), GP_NULLPTR, GP_TRUE );
    test_draw_destroy_path( canvas, path );

    gp_mesh_t mesh;
    test_vertex_t * vertices;
    gp_uint16_t * indices;
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE )
    {
        fprintf( stderr, "compound render even-odd failed\n" );
        return GP_FAILURE;
    }

    if( mesh.vertex_count != 8 || mesh.index_count != 24 )
    {
        fprintf( stderr, "compound even-odd size: %u %u\n", mesh.vertex_count, mesh.index_count );
        return GP_FAILURE;
    }

    float area = 0.f;
    for( gp_uint32_t index = 0; index != mesh.index_count; index += 3 )
    {
        const test_vertex_t * p0 = vertices + indices[index + 0];
        const test_vertex_t * p1 = vertices + indices[index + 1];
        const test_vertex_t * p2 = vertices + indices[index + 2];
        area += ((p1->x - p0->x) * (p2->y - p0->y) - (p1->y - p0->y) * (p2->x - p0->x)) * 0.5f;
    }

    if( fabsf( area - 8400.f ) > 0.1f )
    {
        fprintf( stderr, "compound even-odd area: %f\n", area );
        return GP_FAILURE;
    }

    free( vertices );
    free( indices );

    gp_canvas_clear( canvas );
    gp_set_penumbra( canvas, 0.f );
    gp_begin_fill( canvas );
    gp_set_fill_rule( canvas, GP_FILL_RULE_NON_ZERO );
    gp_path_create( canvas, &path );
    gp_path_polyline( path, outer, 4, sizeof( outer[0] ), GP_NULLPTR, GP_TRUE );
    gp_path_polyline( path, hole, 4, sizeof( hole[0] ), GP_NULLPTR, GP_TRUE );
    test_draw_destroy_path( canvas, path );

    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE )
    {
        fprintf( stderr, "compound render non-zero failed\n" );
        return GP_FAILURE;
    }

    area = 0.f;
    for( gp_uint32_t index = 0; index != mesh.index_count; index += 3 )
    {
        const test_vertex_t * p0 = vertices + indices[index + 0];
        const test_vertex_t * p1 = vertices + indices[index + 1];
        const test_vertex_t * p2 = vertices + indices[index + 2];
        area += ((p1->x - p0->x) * (p2->y - p0->y) - (p1->y - p0->y) * (p2->x - p0->x)) * 0.5f;
    }

    if( fabsf( area - 10000.f ) > 0.1f )
    {
        fprintf( stderr, "compound non-zero area: %f size %u %u\n", area, mesh.vertex_count, mesh.index_count );
        return GP_FAILURE;
    }

    free( vertices );
    free( indices );

    return gp_canvas_destroy( canvas );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_paths_and_primitives( void )
{
    const float points[][2] = {{0.f, 0.f}, {40.f, 0.f}, {50.f, 30.f}, {10.f, 50.f}};
    const float dash[] = {5.f, 3.f};

    gp_canvas_t * canvas;
    if( gp_canvas_create( test_graphics, &canvas ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_set_penumbra( canvas, 0.f );
    gp_set_line_cap( canvas, GP_LINE_CAP_ROUND );
    gp_set_line_join( canvas, GP_LINE_JOIN_ROUND );
    gp_set_miter_limit( canvas, 4.f );
    gp_set_line_dash( canvas, dash, 2, 1.f );
    gp_polyline( canvas, points, 4, sizeof( points[0] ), GP_NULLPTR, GP_FALSE );
    gp_clear_line_dash( canvas );
    gp_rounded_polyline( canvas, points, 4, sizeof( points[0] ), GP_NULLPTR, 5.f, GP_TRUE );
    gp_rounded_rect_varying( canvas, 0.f, 70.f, 50.f, 30.f, 2.f, 4.f, 6.f, 8.f );
    gp_capsule( canvas, 60.f, 0.f, 40.f, 20.f );
    gp_regular_polygon( canvas, 120.f, 20.f, 15.f, 6, 0.f );
    gp_push_state( canvas );
    gp_begin_fill( canvas );
    gp_regular_polygon( canvas, 120.f, 120.f, 15.f, 1100, 0.f );
    gp_pop_state( canvas );
    gp_star( canvas, 160.f, 20.f, 8.f, 16.f, 5, 0.f );
    gp_ring( canvas, 200.f, 20.f, 0.f, 15.f, 0.f, 1.5f );
    gp_ring( canvas, 240.f, 20.f, 8.f, 16.f, 0.f, 3.f );
    gp_polygon_offset( canvas, points, 4, sizeof( points[0] ), 3.f, GP_LINE_JOIN_MITER );
    gp_polygon_offset( canvas, points, 4, sizeof( points[0] ), 3.f, GP_LINE_JOIN_BEVEL );
    gp_polygon_offset( canvas, points, 4, sizeof( points[0] ), 3.f, GP_LINE_JOIN_ROUND );
    gp_polygon_offset( canvas, points, 4, sizeof( points[0] ), -2.f, GP_LINE_JOIN_ROUND );
    gp_path_marker_t marker = {GP_PATH_MARKER_FILLED_ARROW, 8.f, 6.f, 0.f};
    const float arrow_line[][2] = {{280.f, 0.f}, {310.f, 20.f}};
    gp_set_path_markers( canvas, GP_NULLPTR, &marker );
    gp_polyline( canvas, arrow_line, 2, sizeof( arrow_line[0] ), GP_NULLPTR, GP_FALSE );
    gp_polyline( canvas, points, 4, sizeof( points[0] ), GP_NULLPTR, GP_FALSE );
    gp_set_path_markers( canvas, GP_NULLPTR, GP_NULLPTR );
    gp_path_t * tangent_path;
    gp_path_create( canvas, &tangent_path );
    gp_path_move_to( tangent_path, 330.f, 0.f );
    gp_path_tangent_arc_to( tangent_path, 350.f, 0.f, 350.f, 20.f, 5.f );
    test_draw_destroy_path( canvas, tangent_path );

    gp_bounds_t bounds;
    if( gp_calculate_bounds( canvas, &bounds ) == GP_FAILURE )
    {
        fprintf( stderr, "gp_calculate_bounds failed\n" );
        return GP_FAILURE;
    }

    if( bounds.minimum_x >= bounds.maximum_x || bounds.minimum_y >= bounds.maximum_y )
    {
        fprintf( stderr, "invalid bounds\n" );
        return GP_FAILURE;
    }

    gp_mesh_t mesh;
    test_vertex_t * vertices;
    gp_uint16_t * indices;
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE )
    {
        fprintf( stderr, "primitive render failed\n" );
        return GP_FAILURE;
    }

    free( vertices );
    free( indices );

    return gp_canvas_destroy( canvas );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_radial_indicators( void )
{
    const float pi = 3.14159265358979323846f;
    gp_canvas_t * canvas;
    if( gp_canvas_create( test_graphics, &canvas ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_set_penumbra( canvas, 0.f );
    gp_set_ellipse_quality( canvas, 64 );
    gp_set_line_cap( canvas, GP_LINE_CAP_ROUND );
    gp_arc( canvas, 0.f, 0.f, 10.f, 0.f, 0.f );

    gp_mesh_t mesh;
    if( gp_calculate_mesh_size( canvas, &mesh ) == GP_FAILURE || mesh.vertex_count != 0 || mesh.index_count != 0 )
    {
        return GP_FAILURE;
    }

    gp_set_line_cap( canvas, GP_LINE_CAP_BUTT );
    gp_arc( canvas, 0.f, 0.f, 10.f, 0.f, pi * 0.5f );
    test_vertex_t * vertices;
    gp_uint16_t * indices;
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE || mesh.vertex_count != 34 || mesh.index_count != 96 )
    {
        return GP_FAILURE;
    }
    free( vertices );
    free( indices );

    gp_canvas_clear( canvas );
    gp_set_penumbra( canvas, 0.f );
    gp_set_ellipse_quality( canvas, 64 );
    gp_set_line_cap( canvas, GP_LINE_CAP_SQUARE );
    gp_arc( canvas, 0.f, 0.f, 10.f, 0.f, pi * 0.5f );
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE || mesh.vertex_count != 34 || mesh.index_count != 96 )
    {
        return GP_FAILURE;
    }
    free( vertices );
    free( indices );

    gp_canvas_clear( canvas );
    gp_set_penumbra( canvas, 0.f );
    gp_set_ellipse_quality( canvas, 64 );
    gp_set_line_cap( canvas, GP_LINE_CAP_ROUND );

    gp_arc( canvas, 0.f, 0.f, 10.f, 0.f, pi * 0.5f );
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE || mesh.vertex_count != 102 || mesh.index_count != 288 )
    {
        return GP_FAILURE;
    }
    free( vertices );
    free( indices );

    gp_canvas_clear( canvas );
    gp_set_penumbra( canvas, 0.f );
    gp_set_ellipse_quality( canvas, 64 );
    gp_set_line_cap( canvas, GP_LINE_CAP_ROUND );
    gp_arc( canvas, 0.f, 0.f, 10.f, 0.f, pi * 2.f );
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE || mesh.vertex_count != 130 || mesh.index_count != 384 )
    {
        return GP_FAILURE;
    }
    free( vertices );
    free( indices );

    gp_canvas_clear( canvas );
    gp_set_penumbra( canvas, 0.f );
    gp_set_ellipse_quality( canvas, 64 );
    gp_set_line_cap( canvas, GP_LINE_CAP_ROUND );
    gp_arc( canvas, 0.f, 0.f, 10.f, 0.f, -pi * 2.f );
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE || mesh.vertex_count != 130 || mesh.index_count != 384 )
    {
        return GP_FAILURE;
    }
    free( vertices );
    free( indices );

    gp_canvas_clear( canvas );
    gp_ring( canvas, 0.f, 0.f, 0.f, 10.f, 0.f, pi * 0.5f );
    gp_ring( canvas, 30.f, 0.f, 5.f, 10.f, 0.f, -pi * 0.5f );
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }
    free( vertices );
    free( indices );

    return gp_canvas_destroy( canvas );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_arc_growth_stability( void )
{
    const float pi = 3.14159265358979323846f;
    const gp_uint8_t quality = 16;
    const float angle_step = pi * 2.f / (float)quality;
    const float angle_from = 0.37f;
    const float sweep_from = angle_step * 3.25f;
    const float sweep_to = angle_step * 3.75f;

    gp_canvas_t * canvas;
    gp_canvas_create( test_graphics, &canvas );
    gp_set_penumbra( canvas, 0.f );
    gp_set_ellipse_quality( canvas, quality );

    gp_ring( canvas, 10.f, 20.f, 12.f, 24.f, angle_from, angle_from + sweep_from );
    gp_mesh_t first_mesh;
    test_vertex_t * first_vertices;
    gp_uint16_t * first_indices;
    if( test_render( canvas, &first_mesh, &first_vertices, &first_indices ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_canvas_clear( canvas );
    gp_set_penumbra( canvas, 0.f );
    gp_set_ellipse_quality( canvas, quality );
    gp_ring( canvas, 10.f, 20.f, 12.f, 24.f, angle_from, angle_from + sweep_to );
    gp_mesh_t second_mesh;
    test_vertex_t * second_vertices;
    gp_uint16_t * second_indices;
    if( test_render( canvas, &second_mesh, &second_vertices, &second_indices ) == GP_FAILURE ||
        first_mesh.vertex_count != 10 || second_mesh.vertex_count != 10 )
    {
        return GP_FAILURE;
    }

    for( gp_uint32_t index = 0; index != 8; ++index )
    {
        if( first_vertices[index].x != second_vertices[index].x ||
            first_vertices[index].y != second_vertices[index].y )
        {
            return GP_FAILURE;
        }
    }
    if( first_vertices[8].x == second_vertices[8].x && first_vertices[8].y == second_vertices[8].y )
    {
        return GP_FAILURE;
    }

    free( first_vertices );
    free( first_indices );
    free( second_vertices );
    free( second_indices );

    gp_canvas_clear( canvas );
    gp_set_penumbra( canvas, 0.f );
    gp_set_ellipse_quality( canvas, quality );
    gp_set_line_cap( canvas, GP_LINE_CAP_BUTT );
    gp_set_thickness( canvas, 4.f );
    gp_arc( canvas, 10.f, 20.f, 24.f, angle_from, angle_from - sweep_from );
    if( test_render( canvas, &first_mesh, &first_vertices, &first_indices ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_canvas_clear( canvas );
    gp_set_penumbra( canvas, 0.f );
    gp_set_ellipse_quality( canvas, quality );
    gp_set_line_cap( canvas, GP_LINE_CAP_BUTT );
    gp_set_thickness( canvas, 4.f );
    gp_arc( canvas, 10.f, 20.f, 24.f, angle_from, angle_from - sweep_to );
    if( test_render( canvas, &second_mesh, &second_vertices, &second_indices ) == GP_FAILURE ||
        first_mesh.vertex_count != 10 || second_mesh.vertex_count != 10 )
    {
        return GP_FAILURE;
    }

    for( gp_uint32_t index = 0; index != 6; ++index )
    {
        if( first_vertices[index].x != second_vertices[index].x ||
            first_vertices[index].y != second_vertices[index].y )
        {
            return GP_FAILURE;
        }
    }

    free( first_vertices );
    free( first_indices );
    free( second_vertices );
    free( second_indices );

    return gp_canvas_destroy( canvas );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_rounded_ring( void )
{
    const float pi = 3.14159265358979323846f;
    gp_canvas_t * canvas;
    gp_canvas_create( test_graphics, &canvas );
    gp_set_penumbra( canvas, 0.f );
    gp_set_ellipse_quality( canvas, 8 );
    gp_rounded_ring( canvas, 0.f, 0.f, 6.f, 10.f, 0.f, pi * 0.5f );

    gp_mesh_t mesh;
    test_vertex_t * vertices;
    gp_uint16_t * indices;
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE ||
        mesh.vertex_count != 14 || mesh.index_count != 36 )
    {
        return GP_FAILURE;
    }

    if( test_near( vertices[0].x, 10.f, 0.0001f ) == GP_FALSE ||
        test_near( vertices[0].y, 0.f, 0.0001f ) == GP_FALSE ||
        test_near( vertices[1].x, 6.f, 0.0001f ) == GP_FALSE ||
        test_near( vertices[1].y, 0.f, 0.0001f ) == GP_FALSE ||
        test_near( vertices[4].x, 0.f, 0.0001f ) == GP_FALSE ||
        test_near( vertices[4].y, 10.f, 0.0001f ) == GP_FALSE ||
        test_near( vertices[5].x, 0.f, 0.0001f ) == GP_FALSE ||
        test_near( vertices[5].y, 6.f, 0.0001f ) == GP_FALSE ||
        test_near( vertices[9].x, 8.f, 0.0001f ) == GP_FALSE ||
        test_near( vertices[9].y, 0.f, 0.0001f ) == GP_FALSE ||
        test_near( vertices[13].x, 0.f, 0.0001f ) == GP_FALSE ||
        test_near( vertices[13].y, 8.f, 0.0001f ) == GP_FALSE )
    {
        return GP_FAILURE;
    }

    gp_bool_t uses_start_outer = GP_FALSE;
    gp_bool_t uses_start_inner = GP_FALSE;
    gp_bool_t uses_end_outer = GP_FALSE;
    gp_bool_t uses_end_inner = GP_FALSE;
    for( gp_uint32_t index = 12; index != 24; index += 3 )
    {
        if( indices[index] != 9 )
        {
            return GP_FAILURE;
        }

        uses_start_outer = indices[index + 1] == 0 || indices[index + 2] == 0 ? GP_TRUE : uses_start_outer;
        uses_start_inner = indices[index + 1] == 1 || indices[index + 2] == 1 ? GP_TRUE : uses_start_inner;
    }
    for( gp_uint32_t index = 24; index != 36; index += 3 )
    {
        if( indices[index] != 13 )
        {
            return GP_FAILURE;
        }

        uses_end_outer = indices[index + 1] == 4 || indices[index + 2] == 4 ? GP_TRUE : uses_end_outer;
        uses_end_inner = indices[index + 1] == 5 || indices[index + 2] == 5 ? GP_TRUE : uses_end_inner;
    }
    if( uses_start_outer == GP_FALSE || uses_start_inner == GP_FALSE ||
        uses_end_outer == GP_FALSE || uses_end_inner == GP_FALSE )
    {
        return GP_FAILURE;
    }

    gp_bounds_t bounds;
    if( gp_calculate_bounds( canvas, &bounds ) == GP_FAILURE ||
        test_near( bounds.minimum_x, -2.f, 0.0001f ) == GP_FALSE ||
        test_near( bounds.minimum_y, -2.f, 0.0001f ) == GP_FALSE ||
        test_near( bounds.maximum_x, 10.f, 0.0001f ) == GP_FALSE ||
        test_near( bounds.maximum_y, 10.f, 0.0001f ) == GP_FALSE )
    {
        return GP_FAILURE;
    }

    free( vertices );
    free( indices );

    const float angle_from = 0.37f;
    const float angle_step = pi * 2.f / 16.f;
    gp_canvas_clear( canvas );
    gp_set_penumbra( canvas, 0.f );
    gp_set_ellipse_quality( canvas, 16 );
    gp_rounded_ring( canvas, 10.f, 20.f, 12.f, 24.f, angle_from, angle_from + angle_step * 3.25f );

    gp_mesh_t first_mesh;
    test_vertex_t * first_vertices;
    gp_uint16_t * first_indices;
    if( test_render( canvas, &first_mesh, &first_vertices, &first_indices ) == GP_FAILURE ||
        first_mesh.vertex_count != 26 || first_mesh.index_count != 72 )
    {
        return GP_FAILURE;
    }

    gp_canvas_clear( canvas );
    gp_set_penumbra( canvas, 0.f );
    gp_set_ellipse_quality( canvas, 16 );
    gp_rounded_ring( canvas, 10.f, 20.f, 12.f, 24.f, angle_from, angle_from + angle_step * 3.75f );

    gp_mesh_t second_mesh;
    test_vertex_t * second_vertices;
    gp_uint16_t * second_indices;
    if( test_render( canvas, &second_mesh, &second_vertices, &second_indices ) == GP_FAILURE ||
        second_mesh.vertex_count != 26 || second_mesh.index_count != 72 )
    {
        return GP_FAILURE;
    }

    for( gp_uint32_t index = 0; index != 8; ++index )
    {
        if( first_vertices[index].x != second_vertices[index].x ||
            first_vertices[index].y != second_vertices[index].y )
        {
            return GP_FAILURE;
        }
    }
    for( gp_uint32_t index = 10; index != 17; ++index )
    {
        if( first_vertices[index].x != second_vertices[index].x ||
            first_vertices[index].y != second_vertices[index].y )
        {
            return GP_FAILURE;
        }
    }
    if( first_vertices[8].x == second_vertices[8].x && first_vertices[8].y == second_vertices[8].y )
    {
        return GP_FAILURE;
    }

    free( first_vertices );
    free( first_indices );
    free( second_vertices );
    free( second_indices );

    gp_canvas_clear( canvas );
    gp_set_penumbra( canvas, 0.f );
    gp_set_ellipse_quality( canvas, 8 );
    gp_rounded_ring( canvas, 0.f, 0.f, 6.f, 10.f, 0.f, pi * 2.f );
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE ||
        mesh.vertex_count != 18 || mesh.index_count != 48 )
    {
        return GP_FAILURE;
    }
    free( vertices );
    free( indices );

    gp_canvas_clear( canvas );
    gp_rounded_ring( canvas, 0.f, 0.f, 6.f, 10.f, 0.f, 0.f );
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE ||
        mesh.vertex_count != 0 || mesh.index_count != 0 )
    {
        return GP_FAILURE;
    }
    free( vertices );
    free( indices );

    return gp_canvas_destroy( canvas );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_dashed_paths( void )
{
    const float pi = 3.14159265358979323846f;
    const float odd_pattern[] = {5.f, 3.f, 2.f};
    const float doubled_pattern[] = {5.f, 3.f, 2.f, 5.f, 3.f, 2.f};
    gp_canvas_t * odd_canvas;
    gp_canvas_t * doubled_canvas;
    gp_canvas_create( test_graphics, &odd_canvas );
    gp_canvas_create( test_graphics, &doubled_canvas );

    gp_set_penumbra( odd_canvas, 0.f );
    gp_set_penumbra( doubled_canvas, 0.f );
    gp_set_ellipse_quality( odd_canvas, 32 );
    gp_set_ellipse_quality( doubled_canvas, 32 );
    gp_set_line_cap( odd_canvas, GP_LINE_CAP_ROUND );
    gp_set_line_cap( doubled_canvas, GP_LINE_CAP_ROUND );
    gp_set_line_dash( odd_canvas, odd_pattern, 3, 4.f );
    gp_set_line_dash( doubled_canvas, doubled_pattern, 6, 4.f );
    gp_arc( odd_canvas, 0.f, 0.f, 20.f, 0.f, pi );
    gp_arc( doubled_canvas, 0.f, 0.f, 20.f, 0.f, pi );

    gp_mesh_t odd_mesh;
    gp_mesh_t doubled_mesh;
    test_vertex_t * odd_vertices;
    test_vertex_t * doubled_vertices;
    gp_uint16_t * odd_indices;
    gp_uint16_t * doubled_indices;
    if( test_render( odd_canvas, &odd_mesh, &odd_vertices, &odd_indices ) == GP_FAILURE ||
        test_render( doubled_canvas, &doubled_mesh, &doubled_vertices, &doubled_indices ) == GP_FAILURE ||
        odd_mesh.vertex_count != doubled_mesh.vertex_count || odd_mesh.index_count != doubled_mesh.index_count )
    {
        return GP_FAILURE;
    }

    for( gp_uint32_t index = 0; index != odd_mesh.vertex_count; ++index )
    {
        if( test_near( odd_vertices[index].x, doubled_vertices[index].x, 0.0001f ) == GP_FALSE ||
            test_near( odd_vertices[index].y, doubled_vertices[index].y, 0.0001f ) == GP_FALSE ||
            odd_vertices[index].color != doubled_vertices[index].color )
        {
            return GP_FAILURE;
        }
    }

    free( odd_vertices );
    free( doubled_vertices );
    free( odd_indices );
    free( doubled_indices );
    gp_canvas_destroy( odd_canvas );
    gp_canvas_destroy( doubled_canvas );

    gp_canvas_t * closed_canvas;
    gp_canvas_create( test_graphics, &closed_canvas );
    const float seam_pattern[] = {15.f, 5.f};
    gp_set_penumbra( closed_canvas, 0.f );
    gp_set_line_cap( closed_canvas, GP_LINE_CAP_ROUND );
    gp_set_line_dash( closed_canvas, seam_pattern, 2, 2.f );
    gp_arc( closed_canvas, 0.f, 0.f, 10.f, 0.f, pi * 2.f );
    gp_path_t * path;
    gp_path_create( closed_canvas, &path );
    gp_path_move_to( path, 30.f, 0.f );
    gp_path_quadratic_curve_to( path, 40.f, 20.f, 50.f, 0.f );
    gp_path_bezier_curve_to( path, 55.f, -10.f, 65.f, -10.f, 70.f, 0.f );
    test_draw_destroy_path( closed_canvas, path );
    gp_rect( closed_canvas, 80.f, 0.f, 20.f, 10.f );
    gp_rounded_rect( closed_canvas, 110.f, 0.f, 20.f, 10.f, 3.f );
    gp_circle( closed_canvas, 150.f, 5.f, 8.f );
    gp_mesh_t mesh;
    test_vertex_t * vertices;
    gp_uint16_t * indices;
    if( test_render( closed_canvas, &mesh, &vertices, &indices ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }
    free( vertices );
    free( indices );
    gp_canvas_destroy( closed_canvas );

    gp_canvas_t * fill_canvas;
    gp_canvas_create( test_graphics, &fill_canvas );
    const float square[][2] = {{0.f, 0.f}, {10.f, 0.f}, {10.f, 10.f}, {0.f, 10.f}};
    gp_set_penumbra( fill_canvas, 0.f );
    gp_set_line_dash( fill_canvas, seam_pattern, 2, 0.f );
    gp_begin_fill( fill_canvas );
    gp_polyline( fill_canvas, square, 4, sizeof( square[0] ), GP_NULLPTR, GP_TRUE );
    if( test_render( fill_canvas, &mesh, &vertices, &indices ) == GP_FAILURE || mesh.vertex_count != 4 || mesh.index_count != 6 )
    {
        return GP_FAILURE;
    }
    free( vertices );
    free( indices );

    return gp_canvas_destroy( fill_canvas );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_rich_paths( void )
{
    test_path_point_t points[] = {
        {0.f, 0.f, 2.f, {1.f, 0.f, 0.f, 1.f}},
        {10.f, 0.f, 6.f, {0.f, 0.f, 1.f, 1.f}}
    };
    gp_path_layout_t layout;
    layout.width_offset = offsetof( test_path_point_t, width );
    layout.color_offset = offsetof( test_path_point_t, color );
    layout.outline_color_offset = GP_PATH_ATTRIBUTE_NONE;

    gp_canvas_t * canvas;
    gp_canvas_create( test_graphics, &canvas );
    gp_set_penumbra( canvas, 0.f );
    gp_set_line_join( canvas, GP_LINE_JOIN_BEVEL );
    gp_set_curve_quality( canvas, 4 );
    gp_polyline( canvas, points, 2, sizeof( points[0] ), &layout, GP_FALSE );

    float restored_width;
    gp_color_t restored_color;
    gp_get_thickness( canvas, &restored_width );
    gp_get_color( canvas, &restored_color );
    if( restored_width != gp_get_default_thickness() || restored_color.r != 1.f || restored_color.g != 1.f || restored_color.b != 1.f || restored_color.a != 1.f )
    {
        return GP_FAILURE;
    }

    gp_path_t * path;
    test_path_create_quality( canvas, &path, 4, gp_get_default_ellipse_quality() );
    gp_path_style_t style = {GP_PATH_STYLE_WIDTH | GP_PATH_STYLE_COLOR, 2.f, {1.f, 0.f, 0.f, 1.f}, {0.f, 0.f, 0.f, 0.f}};
    gp_path_set_style( path, &style );
    gp_path_move_to( path, 20.f, 0.f );
    style.width = 6.f;
    style.color.r = 0.f;
    style.color.b = 1.f;
    gp_path_set_style( path, &style );
    gp_path_quadratic_curve_to( path, 25.f, 10.f, 30.f, 0.f );
    style.width = 4.f;
    style.color.g = 1.f;
    style.color.b = 0.f;
    gp_path_set_style( path, &style );
    gp_path_bezier_curve_to( path, 32.f, -10.f, 38.f, -10.f, 40.f, 0.f );
    test_draw_destroy_path( canvas, path );

    test_path_point_t rounded[] = {
        {50.f, 0.f, 2.f, {1.f, 0.f, 0.f, 1.f}},
        {70.f, 0.f, 8.f, {0.f, 1.f, 0.f, 1.f}},
        {70.f, 20.f, 4.f, {0.f, 0.f, 1.f, 1.f}},
        {50.f, 20.f, 6.f, {1.f, 1.f, 0.f, 1.f}}
    };
    gp_set_line_join( canvas, GP_LINE_JOIN_MITER );
    gp_polyline( canvas, rounded, 4, sizeof( rounded[0] ), &layout, GP_TRUE );
    gp_rounded_polyline( canvas, rounded, 4, sizeof( rounded[0] ), &layout, 4.f, GP_TRUE );
    gp_path_marker_t marker = {GP_PATH_MARKER_FILLED_ARROW, 5.f, 4.f, 0.f};
    gp_set_path_markers( canvas, GP_NULLPTR, &marker );
    gp_polyline( canvas, rounded, 4, sizeof( rounded[0] ), &layout, GP_FALSE );
    gp_set_path_markers( canvas, GP_NULLPTR, GP_NULLPTR );

    gp_mesh_t mesh;
    test_vertex_t * vertices;
    gp_uint16_t * indices;
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    if( mesh.vertex_count < 22 ||
        test_near( vertices[0].y, -1.f, 0.0001f ) == GP_FALSE ||
        test_near( vertices[1].y, 1.f, 0.0001f ) == GP_FALSE ||
        test_near( vertices[2].y, -3.f, 0.0001f ) == GP_FALSE ||
        test_near( vertices[3].y, 3.f, 0.0001f ) == GP_FALSE ||
        vertices[0].color != 0xffff0000U || vertices[2].color != 0xff0000ffU )
    {
        return GP_FAILURE;
    }

    float quadratic_mid_width = hypotf( vertices[8].x - vertices[9].x, vertices[8].y - vertices[9].y );
    gp_uint32_t quadratic_mid_red = (vertices[8].color >> 16) & 0xffU;
    gp_uint32_t quadratic_mid_green = (vertices[8].color >> 8) & 0xffU;
    gp_uint32_t quadratic_mid_blue = vertices[8].color & 0xffU;
    if( test_near( quadratic_mid_width, 4.f, 0.001f ) == GP_FALSE ||
        quadratic_mid_red < 126U || quadratic_mid_red > 128U || quadratic_mid_green != 0U ||
        quadratic_mid_blue < 126U || quadratic_mid_blue > 128U ||
        vertices[12].color != 0xff0000ffU || vertices[20].color != 0xff00ff00U )
    {
        return GP_FAILURE;
    }

    gp_uint32_t rounded_entry_red = (vertices[32].color >> 16) & 0xffU;
    gp_uint32_t rounded_entry_green = (vertices[32].color >> 8) & 0xffU;
    gp_uint32_t rounded_entry_blue = vertices[32].color & 0xffU;
    if( rounded_entry_red != 255U || rounded_entry_green < 50U || rounded_entry_green > 52U || rounded_entry_blue != 0U )
    {
        return GP_FAILURE;
    }

    if( vertices[mesh.vertex_count - 1].color != 0xffffff00U )
    {
        return GP_FAILURE;
    }

    free( vertices );
    free( indices );

    return gp_canvas_destroy( canvas );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_soft_round_geometry( void )
{
    const float points[][2] = {{0.f, 0.f}, {10.f, 0.f}};
    gp_canvas_t * canvas;
    gp_canvas_create( test_graphics, &canvas );
    gp_set_thickness( canvas, 10.f );
    gp_set_penumbra( canvas, 2.f );
    gp_set_line_cap( canvas, GP_LINE_CAP_ROUND );
    gp_polyline( canvas, points, 2, sizeof( points[0] ), GP_NULLPTR, GP_FALSE );

    gp_bounds_t bounds;
    if( gp_calculate_bounds( canvas, &bounds ) == GP_FAILURE ||
        test_near( bounds.minimum_x, -5.f, 0.001f ) == GP_FALSE || test_near( bounds.maximum_x, 15.f, 0.001f ) == GP_FALSE ||
        test_near( bounds.minimum_y, -5.f, 0.001f ) == GP_FALSE || test_near( bounds.maximum_y, 5.f, 0.001f ) == GP_FALSE )
    {
        return GP_FAILURE;
    }

    gp_mesh_t mesh;
    test_vertex_t * vertices;
    gp_uint16_t * indices;
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE || mesh.vertex_count != 142 || mesh.index_count != 594 )
    {
        return GP_FAILURE;
    }

    for( gp_uint32_t index = 8; index != 75; ++index )
    {
        if( vertices[index].x > 0.001f )
        {
            return GP_FAILURE;
        }
    }
    for( gp_uint32_t index = 75; index != mesh.vertex_count; ++index )
    {
        if( vertices[index].x < 9.999f )
        {
            return GP_FAILURE;
        }
    }

    gp_bool_t transparent_found = GP_FALSE;
    for( gp_uint32_t index = 0; index != mesh.vertex_count; ++index )
    {
        if( (vertices[index].color >> 24) == 0 )
        {
            transparent_found = GP_TRUE;
            break;
        }
    }
    if( transparent_found == GP_FALSE )
    {
        return GP_FAILURE;
    }
    free( vertices );
    free( indices );

    gp_canvas_clear( canvas );
    test_path_point_t narrow[] = {
        {0.f, 0.f, 2.f, {1.f, 1.f, 1.f, 1.f}},
        {5.f, 5.f, 3.f, {1.f, 1.f, 1.f, 1.f}},
        {10.f, 0.f, 4.f, {1.f, 1.f, 1.f, 1.f}}
    };
    gp_path_layout_t layout = {offsetof( test_path_point_t, width ), offsetof( test_path_point_t, color ), GP_PATH_ATTRIBUTE_NONE};
    gp_set_thickness( canvas, 10.f );
    gp_set_penumbra( canvas, 4.f );
    gp_set_line_cap( canvas, GP_LINE_CAP_ROUND );
    gp_set_line_join( canvas, GP_LINE_JOIN_ROUND );
    gp_polyline( canvas, narrow, 3, sizeof( narrow[0] ), &layout, GP_FALSE );
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE ||
        mesh.vertex_count != 185 || mesh.index_count != 756 )
    {
        return GP_FAILURE;
    }

    for( gp_uint32_t index = 16; index != 51; ++index )
    {
        if( vertices[index].y < 4.999f )
        {
            return GP_FAILURE;
        }
    }
    free( vertices );
    free( indices );

    return gp_canvas_destroy( canvas );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_smooth_round_curve_geometry( void )
{
    gp_canvas_t * canvas;
    gp_canvas_create( test_graphics, &canvas );
    gp_set_thickness( canvas, 4.f );
    gp_set_penumbra( canvas, 0.f );
    gp_set_line_cap( canvas, GP_LINE_CAP_BUTT );
    gp_set_line_join( canvas, GP_LINE_JOIN_ROUND );
    gp_set_curve_quality( canvas, 4 );
    gp_path_t * path;
    test_path_create_quality( canvas, &path, 4, gp_get_default_ellipse_quality() );
    gp_path_move_to( path, 0.f, 0.f );
    gp_path_line_to( path, 10.f, 0.f );
    gp_path_quadratic_curve_to( path, 15.f, 0.f, 20.f, 5.f );
    gp_path_line_to( path, 25.f, 10.f );
    test_draw_destroy_path( canvas, path );

    gp_mesh_t mesh;
    test_vertex_t * vertices;
    gp_uint16_t * indices;
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE ||
        mesh.vertex_count != 14 || mesh.index_count != 36 )
    {
        return GP_FAILURE;
    }
    free( vertices );
    free( indices );

    return gp_canvas_destroy( canvas );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_reusable_path_queries_and_snapshot( void )
{
    gp_canvas_t * canvas;
    gp_canvas_create( test_graphics, &canvas );

    gp_path_t * path;
    if( test_path_create_quality( canvas, &path, 8, 16 ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_path_move_to( path, 0.f, 0.f );
    gp_path_line_to( path, 10.f, 0.f );
    gp_path_move_to( path, 100.f, 0.f );
    gp_path_line_to( path, 100.f, 10.f );

    float length;
    gp_bounds_t bounds;
    gp_path_sample_t sample;
    if( gp_path_get_length( path, &length ) == GP_FAILURE || test_near( length, 20.f, 0.0001f ) == GP_FALSE ||
        gp_path_get_bounds( path, &bounds ) == GP_FAILURE ||
        test_near( bounds.minimum_x, 0.f, 0.0001f ) == GP_FALSE ||
        test_near( bounds.minimum_y, 0.f, 0.0001f ) == GP_FALSE ||
        test_near( bounds.maximum_x, 100.f, 0.0001f ) == GP_FALSE ||
        test_near( bounds.maximum_y, 10.f, 0.0001f ) == GP_FALSE ||
        gp_path_sample( path, 0.75f, GP_PATH_SAMPLE_NORMALIZED, &sample ) == GP_FAILURE ||
        test_near( sample.x, 100.f, 0.0001f ) == GP_FALSE || test_near( sample.y, 5.f, 0.0001f ) == GP_FALSE ||
        test_near( sample.tangent_x, 0.f, 0.0001f ) == GP_FALSE || test_near( sample.tangent_y, 1.f, 0.0001f ) == GP_FALSE ||
        test_near( sample.distance, 15.f, 0.0001f ) == GP_FALSE || sample.contour_index != 1 )
    {
        return GP_FAILURE;
    }

    gp_set_penumbra( canvas, 0.f );
    gp_set_line_cap( canvas, GP_LINE_CAP_BUTT );
    gp_set_path_trim( canvas, 0.6f, 0.8f );
    gp_draw_path( canvas, path );

    gp_mesh_t before_mutation;
    gp_calculate_mesh_size( canvas, &before_mutation );
    gp_path_line_to( path, 110.f, 10.f );
    gp_mesh_t after_mutation;
    gp_calculate_mesh_size( canvas, &after_mutation );
    if( before_mutation.vertex_count != after_mutation.vertex_count ||
        before_mutation.index_count != after_mutation.index_count ||
        before_mutation.batch_count != after_mutation.batch_count )
    {
        return GP_FAILURE;
    }

    gp_mesh_t mesh;
    test_vertex_t * vertices;
    gp_uint16_t * indices;
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE || mesh.vertex_count != 4 ||
        test_near( vertices[0].x, 101.5f, 0.0001f ) == GP_FALSE ||
        test_near( vertices[0].y, 2.f, 0.0001f ) == GP_FALSE ||
        test_near( vertices[2].x, 101.5f, 0.0001f ) == GP_FALSE ||
        test_near( vertices[2].y, 6.f, 0.0001f ) == GP_FALSE )
    {
        return GP_FAILURE;
    }

    free( vertices );
    free( indices );
    gp_canvas_destroy( canvas );
    return gp_path_destroy( path );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_stable_path_trim( void )
{
    const float points[][2] = {{0.f, 0.f}, {10.f, 0.f}, {20.f, 0.f}, {30.f, 0.f}};
    gp_canvas_t * first_canvas;
    gp_canvas_t * second_canvas;
    gp_canvas_create( test_graphics, &first_canvas );
    gp_canvas_create( test_graphics, &second_canvas );

    gp_path_t * path;
    test_path_create_quality( first_canvas, &path, 8, 16 );
    gp_path_polyline( path, points, 4, sizeof( points[0] ), GP_NULLPTR, GP_FALSE );

    gp_set_penumbra( first_canvas, 0.f );
    gp_set_penumbra( second_canvas, 0.f );
    gp_set_line_cap( first_canvas, GP_LINE_CAP_BUTT );
    gp_set_line_cap( second_canvas, GP_LINE_CAP_BUTT );
    gp_set_uv_mode( first_canvas, GP_UV_MODE_PATH );
    gp_set_uv_mode( second_canvas, GP_UV_MODE_PATH );
    gp_set_path_trim( first_canvas, 0.f, 0.5f );
    gp_set_path_trim( second_canvas, 0.f, 0.75f );
    gp_draw_path( first_canvas, path );
    gp_draw_path( second_canvas, path );
    gp_path_destroy( path );

    gp_mesh_t first_mesh;
    gp_mesh_t second_mesh;
    test_vertex_t * first_vertices;
    test_vertex_t * second_vertices;
    gp_uint16_t * first_indices;
    gp_uint16_t * second_indices;
    if( test_render( first_canvas, &first_mesh, &first_vertices, &first_indices ) == GP_FAILURE ||
        test_render( second_canvas, &second_mesh, &second_vertices, &second_indices ) == GP_FAILURE ||
        first_mesh.vertex_count != 6 || second_mesh.vertex_count != 8 ||
        memcmp( first_vertices, second_vertices, sizeof( test_vertex_t ) * 4 ) != 0 ||
        test_near( first_vertices[4].x, 15.f, 0.0001f ) == GP_FALSE ||
        test_near( second_vertices[6].x, 22.5f, 0.0001f ) == GP_FALSE )
    {
        return GP_FAILURE;
    }

    free( first_vertices );
    free( first_indices );
    free( second_vertices );
    free( second_indices );
    gp_canvas_destroy( first_canvas );
    return gp_canvas_destroy( second_canvas );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_path_ellipse_arc( void )
{
    gp_canvas_t * canvas;
    gp_canvas_create( test_graphics, &canvas );

    gp_path_t * path;
    test_path_create_quality( canvas, &path, 8, 16 );
    gp_path_ellipse_arc( path, 10.f, 20.f, 8.f, 3.f, 0.f, 3.14159265358979323846f );

    gp_bounds_t bounds;
    gp_path_sample_t middle;
    if( gp_path_get_bounds( path, &bounds ) == GP_FAILURE ||
        gp_path_sample( path, 0.5f, GP_PATH_SAMPLE_NORMALIZED, &middle ) == GP_FAILURE ||
        test_near( bounds.minimum_x, 2.f, 0.0001f ) == GP_FALSE ||
        test_near( bounds.minimum_y, 20.f, 0.0001f ) == GP_FALSE ||
        test_near( bounds.maximum_x, 18.f, 0.0001f ) == GP_FALSE ||
        test_near( bounds.maximum_y, 23.f, 0.0001f ) == GP_FALSE ||
        test_near( middle.x, 10.f, 0.0001f ) == GP_FALSE ||
        test_near( middle.y, 23.f, 0.0001f ) == GP_FALSE )
    {
        return GP_FAILURE;
    }

    gp_canvas_destroy( canvas );

    return gp_path_destroy( path );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_batches_and_uv( void )
{
    gp_canvas_t * canvas;
    gp_canvas_create( test_graphics, &canvas );
    gp_set_penumbra( canvas, 0.f );
    gp_begin_fill( canvas );
    gp_rect( canvas, 0.f, 0.f, 10.f, 10.f );
    gp_rect( canvas, 12.f, 0.f, 10.f, 10.f );
    gp_rect( canvas, 24.f, 0.f, 10.f, 10.f );
    gp_rect( canvas, 36.f, 0.f, 10.f, 10.f );

    gp_mesh_t mesh;
    if( gp_calculate_mesh_size( canvas, &mesh ) == GP_FAILURE || mesh.batch_count != 1 ||
        mesh.vertex_count != 16 || mesh.index_count != 24 )
    {
        return GP_FAILURE;
    }

    test_vertex_t * vertices;
    gp_uint16_t * indices;
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    for( gp_uint32_t index = 0; index != mesh.vertex_count; ++index )
    {
        if( vertices[index].u0 < 0.f || vertices[index].u0 > 1.f ||
            vertices[index].v0 < 0.f || vertices[index].v0 > 1.f )
        {
            return GP_FAILURE;
        }
    }

    free( vertices );
    free( indices );
    gp_canvas_destroy( canvas );

    gp_canvas_create( test_graphics, &canvas );
    gp_set_penumbra( canvas, 0.f );
    gp_begin_fill( canvas );
    const float outer[][2] = {{0.f, 0.f}, {20.f, 0.f}, {20.f, 20.f}, {0.f, 20.f}};
    const float hole[][2] = {{5.f, 5.f}, {5.f, 15.f}, {15.f, 15.f}, {15.f, 5.f}};
    gp_path_t * compound_path;
    gp_path_create( canvas, &compound_path );
    gp_path_polyline( compound_path, outer, 4, sizeof( outer[0] ), GP_NULLPTR, GP_TRUE );
    gp_path_polyline( compound_path, hole, 4, sizeof( hole[0] ), GP_NULLPTR, GP_TRUE );
    test_draw_destroy_path( canvas, compound_path );
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    float minimum_u = vertices[0].u0;
    float maximum_u = vertices[0].u0;
    float minimum_v = vertices[0].v0;
    float maximum_v = vertices[0].v0;
    for( gp_uint32_t index = 1; index != mesh.vertex_count; ++index )
    {
        minimum_u = vertices[index].u0 < minimum_u ? vertices[index].u0 : minimum_u;
        maximum_u = vertices[index].u0 > maximum_u ? vertices[index].u0 : maximum_u;
        minimum_v = vertices[index].v0 < minimum_v ? vertices[index].v0 : minimum_v;
        maximum_v = vertices[index].v0 > maximum_v ? vertices[index].v0 : maximum_v;
    }
    if( test_near( minimum_u, 0.f, 0.0001f ) == GP_FALSE ||
        test_near( maximum_u, 1.f, 0.0001f ) == GP_FALSE ||
        test_near( minimum_v, 0.f, 0.0001f ) == GP_FALSE ||
        test_near( maximum_v, 1.f, 0.0001f ) == GP_FALSE )
    {
        return GP_FAILURE;
    }

    free( vertices );
    free( indices );
    gp_canvas_destroy( canvas );

    gp_canvas_create( test_graphics, &canvas );
    gp_set_penumbra( canvas, 0.f );
    gp_set_thickness( canvas, 4.f );
    gp_set_line_cap( canvas, GP_LINE_CAP_ROUND );
    gp_set_uv_mode( canvas, GP_UV_MODE_PATH );
    const float line[][2] = {{0.f, 0.f}, {10.f, 0.f}};
    gp_polyline( canvas, line, 2, sizeof( line[0] ), GP_NULLPTR, GP_FALSE );
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    minimum_u = vertices[0].u0;
    maximum_u = vertices[0].u0;
    for( gp_uint32_t index = 1; index != mesh.vertex_count; ++index )
    {
        minimum_u = vertices[index].u0 < minimum_u ? vertices[index].u0 : minimum_u;
        maximum_u = vertices[index].u0 > maximum_u ? vertices[index].u0 : maximum_u;
    }
    if( minimum_u >= 0.f || maximum_u <= 1.f )
    {
        return GP_FAILURE;
    }

    free( vertices );
    free( indices );
    return gp_canvas_destroy( canvas );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_catmull_rom_and_superellipse( void )
{
    const float anchors[][2] = {{0.f, 0.f}, {10.f, 10.f}, {20.f, -5.f}, {30.f, 0.f}};
    gp_canvas_t * canvas;
    gp_canvas_create( test_graphics, &canvas );

    gp_path_t * path;
    test_path_create_quality( canvas, &path, 8, 16 );
    if( gp_path_catmull_rom( path, anchors, 4, sizeof( anchors[0] ), GP_NULLPTR, GP_FALSE ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_path_sample_t first;
    gp_path_sample_t last;
    if( gp_path_sample( path, 0.f, GP_PATH_SAMPLE_NORMALIZED, &first ) == GP_FAILURE ||
        gp_path_sample( path, 1.f, GP_PATH_SAMPLE_NORMALIZED, &last ) == GP_FAILURE ||
        test_near( first.x, 0.f, 0.0001f ) == GP_FALSE || test_near( first.y, 0.f, 0.0001f ) == GP_FALSE ||
        test_near( last.x, 30.f, 0.0001f ) == GP_FALSE || test_near( last.y, 0.f, 0.0001f ) == GP_FALSE )
    {
        return GP_FAILURE;
    }

    gp_set_penumbra( canvas, 0.f );
    gp_draw_path( canvas, path );
    gp_path_destroy( path );
    gp_begin_fill( canvas );
    gp_superellipse( canvas, 50.f, 10.f, 12.f, 8.f, 4.f, 0.35f );

    gp_mesh_t mesh;
    test_vertex_t * vertices;
    gp_uint16_t * indices;
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE || mesh.vertex_count < 20 )
    {
        return GP_FAILURE;
    }
    free( vertices );
    free( indices );
    return gp_canvas_destroy( canvas );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_trim_markers( void )
{
    const float points[][2] = {{0.f, 0.f}, {20.f, 0.f}};
    for( gp_uint32_t type = GP_PATH_MARKER_FILLED_ARROW; type <= GP_PATH_MARKER_DIAMOND; ++type )
    {
        gp_canvas_t * canvas;
        gp_canvas_create( test_graphics, &canvas );
        gp_set_thickness( canvas, 2.f );
        gp_set_penumbra( canvas, 1.f );
        gp_set_path_trim( canvas, 0.25f, 0.75f );
        gp_path_marker_t start = {(gp_path_marker_type_t)type, 4.f, 5.f, 0.f};
        gp_path_marker_t end = {(gp_path_marker_type_t)type, 4.f, 5.f, 1.f};
        gp_set_path_markers( canvas, &start, &end );
        gp_polyline( canvas, points, 2, sizeof( points[0] ), GP_NULLPTR, GP_FALSE );

        gp_mesh_t mesh;
        test_vertex_t * vertices;
        gp_uint16_t * indices;
        if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE )
        {
            return GP_FAILURE;
        }

        gp_bool_t transparent = GP_FALSE;
        for( gp_uint32_t index = 0; index != mesh.vertex_count; ++index )
        {
            transparent |= (vertices[index].color >> 24) == 0 ? GP_TRUE : GP_FALSE;
        }
        if( transparent == GP_FALSE )
        {
            return GP_FAILURE;
        }
        free( vertices );
        free( indices );
        gp_canvas_destroy( canvas );
    }

    gp_canvas_t * canvas;
    gp_canvas_create( test_graphics, &canvas );
    gp_set_penumbra( canvas, 0.f );
    gp_set_outline_width( canvas, 2.f );
    gp_set_outline_color( canvas, 0.f, 1.f, 0.f, 1.f );
    gp_path_marker_t marker = {GP_PATH_MARKER_DIAMOND, 5.f, 5.f, 0.f};
    gp_set_path_markers( canvas, &marker, &marker );
    gp_polyline( canvas, points, 2, sizeof( points[0] ), GP_NULLPTR, GP_FALSE );
    gp_mesh_t mesh;
    test_vertex_t * vertices;
    gp_uint16_t * indices;
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }
    gp_bool_t outline_color = GP_FALSE;
    for( gp_uint32_t index = 0; index != mesh.vertex_count; ++index )
    {
        outline_color |= vertices[index].color == 0xff00ff00U ? GP_TRUE : GP_FALSE;
    }
    if( outline_color == GP_FALSE )
    {
        return GP_FAILURE;
    }
    free( vertices );
    free( indices );
    return gp_canvas_destroy( canvas );
}
//////////////////////////////////////////////////////////////////////////
#if defined(GP_DEBUG)
static gp_result_t test_invalid_path_layout( void )
{
    test_path_point_t points[] = {
        {0.f, 0.f, 2.f, {1.f, 1.f, 1.f, 1.f}},
        {10.f, 0.f, 2.f, {1.f, 1.f, 1.f, 1.f}}
    };
    gp_canvas_t * canvas;
    gp_canvas_create( test_graphics, &canvas );

    gp_path_layout_t bad_width_offset = {0, GP_PATH_ATTRIBUTE_NONE, GP_PATH_ATTRIBUTE_NONE};
    gp_path_layout_t bad_color_offset = {GP_PATH_ATTRIBUTE_NONE, sizeof( points[0] ), GP_PATH_ATTRIBUTE_NONE};
    if( gp_polyline( canvas, points, 2, sizeof( points[0] ), &bad_width_offset, GP_FALSE ) != GP_FAILURE ||
        gp_polyline( canvas, points, 2, sizeof( points[0] ), &bad_color_offset, GP_FALSE ) != GP_FAILURE ||
        gp_polyline( canvas, points, 2, sizeof( float ), GP_NULLPTR, GP_FALSE ) != GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_path_layout_t layout = {offsetof( test_path_point_t, width ), offsetof( test_path_point_t, color ), GP_PATH_ATTRIBUTE_NONE};
    points[0].width = 0.f;
    if( gp_polyline( canvas, points, 2, sizeof( points[0] ), &layout, GP_FALSE ) != GP_FAILURE )
    {
        return GP_FAILURE;
    }
    points[0].width = 2.f;
    points[0].color.r = 2.f;
    if( gp_polyline( canvas, points, 2, sizeof( points[0] ), &layout, GP_FALSE ) != GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_path_marker_t invalid_marker = {GP_PATH_MARKER_CIRCLE, NAN, 2.f, 0.f};
    if( gp_set_path_markers( canvas, &invalid_marker, GP_NULLPTR ) != GP_FAILURE ||
        gp_set_path_trim( canvas, NAN, 1.f ) != GP_FAILURE ||
        gp_set_thickness( canvas, NAN ) != GP_FAILURE || gp_set_thickness( canvas, 0.f ) != GP_FAILURE ||
        gp_set_penumbra( canvas, -1.f ) != GP_FAILURE || gp_set_outline_width( canvas, NAN ) != GP_FAILURE ||
        gp_set_color( canvas, NAN, 1.f, 1.f, 1.f ) != GP_FAILURE ||
        gp_set_uv_offset( canvas, 0.f, 0.f, NAN, 1.f ) != GP_FAILURE ||
        gp_set_ellipse_quality( canvas, 253 ) != GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_path_t * path;
    gp_path_create( canvas, &path );
    if( gp_path_polyline( path, points, 2, sizeof( float ) * 2, &bad_color_offset, GP_FALSE ) != GP_FAILURE )
    {
        return GP_FAILURE;
    }
    gp_path_move_to( path, 0.f, 0.f );
    gp_path_line_to( path, 10.f, 0.f );
    gp_set_stroke_alignment( canvas, GP_STROKE_ALIGNMENT_INNER );
    if( gp_draw_path( canvas, path ) != GP_FAILURE )
    {
        return GP_FAILURE;
    }
    gp_path_destroy( path );

    return gp_canvas_destroy( canvas );
}
#endif
//////////////////////////////////////////////////////////////////////////
int main( int argc, char ** argv )
{
    GP_UNUSED( argc );
    GP_UNUSED( argv );

    if( gp_graphics_create( &test_graphics, &test_malloc, &test_realloc, &test_free, GP_NULLPTR,
        GP_CAPACITY_DEFAULT, GP_CAPACITY_DEFAULT, GP_CAPACITY_DEFAULT ) == GP_FAILURE )
    {
        fprintf( stderr, "gp_graphics_create failed\n" );
        return EXIT_FAILURE;
    }

    if( test_graphics_capacity() == GP_FAILURE )
    {
        fprintf( stderr, "test_graphics_capacity failed\n" );
        return EXIT_FAILURE;
    }

    if( test_multi_canvas() == GP_FAILURE )
    {
        fprintf( stderr, "test_multi_canvas failed\n" );
        return EXIT_FAILURE;
    }

    if( test_large_index_count() == GP_FAILURE )
    {
        fprintf( stderr, "test_large_index_count failed\n" );
        return EXIT_FAILURE;
    }

    if( test_large_line_index_count() == GP_FAILURE )
    {
        fprintf( stderr, "test_large_line_index_count failed\n" );
        return EXIT_FAILURE;
    }

#if defined(GP_DEBUG)
    if( test_unique_vertex_limit() == GP_FAILURE )
    {
        fprintf( stderr, "test_unique_vertex_limit failed\n" );
        return EXIT_FAILURE;
    }
#endif

    if( test_state_restore() == GP_FAILURE )
    {
        fprintf( stderr, "test_state_restore failed\n" );
        return EXIT_FAILURE;
    }

    if( test_transform_state() == GP_FAILURE )
    {
        fprintf( stderr, "test_transform_state failed\n" );
        return EXIT_FAILURE;
    }

    if( test_close_path() == GP_FAILURE )
    {
        fprintf( stderr, "test_close_path failed\n" );
        return EXIT_FAILURE;
    }

    if( test_compound_fill() == GP_FAILURE )
    {
        fprintf( stderr, "test_compound_fill failed\n" );
        return EXIT_FAILURE;
    }

    if( test_paths_and_primitives() == GP_FAILURE )
    {
        fprintf( stderr, "test_paths_and_primitives failed\n" );
        return EXIT_FAILURE;
    }

    if( test_radial_indicators() == GP_FAILURE )
    {
        fprintf( stderr, "test_radial_indicators failed\n" );
        return EXIT_FAILURE;
    }

    if( test_arc_growth_stability() == GP_FAILURE )
    {
        fprintf( stderr, "test_arc_growth_stability failed\n" );
        return EXIT_FAILURE;
    }

    if( test_rounded_ring() == GP_FAILURE )
    {
        fprintf( stderr, "test_rounded_ring failed\n" );
        return EXIT_FAILURE;
    }

    if( test_dashed_paths() == GP_FAILURE )
    {
        fprintf( stderr, "test_dashed_paths failed\n" );
        return EXIT_FAILURE;
    }

    if( test_rich_paths() == GP_FAILURE )
    {
        fprintf( stderr, "test_rich_paths failed\n" );
        return EXIT_FAILURE;
    }

    if( test_soft_round_geometry() == GP_FAILURE )
    {
        fprintf( stderr, "test_soft_round_geometry failed\n" );
        return EXIT_FAILURE;
    }

    if( test_smooth_round_curve_geometry() == GP_FAILURE )
    {
        fprintf( stderr, "test_smooth_round_curve_geometry failed\n" );
        return EXIT_FAILURE;
    }

    if( test_reusable_path_queries_and_snapshot() == GP_FAILURE )
    {
        fprintf( stderr, "test_reusable_path_queries_and_snapshot failed\n" );
        return EXIT_FAILURE;
    }

    if( test_stable_path_trim() == GP_FAILURE )
    {
        fprintf( stderr, "test_stable_path_trim failed\n" );
        return EXIT_FAILURE;
    }

    if( test_path_ellipse_arc() == GP_FAILURE )
    {
        fprintf( stderr, "test_path_ellipse_arc failed\n" );
        return EXIT_FAILURE;
    }

    if( test_batches_and_uv() == GP_FAILURE )
    {
        fprintf( stderr, "test_batches_and_uv failed\n" );
        return EXIT_FAILURE;
    }

    if( test_catmull_rom_and_superellipse() == GP_FAILURE )
    {
        fprintf( stderr, "test_catmull_rom_and_superellipse failed\n" );
        return EXIT_FAILURE;
    }

    if( test_trim_markers() == GP_FAILURE )
    {
        fprintf( stderr, "test_trim_markers failed\n" );
        return EXIT_FAILURE;
    }

#if defined(GP_DEBUG)
    if( test_invalid_path_layout() == GP_FAILURE )
    {
        fprintf( stderr, "test_invalid_path_layout failed\n" );
        return EXIT_FAILURE;
    }
#endif

    if( gp_graphics_destroy( test_graphics ) == GP_FAILURE )
    {
        fprintf( stderr, "gp_graphics_destroy failed\n" );
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
