#include "rect.h"

#include "struct.h"
#include "detail.h"

//////////////////////////////////////////////////////////////////////////
static gp_result_t __calculate_mesh_rect_size( const gp_canvas_t * _canvas, gp_uint16_t * _vertex_count, gp_uint16_t * _index_count )
{
    gp_uint16_t vertex_count = 0;
    gp_uint16_t index_count = 0;

    for( const gp_rect_t * r = _canvas->rects; r != GP_NULLPTR; r = r->next )
    {
        if( r->state.fill == GP_FALSE )
        {
            if( r->state.penumbra > 0.f )
            {
                vertex_count += 16;
                index_count += 72;
            }
            else
            {
                vertex_count += 8;
                index_count += 24;
            }
        }
        else
        {
            if( r->state.penumbra > 0.f )
            {
                vertex_count += 12;
                index_count += 48 + 6;
            }
            else
            {
                vertex_count += 8;
                index_count += 24 + 6;
            }
        }
    }

    *_vertex_count = vertex_count;
    *_index_count = index_count;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_calculate_mesh_rect_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh )
{
    gp_uint16_t vertex_count;
    gp_uint16_t index_count;

    if( __calculate_mesh_rect_size( _canvas, &vertex_count, &index_count ) == GP_FAILURE )
    {
        return GP_FAILURE;
    }

    _mesh->vertex_count += vertex_count;
    _mesh->index_count += index_count;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_render_rect( const gp_canvas_t * _canvas, const gp_mesh_t * _mesh, gp_uint16_t * _vertex_iterator, gp_uint16_t * _index_iterator )
{
    gp_uint16_t vertex_iterator = *_vertex_iterator;
    gp_uint16_t index_iterator = *_index_iterator;

    for( const gp_rect_t * r = _canvas->rects; r != GP_NULLPTR; r = r->next )
    {
        gp_color_t line_color;
        gp_color_mul( &line_color, &_mesh->color, &r->state.color );
        gp_uint32_t argb = gp_color_argb( &line_color );

        gp_vec2f_t p0;
        p0.x = r->point.x + 0.f;
        p0.y = r->point.y + 0.f;

        gp_vec2f_t p1;
        p1.x = r->point.x + r->width;
        p1.y = r->point.y + 0.f;

        gp_vec2f_t p2;
        p2.x = r->point.x + r->width;
        p2.y = r->point.y + r->height;

        gp_vec2f_t p3;
        p3.x = r->point.x + 0.f;
        p3.y = r->point.y + r->height;

        float penumbra = r->state.penumbra;
        float half_width = r->state.line_width * 0.5f;
        float half_width_soft = half_width - penumbra;

        if( r->state.fill == GP_FALSE )
        {
            if( penumbra > 0.f )
            {
                for( gp_uint16_t index = 0; index != 4; ++index )
                {
                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 4 + 0) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 4 + 1) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 4 + 4) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 4 + 4) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 4 + 1) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + (index * 4 + 5) % 16 );

                    index_iterator += 6;

                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 4 + 1) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 4 + 2) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 4 + 5) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 4 + 5) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 4 + 2) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + (index * 4 + 6) % 16 );

                    index_iterator += 6;

                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 4 + 2) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 4 + 3) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 4 + 6) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 4 + 6) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 4 + 3) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + (index * 4 + 7) % 16 );

                    index_iterator += 6;
                }

                gp_mesh_position( _mesh, vertex_iterator + 0, p0.x - half_width, p0.y - half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                gp_mesh_position( _mesh, vertex_iterator + 1, p0.x - half_width_soft, p0.y - half_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                gp_mesh_position( _mesh, vertex_iterator + 2, p0.x + half_width_soft, p0.y + half_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 2, argb );

                gp_mesh_position( _mesh, vertex_iterator + 3, p0.x + half_width, p0.y + half_width );
                gp_mesh_color( _mesh, vertex_iterator + 3, argb & 0x00ffffff );

                vertex_iterator += 4;

                gp_mesh_position( _mesh, vertex_iterator + 0, p1.x + half_width, p1.y - half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                gp_mesh_position( _mesh, vertex_iterator + 1, p1.x + half_width_soft, p1.y - half_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                gp_mesh_position( _mesh, vertex_iterator + 2, p1.x - half_width_soft, p1.y + half_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 2, argb );

                gp_mesh_position( _mesh, vertex_iterator + 3, p1.x - half_width, p1.y + half_width );
                gp_mesh_color( _mesh, vertex_iterator + 3, argb & 0x00ffffff );

                vertex_iterator += 4;

                gp_mesh_position( _mesh, vertex_iterator + 0, p2.x + half_width, p2.y + half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                gp_mesh_position( _mesh, vertex_iterator + 1, p2.x + half_width_soft, p2.y + half_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                gp_mesh_position( _mesh, vertex_iterator + 2, p2.x - half_width_soft, p2.y - half_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 2, argb );

                gp_mesh_position( _mesh, vertex_iterator + 3, p2.x - half_width, p2.y - half_width );
                gp_mesh_color( _mesh, vertex_iterator + 3, argb & 0x00ffffff );

                vertex_iterator += 4;

                gp_mesh_position( _mesh, vertex_iterator + 0, p3.x - half_width, p3.y + half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                gp_mesh_position( _mesh, vertex_iterator + 1, p3.x - half_width_soft, p3.y + half_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                gp_mesh_position( _mesh, vertex_iterator + 2, p3.x + half_width_soft, p3.y - half_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 2, argb );

                gp_mesh_position( _mesh, vertex_iterator + 3, p3.x + half_width, p3.y - half_width );
                gp_mesh_color( _mesh, vertex_iterator + 3, argb & 0x00ffffff );

                vertex_iterator += 4;
            }
            else
            {
                for( gp_uint16_t index = 0; index != 4; ++index )
                {
                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 2 + 0) % 8 );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 2 + 1) % 8 );
                    gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 2 + 2) % 8 );
                    gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 2 + 2) % 8 );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 2 + 1) % 8 );
                    gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + (index * 2 + 3) % 8 );

                    index_iterator += 6;
                }

                gp_mesh_position( _mesh, vertex_iterator + 0, p0.x - half_width, p0.y - half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                gp_mesh_position( _mesh, vertex_iterator + 1, p0.x + half_width, p0.y + half_width );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                vertex_iterator += 2;

                gp_mesh_position( _mesh, vertex_iterator + 0, p1.x + half_width, p1.y - half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                gp_mesh_position( _mesh, vertex_iterator + 1, p1.x - half_width, p1.y + half_width );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                vertex_iterator += 2;

                gp_mesh_position( _mesh, vertex_iterator + 0, p2.x + half_width, p2.y + half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                gp_mesh_position( _mesh, vertex_iterator + 1, p2.x - half_width, p2.y - half_width );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                vertex_iterator += 2;

                gp_mesh_position( _mesh, vertex_iterator + 0, p3.x - half_width, p3.y + half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                gp_mesh_position( _mesh, vertex_iterator + 1, p3.x + half_width, p3.y - half_width );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                vertex_iterator += 2;
            }
        }
        else
        {
            if( penumbra > 0.f )
            {
                for( gp_uint16_t index = 0; index != 4; ++index )
                {
                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 3 + 0) % 12 );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 3 + 1) % 12 );
                    gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 3 + 3) % 12 );
                    gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 3 + 3) % 12 );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 3 + 1) % 12 );
                    gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + (index * 3 + 4) % 12 );

                    index_iterator += 6;

                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 3 + 1) % 12 );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 3 + 2) % 12 );
                    gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 3 + 4) % 12 );
                    gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 3 + 4) % 12 );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 3 + 2) % 12 );
                    gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + (index * 3 + 5) % 12 );

                    index_iterator += 6;
                }

                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + 2 );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + 5 );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + 11 );
                gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + 11 );
                gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + 5 );
                gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + 8 );

                index_iterator += 6;

                gp_mesh_position( _mesh, vertex_iterator + 0, p0.x - half_width, p0.y - half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                gp_mesh_position( _mesh, vertex_iterator + 1, p0.x - half_width_soft, p0.y - half_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                gp_mesh_position( _mesh, vertex_iterator + 2, p0.x, p0.y );
                gp_mesh_color( _mesh, vertex_iterator + 2, argb );

                vertex_iterator += 3;

                gp_mesh_position( _mesh, vertex_iterator + 0, p1.x + half_width, p1.y - half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                gp_mesh_position( _mesh, vertex_iterator + 1, p1.x + half_width_soft, p1.y - half_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                gp_mesh_position( _mesh, vertex_iterator + 2, p1.x, p1.y );
                gp_mesh_color( _mesh, vertex_iterator + 2, argb );

                vertex_iterator += 3;

                gp_mesh_position( _mesh, vertex_iterator + 0, p2.x + half_width, p2.y + half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                gp_mesh_position( _mesh, vertex_iterator + 1, p2.x + half_width_soft, p2.y + half_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                gp_mesh_position( _mesh, vertex_iterator + 2, p2.x, p2.y );
                gp_mesh_color( _mesh, vertex_iterator + 2, argb );

                vertex_iterator += 3;

                gp_mesh_position( _mesh, vertex_iterator + 0, p3.x - half_width, p3.y + half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                gp_mesh_position( _mesh, vertex_iterator + 1, p3.x - half_width_soft, p3.y + half_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                gp_mesh_position( _mesh, vertex_iterator + 2, p3.x, p3.y );
                gp_mesh_color( _mesh, vertex_iterator + 2, argb );

                vertex_iterator += 3;
            }
            else
            {
                for( gp_uint16_t index = 0; index != 4; ++index )
                {
                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 2 + 0) % 8 );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 2 + 1) % 8 );
                    gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 2 + 2) % 8 );
                    gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 2 + 2) % 8 );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 2 + 1) % 8 );
                    gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + (index * 2 + 3) % 8 );

                    index_iterator += 6;
                }

                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + 1 );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + 3 );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + 7 );
                gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + 7 );
                gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + 3 );
                gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + 5 );

                index_iterator += 6;

                gp_mesh_position( _mesh, vertex_iterator + 0, p0.x - half_width, p0.y - half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                gp_mesh_position( _mesh, vertex_iterator + 1, p0.x, p0.y );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                vertex_iterator += 2;

                gp_mesh_position( _mesh, vertex_iterator + 0, p1.x + half_width, p1.y - half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                gp_mesh_position( _mesh, vertex_iterator + 1, p1.x, p1.y );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                vertex_iterator += 2;

                gp_mesh_position( _mesh, vertex_iterator + 0, p2.x + half_width, p2.y + half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                gp_mesh_position( _mesh, vertex_iterator + 1, p2.x, p2.y );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                vertex_iterator += 2;

                gp_mesh_position( _mesh, vertex_iterator + 0, p3.x - half_width, p3.y + half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                gp_mesh_position( _mesh, vertex_iterator + 1, p3.x, p3.y );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                vertex_iterator += 2;
            }
        }
    }

#ifdef GP_DEBUG
    gp_uint16_t test_vertex_count;
    gp_uint16_t test_index_count;

    if( __calculate_mesh_rect_size( _canvas, &test_vertex_count, &test_index_count ) == GP_FAILURE )
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