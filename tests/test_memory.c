#include "graphics/graphics.h"

#include "../src/detail.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if( !(condition) ) { fprintf( stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition ); return GP_FAILURE; } } while( 0 )
#define CALL(method, args) CHECK( method args == GP_SUCCESSFUL )

//////////////////////////////////////////////////////////////////////////
typedef union test_allocation_t
{
    max_align_t alignment;
    gp_size_t size;
} test_allocation_t;
//////////////////////////////////////////////////////////////////////////
typedef struct test_memory_t
{
    gp_size_t bytes;
    gp_size_t calls;
    gp_size_t fail_at;
} test_memory_t;
//////////////////////////////////////////////////////////////////////////
static void * test_malloc( gp_size_t _size, void * _ud )
{
    test_memory_t * memory = (test_memory_t *)_ud;
    if( ++memory->calls == memory->fail_at )
    {
        return GP_NULLPTR;
    }

    test_allocation_t * allocation = (test_allocation_t *)malloc( sizeof( test_allocation_t ) + _size );
    if( allocation == GP_NULLPTR )
    {
        return GP_NULLPTR;
    }

    allocation->size = _size;
    memory->bytes += _size;
    return allocation + 1;
}
//////////////////////////////////////////////////////////////////////////
static void test_free( void * _pointer, void * _ud )
{
    if( _pointer == GP_NULLPTR )
    {
        return;
    }

    test_memory_t * memory = (test_memory_t *)_ud;
    test_allocation_t * allocation = (test_allocation_t *)_pointer - 1;
    memory->bytes -= allocation->size;
    free( allocation );
}
//////////////////////////////////////////////////////////////////////////
static void * test_realloc( void * _pointer, gp_size_t _size, void * _ud )
{
    void * result = test_malloc( _size, _ud );
    if( result == GP_NULLPTR )
    {
        return GP_NULLPTR;
    }

    if( _pointer != GP_NULLPTR )
    {
        const test_allocation_t * allocation = (const test_allocation_t *)_pointer - 1;
        gp_size_t copy_size = allocation->size < _size ? allocation->size : _size;
        memcpy( result, _pointer, copy_size );
        test_free( _pointer, _ud );
    }

    return result;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_create( test_memory_t * _memory, gp_graphics_t ** _graphics )
{
    gp_result_t result = gp_graphics_create( _graphics, &test_malloc, &test_realloc, &test_free, _memory, GP_CAPACITY_DEFAULT, GP_CAPACITY_DEFAULT, GP_CAPACITY_DEFAULT );
    return result;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_mesh( gp_canvas_t * _canvas, gp_uint32_t _vertices, gp_uint32_t _indices )
{
    gp_mesh_t mesh;
    CALL( gp_calculate_mesh_size, (_canvas, &mesh) );
    CHECK( mesh.vertex_count == _vertices );
    CHECK( mesh.index_count == _indices );
    CALL( gp_render, (_canvas, &mesh) );
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_canvas_memory( void )
{
    test_memory_t memory = {0};
    gp_graphics_t * graphics;
    CALL( test_create, (&memory, &graphics) );
    gp_size_t graphics_bytes = memory.bytes;
    gp_canvas_t * canvases[100];
    for( gp_size_t index = 0; index != 100; ++index )
    {
        CALL( gp_canvas_create, (graphics, &canvases[index]) );
        gp_canvas_memory_stats_t stats;
        CALL( gp_canvas_get_memory_stats, (canvases[index], &stats) );
        CHECK( stats.recording_capacity == 0 && stats.render_order_capacity == 0 && stats.scratch_capacity == 0 );
    }

    gp_size_t empty_bytes = memory.bytes - graphics_bytes;
    CHECK( empty_bytes < 100U * 16U * 1024U );
    for( gp_size_t index = 0; index != 100; ++index )
    {
        CALL( gp_set_penumbra, (canvases[index], 0.f) );
        CALL( gp_begin_fill, (canvases[index]) );
        CALL( gp_rect, (canvases[index], 0.f, 0.f, 10.f, 10.f) );
    }

    gp_size_t simple_bytes = memory.bytes - graphics_bytes;
    CHECK( simple_bytes < 100U * 24U * 1024U );
    printf( "graphics=%zu bytes; canvas: empty=%zu bytes, one rectangle=%zu bytes; 100 rectangles=%zu bytes\n", graphics_bytes, empty_bytes / 100U, simple_bytes / 100U, simple_bytes + graphics_bytes );

    for( gp_uint32_t index = 0; index != 2000; ++index )
    {
        CALL( gp_rect, (canvases[0], (float)index, 0.f, 1.f, 1.f) );
    }
    CALL( test_mesh, (canvases[0], 2001U * 4U, 2001U * 6U) );
    gp_canvas_memory_stats_t stats;
    CALL( gp_canvas_get_memory_stats, (canvases[0], &stats) );
    CHECK( stats.recording_used > GP_CANVAS_RECORDING_MEMORY_SIZE );
    CHECK( stats.render_order_capacity >= 2001U );
    gp_size_t grown_bytes = memory.bytes;
    gp_size_t grown_recording_capacity = stats.recording_capacity;
    gp_size_t grown_render_order_capacity = stats.render_order_capacity;
    CALL( gp_canvas_clear, (canvases[0]) );
    CHECK( memory.bytes == grown_bytes );
    CALL( gp_canvas_get_memory_stats, (canvases[0], &stats) );
    CHECK( stats.recording_used == 0 );
    CHECK( stats.recording_capacity == grown_recording_capacity );
    CHECK( stats.render_order_capacity == grown_render_order_capacity );
    CHECK( stats.recording_peak > GP_CANVAS_RECORDING_MEMORY_SIZE );

    CALL( gp_set_penumbra, (canvases[0], 0.f) );
    CALL( gp_begin_fill, (canvases[0]) );
    CALL( gp_rect, (canvases[0], 0.f, 0.f, 10.f, 10.f) );
    for( gp_uint32_t index = 0; index != 2000; ++index )
    {
        CALL( gp_rect, (canvases[0], (float)index, 0.f, 1.f, 1.f) );
    }
    CALL( test_mesh, (canvases[0], 2001U * 4U, 2001U * 6U) );
    CHECK( memory.bytes == grown_bytes );
    CALL( gp_canvas_clear, (canvases[0]) );
    CALL( test_mesh, (canvases[1], 4, 6) );

    for( gp_size_t index = 0; index != 100; ++index )
    {
        CALL( gp_canvas_destroy, (canvases[index]) );
    }
    CALL( gp_graphics_destroy, (graphics) );
    CHECK( memory.bytes == 0 );
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_shared_scratch( void )
{
    test_memory_t memory = {0};
    gp_graphics_t * graphics;
    CALL( test_create, (&memory, &graphics) );
    gp_canvas_t * first;
    gp_canvas_t * second;
    CALL( gp_canvas_create, (graphics, &first) );
    CALL( gp_canvas_create, (graphics, &second) );
    float points[256][2];
    for( gp_size_t index = 0; index != 256; ++index )
    {
        points[index][0] = (float)index;
        points[index][1] = 0.f;
    }
    CALL( gp_set_penumbra, (first, 0.f) );
    CALL( gp_set_penumbra, (second, 0.f) );
    CALL( gp_polyline, (first, points, 256, sizeof( points[0] ), GP_NULLPTR, GP_FALSE) );
    CALL( gp_polyline, (second, points, 256, sizeof( points[0] ), GP_NULLPTR, GP_FALSE) );
    CALL( test_mesh, (first, 512, 1530) );
    gp_size_t warm_calls = memory.calls;
    CALL( test_mesh, (second, 512, 1530) );
    CHECK( memory.calls == warm_calls );

    gp_canvas_memory_stats_t first_stats;
    gp_canvas_memory_stats_t second_stats;
    CALL( gp_canvas_get_memory_stats, (first, &first_stats) );
    CALL( gp_canvas_get_memory_stats, (second, &second_stats) );
    CHECK( first_stats.scratch_used == 0 && second_stats.scratch_used == 0 );
    CHECK( first_stats.scratch_capacity > 0 && first_stats.scratch_capacity == second_stats.scratch_capacity );
    CHECK( first_stats.scratch_peak == second_stats.scratch_peak );

    gp_uint8_t * original = (gp_uint8_t *)gp_canvas_scratch_reallocate( first, GP_NULLPTR, 256 );
    CHECK( original != GP_NULLPTR );
    memset( original, 0x5a, 256 );
    gp_size_t mark = gp_canvas_scratch_mark( first );
    CHECK( gp_canvas_scratch_reallocate( second, GP_NULLPTR, 8192 ) != GP_NULLPTR );
    gp_uint8_t * grown = (gp_uint8_t *)gp_canvas_scratch_reallocate( first, original, 16384 );
    CHECK( grown != GP_NULLPTR );
    for( gp_size_t index = 0; index != 256; ++index )
    {
        CHECK( grown[index] == 0x5a && original[index] == 0x5a );
    }
    gp_size_t used = gp_canvas_scratch_mark( first );
    CHECK( gp_canvas_scratch_reallocate( first, original, SIZE_MAX ) == GP_NULLPTR );
    CHECK( gp_canvas_recording_allocate( first, SIZE_MAX ) == GP_NULLPTR );
    CALL( gp_canvas_clear, (second) );
    CHECK( gp_canvas_scratch_mark( first ) == used );
    gp_canvas_scratch_reset( second, mark );
    CHECK( original[0] == 0x5a && original[255] == 0x5a );
    gp_canvas_scratch_free( first, original );
    CHECK( gp_canvas_scratch_mark( first ) == 0 );
    CALL( gp_canvas_destroy, (second) );
    CALL( test_mesh, (first, 512, 1530) );
    printf( "shared scratch for two 256-point paths=%zu bytes\n", first_stats.scratch_capacity );
    CALL( gp_canvas_destroy, (first) );
    CALL( gp_graphics_destroy, (graphics) );
    CHECK( memory.bytes == 0 );
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_limits_and_reuse( void )
{
    test_memory_t memory = {0};
    gp_graphics_t * graphics;
    CALL( gp_graphics_create, (&graphics, &test_malloc, &test_realloc, &test_free, &memory, 1024, 2, 1024U * 1024U) );
    gp_canvas_t * canvas;
    CALL( gp_canvas_create, (graphics, &canvas) );
    CALL( gp_set_penumbra, (canvas, 0.f) );
    CALL( gp_begin_fill, (canvas) );
    CALL( gp_rect, (canvas, 0.f, 0.f, 1.f, 1.f) );
    CALL( gp_rect, (canvas, 2.f, 0.f, 1.f, 1.f) );
    CHECK( gp_rect( canvas, 4.f, 0.f, 1.f, 1.f ) == GP_FAILURE );
    CALL( test_mesh, (canvas, 8, 12) );
    gp_canvas_memory_stats_t stats;
    CALL( gp_canvas_get_memory_stats, (canvas, &stats) );
    CHECK( stats.render_order_capacity == 2 );
    CHECK( stats.recording_capacity > 1024 );

    CHECK( gp_canvas_scratch_reallocate( canvas, GP_NULLPTR, 800U * 1024U ) != GP_NULLPTR );
    gp_canvas_scratch_reset( canvas, 0 );
    CHECK( gp_canvas_scratch_reallocate( canvas, GP_NULLPTR, 900U * 1024U ) != GP_NULLPTR );
    gp_canvas_scratch_reset( canvas, 0 );
    CHECK( gp_canvas_scratch_reallocate( canvas, GP_NULLPTR, 1024U * 1024U ) == GP_NULLPTR );
    CHECK( gp_canvas_scratch_mark( canvas ) == 0 );
    CHECK( gp_canvas_scratch_reallocate( canvas, GP_NULLPTR, 512U ) != GP_NULLPTR );
    gp_canvas_scratch_reset( canvas, 0 );
    CALL( gp_canvas_get_memory_stats, (canvas, &stats) );
    CHECK( stats.scratch_capacity <= 1024U * 1024U );
    CALL( gp_canvas_destroy, (canvas) );
    CALL( gp_graphics_destroy, (graphics) );
    CHECK( memory.bytes == 0 );
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_record( gp_canvas_t * _canvas )
{
    float points[128][2];
    for( gp_size_t index = 0; index != 128; ++index )
    {
        points[index][0] = (float)index;
        points[index][1] = (float)(index % 2U);
    }
    CALL( gp_set_penumbra, (_canvas, 0.f) );
    gp_result_t result = gp_polyline( _canvas, points, 128, sizeof( points[0] ), GP_NULLPTR, GP_FALSE );
    if( result == GP_FAILURE )
    {
        return result;
    }
    for( gp_size_t index = 0; index != 65; ++index )
    {
        result = gp_rect( _canvas, (float)index, 0.f, 1.f, 1.f );
        if( result == GP_FAILURE )
        {
            return result;
        }
    }
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_failure_run( gp_size_t _fail_at, gp_size_t * _calls )
{
    test_memory_t memory = {0};
    memory.fail_at = _fail_at;
    gp_graphics_t * graphics;
    if( test_create( &memory, &graphics ) == GP_SUCCESSFUL )
    {
        gp_canvas_t * canvas;
        if( gp_canvas_create( graphics, &canvas ) == GP_SUCCESSFUL )
        {
            gp_result_t result = test_record( canvas );
            gp_canvas_memory_stats_t stats;
            CALL( gp_canvas_get_memory_stats, (canvas, &stats) );
            CHECK( stats.scratch_used == 0 );
            if( result == GP_SUCCESSFUL )
            {
                gp_mesh_t mesh;
                result = gp_calculate_mesh_size( canvas, &mesh );
                if( result == GP_SUCCESSFUL )
                {
                    result = gp_render( canvas, &mesh );
                }
                CALL( gp_canvas_get_memory_stats, (canvas, &stats) );
                CHECK( stats.scratch_used == 0 );
                if( result == GP_FAILURE )
                {
                    CALL( gp_calculate_mesh_size, (canvas, &mesh) );
                    CALL( gp_render, (canvas, &mesh) );
                }
                gp_bounds_t bounds;
                result = gp_calculate_bounds( canvas, &bounds );
                CALL( gp_canvas_get_memory_stats, (canvas, &stats) );
                CHECK( stats.scratch_used == 0 );
                if( result == GP_FAILURE )
                {
                    CALL( gp_calculate_bounds, (canvas, &bounds) );
                }
                CALL( gp_flush, (canvas, &mesh) );
                CALL( gp_canvas_get_memory_stats, (canvas, &stats) );
                CHECK( stats.recording_used == 0 && stats.render_order_count == 0 && stats.scratch_used == 0 );
            }
            CALL( gp_canvas_destroy, (canvas) );
        }
        CALL( gp_graphics_destroy, (graphics) );
    }
    CHECK( memory.bytes == 0 );
    *_calls = memory.calls;
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t test_allocation_failures( void )
{
    gp_size_t calls;
    CALL( test_failure_run, (0, &calls) );
    for( gp_size_t fail_at = 1; fail_at <= calls; ++fail_at )
    {
        gp_size_t failed_calls;
        CALL( test_failure_run, (fail_at, &failed_calls) );
    }
    printf( "allocation failure positions checked=%zu\n", calls );
    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
int main( void )
{
    if( test_canvas_memory() == GP_FAILURE )
    {
        return EXIT_FAILURE;
    }
    if( test_shared_scratch() == GP_FAILURE )
    {
        return EXIT_FAILURE;
    }
    if( test_limits_and_reuse() == GP_FAILURE )
    {
        return EXIT_FAILURE;
    }
    if( test_allocation_failures() == GP_FAILURE )
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
