#include "graphics/graphics.h"

#include "detail.h"
#include "struct.h"

//////////////////////////////////////////////////////////////////////////
gp_result_t gp_calculate_bounds( const gp_canvas_t * _canvas, gp_bounds_t * _bounds )
{
#if defined(GP_DEBUG)
    if( _canvas == GP_NULLPTR || _bounds == GP_NULLPTR )
    {
        return GP_FAILURE;
    }
#endif

    gp_mesh_t mesh;
    GP_CALL( gp_calculate_mesh_size, (_canvas, &mesh) );

    if( mesh.vertex_count == 0 )
    {
        _bounds->minimum_x = 0.f;
        _bounds->minimum_y = 0.f;
        _bounds->maximum_x = 0.f;
        _bounds->maximum_y = 0.f;

        return GP_SUCCESSFUL;
    }

    gp_canvas_t * canvas = (gp_canvas_t *)_canvas;
    const gp_size_t scratch_mark = gp_canvas_scratch_mark( canvas );
    gp_vec2f_t * positions = GP_REALLOC( canvas, GP_NULLPTR, gp_vec2f_t, mesh.vertex_count );
    if( positions == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    mesh.positions_buffer = positions;
    mesh.positions_offset = 0;
    mesh.positions_stride = sizeof( gp_vec2f_t );
    mesh.colors_buffer = GP_NULLPTR;
    mesh.uv_buffer = GP_NULLPTR;
    mesh.indices_buffer = GP_NULLPTR;

    gp_result_t render_result = gp_render( _canvas, &mesh );
    if( render_result == GP_FAILURE )
    {
        GP_FREE( canvas, positions );
        gp_canvas_scratch_reset( canvas, scratch_mark );

        return GP_FAILURE;
    }

    float minimum_x = positions[0].x;
    float minimum_y = positions[0].y;
    float maximum_x = positions[0].x;
    float maximum_y = positions[0].y;

    for( gp_uint32_t index = 1; index != mesh.vertex_count; ++index )
    {
        const gp_vec2f_t * position = positions + index;
        if( position->x < minimum_x )
        {
            minimum_x = position->x;
        }
        if( position->y < minimum_y )
        {
            minimum_y = position->y;
        }
        if( position->x > maximum_x )
        {
            maximum_x = position->x;
        }
        if( position->y > maximum_y )
        {
            maximum_y = position->y;
        }
    }

    GP_FREE( canvas, positions );
    gp_canvas_scratch_reset( canvas, scratch_mark );

    _bounds->minimum_x = minimum_x;
    _bounds->minimum_y = minimum_y;
    _bounds->maximum_x = maximum_x;
    _bounds->maximum_y = maximum_y;

    return GP_SUCCESSFUL;
}
