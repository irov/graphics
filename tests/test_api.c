#include "graphics/graphics.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////
typedef struct test_vertex_t
{
    float x;
    float y;
    gp_uint32_t color;
} test_vertex_t;
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
static gp_result_t test_render( gp_canvas_t * _canvas, gp_mesh_t * _mesh, test_vertex_t ** _vertices, gp_uint16_t ** _indices )
{
    if( gp_calculate_mesh_size( _canvas, _mesh ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    *_vertices = (test_vertex_t *)malloc( sizeof( test_vertex_t ) * _mesh->vertex_count );
    *_indices = (gp_uint16_t *)malloc( sizeof( gp_uint16_t ) * _mesh->index_count );

    _mesh->positions_buffer = *_vertices;
    _mesh->positions_offset = offsetof( test_vertex_t, x );
    _mesh->positions_stride = sizeof( test_vertex_t );
    _mesh->colors_buffer = *_vertices;
    _mesh->colors_offset = offsetof( test_vertex_t, color );
    _mesh->colors_stride = sizeof( test_vertex_t );
    _mesh->uv_buffer = GP_NULLPTR;
    _mesh->indices_buffer = *_indices;
    _mesh->indices_offset = 0;
    _mesh->indices_stride = sizeof( gp_uint16_t );

    if( gp_render( _canvas, _mesh ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    for( gp_uint32_t index = 0; index != _mesh->vertex_count; ++index )
    {
        if( isfinite( (*_vertices)[index].x ) == 0 || isfinite( (*_vertices)[index].y ) == 0 )
        {
            return GP_FAILURE;
        }
    }

    for( gp_uint32_t index = 0; index != _mesh->index_count; ++index )
    {
        if( (*_indices)[index] >= _mesh->vertex_count )
        {
            return GP_FAILURE;
        }
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_large_index_count( void )
{
    gp_canvas_t * canvas;
    if( gp_canvas_create( &canvas, &test_malloc, &test_realloc, &test_free, GP_NULLPTR ) == GP_FAILURE )
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
#if defined(GP_DEBUG)
static gp_result_t test_unique_vertex_limit( void )
{
    gp_canvas_t * canvas;
    if( gp_canvas_create( &canvas, &test_malloc, &test_realloc, &test_free, GP_NULLPTR ) == GP_FAILURE )
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
    gp_canvas_destroy( canvas );

    return result == GP_FAILURE ? GP_SUCCESSFUL : GP_FAILURE;
}
#endif
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_state_restore( void )
{
    gp_canvas_t * canvas;
    if( gp_canvas_create( &canvas, &test_malloc, &test_realloc, &test_free, GP_NULLPTR ) == GP_FAILURE )
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
static gp_result_t test_close_path( void )
{
    gp_canvas_t * canvas;
    if( gp_canvas_create( &canvas, &test_malloc, &test_realloc, &test_free, GP_NULLPTR ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_set_penumbra( canvas, 0.f );
    gp_point_move_to( canvas, 0.f, 0.f );
    gp_point_line_to( canvas, 10.f, 0.f );
    gp_point_line_to( canvas, 0.f, 10.f );
    gp_point_close( canvas );

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
    if( gp_canvas_create( &canvas, &test_malloc, &test_realloc, &test_free, GP_NULLPTR ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_set_penumbra( canvas, 0.f );
    gp_begin_compound_fill( canvas, GP_FILL_RULE_EVEN_ODD );
    gp_polyline( canvas, outer, 4, sizeof( outer[0] ), GP_TRUE );
    gp_polyline( canvas, hole, 4, sizeof( hole[0] ), GP_TRUE );
    gp_end_compound_fill( canvas );

    gp_mesh_t mesh;
    test_vertex_t * vertices;
    gp_uint16_t * indices;
    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    if( mesh.vertex_count != 8 || mesh.index_count != 24 )
    {
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
        return GP_FAILURE;
    }

    free( vertices );
    free( indices );

    gp_canvas_clear( canvas );
    gp_set_penumbra( canvas, 0.f );
    gp_begin_compound_fill( canvas, GP_FILL_RULE_NON_ZERO );
    gp_polyline( canvas, outer, 4, sizeof( outer[0] ), GP_TRUE );
    gp_polyline( canvas, hole, 4, sizeof( hole[0] ), GP_TRUE );
    gp_end_compound_fill( canvas );

    if( test_render( canvas, &mesh, &vertices, &indices ) == GP_FAILURE )
    {
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
    if( gp_canvas_create( &canvas, &test_malloc, &test_realloc, &test_free, GP_NULLPTR ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_set_penumbra( canvas, 0.f );
    gp_set_line_cap( canvas, GP_LINE_CAP_ROUND );
    gp_set_line_join( canvas, GP_LINE_JOIN_ROUND );
    gp_set_miter_limit( canvas, 4.f );
    gp_set_line_dash( canvas, dash, 2, 1.f );
    gp_polyline( canvas, points, 4, sizeof( points[0] ), GP_FALSE );
    gp_clear_line_dash( canvas );
    gp_rounded_polyline( canvas, points, 4, sizeof( points[0] ), 5.f, GP_TRUE );
    gp_rounded_rect_varying( canvas, 0.f, 70.f, 50.f, 30.f, 2.f, 4.f, 6.f, 8.f );
    gp_capsule( canvas, 60.f, 0.f, 40.f, 20.f );
    gp_regular_polygon( canvas, 120.f, 20.f, 15.f, 6, 0.f );
    gp_push_state( canvas );
    gp_begin_fill( canvas );
    gp_regular_polygon( canvas, 120.f, 120.f, 15.f, 1100, 0.f );
    gp_pop_state( canvas );
    gp_star( canvas, 160.f, 20.f, 8.f, 16.f, 5, 0.f );
    gp_sector( canvas, 200.f, 20.f, 15.f, 0.f, 1.5f );
    gp_ring( canvas, 240.f, 20.f, 8.f, 16.f, 0.f, 3.f );
    gp_polygon_offset( canvas, points, 4, sizeof( points[0] ), 3.f, GP_LINE_JOIN_MITER );
    gp_polygon_offset( canvas, points, 4, sizeof( points[0] ), 3.f, GP_LINE_JOIN_BEVEL );
    gp_polygon_offset( canvas, points, 4, sizeof( points[0] ), 3.f, GP_LINE_JOIN_ROUND );
    gp_polygon_offset( canvas, points, 4, sizeof( points[0] ), -2.f, GP_LINE_JOIN_ROUND );
    gp_arrow( canvas, 280.f, 0.f, 310.f, 20.f, 8.f, 6.f );
    gp_arrowed_polyline( canvas, points, 4, sizeof( points[0] ), 8.f, 6.f );
    gp_point_move_to( canvas, 330.f, 0.f );
    gp_point_tangent_arc_to( canvas, 350.f, 0.f, 350.f, 20.f, 5.f );

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
int main( int argc, char ** argv )
{
    GP_UNUSED( argc );
    GP_UNUSED( argv );

    if( test_large_index_count() == GP_FAILURE )
    {
        fprintf( stderr, "test_large_index_count failed\n" );
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

    return EXIT_SUCCESS;
}
