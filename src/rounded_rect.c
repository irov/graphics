#include "rounded_rect.h"

#include "struct.h"
#include "detail.h"

//////////////////////////////////////////////////////////////////////////
gp_result_t gp_calculate_mesh_rounded_rect_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh )
{
    gp_uint32_t vertex_count = 0;
    gp_uint16_t index_count = 0;

    for( const gp_rounded_rect_t * r = _canvas->rounded_rects; r != GP_NULLPTR; r = r->next )
    {
        if( r->line_penumbra > 0.f )
        {
            vertex_count += 32;
            index_count += 72;

            for( gp_uint32_t index = 0; index != 4; ++index )
            {
                vertex_count += (r->quality + 1) * 4;
                index_count += r->quality * 6 * 3;
            }
        }
        else
        {
            vertex_count += 16;
            index_count += 24;

            for( gp_uint32_t index = 0; index != 4; ++index )
            {
                vertex_count += (r->quality + 1) * 2;
                index_count += r->quality * 6;
            }
        }

        if( r->fill == GP_TRUE )
        {
            for( gp_uint32_t index = 0; index != 4; ++index )
            {
                vertex_count += (r->quality + 1) + 1;
                index_count += r->quality * 3;
            }

            vertex_count += 12;
            index_count += 6 * 5;
        }
    }

    _mesh->vertex_count += vertex_count;
    _mesh->index_count += index_count;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_render_rounded_rect( const gp_canvas_t * _canvas, const gp_mesh_t * _mesh, gp_uint16_t * _vertex_iterator, gp_uint16_t * _index_iterator )
{
    gp_uint16_t vertex_iterator = *_vertex_iterator;
    gp_uint16_t index_iterator = *_index_iterator;

    for( const gp_rounded_rect_t * rr = _canvas->rounded_rects; rr != GP_NULLPTR; rr = rr->next )
    {
        gp_color_t line_color;
        gp_color_mul( &line_color, &_mesh->color, &rr->line_color );
        gp_uint32_t argb = gp_color_argb( &line_color );

        if( rr->line_penumbra > 0.f )
        {
            for( gp_uint16_t index = 0; index != 4; ++index )
            {
                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 8 + 0) );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 8 + 1) );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 8 + 4) );
                gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 8 + 4) );
                gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 8 + 1) );
                gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + (index * 8 + 5) );

                index_iterator += 6;

                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 8 + 1) );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 8 + 2) );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 8 + 5) );
                gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 8 + 5) );
                gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 8 + 2) );
                gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + (index * 8 + 6) );

                index_iterator += 6;

                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 8 + 2) );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 8 + 3) );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 8 + 6) );
                gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 8 + 6) );
                gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 8 + 3) );
                gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + (index * 8 + 7) );

                index_iterator += 6;
            }
        }
        else
        {
            for( gp_uint16_t index = 0; index != 4; ++index )
            {
                gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 4 + 0) );
                gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 4 + 1) );
                gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 4 + 2) );
                gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 4 + 2) );
                gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 4 + 1) );
                gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + (index * 4 + 3) );

                index_iterator += 6;
            }
        }

        gp_vec2f_t p0;
        p0.x = rr->point.x + 0.f;
        p0.y = rr->point.y + 0.f;

        gp_vec2f_t p1;
        p1.x = rr->point.x + rr->width;
        p1.y = rr->point.y + 0.f;

        gp_vec2f_t p2;
        p2.x = rr->point.x + rr->width;
        p2.y = rr->point.y + rr->height;

        gp_vec2f_t p3;
        p3.x = rr->point.x + 0.f;
        p3.y = rr->point.y + rr->height;

        float line_width = rr->line_width;
        float line_half_width = rr->line_width * 0.5f;
        float radius = rr->radius;
        float line_penumbra = rr->line_penumbra;

        if( line_penumbra > 0.f )
        {
            float half_width_soft = (line_width - line_penumbra * 2.f) * 0.5f;

            gp_mesh_position( _mesh, vertex_iterator + 0, p0.x + radius, p0.y - line_half_width );
            gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

            gp_mesh_position( _mesh, vertex_iterator + 1, p0.x + radius, p0.y - half_width_soft );
            gp_mesh_color( _mesh, vertex_iterator + 1, argb );

            gp_mesh_position( _mesh, vertex_iterator + 2, p0.x + radius, p0.y + half_width_soft );
            gp_mesh_color( _mesh, vertex_iterator + 2, argb );

            gp_mesh_position( _mesh, vertex_iterator + 3, p0.x + radius, p0.y + line_half_width );
            gp_mesh_color( _mesh, vertex_iterator + 3, argb & 0x00ffffff );

            vertex_iterator += 4;

            gp_mesh_position( _mesh, vertex_iterator + 0, p1.x - radius, p1.y - line_half_width );
            gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

            gp_mesh_position( _mesh, vertex_iterator + 1, p1.x - radius, p1.y - half_width_soft );
            gp_mesh_color( _mesh, vertex_iterator + 1, argb );

            gp_mesh_position( _mesh, vertex_iterator + 2, p1.x - radius, p1.y + half_width_soft );
            gp_mesh_color( _mesh, vertex_iterator + 2, argb );

            gp_mesh_position( _mesh, vertex_iterator + 3, p1.x - radius, p1.y + line_half_width );
            gp_mesh_color( _mesh, vertex_iterator + 3, argb & 0x00ffffff );

            vertex_iterator += 4;

            gp_mesh_position( _mesh, vertex_iterator + 0, p1.x + line_half_width, p1.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

            gp_mesh_position( _mesh, vertex_iterator + 1, p1.x + half_width_soft, p1.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 1, argb );

            gp_mesh_position( _mesh, vertex_iterator + 2, p1.x - half_width_soft, p1.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 2, argb );

            gp_mesh_position( _mesh, vertex_iterator + 3, p1.x - line_half_width, p1.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 3, argb & 0x00ffffff );

            vertex_iterator += 4;

            gp_mesh_position( _mesh, vertex_iterator + 0, p2.x + line_half_width, p2.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

            gp_mesh_position( _mesh, vertex_iterator + 1, p2.x + half_width_soft, p2.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 1, argb );

            gp_mesh_position( _mesh, vertex_iterator + 2, p2.x - half_width_soft, p2.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 2, argb );

            gp_mesh_position( _mesh, vertex_iterator + 3, p2.x - line_half_width, p2.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 3, argb & 0x00ffffff );

            vertex_iterator += 4;

            gp_mesh_position( _mesh, vertex_iterator + 0, p2.x - radius, p2.y + line_half_width );
            gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

            gp_mesh_position( _mesh, vertex_iterator + 1, p2.x - radius, p2.y + half_width_soft );
            gp_mesh_color( _mesh, vertex_iterator + 1, argb );

            gp_mesh_position( _mesh, vertex_iterator + 2, p2.x - radius, p2.y - half_width_soft );
            gp_mesh_color( _mesh, vertex_iterator + 2, argb );

            gp_mesh_position( _mesh, vertex_iterator + 3, p2.x - radius, p2.y - line_half_width );
            gp_mesh_color( _mesh, vertex_iterator + 3, argb & 0x00ffffff );

            vertex_iterator += 4;

            gp_mesh_position( _mesh, vertex_iterator + 0, p3.x + radius, p3.y + line_half_width );
            gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

            gp_mesh_position( _mesh, vertex_iterator + 1, p3.x + radius, p3.y + half_width_soft );
            gp_mesh_color( _mesh, vertex_iterator + 1, argb );

            gp_mesh_position( _mesh, vertex_iterator + 2, p3.x + radius, p3.y - half_width_soft );
            gp_mesh_color( _mesh, vertex_iterator + 2, argb );

            gp_mesh_position( _mesh, vertex_iterator + 3, p3.x + radius, p3.y - line_half_width );
            gp_mesh_color( _mesh, vertex_iterator + 3, argb & 0x00ffffff );

            vertex_iterator += 4;

            gp_mesh_position( _mesh, vertex_iterator + 0, p3.x - line_half_width, p3.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

            gp_mesh_position( _mesh, vertex_iterator + 1, p3.x - half_width_soft, p3.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 1, argb );

            gp_mesh_position( _mesh, vertex_iterator + 2, p3.x + half_width_soft, p3.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 2, argb );

            gp_mesh_position( _mesh, vertex_iterator + 3, p3.x + line_half_width, p3.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 3, argb & 0x00ffffff );

            vertex_iterator += 4;

            gp_mesh_position( _mesh, vertex_iterator + 0, p0.x - line_half_width, p0.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

            gp_mesh_position( _mesh, vertex_iterator + 1, p0.x - half_width_soft, p0.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 1, argb );

            gp_mesh_position( _mesh, vertex_iterator + 2, p0.x + half_width_soft, p0.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 2, argb );

            gp_mesh_position( _mesh, vertex_iterator + 3, p0.x + line_half_width, p0.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 3, argb & 0x00ffffff );

            vertex_iterator += 4;
        }
        else
        {
            gp_mesh_position( _mesh, vertex_iterator + 0, p0.x + radius, p0.y - line_half_width );
            gp_mesh_color( _mesh, vertex_iterator + 0, argb );

            gp_mesh_position( _mesh, vertex_iterator + 1, p0.x + radius, p0.y + line_half_width );
            gp_mesh_color( _mesh, vertex_iterator + 1, argb );

            vertex_iterator += 2;

            gp_mesh_position( _mesh, vertex_iterator + 0, p1.x - radius, p1.y - line_half_width );
            gp_mesh_color( _mesh, vertex_iterator + 0, argb );

            gp_mesh_position( _mesh, vertex_iterator + 1, p1.x - radius, p1.y + line_half_width );
            gp_mesh_color( _mesh, vertex_iterator + 1, argb );

            vertex_iterator += 2;

            gp_mesh_position( _mesh, vertex_iterator + 0, p1.x + line_half_width, p1.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 0, argb );

            gp_mesh_position( _mesh, vertex_iterator + 1, p1.x - line_half_width, p1.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 1, argb );

            vertex_iterator += 2;

            gp_mesh_position( _mesh, vertex_iterator + 0, p2.x + line_half_width, p2.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 0, argb );

            gp_mesh_position( _mesh, vertex_iterator + 1, p2.x - line_half_width, p2.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 1, argb );

            vertex_iterator += 2;

            gp_mesh_position( _mesh, vertex_iterator + 0, p2.x - radius, p2.y - line_half_width );
            gp_mesh_color( _mesh, vertex_iterator + 0, argb );

            gp_mesh_position( _mesh, vertex_iterator + 1, p2.x - radius, p2.y + line_half_width );
            gp_mesh_color( _mesh, vertex_iterator + 1, argb );

            vertex_iterator += 2;

            gp_mesh_position( _mesh, vertex_iterator + 0, p3.x + radius, p3.y - line_half_width );
            gp_mesh_color( _mesh, vertex_iterator + 0, argb );

            gp_mesh_position( _mesh, vertex_iterator + 1, p3.x + radius, p3.y + line_half_width );
            gp_mesh_color( _mesh, vertex_iterator + 1, argb );

            vertex_iterator += 2;

            gp_mesh_position( _mesh, vertex_iterator + 0, p3.x - line_half_width, p3.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 0, argb );

            gp_mesh_position( _mesh, vertex_iterator + 1, p3.x + line_half_width, p3.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 1, argb );

            vertex_iterator += 2;

            gp_mesh_position( _mesh, vertex_iterator + 0, p0.x - line_half_width, p0.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 0, argb );

            gp_mesh_position( _mesh, vertex_iterator + 1, p0.x + line_half_width, p0.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 1, argb );

            vertex_iterator += 2;
        }

        gp_uint8_t quality = rr->quality;

        if( line_penumbra > 0.f )
        {
            for( gp_uint16_t index_arc = 0; index_arc != 4; ++index_arc )
            {
                for( gp_uint16_t index = 0; index != quality; ++index )
                {
                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index_arc * (quality + 1) * 4 + (index * 4 + 0) );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality + 1) * 4 + (index * 4 + 1) );
                    gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + index_arc * (quality + 1) * 4 + (index * 4 + 4) );
                    gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + index_arc * (quality + 1) * 4 + (index * 4 + 4) );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index_arc * (quality + 1) * 4 + (index * 4 + 1) );
                    gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + index_arc * (quality + 1) * 4 + (index * 4 + 5) );

                    index_iterator += 6;

                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index_arc * (quality + 1) * 4 + (index * 4 + 1) );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality + 1) * 4 + (index * 4 + 2) );
                    gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + index_arc * (quality + 1) * 4 + (index * 4 + 5) );
                    gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + index_arc * (quality + 1) * 4 + (index * 4 + 5) );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index_arc * (quality + 1) * 4 + (index * 4 + 2) );
                    gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + index_arc * (quality + 1) * 4 + (index * 4 + 6) );

                    index_iterator += 6;

                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index_arc * (quality + 1) * 4 + (index * 4 + 2) );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality + 1) * 4 + (index * 4 + 3) );
                    gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + index_arc * (quality + 1) * 4 + (index * 4 + 6) );
                    gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + index_arc * (quality + 1) * 4 + (index * 4 + 6) );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index_arc * (quality + 1) * 4 + (index * 4 + 3) );
                    gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + index_arc * (quality + 1) * 4 + (index * 4 + 7) );

                    index_iterator += 6;
                }
            }
        }
        else
        {
            for( gp_uint16_t index_arc = 0; index_arc != 4; ++index_arc )
            {
                for( gp_uint16_t index = 0; index != quality; ++index )
                {
                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index_arc * (quality + 1) * 2 + (index * 2 + 0) );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality + 1) * 2 + (index * 2 + 1) );
                    gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + index_arc * (quality + 1) * 2 + (index * 2 + 2) );
                    gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + index_arc * (quality + 1) * 2 + (index * 2 + 2) );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index_arc * (quality + 1) * 2 + (index * 2 + 1) );
                    gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + index_arc * (quality + 1) * 2 + (index * 2 + 3) );

                    index_iterator += 6;
                }
            }
        }

        float dt = gp_constant_half_pi * rr->quality_inv;

        float t0 = gp_constant_half_pi;

        for( gp_uint8_t index = 0; index != quality + 1; ++index, t0 += dt )
        {
            float ct = GP_MATH_COSF( t0 );
            float st = GP_MATH_SINF( t0 );

            float x0 = p0.x + radius + (radius + line_half_width) * ct;
            float y0 = p0.y + radius - (radius + line_half_width) * st;

            float x1 = p0.x + radius + (radius - line_half_width) * ct;
            float y1 = p0.y + radius - (radius - line_half_width) * st;

            if( line_penumbra > 0.f )
            {
                float line_width_soft = (line_width - line_penumbra * 2.f) * 0.5f;

                float x0_soft = p0.x + radius + (radius + line_width_soft) * ct;
                float y0_soft = p0.y + radius - (radius + line_width_soft) * st;

                float x1_soft = p0.x + radius + (radius - line_width_soft) * ct;
                float y1_soft = p0.y + radius - (radius - line_width_soft) * st;

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

        float t1 = 0.f;

        for( gp_uint8_t index = 0; index != quality + 1; ++index, t1 += dt )
        {
            float ct = GP_MATH_COSF( t1 );
            float st = GP_MATH_SINF( t1 );

            float x0 = p1.x - radius + (radius + line_half_width) * ct;
            float y0 = p1.y + radius - (radius + line_half_width) * st;

            float x1 = p1.x - radius + (radius - line_half_width) * ct;
            float y1 = p1.y + radius - (radius - line_half_width) * st;

            if( line_penumbra > 0.f )
            {
                float line_width_soft = (line_width - line_penumbra * 2.f) * 0.5f;

                float x0_soft = p1.x - radius + (radius + line_width_soft) * ct;
                float y0_soft = p1.y + radius - (radius + line_width_soft) * st;

                float x1_soft = p1.x - radius + (radius - line_width_soft) * ct;
                float y1_soft = p1.y + radius - (radius - line_width_soft) * st;

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

        float t2 = gp_constant_one_and_a_half_pi;

        for( gp_uint8_t index = 0; index != quality + 1; ++index, t2 += dt )
        {
            float ct = GP_MATH_COSF( t2 );
            float st = GP_MATH_SINF( t2 );

            float x0 = p2.x - radius + (radius + line_half_width) * ct;
            float y0 = p2.y - radius - (radius + line_half_width) * st;

            float x1 = p2.x - radius + (radius - line_half_width) * ct;
            float y1 = p2.y - radius - (radius - line_half_width) * st;

            if( line_penumbra > 0.f )
            {
                float line_width_soft = (line_width - line_penumbra * 2.f) * 0.5f;

                float x0_soft = p2.x - radius + (radius + line_width_soft) * ct;
                float y0_soft = p2.y - radius - (radius + line_width_soft) * st;

                float x1_soft = p2.x - radius + (radius - line_width_soft) * ct;
                float y1_soft = p2.y - radius - (radius - line_width_soft) * st;

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

        float t3 = gp_constant_pi;

        for( gp_uint8_t index = 0; index != quality + 1; ++index, t3 += dt )
        {
            float ct = GP_MATH_COSF( t3 );
            float st = GP_MATH_SINF( t3 );

            float x0 = p3.x + radius + (radius + line_half_width) * ct;
            float y0 = p3.y - radius - (radius + line_half_width) * st;

            float x1 = p3.x + radius + (radius - line_half_width) * ct;
            float y1 = p3.y - radius - (radius - line_half_width) * st;

            if( line_penumbra > 0.f )
            {
                float line_width_soft = (line_width - line_penumbra * 2.f) * 0.5f;

                float x0_soft = p3.x + radius + (radius + line_width_soft) * ct;
                float y0_soft = p3.y - radius - (radius + line_width_soft) * st;

                float x1_soft = p3.x + radius + (radius - line_width_soft) * ct;
                float y1_soft = p3.y - radius - (radius - line_width_soft) * st;

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

        if( rr->fill == GP_TRUE )
        {
            for( gp_uint16_t index_arc = 0; index_arc != 4; ++index_arc )
            {
                for( gp_uint16_t index = 0; index != quality; ++index )
                {
                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index_arc * (quality + 1 + 1) + (index + 0) + 1 );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality + 1 + 1) + (index + 1) + 1 );
                    gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + index_arc * (quality + 1 + 1) );

                    index_iterator += 3;
                }
            }

            gp_color_t fill_color;
            gp_color_mul( &fill_color, &_mesh->color, &rr->fill_color );
            gp_uint32_t fill_argb = gp_color_argb( &fill_color );

            gp_mesh_position( _mesh, vertex_iterator + 0, p0.x + radius, p0.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 0, fill_argb );

            vertex_iterator += 1;

            float tf0 = gp_constant_half_pi;

            for( gp_uint8_t index = 0; index != quality + 1; ++index, tf0 += dt )
            {
                float ct = GP_MATH_COSF( tf0 );
                float st = GP_MATH_SINF( tf0 );

                float x = p0.x + radius + radius * ct;
                float y = p0.y + radius - radius * st;

                gp_mesh_position( _mesh, vertex_iterator + 0, x, y );
                gp_mesh_color( _mesh, vertex_iterator + 0, fill_argb );

                vertex_iterator += 1;
            }

            gp_mesh_position( _mesh, vertex_iterator + 0, p1.x - radius, p1.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 0, fill_argb );

            vertex_iterator += 1;

            float tf1 = 0.f;

            for( gp_uint8_t index = 0; index != quality + 1; ++index, tf1 += dt )
            {
                float ct = GP_MATH_COSF( tf1 );
                float st = GP_MATH_SINF( tf1 );

                float x = p1.x - radius + radius * ct;
                float y = p1.y + radius - radius * st;

                gp_mesh_position( _mesh, vertex_iterator + 0, x, y );
                gp_mesh_color( _mesh, vertex_iterator + 0, fill_argb );

                vertex_iterator += 1;
            }

            gp_mesh_position( _mesh, vertex_iterator + 0, p2.x - radius, p2.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 0, fill_argb );

            vertex_iterator += 1;

            float tf2 = gp_constant_one_and_a_half_pi;

            for( gp_uint8_t index = 0; index != quality + 1; ++index, tf2 += dt )
            {
                float ct = GP_MATH_COSF( tf2 );
                float st = GP_MATH_SINF( tf2 );

                float x = p2.x - radius + radius * ct;
                float y = p2.y - radius - radius * st;

                gp_mesh_position( _mesh, vertex_iterator + 0, x, y );
                gp_mesh_color( _mesh, vertex_iterator + 0, fill_argb );

                vertex_iterator += 1;
            }

            gp_mesh_position( _mesh, vertex_iterator + 0, p3.x + radius, p3.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 0, fill_argb );

            vertex_iterator += 1;

            float tf3 = gp_constant_pi;

            for( gp_uint8_t index = 0; index != quality + 1; ++index, tf3 += dt )
            {
                float ct = GP_MATH_COSF( tf3 );
                float st = GP_MATH_SINF( tf3 );

                float x = p3.x + radius + radius * ct;
                float y = p3.y - radius - radius * st;

                gp_mesh_position( _mesh, vertex_iterator + 0, x, y );
                gp_mesh_color( _mesh, vertex_iterator + 0, fill_argb );

                vertex_iterator += 1;
            }

            gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + 0 );
            gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + 1 );
            gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + 3 );
            gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + 3 );
            gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + 1 );
            gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + 4 );

            index_iterator += 6;

            gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + 2 );
            gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + 3 );
            gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + 6 );
            gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + 6 );
            gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + 3 );
            gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + 7 );

            index_iterator += 6;

            gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + 3 );
            gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + 4 );
            gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + 7 );
            gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + 7 );
            gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + 4 );
            gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + 8 );

            index_iterator += 6;

            gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + 4 );
            gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + 5 );
            gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + 8 );
            gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + 8 );
            gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + 5 );
            gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + 9 );

            index_iterator += 6;

            gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + 7 );
            gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + 8 );
            gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + 10 );
            gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + 10 );
            gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + 8 );
            gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + 11 );

            index_iterator += 6;

            gp_mesh_position( _mesh, vertex_iterator + 0, p0.x + radius, p0.y );
            gp_mesh_color( _mesh, vertex_iterator + 0, fill_argb );

            gp_mesh_position( _mesh, vertex_iterator + 1, p1.x - radius, p1.y );
            gp_mesh_color( _mesh, vertex_iterator + 1, fill_argb );

            gp_mesh_position( _mesh, vertex_iterator + 2, p0.x, p0.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 2, fill_argb );

            gp_mesh_position( _mesh, vertex_iterator + 3, p0.x + radius, p0.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 3, fill_argb );

            gp_mesh_position( _mesh, vertex_iterator + 4, p1.x - radius, p1.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 4, fill_argb );

            gp_mesh_position( _mesh, vertex_iterator + 5, p1.x, p1.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 5, fill_argb );

            gp_mesh_position( _mesh, vertex_iterator + 6, p3.x, p3.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 6, fill_argb );

            gp_mesh_position( _mesh, vertex_iterator + 7, p3.x + radius, p3.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 7, fill_argb );

            gp_mesh_position( _mesh, vertex_iterator + 8, p2.x - radius, p2.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 8, fill_argb );

            gp_mesh_position( _mesh, vertex_iterator + 9, p2.x, p2.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 9, fill_argb );

            gp_mesh_position( _mesh, vertex_iterator + 10, p3.x + radius, p3.y );
            gp_mesh_color( _mesh, vertex_iterator + 10, fill_argb );

            gp_mesh_position( _mesh, vertex_iterator + 11, p2.x - radius, p2.y );
            gp_mesh_color( _mesh, vertex_iterator + 11, fill_argb );

            vertex_iterator += 12;
        }
    }

    *_vertex_iterator = vertex_iterator;
    *_index_iterator = index_iterator;

    return GP_SUCCESSFUL;
}