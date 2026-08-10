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
static gp_result_t test_large_line_index_count( void )
{
    const gp_uint32_t point_count = 11001;
    float * points = (float *)malloc( sizeof( float ) * point_count * 2 );
    for( gp_uint32_t index = 0; index != point_count; ++index )
    {
        points[index * 2 + 0] = (float)index;
        points[index * 2 + 1] = 0.f;
    }

    gp_canvas_t * canvas;
    if( gp_canvas_create( &canvas, &test_malloc, &test_realloc, &test_free, GP_NULLPTR ) == GP_FAILURE )
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
        mesh.vertex_count != 22002 || mesh.index_count != 66000 )
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
    gp_polyline( canvas, outer, 4, sizeof( outer[0] ), GP_NULLPTR, GP_TRUE );
    gp_polyline( canvas, hole, 4, sizeof( hole[0] ), GP_NULLPTR, GP_TRUE );
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
    gp_polyline( canvas, outer, 4, sizeof( outer[0] ), GP_NULLPTR, GP_TRUE );
    gp_polyline( canvas, hole, 4, sizeof( hole[0] ), GP_NULLPTR, GP_TRUE );
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
    gp_arrow( canvas, 280.f, 0.f, 310.f, 20.f, 8.f, 6.f );
    gp_arrowed_polyline( canvas, points, 4, sizeof( points[0] ), GP_NULLPTR, 8.f, 6.f );
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
static gp_result_t test_radial_indicators( void )
{
    const float pi = 3.14159265358979323846f;
    gp_canvas_t * canvas;
    if( gp_canvas_create( &canvas, &test_malloc, &test_realloc, &test_free, GP_NULLPTR ) == GP_FAILURE )
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
    gp_canvas_create( &canvas, &test_malloc, &test_realloc, &test_free, GP_NULLPTR );
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
    gp_canvas_create( &canvas, &test_malloc, &test_realloc, &test_free, GP_NULLPTR );
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
    gp_canvas_create( &odd_canvas, &test_malloc, &test_realloc, &test_free, GP_NULLPTR );
    gp_canvas_create( &doubled_canvas, &test_malloc, &test_realloc, &test_free, GP_NULLPTR );

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
    gp_canvas_create( &closed_canvas, &test_malloc, &test_realloc, &test_free, GP_NULLPTR );
    const float seam_pattern[] = {15.f, 5.f};
    gp_set_penumbra( closed_canvas, 0.f );
    gp_set_line_cap( closed_canvas, GP_LINE_CAP_ROUND );
    gp_set_line_dash( closed_canvas, seam_pattern, 2, 2.f );
    gp_arc( closed_canvas, 0.f, 0.f, 10.f, 0.f, pi * 2.f );
    gp_point_move_to( closed_canvas, 30.f, 0.f );
    gp_point_quadratic_curve_to( closed_canvas, 40.f, 20.f, 50.f, 0.f );
    gp_point_bezier_curve_to( closed_canvas, 55.f, -10.f, 65.f, -10.f, 70.f, 0.f );
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
    gp_canvas_create( &fill_canvas, &test_malloc, &test_realloc, &test_free, GP_NULLPTR );
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

    gp_canvas_t * canvas;
    gp_canvas_create( &canvas, &test_malloc, &test_realloc, &test_free, GP_NULLPTR );
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

    gp_set_thickness( canvas, 2.f );
    gp_set_color( canvas, 1.f, 0.f, 0.f, 1.f );
    gp_point_move_to( canvas, 20.f, 0.f );
    gp_set_thickness( canvas, 6.f );
    gp_set_color( canvas, 0.f, 0.f, 1.f, 1.f );
    gp_point_quadratic_curve_to( canvas, 25.f, 10.f, 30.f, 0.f );
    gp_set_thickness( canvas, 4.f );
    gp_set_color( canvas, 0.f, 1.f, 0.f, 1.f );
    gp_point_bezier_curve_to( canvas, 32.f, -10.f, 38.f, -10.f, 40.f, 0.f );

    test_path_point_t rounded[] = {
        {50.f, 0.f, 2.f, {1.f, 0.f, 0.f, 1.f}},
        {70.f, 0.f, 8.f, {0.f, 1.f, 0.f, 1.f}},
        {70.f, 20.f, 4.f, {0.f, 0.f, 1.f, 1.f}},
        {50.f, 20.f, 6.f, {1.f, 1.f, 0.f, 1.f}}
    };
    gp_set_line_join( canvas, GP_LINE_JOIN_MITER );
    gp_polyline( canvas, rounded, 4, sizeof( rounded[0] ), &layout, GP_TRUE );
    gp_rounded_polyline( canvas, rounded, 4, sizeof( rounded[0] ), &layout, 4.f, GP_TRUE );
    gp_arrowed_polyline( canvas, rounded, 4, sizeof( rounded[0] ), &layout, 5.f, 4.f );

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
    gp_canvas_create( &canvas, &test_malloc, &test_realloc, &test_free, GP_NULLPTR );
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
    gp_path_layout_t layout = {offsetof( test_path_point_t, width ), offsetof( test_path_point_t, color )};
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
    gp_canvas_create( &canvas, &test_malloc, &test_realloc, &test_free, GP_NULLPTR );
    gp_set_thickness( canvas, 4.f );
    gp_set_penumbra( canvas, 0.f );
    gp_set_line_cap( canvas, GP_LINE_CAP_BUTT );
    gp_set_line_join( canvas, GP_LINE_JOIN_ROUND );
    gp_set_curve_quality( canvas, 4 );
    gp_point_move_to( canvas, 0.f, 0.f );
    gp_point_line_to( canvas, 10.f, 0.f );
    gp_point_quadratic_curve_to( canvas, 15.f, 0.f, 20.f, 5.f );
    gp_point_line_to( canvas, 25.f, 10.f );

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
#if defined(GP_DEBUG)
static gp_result_t test_invalid_path_layout( void )
{
    test_path_point_t points[] = {
        {0.f, 0.f, 2.f, {1.f, 1.f, 1.f, 1.f}},
        {10.f, 0.f, 2.f, {1.f, 1.f, 1.f, 1.f}}
    };
    gp_canvas_t * canvas;
    gp_canvas_create( &canvas, &test_malloc, &test_realloc, &test_free, GP_NULLPTR );

    gp_path_layout_t bad_width_offset = {0, GP_PATH_ATTRIBUTE_NONE};
    gp_path_layout_t bad_color_offset = {GP_PATH_ATTRIBUTE_NONE, sizeof( points[0] )};
    if( gp_polyline( canvas, points, 2, sizeof( points[0] ), &bad_width_offset, GP_FALSE ) != GP_FAILURE ||
        gp_polyline( canvas, points, 2, sizeof( points[0] ), &bad_color_offset, GP_FALSE ) != GP_FAILURE ||
        gp_polyline( canvas, points, 2, sizeof( float ), GP_NULLPTR, GP_FALSE ) != GP_FAILURE )
    {
        return GP_FAILURE;
    }

    gp_path_layout_t layout = {offsetof( test_path_point_t, width ), offsetof( test_path_point_t, color )};
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

    return gp_canvas_destroy( canvas );
}
#endif
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

#if defined(GP_DEBUG)
    if( test_invalid_path_layout() == GP_FAILURE )
    {
        fprintf( stderr, "test_invalid_path_layout failed\n" );
        return EXIT_FAILURE;
    }
#endif

    return EXIT_SUCCESS;
}
