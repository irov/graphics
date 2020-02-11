#include "ellipse.h"

#include "detail.h"
#include "struct.h"

//////////////////////////////////////////////////////////////////////////
static gp_result_t __calculate_mesh_ellipse_size( const gp_canvas_t * _canvas, gp_uint16_t * _vertex_count, gp_uint16_t * _index_count )
{
    gp_uint16_t vertex_count = 0;
    gp_uint16_t index_count = 0;

    for( const gp_ellipse_t * e = _canvas->ellipses; e != GP_NULLPTR; e = e->next )
    {
        uint8_t ellipse_quality = e->state.ellipse_quality;

        if( e->state.penumbra > 0.f )
        {
            vertex_count += ellipse_quality * 4;
            index_count += ellipse_quality * 18;
        }
        else
        {
            vertex_count += ellipse_quality * 2;
            index_count += ellipse_quality * 6;
        }

        if( e->state.fill == GP_TRUE )
        {
            vertex_count += 1;
            vertex_count += ellipse_quality;
            index_count += ellipse_quality * 3;
        }
    }

    *_vertex_count = vertex_count;
    *_index_count = index_count;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_calculate_mesh_ellipse_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh )
{
    gp_uint16_t vertex_count;
    gp_uint16_t index_count;

    if( __calculate_mesh_ellipse_size( _canvas, &vertex_count, &index_count ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    _mesh->vertex_count += vertex_count;
    _mesh->index_count += index_count;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_render_ellipse( const gp_canvas_t * _canvas, const gp_mesh_t * _mesh, gp_uint16_t * _vertex_iterator, gp_uint16_t * _index_iterator )
{
    gp_uint16_t vertex_iterator = *_vertex_iterator;
    gp_uint16_t index_iterator = *_index_iterator;

    for( const gp_ellipse_t * e = _canvas->ellipses; e != GP_NULLPTR; e = e->next )
    {
        gp_color_t total_color;
        gp_color_mul( &total_color, &_mesh->color, &e->state.color );
        gp_uint32_t argb = gp_color_argb( &total_color );

        gp_uint8_t quality = e->state.ellipse_quality;
        float line_penumbra = e->state.penumbra;

        if( line_penumbra > 0.f )
        {
            gp_uint32_t ellipse_quality2 = quality * 4;

            for( gp_uint16_t index = 0; index != quality; ++index )
            {
                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 4 + 0) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 4 + 1) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 4 + 4) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 4 + 4) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 4 + 1) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + (index * 4 + 5) % ellipse_quality2 );

                index_iterator += 6;

                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 4 + 1) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 4 + 2) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 4 + 5) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 4 + 5) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 4 + 2) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + (index * 4 + 6) % ellipse_quality2 );

                index_iterator += 6;

                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 4 + 2) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 4 + 3) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 4 + 6) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 4 + 6) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 4 + 3) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + (index * 4 + 7) % ellipse_quality2 );

                index_iterator += 6;
            }
        }
        else
        {
            gp_uint32_t ellipse_quality2 = quality * 2;

            for( gp_uint16_t index = 0; index != quality; ++index )
            {
                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 2 + 0) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 2 + 1) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 2 + 2) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 2 + 2) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 2 + 1) % ellipse_quality2 );
                gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + (index * 2 + 3) % ellipse_quality2 );

                index_iterator += 6;
            }
        }

        float line_width = e->state.line_width;
        float half_line_width = line_width * 0.5f;

        float ellipse_quality_inv = e->state.ellipse_quality_inv;

        float dt = gp_constant_two_pi * ellipse_quality_inv;

        float t = 0.f;

        for( gp_uint32_t index = 0; index != quality; ++index, t += dt )
        {
            float ct = GP_MATH_COSF( t );
            float st = GP_MATH_SINF( t );

            float x0 = e->point.x + (e->width + half_line_width) * ct;
            float y0 = e->point.y + (e->height + half_line_width) * st;

            float x1 = e->point.x + (e->width - half_line_width) * ct;
            float y1 = e->point.y + (e->height - half_line_width) * st;

            if( line_penumbra > 0.f )
            {
                float line_width_soft = half_line_width - line_penumbra;

                float x0_soft = e->point.x + (e->width + line_width_soft) * ct;
                float y0_soft = e->point.y + (e->height + line_width_soft) * st;

                float x1_soft = e->point.x + (e->width - line_width_soft) * ct;
                float y1_soft = e->point.y + (e->height - line_width_soft) * st;

                gp_mesh_position( _mesh, vertex_iterator + 0, x0, y0 );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                gp_mesh_position( _mesh, vertex_iterator + 1, x0_soft, y0_soft );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                gp_mesh_position( _mesh, vertex_iterator + 2, x1_soft, y1_soft );
                gp_mesh_color( _mesh, vertex_iterator + 2, argb );

                gp_mesh_position( _mesh, vertex_iterator + 3, x1, y1 );
                gp_mesh_color( _mesh, vertex_iterator + 3, argb & 0x00ffffff );

                vertex_iterator += 4;
            }
            else
            {
                gp_mesh_position( _mesh, vertex_iterator + 0, x0, y0 );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                gp_mesh_position( _mesh, vertex_iterator + 1, x1, y1 );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                vertex_iterator += 2;
            }
        }

        if( e->state.fill == GP_TRUE )
        {
            for( gp_uint16_t index = 0; index != quality; ++index )
            {
                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index + 0) % quality + 1 );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index + 1) % quality + 1 );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator );

                index_iterator += 3;
            }

            gp_mesh_position( _mesh, vertex_iterator + 0, e->point.x, e->point.y );
            gp_mesh_color( _mesh, vertex_iterator + 0, argb );

            vertex_iterator += 1;

            float tf = 0.f;

            for( gp_uint32_t index = 0; index != quality; ++index, tf += dt )
            {
                float ct = GP_MATH_COSF( tf );
                float st = GP_MATH_SINF( tf );

                float x = e->point.x + e->width * ct;
                float y = e->point.y + e->height * st;

                gp_mesh_position( _mesh, vertex_iterator + 0, x, y );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                vertex_iterator += 1;
            }
        }
    }

#ifdef GP_DEBUG
    gp_uint16_t test_vertex_count;
    gp_uint16_t test_index_count;

    if( __calculate_mesh_ellipse_size( _canvas, &test_vertex_count, &test_index_count ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    if( test_vertex_count != vertex_iterator - *_vertex_iterator )
    {
        return GP_FAILURE;
    }

    if( test_index_count != index_iterator - *_index_iterator )
    {
        return GP_FAILURE;
    }
#endif

    * _vertex_iterator = vertex_iterator;
    *_index_iterator = index_iterator;

    return GP_SUCCESSFUL;
}