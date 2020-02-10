#include "rounded_rect.h"

#include "struct.h"
#include "detail.h"

//////////////////////////////////////////////////////////////////////////
static gp_result_t __calculate_mesh_rounded_rect_size( const gp_canvas_t * _canvas, gp_uint16_t * _vertex_count, gp_uint16_t * _index_count )
{
    gp_uint16_t vertex_count = 0;
    gp_uint16_t index_count = 0;

    for( const gp_rounded_rect_t * r = _canvas->rounded_rects; r != GP_NULLPTR; r = r->next )
    {
        if( r->fill == GP_FALSE )
        {
            if( r->line_penumbra > 0.f )
            {
                vertex_count += 32;
                index_count += 72;

                for( gp_uint32_t index = 0; index != 4; ++index )
                {
                    vertex_count += (r->quality - 1) * 4;
                    index_count += r->quality * 6 * 3;
                }
            }
            else
            {
                vertex_count += 16;
                index_count += 24;

                for( gp_uint32_t index = 0; index != 4; ++index )
                {
                    vertex_count += (r->quality - 1) * 2;
                    index_count += r->quality * 6;
                }
            }
        }
        else
        {
            if( r->line_penumbra > 0.f )
            {
                vertex_count += 16 + 4;
                index_count += 6 + 48;

                for( gp_uint32_t index = 0; index != 4; ++index )
                {
                    vertex_count += (r->quality - 1) * 2;
                    index_count += r->quality * 6;
                    index_count += r->quality * 3;
                }
            }
            else
            {
                vertex_count += 12;
                index_count += 6 + 24;

                for( gp_uint32_t index = 0; index != 4; ++index )
                {
                    vertex_count += (r->quality - 1);
                    index_count += r->quality * 3;
                }
            }
        }
    }

    *_vertex_count = vertex_count;
    *_index_count = index_count;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_calculate_mesh_rounded_rect_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh )
{
    gp_uint16_t vertex_count;
    gp_uint16_t index_count;

    if( __calculate_mesh_rounded_rect_size( _canvas, &vertex_count, &index_count ) == GP_FAILURE )
    {
        return GP_FAILURE;
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
        gp_color_mul( &line_color, &_mesh->color, &rr->color );
        gp_uint32_t argb = gp_color_argb( &line_color );

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
        float line_half_width = line_width * 0.5f;
        float radius = rr->radius;
        float line_penumbra = rr->line_penumbra;

        gp_uint8_t quality = rr->quality;

        float dt = gp_constant_half_pi * rr->quality_inv;

        gp_uint16_t base_vertex_iterator = vertex_iterator;

        if( rr->fill == GP_FALSE )
        {
            if( line_penumbra > 0.f )
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

                float half_width_soft = line_half_width - line_penumbra;

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

                for( gp_uint16_t index_arc = 0; index_arc != 4; ++index_arc )
                {
                    gp_mesh_index( _mesh, index_iterator + 0, base_vertex_iterator + (0 + 8 * index_arc) % 32 );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality - 1) * 4 + 0 );
                    gp_mesh_index( _mesh, index_iterator + 2, base_vertex_iterator + (1 + 8 * index_arc) % 32 );
                    gp_mesh_index( _mesh, index_iterator + 3, base_vertex_iterator + (1 + 8 * index_arc) % 32 );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index_arc * (quality - 1) * 4 + 0 );
                    gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + index_arc * (quality - 1) * 4 + 1 );

                    index_iterator += 6;

                    gp_mesh_index( _mesh, index_iterator + 0, base_vertex_iterator + (1 + 8 * index_arc) % 32 );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality - 1) * 4 + 1 );
                    gp_mesh_index( _mesh, index_iterator + 2, base_vertex_iterator + (2 + 8 * index_arc) % 32 );
                    gp_mesh_index( _mesh, index_iterator + 3, base_vertex_iterator + (2 + 8 * index_arc) % 32 );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index_arc * (quality - 1) * 4 + 1 );
                    gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + index_arc * (quality - 1) * 4 + 2 );

                    index_iterator += 6;

                    gp_mesh_index( _mesh, index_iterator + 0, base_vertex_iterator + (2 + 8 * index_arc) % 32 );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality - 1) * 4 + 2 );
                    gp_mesh_index( _mesh, index_iterator + 2, base_vertex_iterator + (3 + 8 * index_arc) % 32 );
                    gp_mesh_index( _mesh, index_iterator + 3, base_vertex_iterator + (3 + 8 * index_arc) % 32 );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index_arc * (quality - 1) * 4 + 2 );
                    gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + index_arc * (quality - 1) * 4 + 3 );

                    index_iterator += 6;

                    for( gp_uint16_t index = 0; index != quality - 2; ++index )
                    {
                        gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index_arc * (quality - 1) * 4 + (index * 4 + 0) );
                        gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality - 1) * 4 + (index * 4 + 1) );
                        gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + index_arc * (quality - 1) * 4 + (index * 4 + 4) );
                        gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + index_arc * (quality - 1) * 4 + (index * 4 + 4) );
                        gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index_arc * (quality - 1) * 4 + (index * 4 + 1) );
                        gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + index_arc * (quality - 1) * 4 + (index * 4 + 5) );

                        index_iterator += 6;

                        gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index_arc * (quality - 1) * 4 + (index * 4 + 1) );
                        gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality - 1) * 4 + (index * 4 + 2) );
                        gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + index_arc * (quality - 1) * 4 + (index * 4 + 5) );
                        gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + index_arc * (quality - 1) * 4 + (index * 4 + 5) );
                        gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index_arc * (quality - 1) * 4 + (index * 4 + 2) );
                        gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + index_arc * (quality - 1) * 4 + (index * 4 + 6) );

                        index_iterator += 6;

                        gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index_arc * (quality - 1) * 4 + (index * 4 + 2) );
                        gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality - 1) * 4 + (index * 4 + 3) );
                        gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + index_arc * (quality - 1) * 4 + (index * 4 + 6) );
                        gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + index_arc * (quality - 1) * 4 + (index * 4 + 6) );
                        gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index_arc * (quality - 1) * 4 + (index * 4 + 3) );
                        gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + index_arc * (quality - 1) * 4 + (index * 4 + 7) );

                        index_iterator += 6;
                    }

                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index_arc * (quality - 1) * 4 + (quality - 2) * 4 + 0 );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality - 1) * 4 + (quality - 2) * 4 + 1 );
                    gp_mesh_index( _mesh, index_iterator + 2, base_vertex_iterator + (28 + 8 * index_arc) % 32 );
                    gp_mesh_index( _mesh, index_iterator + 3, base_vertex_iterator + (28 + 8 * index_arc) % 32 );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index_arc * (quality - 1) * 4 + (quality - 2) * 4 + 1 );
                    gp_mesh_index( _mesh, index_iterator + 5, base_vertex_iterator + (29 + 8 * index_arc) % 32 );

                    index_iterator += 6;

                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index_arc * (quality - 1) * 4 + (quality - 2) * 4 + 1 );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality - 1) * 4 + (quality - 2) * 4 + 2 );
                    gp_mesh_index( _mesh, index_iterator + 2, base_vertex_iterator + (29 + 8 * index_arc) % 32 );
                    gp_mesh_index( _mesh, index_iterator + 3, base_vertex_iterator + (29 + 8 * index_arc) % 32 );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index_arc * (quality - 1) * 4 + (quality - 2) * 4 + 2 );
                    gp_mesh_index( _mesh, index_iterator + 5, base_vertex_iterator + (30 + 8 * index_arc) % 32 );

                    index_iterator += 6;

                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index_arc * (quality - 1) * 4 + (quality - 2) * 4 + 2 );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality - 1) * 4 + (quality - 2) * 4 + 3 );
                    gp_mesh_index( _mesh, index_iterator + 2, base_vertex_iterator + (30 + 8 * index_arc) % 32 );
                    gp_mesh_index( _mesh, index_iterator + 3, base_vertex_iterator + (30 + 8 * index_arc) % 32 );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index_arc * (quality - 1) * 4 + (quality - 2) * 4 + 3 );
                    gp_mesh_index( _mesh, index_iterator + 5, base_vertex_iterator + (31 + 8 * index_arc) % 32 );

                    index_iterator += 6;
                }

                for( gp_uint8_t index = 1; index != quality; ++index )
                {
                    float t = gp_constant_half_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p0.x + radius + (radius + line_half_width) * ct;
                    float y0 = p0.y + radius - (radius + line_half_width) * st;

                    float x1 = p0.x + radius + (radius - line_half_width) * ct;
                    float y1 = p0.y + radius - (radius - line_half_width) * st;

                    float line_width_soft = line_half_width - line_penumbra;

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

                for( gp_uint8_t index = 1; index != quality; ++index )
                {
                    float t = 0.f + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p1.x - radius + (radius + line_half_width) * ct;
                    float y0 = p1.y + radius - (radius + line_half_width) * st;

                    float x1 = p1.x - radius + (radius - line_half_width) * ct;
                    float y1 = p1.y + radius - (radius - line_half_width) * st;

                    float line_width_soft = line_half_width - line_penumbra;

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

                for( gp_uint8_t index = 1; index != quality; ++index )
                {
                    float t = gp_constant_one_and_a_half_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p2.x - radius + (radius + line_half_width) * ct;
                    float y0 = p2.y - radius - (radius + line_half_width) * st;

                    float x1 = p2.x - radius + (radius - line_half_width) * ct;
                    float y1 = p2.y - radius - (radius - line_half_width) * st;

                    float line_width_soft = line_half_width - line_penumbra;

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

                for( gp_uint8_t index = 1; index != quality; ++index )
                {
                    float t = gp_constant_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p3.x + radius + (radius + line_half_width) * ct;
                    float y0 = p3.y - radius - (radius + line_half_width) * st;

                    float x1 = p3.x + radius + (radius - line_half_width) * ct;
                    float y1 = p3.y - radius - (radius - line_half_width) * st;

                    float line_width_soft = line_half_width - line_penumbra;

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

                for( gp_uint16_t index_arc = 0; index_arc != 4; ++index_arc )
                {
                    gp_mesh_index( _mesh, index_iterator + 0, base_vertex_iterator + (0 + 4 * index_arc) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality - 1) * 2 + 0 );
                    gp_mesh_index( _mesh, index_iterator + 2, base_vertex_iterator + (1 + 4 * index_arc) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 3, base_vertex_iterator + (1 + 4 * index_arc) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index_arc * (quality - 1) * 2 + 0 );
                    gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + index_arc * (quality - 1) * 2 + 1 );

                    index_iterator += 6;

                    for( gp_uint16_t index = 0; index != quality - 2; ++index )
                    {
                        gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index_arc * (quality - 1) * 2 + (index * 2 + 0) );
                        gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality - 1) * 2 + (index * 2 + 1) );
                        gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + index_arc * (quality - 1) * 2 + (index * 2 + 2) );
                        gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + index_arc * (quality - 1) * 2 + (index * 2 + 2) );
                        gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index_arc * (quality - 1) * 2 + (index * 2 + 1) );
                        gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + index_arc * (quality - 1) * 2 + (index * 2 + 3) );

                        index_iterator += 6;
                    }

                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index_arc * (quality - 1) * 2 + (quality - 2) * 2 + 0 );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality - 1) * 2 + (quality - 2) * 2 + 1 );
                    gp_mesh_index( _mesh, index_iterator + 2, base_vertex_iterator + (14 + 4 * index_arc) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 3, base_vertex_iterator + (14 + 4 * index_arc) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index_arc * (quality - 1) * 2 + (quality - 2) * 2 + 1 );
                    gp_mesh_index( _mesh, index_iterator + 5, base_vertex_iterator + (15 + 4 * index_arc) % 16 );

                    index_iterator += 6;
                }

                for( gp_uint8_t index = 1; index != quality; ++index )
                {
                    float t = gp_constant_half_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p0.x + radius + (radius + line_half_width) * ct;
                    float y0 = p0.y + radius - (radius + line_half_width) * st;

                    float x1 = p0.x + radius + (radius - line_half_width) * ct;
                    float y1 = p0.y + radius - (radius - line_half_width) * st;

                    gp_mesh_position( _mesh, vertex_iterator + 0, x0, y0 );
                    gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                    gp_mesh_position( _mesh, vertex_iterator + 1, x1, y1 );
                    gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                    vertex_iterator += 2;
                }

                for( gp_uint8_t index = 1; index != quality; ++index )
                {
                    float t = 0.f + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p1.x - radius + (radius + line_half_width) * ct;
                    float y0 = p1.y + radius - (radius + line_half_width) * st;

                    float x1 = p1.x - radius + (radius - line_half_width) * ct;
                    float y1 = p1.y + radius - (radius - line_half_width) * st;

                    gp_mesh_position( _mesh, vertex_iterator + 0, x0, y0 );
                    gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                    gp_mesh_position( _mesh, vertex_iterator + 1, x1, y1 );
                    gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                    vertex_iterator += 2;
                }

                for( gp_uint8_t index = 1; index != quality; ++index )
                {
                    float t = gp_constant_one_and_a_half_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p2.x - radius + (radius + line_half_width) * ct;
                    float y0 = p2.y - radius - (radius + line_half_width) * st;

                    float x1 = p2.x - radius + (radius - line_half_width) * ct;
                    float y1 = p2.y - radius - (radius - line_half_width) * st;

                    gp_mesh_position( _mesh, vertex_iterator + 0, x0, y0 );
                    gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                    gp_mesh_position( _mesh, vertex_iterator + 1, x1, y1 );
                    gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                    vertex_iterator += 2;
                }

                for( gp_uint8_t index = 1; index != quality; ++index )
                {
                    float t = gp_constant_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p3.x + radius + (radius + line_half_width) * ct;
                    float y0 = p3.y - radius - (radius + line_half_width) * st;

                    float x1 = p3.x + radius + (radius - line_half_width) * ct;
                    float y1 = p3.y - radius - (radius - line_half_width) * st;

                    gp_mesh_position( _mesh, vertex_iterator + 0, x0, y0 );
                    gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                    gp_mesh_position( _mesh, vertex_iterator + 1, x1, y1 );
                    gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                    vertex_iterator += 2;
                }
            }
        }
        else
        {
            gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + 0 );
            gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + 1 );
            gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + 3 );
            gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + 3 );
            gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + 1 );
            gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + 2 );

            index_iterator += 6;

            gp_mesh_position( _mesh, vertex_iterator + 0, p0.x + radius, p0.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 0, argb );

            gp_mesh_position( _mesh, vertex_iterator + 1, p1.x - radius, p1.y + radius );
            gp_mesh_color( _mesh, vertex_iterator + 1, argb );

            gp_mesh_position( _mesh, vertex_iterator + 2, p2.x - radius, p2.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 2, argb );

            gp_mesh_position( _mesh, vertex_iterator + 3, p3.x + radius, p3.y - radius );
            gp_mesh_color( _mesh, vertex_iterator + 3, argb );

            vertex_iterator += 4;

            if( line_penumbra > 0.f )
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

                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 4 + 1) );
                    gp_mesh_index( _mesh, index_iterator + 1, base_vertex_iterator + (index + 0) % 4 );
                    gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + (index * 4 + 3) );
                    gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + (index * 4 + 3) );
                    gp_mesh_index( _mesh, index_iterator + 4, base_vertex_iterator + (index + 0) % 4 );
                    gp_mesh_index( _mesh, index_iterator + 5, base_vertex_iterator + (index + 1) % 4 );

                    index_iterator += 6;
                }

                float half_width_soft = line_half_width - line_penumbra;

                gp_mesh_position( _mesh, vertex_iterator + 0, p0.x + radius, p0.y - line_half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                gp_mesh_position( _mesh, vertex_iterator + 1, p0.x + radius, p0.y - half_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                vertex_iterator += 2;

                gp_mesh_position( _mesh, vertex_iterator + 0, p1.x - radius, p1.y - line_half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                gp_mesh_position( _mesh, vertex_iterator + 1, p1.x - radius, p1.y - half_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                vertex_iterator += 2;

                gp_mesh_position( _mesh, vertex_iterator + 0, p1.x + line_half_width, p1.y + radius );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                gp_mesh_position( _mesh, vertex_iterator + 1, p1.x + half_width_soft, p1.y + radius );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                vertex_iterator += 2;

                gp_mesh_position( _mesh, vertex_iterator + 0, p2.x + line_half_width, p2.y - radius );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                gp_mesh_position( _mesh, vertex_iterator + 1, p2.x + half_width_soft, p2.y - radius );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                vertex_iterator += 2;

                gp_mesh_position( _mesh, vertex_iterator + 0, p2.x - radius, p2.y + line_half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                gp_mesh_position( _mesh, vertex_iterator + 1, p2.x - radius, p2.y + half_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                vertex_iterator += 2;

                gp_mesh_position( _mesh, vertex_iterator + 0, p3.x + radius, p3.y + line_half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                gp_mesh_position( _mesh, vertex_iterator + 1, p3.x + radius, p3.y + half_width_soft );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                vertex_iterator += 2;

                gp_mesh_position( _mesh, vertex_iterator + 0, p3.x - line_half_width, p3.y - radius );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                gp_mesh_position( _mesh, vertex_iterator + 1, p3.x - half_width_soft, p3.y - radius );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                vertex_iterator += 2;

                gp_mesh_position( _mesh, vertex_iterator + 0, p0.x - line_half_width, p0.y + radius );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                gp_mesh_position( _mesh, vertex_iterator + 1, p0.x - half_width_soft, p0.y + radius );
                gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                vertex_iterator += 2;

                for( gp_uint16_t index_arc = 0; index_arc != 4; ++index_arc )
                {
                    gp_mesh_index( _mesh, index_iterator + 0, base_vertex_iterator + 4 + (0 + 4 * index_arc) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality - 1) * 2 + 0 );
                    gp_mesh_index( _mesh, index_iterator + 2, base_vertex_iterator + 4 + (1 + 4 * index_arc) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 3, base_vertex_iterator + 4 + (1 + 4 * index_arc) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index_arc * (quality - 1) * 2 + 0 );
                    gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + index_arc * (quality - 1) * 2 + 1 );

                    index_iterator += 6;

                    gp_mesh_index( _mesh, index_iterator + 0, base_vertex_iterator + 4 + (1 + 4 * index_arc) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 1, base_vertex_iterator + index_arc );
                    gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + index_arc * (quality - 1) * 2 + 1 );

                    index_iterator += 3;

                    for( gp_uint16_t index = 0; index != quality - 2; ++index )
                    {
                        gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index_arc * (quality - 1) * 2 + (index * 2 + 0) );
                        gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality - 1) * 2 + (index * 2 + 1) );
                        gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + index_arc * (quality - 1) * 2 + (index * 2 + 2) );
                        gp_mesh_index( _mesh, index_iterator + 3, vertex_iterator + index_arc * (quality - 1) * 2 + (index * 2 + 2) );
                        gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index_arc * (quality - 1) * 2 + (index * 2 + 1) );
                        gp_mesh_index( _mesh, index_iterator + 5, vertex_iterator + index_arc * (quality - 1) * 2 + (index * 2 + 3) );

                        index_iterator += 6;

                        gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index_arc * (quality - 1) * 2 + (index * 2 + 1) );
                        gp_mesh_index( _mesh, index_iterator + 1, base_vertex_iterator + index_arc );
                        gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + index_arc * (quality - 1) * 2 + (index * 2 + 3) );

                        index_iterator += 3;
                    }

                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index_arc * (quality - 1) * 2 + (quality - 2) * 2 + 0 );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + index_arc * (quality - 1) * 2 + (quality - 2) * 2 + 1 );
                    gp_mesh_index( _mesh, index_iterator + 2, base_vertex_iterator + 4 + (14 + 4 * index_arc) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 3, base_vertex_iterator + 4 + (14 + 4 * index_arc) % 16 );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + index_arc * (quality - 1) * 2 + (quality - 2) * 2 + 1 );
                    gp_mesh_index( _mesh, index_iterator + 5, base_vertex_iterator + 4 + (15 + 4 * index_arc) % 16 );

                    index_iterator += 6;

                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index_arc * (quality - 1) * 2 + (quality - 2) * 2 + 1 );
                    gp_mesh_index( _mesh, index_iterator + 1, base_vertex_iterator + index_arc );
                    gp_mesh_index( _mesh, index_iterator + 2, base_vertex_iterator + 4 + (15 + 4 * index_arc) % 16 );

                    index_iterator += 3;
                }

                for( gp_uint8_t index = 1; index != quality; ++index )
                {
                    float t = gp_constant_half_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p0.x + radius + (radius + line_half_width) * ct;
                    float y0 = p0.y + radius - (radius + line_half_width) * st;

                    float line_width_soft = line_half_width - line_penumbra;

                    float x0_soft = p0.x + radius + (radius + line_width_soft) * ct;
                    float y0_soft = p0.y + radius - (radius + line_width_soft) * st;

                    gp_mesh_position( _mesh, vertex_iterator + 0, x0, y0 );
                    gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                    gp_mesh_position( _mesh, vertex_iterator + 1, x0_soft, y0_soft );
                    gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                    vertex_iterator += 2;
                }

                for( gp_uint8_t index = 1; index != quality; ++index )
                {
                    float t = 0.f + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p1.x - radius + (radius + line_half_width) * ct;
                    float y0 = p1.y + radius - (radius + line_half_width) * st;

                    float line_width_soft = line_half_width - line_penumbra;

                    float x0_soft = p1.x - radius + (radius + line_width_soft) * ct;
                    float y0_soft = p1.y + radius - (radius + line_width_soft) * st;

                    gp_mesh_position( _mesh, vertex_iterator + 0, x0, y0 );
                    gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                    gp_mesh_position( _mesh, vertex_iterator + 1, x0_soft, y0_soft );
                    gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                    vertex_iterator += 2;
                }

                for( gp_uint8_t index = 1; index != quality; ++index )
                {
                    float t = gp_constant_one_and_a_half_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p2.x - radius + (radius + line_half_width) * ct;
                    float y0 = p2.y - radius - (radius + line_half_width) * st;

                    float line_width_soft = line_half_width - line_penumbra;

                    float x0_soft = p2.x - radius + (radius + line_width_soft) * ct;
                    float y0_soft = p2.y - radius - (radius + line_width_soft) * st;

                    gp_mesh_position( _mesh, vertex_iterator + 0, x0, y0 );
                    gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                    gp_mesh_position( _mesh, vertex_iterator + 1, x0_soft, y0_soft );
                    gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                    vertex_iterator += 2;
                }

                for( gp_uint8_t index = 1; index != quality; ++index )
                {
                    float t = gp_constant_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p3.x + radius + (radius + line_half_width) * ct;
                    float y0 = p3.y - radius - (radius + line_half_width) * st;

                    float line_width_soft = line_half_width - line_penumbra;

                    float x0_soft = p3.x + radius + (radius + line_width_soft) * ct;
                    float y0_soft = p3.y - radius - (radius + line_width_soft) * st;

                    gp_mesh_position( _mesh, vertex_iterator + 0, x0, y0 );
                    gp_mesh_color( _mesh, vertex_iterator + 0, argb & 0x00ffffff );

                    gp_mesh_position( _mesh, vertex_iterator + 1, x0_soft, y0_soft );
                    gp_mesh_color( _mesh, vertex_iterator + 1, argb );

                    vertex_iterator += 2;
                }
            }
            else
            {
                for( gp_uint16_t index = 0; index != 4; ++index )
                {
                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + (index * 2 + 0) % 8 );
                    gp_mesh_index( _mesh, index_iterator + 1, vertex_iterator + (index * 2 + 1) % 8 );
                    gp_mesh_index( _mesh, index_iterator + 2, base_vertex_iterator + (index + 0) % 4 );
                    gp_mesh_index( _mesh, index_iterator + 3, base_vertex_iterator + (index + 0) % 4 );
                    gp_mesh_index( _mesh, index_iterator + 4, vertex_iterator + (index * 2 + 1) % 8 );
                    gp_mesh_index( _mesh, index_iterator + 5, base_vertex_iterator + (index + 1) % 4 );

                    index_iterator += 6;
                }

                gp_mesh_position( _mesh, vertex_iterator + 0, p0.x + radius, p0.y - line_half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                vertex_iterator += 1;

                gp_mesh_position( _mesh, vertex_iterator + 0, p1.x - radius, p1.y - line_half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                vertex_iterator += 1;

                gp_mesh_position( _mesh, vertex_iterator + 0, p1.x + line_half_width, p1.y + radius );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                vertex_iterator += 1;

                gp_mesh_position( _mesh, vertex_iterator + 0, p2.x + line_half_width, p2.y - radius );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                vertex_iterator += 1;

                gp_mesh_position( _mesh, vertex_iterator + 0, p2.x - radius, p2.y + line_half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                vertex_iterator += 1;

                gp_mesh_position( _mesh, vertex_iterator + 0, p3.x + radius, p3.y + line_half_width );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                vertex_iterator += 1;

                gp_mesh_position( _mesh, vertex_iterator + 0, p3.x - line_half_width, p3.y - radius );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                vertex_iterator += 1;

                gp_mesh_position( _mesh, vertex_iterator + 0, p0.x - line_half_width, p0.y + radius );
                gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                vertex_iterator += 1;

                for( gp_uint16_t index_arc = 0; index_arc != 4; ++index_arc )
                {
                    gp_mesh_index( _mesh, index_iterator + 0, base_vertex_iterator + 4 + (2 * index_arc + 0) % 8 );
                    gp_mesh_index( _mesh, index_iterator + 1, base_vertex_iterator + index_arc );
                    gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + index_arc * (quality - 1) );

                    index_iterator += 3;

                    for( gp_uint16_t index = 0; index != quality - 2; ++index )
                    {
                        gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index_arc * (quality - 1) + (index + 0) );
                        gp_mesh_index( _mesh, index_iterator + 1, base_vertex_iterator + index_arc );
                        gp_mesh_index( _mesh, index_iterator + 2, vertex_iterator + index_arc * (quality - 1) + (index + 1) );

                        index_iterator += 3;
                    }

                    gp_mesh_index( _mesh, index_iterator + 0, vertex_iterator + index_arc * (quality - 1) + (quality - 1) - 1 );
                    gp_mesh_index( _mesh, index_iterator + 1, base_vertex_iterator + index_arc );
                    gp_mesh_index( _mesh, index_iterator + 2, base_vertex_iterator + 4 + (2 * index_arc + 7) % 8 );

                    index_iterator += 3;
                }

                for( gp_uint8_t index = 1; index != quality; ++index )
                {
                    float t = gp_constant_half_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p0.x + radius + (radius + line_half_width) * ct;
                    float y0 = p0.y + radius - (radius + line_half_width) * st;

                    gp_mesh_position( _mesh, vertex_iterator + 0, x0, y0 );
                    gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                    vertex_iterator += 1;
                }

                for( gp_uint8_t index = 1; index != quality; ++index )
                {
                    float t = 0.f + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p1.x - radius + (radius + line_half_width) * ct;
                    float y0 = p1.y + radius - (radius + line_half_width) * st;

                    gp_mesh_position( _mesh, vertex_iterator + 0, x0, y0 );
                    gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                    vertex_iterator += 1;
                }

                for( gp_uint8_t index = 1; index != quality; ++index )
                {
                    float t = gp_constant_one_and_a_half_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p2.x - radius + (radius + line_half_width) * ct;
                    float y0 = p2.y - radius - (radius + line_half_width) * st;

                    gp_mesh_position( _mesh, vertex_iterator + 0, x0, y0 );
                    gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                    vertex_iterator += 1;
                }

                for( gp_uint8_t index = 1; index != quality; ++index )
                {
                    float t = gp_constant_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p3.x + radius + (radius + line_half_width) * ct;
                    float y0 = p3.y - radius - (radius + line_half_width) * st;

                    gp_mesh_position( _mesh, vertex_iterator + 0, x0, y0 );
                    gp_mesh_color( _mesh, vertex_iterator + 0, argb );

                    vertex_iterator += 1;
                }
            }
        }
    }

#ifdef GP_DEBUG
    gp_uint16_t test_vertex_count;
    gp_uint16_t test_index_count;

    if( __calculate_mesh_rounded_rect_size( _canvas, &test_vertex_count, &test_index_count ) == GP_FAILURE )
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

    *_vertex_iterator = vertex_iterator;
    *_index_iterator = index_iterator;

    return GP_SUCCESSFUL;
}