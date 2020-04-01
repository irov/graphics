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
        gp_uint8_t rect_quality = r->state->rect_quality;

        if( r->state->fill == GP_FALSE )
        {
            if( r->state->penumbra > 0.f )
            {
                vertex_count += 32;
                index_count += 72;

                for( gp_uint32_t index = 0; index != 4; ++index )
                {
                    vertex_count += (rect_quality - 1) * 4;
                    index_count += rect_quality * 6 * 3;
                }
            }
            else
            {
                vertex_count += 16;
                index_count += 24;

                for( gp_uint32_t index = 0; index != 4; ++index )
                {
                    vertex_count += (rect_quality - 1) * 2;
                    index_count += rect_quality * 6;
                }
            }
        }
        else
        {
            if( r->state->penumbra > 0.f )
            {
                vertex_count += 16 + 4;
                index_count += 6 + 48;

                for( gp_uint32_t index = 0; index != 4; ++index )
                {
                    vertex_count += (rect_quality - 1) * 2;
                    index_count += rect_quality * 6;
                    index_count += rect_quality * 3;
                }
            }
            else
            {
                vertex_count += 12;
                index_count += 6 + 24;

                for( gp_uint32_t index = 0; index != 4; ++index )
                {
                    vertex_count += (rect_quality - 1);
                    index_count += rect_quality * 3;
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

    GP_DEBUG_CALL( __calculate_mesh_rounded_rect_size, (_canvas, &vertex_count, &index_count) );

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
        gp_color_mul( &line_color, &_mesh->color, &rr->state->color );
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

        float line_thickness = rr->state->line_thickness;
        float line_half_thickness = line_thickness * 0.5f;
        float radius = rr->radius;
        float penumbra = rr->state->penumbra;

        float total_width = rr->width + line_thickness;
        float total_height = rr->height + line_thickness;

        float u_offset = -(p0.x - line_half_thickness);
        float v_offset = -(p0.y - line_half_thickness);

        gp_uint8_t rect_quality = rr->state->rect_quality;
        float rect_quality_inv = rr->state->rect_quality_inv;

        float dt = gp_constant_half_pi * rect_quality_inv;

        gp_uint16_t base_vertex_iterator = vertex_iterator;

        if( rr->state->fill == GP_FALSE )
        {
            if( penumbra > 0.f )
            {
                float line_half_thickness_soft = line_half_thickness - penumbra;

                for( gp_uint16_t index = 0; index != 4; ++index )
                {
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 8 + 0)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + (index * 8 + 1)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + (index * 8 + 4)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + (index * 8 + 4)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + (index * 8 + 1)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + (index * 8 + 5)) );

                    index_iterator += 6;

                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 8 + 1)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + (index * 8 + 2)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + (index * 8 + 5)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + (index * 8 + 5)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + (index * 8 + 2)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + (index * 8 + 6)) );

                    index_iterator += 6;

                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 8 + 2)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + (index * 8 + 3)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + (index * 8 + 6)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + (index * 8 + 6)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + (index * 8 + 3)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + (index * 8 + 7)) );

                    index_iterator += 6;
                }

                float line_half_width_soft = line_half_thickness - penumbra;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p0.x + radius, p0.y - line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p0.x + radius, p0.y - line_half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p0.x + radius, p0.y - line_half_width_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p0.x + radius, p0.y - line_half_width_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p0.x + radius, p0.y + line_half_width_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, p0.x + radius, p0.y + line_half_width_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, p0.x + radius, p0.y + line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 3, p0.x + radius, p0.y + line_half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 4;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p1.x - radius, p1.y - line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p1.x - radius, p1.y - line_half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p1.x - radius, p1.y - line_half_width_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p1.x - radius, p1.y - line_half_width_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p1.x - radius, p1.y + line_half_width_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, p1.x - radius, p1.y + line_half_width_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, p1.x - radius, p1.y + line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 3, p1.x - radius, p1.y + line_half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 4;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p1.x + line_half_thickness, p1.y + radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p1.x + line_half_thickness, p1.y + radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p1.x + line_half_width_soft, p1.y + radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p1.x + line_half_width_soft, p1.y + radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p1.x - line_half_width_soft, p1.y + radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, p1.x - line_half_width_soft, p1.y + radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, p1.x - line_half_thickness, p1.y + radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 3, p1.x - line_half_thickness, p1.y + radius, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 4;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p2.x + line_half_thickness, p2.y - radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p2.x + line_half_thickness, p2.y - radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p2.x + line_half_width_soft, p2.y - radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p2.x + line_half_width_soft, p2.y - radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p2.x - line_half_width_soft, p2.y - radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, p2.x - line_half_width_soft, p2.y - radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, p2.x - line_half_thickness, p2.y - radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 3, p2.x - line_half_thickness, p2.y - radius, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 4;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p2.x - radius, p2.y + line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p2.x - radius, p2.y + line_half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p2.x - radius, p2.y + line_half_width_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p2.x - radius, p2.y + line_half_width_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p2.x - radius, p2.y - line_half_width_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, p2.x - radius, p2.y - line_half_width_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, p2.x - radius, p2.y - line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 3, p2.x - radius, p2.y - line_half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 4;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p3.x + radius, p3.y + line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p3.x + radius, p3.y + line_half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p3.x + radius, p3.y + line_half_width_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p3.x + radius, p3.y + line_half_width_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p3.x + radius, p3.y - line_half_width_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, p3.x + radius, p3.y - line_half_width_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, p3.x + radius, p3.y - line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 3, p3.x + radius, p3.y - line_half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 4;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p3.x - line_half_thickness, p3.y - radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p3.x - line_half_thickness, p3.y - radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p3.x - line_half_width_soft, p3.y - radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p3.x - line_half_width_soft, p3.y - radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p3.x + line_half_width_soft, p3.y - radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, p3.x + line_half_width_soft, p3.y - radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, p3.x + line_half_thickness, p3.y - radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 3, p3.x + line_half_thickness, p3.y - radius, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 4;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p0.x - line_half_thickness, p0.y + radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p0.x - line_half_thickness, p0.y + radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p0.x - line_half_width_soft, p0.y + radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p0.x - line_half_width_soft, p0.y + radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p0.x + line_half_width_soft, p0.y + radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, p0.x + line_half_width_soft, p0.y + radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, p0.x + line_half_thickness, p0.y + radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 3, p0.x + line_half_thickness, p0.y + radius, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 4;

                for( gp_uint16_t index_arc = 0; index_arc != 4; ++index_arc )
                {
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, base_vertex_iterator + (0 + 8 * index_arc) % 32) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index_arc * (rect_quality - 1) * 4 + 0) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, base_vertex_iterator + (1 + 8 * index_arc) % 32) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, base_vertex_iterator + (1 + 8 * index_arc) % 32) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index_arc * (rect_quality - 1) * 4 + 0) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + index_arc * (rect_quality - 1) * 4 + 1) );

                    index_iterator += 6;

                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, base_vertex_iterator + (1 + 8 * index_arc) % 32) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index_arc * (rect_quality - 1) * 4 + 1) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, base_vertex_iterator + (2 + 8 * index_arc) % 32) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, base_vertex_iterator + (2 + 8 * index_arc) % 32) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index_arc * (rect_quality - 1) * 4 + 1) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + index_arc * (rect_quality - 1) * 4 + 2) );

                    index_iterator += 6;

                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, base_vertex_iterator + (2 + 8 * index_arc) % 32) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index_arc * (rect_quality - 1) * 4 + 2) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, base_vertex_iterator + (3 + 8 * index_arc) % 32) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, base_vertex_iterator + (3 + 8 * index_arc) % 32) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index_arc * (rect_quality - 1) * 4 + 2) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + index_arc * (rect_quality - 1) * 4 + 3) );

                    index_iterator += 6;

                    for( gp_uint16_t index = 0; index != rect_quality - 2; ++index )
                    {
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (index * 4 + 0)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (index * 4 + 1)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (index * 4 + 4)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (index * 4 + 4)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (index * 4 + 1)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (index * 4 + 5)) );

                        index_iterator += 6;

                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (index * 4 + 1)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (index * 4 + 2)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (index * 4 + 5)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (index * 4 + 5)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (index * 4 + 2)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (index * 4 + 6)) );

                        index_iterator += 6;

                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (index * 4 + 2)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (index * 4 + 3)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (index * 4 + 6)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (index * 4 + 6)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (index * 4 + 3)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (index * 4 + 7)) );

                        index_iterator += 6;
                    }

                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (rect_quality - 2) * 4 + 0) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (rect_quality - 2) * 4 + 1) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, base_vertex_iterator + (28 + 8 * index_arc) % 32) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, base_vertex_iterator + (28 + 8 * index_arc) % 32) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (rect_quality - 2) * 4 + 1) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, base_vertex_iterator + (29 + 8 * index_arc) % 32) );

                    index_iterator += 6;

                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (rect_quality - 2) * 4 + 1) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (rect_quality - 2) * 4 + 2) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, base_vertex_iterator + (29 + 8 * index_arc) % 32) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, base_vertex_iterator + (29 + 8 * index_arc) % 32) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (rect_quality - 2) * 4 + 2) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, base_vertex_iterator + (30 + 8 * index_arc) % 32) );

                    index_iterator += 6;

                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (rect_quality - 2) * 4 + 2) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (rect_quality - 2) * 4 + 3) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, base_vertex_iterator + (30 + 8 * index_arc) % 32) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, base_vertex_iterator + (30 + 8 * index_arc) % 32) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index_arc * (rect_quality - 1) * 4 + (rect_quality - 2) * 4 + 3) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, base_vertex_iterator + (31 + 8 * index_arc) % 32) );

                    index_iterator += 6;
                }

                for( gp_uint8_t index = 1; index != rect_quality; ++index )
                {
                    float t = gp_constant_half_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p0.x + radius + (radius + line_half_thickness) * ct;
                    float y0 = p0.y + radius - (radius + line_half_thickness) * st;

                    float x1 = p0.x + radius + (radius - line_half_thickness) * ct;
                    float y1 = p0.y + radius - (radius - line_half_thickness) * st;

                    float x0_soft = p0.x + radius + (radius + line_half_thickness_soft) * ct;
                    float y0_soft = p0.y + radius - (radius + line_half_thickness_soft) * st;

                    float x1_soft = p0.x + radius + (radius - line_half_thickness_soft) * ct;
                    float y1_soft = p0.y + radius - (radius - line_half_thickness_soft) * st;

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, x0, y0) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, x0, y0, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, x0_soft, y0_soft) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, x0_soft, y0_soft, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, x1_soft, y1_soft) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, x1_soft, y1_soft, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, x1, y1) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb & 0x00ffffff) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 3, x1, y1, u_offset, v_offset, total_width, total_height) );

                    vertex_iterator += 4;
                }

                for( gp_uint8_t index = 1; index != rect_quality; ++index )
                {
                    float t = 0.f + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p1.x - radius + (radius + line_half_thickness) * ct;
                    float y0 = p1.y + radius - (radius + line_half_thickness) * st;

                    float x1 = p1.x - radius + (radius - line_half_thickness) * ct;
                    float y1 = p1.y + radius - (radius - line_half_thickness) * st;

                    float x0_soft = p1.x - radius + (radius + line_half_thickness_soft) * ct;
                    float y0_soft = p1.y + radius - (radius + line_half_thickness_soft) * st;

                    float x1_soft = p1.x - radius + (radius - line_half_thickness_soft) * ct;
                    float y1_soft = p1.y + radius - (radius - line_half_thickness_soft) * st;

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, x0, y0) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, x0, y0, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, x0_soft, y0_soft) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, x0_soft, y0_soft, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, x1_soft, y1_soft) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, x1_soft, y1_soft, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, x1, y1) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb & 0x00ffffff) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 3, x1, y1, u_offset, v_offset, total_width, total_height) );

                    vertex_iterator += 4;
                }

                for( gp_uint8_t index = 1; index != rect_quality; ++index )
                {
                    float t = gp_constant_one_and_a_half_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p2.x - radius + (radius + line_half_thickness) * ct;
                    float y0 = p2.y - radius - (radius + line_half_thickness) * st;

                    float x1 = p2.x - radius + (radius - line_half_thickness) * ct;
                    float y1 = p2.y - radius - (radius - line_half_thickness) * st;

                    float x0_soft = p2.x - radius + (radius + line_half_thickness_soft) * ct;
                    float y0_soft = p2.y - radius - (radius + line_half_thickness_soft) * st;

                    float x1_soft = p2.x - radius + (radius - line_half_thickness_soft) * ct;
                    float y1_soft = p2.y - radius - (radius - line_half_thickness_soft) * st;

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, x0, y0) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, x0, y0, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, x0_soft, y0_soft) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, x0_soft, y0_soft, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, x1_soft, y1_soft) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, x1_soft, y1_soft, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, x1, y1) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb & 0x00ffffff) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 3, x1, y1, u_offset, v_offset, total_width, total_height) );

                    vertex_iterator += 4;
                }

                for( gp_uint8_t index = 1; index != rect_quality; ++index )
                {
                    float t = gp_constant_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p3.x + radius + (radius + line_half_thickness) * ct;
                    float y0 = p3.y - radius - (radius + line_half_thickness) * st;

                    float x1 = p3.x + radius + (radius - line_half_thickness) * ct;
                    float y1 = p3.y - radius - (radius - line_half_thickness) * st;

                    float x0_soft = p3.x + radius + (radius + line_half_thickness_soft) * ct;
                    float y0_soft = p3.y - radius - (radius + line_half_thickness_soft) * st;

                    float x1_soft = p3.x + radius + (radius - line_half_thickness_soft) * ct;
                    float y1_soft = p3.y - radius - (radius - line_half_thickness_soft) * st;

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, x0, y0) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, x0, y0, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, x0_soft, y0_soft) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, x0_soft, y0_soft, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, x1_soft, y1_soft) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, x1_soft, y1_soft, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, x1, y1) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb & 0x00ffffff) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 3, x1, y1, u_offset, v_offset, total_width, total_height) );

                    vertex_iterator += 4;
                }
            }
            else
            {
                for( gp_uint16_t index = 0; index != 4; ++index )
                {
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 4 + 0)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + (index * 4 + 1)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + (index * 4 + 2)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + (index * 4 + 2)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + (index * 4 + 1)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + (index * 4 + 3)) );

                    index_iterator += 6;
                }

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p0.x + radius, p0.y - line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p0.x + radius, p0.y - line_half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p0.x + radius, p0.y + line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p0.x + radius, p0.y + line_half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p1.x - radius, p1.y - line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p1.x - radius, p1.y - line_half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p1.x - radius, p1.y + line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p1.x - radius, p1.y + line_half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p1.x + line_half_thickness, p1.y + radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p1.x + line_half_thickness, p1.y + radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p1.x - line_half_thickness, p1.y + radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p1.x - line_half_thickness, p1.y + radius, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p2.x + line_half_thickness, p2.y - radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p2.x + line_half_thickness, p2.y - radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p2.x - line_half_thickness, p2.y - radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p2.x - line_half_thickness, p2.y - radius, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p2.x - radius, p2.y + line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p2.x - radius, p2.y + line_half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p2.x - radius, p2.y - line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p2.x - radius, p2.y - line_half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p3.x + radius, p3.y + line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p3.x + radius, p3.y + line_half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p3.x + radius, p3.y - line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p3.x + radius, p3.y - line_half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p3.x - line_half_thickness, p3.y - radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p3.x - line_half_thickness, p3.y - radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p3.x + line_half_thickness, p3.y - radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p3.x + line_half_thickness, p3.y - radius, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p0.x - line_half_thickness, p0.y + radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p0.x - line_half_thickness, p0.y + radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p0.x + line_half_thickness, p0.y + radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p0.x + line_half_thickness, p0.y + radius, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                for( gp_uint16_t index_arc = 0; index_arc != 4; ++index_arc )
                {
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, base_vertex_iterator + (0 + 4 * index_arc) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index_arc * (rect_quality - 1) * 2 + 0) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, base_vertex_iterator + (1 + 4 * index_arc) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, base_vertex_iterator + (1 + 4 * index_arc) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index_arc * (rect_quality - 1) * 2 + 0) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + index_arc * (rect_quality - 1) * 2 + 1) );

                    index_iterator += 6;

                    for( gp_uint16_t index = 0; index != rect_quality - 2; ++index )
                    {
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (index * 2 + 0)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (index * 2 + 1)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (index * 2 + 2)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (index * 2 + 2)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (index * 2 + 1)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (index * 2 + 3)) );

                        index_iterator += 6;
                    }

                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (rect_quality - 2) * 2 + 0) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (rect_quality - 2) * 2 + 1) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, base_vertex_iterator + (14 + 4 * index_arc) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, base_vertex_iterator + (14 + 4 * index_arc) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (rect_quality - 2) * 2 + 1) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, base_vertex_iterator + (15 + 4 * index_arc) % 16) );

                    index_iterator += 6;
                }

                for( gp_uint8_t index = 1; index != rect_quality; ++index )
                {
                    float t = gp_constant_half_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p0.x + radius + (radius + line_half_thickness) * ct;
                    float y0 = p0.y + radius - (radius + line_half_thickness) * st;

                    float x1 = p0.x + radius + (radius - line_half_thickness) * ct;
                    float y1 = p0.y + radius - (radius - line_half_thickness) * st;

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, x0, y0) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, x0, y0, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, x1, y1) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, x1, y1, u_offset, v_offset, total_width, total_height) );

                    vertex_iterator += 2;
                }

                for( gp_uint8_t index = 1; index != rect_quality; ++index )
                {
                    float t = 0.f + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p1.x - radius + (radius + line_half_thickness) * ct;
                    float y0 = p1.y + radius - (radius + line_half_thickness) * st;

                    float x1 = p1.x - radius + (radius - line_half_thickness) * ct;
                    float y1 = p1.y + radius - (radius - line_half_thickness) * st;

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, x0, y0) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, x0, y0, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, x1, y1) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, x1, y1, u_offset, v_offset, total_width, total_height) );

                    vertex_iterator += 2;
                }

                for( gp_uint8_t index = 1; index != rect_quality; ++index )
                {
                    float t = gp_constant_one_and_a_half_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p2.x - radius + (radius + line_half_thickness) * ct;
                    float y0 = p2.y - radius - (radius + line_half_thickness) * st;

                    float x1 = p2.x - radius + (radius - line_half_thickness) * ct;
                    float y1 = p2.y - radius - (radius - line_half_thickness) * st;

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, x0, y0) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, x0, y0, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, x1, y1) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, x1, y1, u_offset, v_offset, total_width, total_height) );

                    vertex_iterator += 2;
                }

                for( gp_uint8_t index = 1; index != rect_quality; ++index )
                {
                    float t = gp_constant_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p3.x + radius + (radius + line_half_thickness) * ct;
                    float y0 = p3.y - radius - (radius + line_half_thickness) * st;

                    float x1 = p3.x + radius + (radius - line_half_thickness) * ct;
                    float y1 = p3.y - radius - (radius - line_half_thickness) * st;

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, x0, y0) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, x0, y0, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, x1, y1) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, x1, y1, u_offset, v_offset, total_width, total_height) );

                    vertex_iterator += 2;
                }
            }
        }
        else
        {
            GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + 0) );
            GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + 1) );
            GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + 3) );
            GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + 3) );
            GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + 1) );
            GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + 2) );

            index_iterator += 6;

            GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p0.x + radius, p0.y + radius) );
            GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
            GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p0.x + radius, p0.y + radius, u_offset, v_offset, total_width, total_height) );

            GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p1.x - radius, p1.y + radius) );
            GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
            GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p1.x - radius, p1.y + radius, u_offset, v_offset, total_width, total_height) );

            GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p2.x - radius, p2.y - radius) );
            GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
            GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, p2.x - radius, p2.y - radius, u_offset, v_offset, total_width, total_height) );

            GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, p3.x + radius, p3.y - radius) );
            GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb) );
            GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 3, p3.x + radius, p3.y - radius, u_offset, v_offset, total_width, total_height) );

            vertex_iterator += 4;

            if( penumbra > 0.f )
            {
                for( gp_uint16_t index = 0; index != 4; ++index )
                {
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 4 + 0)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + (index * 4 + 1)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + (index * 4 + 2)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + (index * 4 + 2)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + (index * 4 + 1)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + (index * 4 + 3)) );

                    index_iterator += 6;

                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 4 + 1)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, base_vertex_iterator + (index + 0) % 4) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + (index * 4 + 3)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + (index * 4 + 3)) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, base_vertex_iterator + (index + 0) % 4) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, base_vertex_iterator + (index + 1) % 4) );

                    index_iterator += 6;
                }

                float line_half_thickness_soft = line_half_thickness - penumbra;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p0.x + radius, p0.y - line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p0.x + radius, p0.y - line_half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p0.x + radius, p0.y - line_half_thickness_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p0.x + radius, p0.y - line_half_thickness_soft, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p1.x - radius, p1.y - line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p1.x - radius, p1.y - line_half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p1.x - radius, p1.y - line_half_thickness_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p1.x - radius, p1.y - line_half_thickness_soft, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p1.x + line_half_thickness, p1.y + radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p1.x + line_half_thickness, p1.y + radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p1.x + line_half_thickness_soft, p1.y + radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p1.x + line_half_thickness_soft, p1.y + radius, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p2.x + line_half_thickness, p2.y - radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p2.x + line_half_thickness, p2.y - radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p2.x + line_half_thickness_soft, p2.y - radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p2.x + line_half_thickness_soft, p2.y - radius, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p2.x - radius, p2.y + line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p2.x - radius, p2.y + line_half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p2.x - radius, p2.y + line_half_thickness_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p2.x - radius, p2.y + line_half_thickness_soft, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p3.x + radius, p3.y + line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p3.x + radius, p3.y + line_half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p3.x + radius, p3.y + line_half_thickness_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p3.x + radius, p3.y + line_half_thickness_soft, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p3.x - line_half_thickness, p3.y - radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p3.x - line_half_thickness, p3.y - radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p3.x - line_half_thickness_soft, p3.y - radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p3.x - line_half_thickness_soft, p3.y - radius, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p0.x - line_half_thickness, p0.y + radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p0.x - line_half_thickness, p0.y + radius, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p0.x - line_half_thickness_soft, p0.y + radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p0.x - line_half_thickness_soft, p0.y + radius, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                for( gp_uint16_t index_arc = 0; index_arc != 4; ++index_arc )
                {
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, base_vertex_iterator + 4 + (0 + 4 * index_arc) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index_arc * (rect_quality - 1) * 2 + 0) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, base_vertex_iterator + 4 + (1 + 4 * index_arc) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, base_vertex_iterator + 4 + (1 + 4 * index_arc) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index_arc * (rect_quality - 1) * 2 + 0) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + index_arc * (rect_quality - 1) * 2 + 1) );

                    index_iterator += 6;

                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, base_vertex_iterator + 4 + (1 + 4 * index_arc) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, base_vertex_iterator + index_arc) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + index_arc * (rect_quality - 1) * 2 + 1) );

                    index_iterator += 3;

                    for( gp_uint16_t index = 0; index != rect_quality - 2; ++index )
                    {
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (index * 2 + 0)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (index * 2 + 1)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (index * 2 + 2)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (index * 2 + 2)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (index * 2 + 1)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (index * 2 + 3)) );

                        index_iterator += 6;

                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (index * 2 + 1)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, base_vertex_iterator + index_arc) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (index * 2 + 3)) );

                        index_iterator += 3;
                    }

                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (rect_quality - 2) * 2 + 0) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (rect_quality - 2) * 2 + 1) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, base_vertex_iterator + 4 + (14 + 4 * index_arc) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, base_vertex_iterator + 4 + (14 + 4 * index_arc) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (rect_quality - 2) * 2 + 1) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, base_vertex_iterator + 4 + (15 + 4 * index_arc) % 16) );

                    index_iterator += 6;

                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index_arc * (rect_quality - 1) * 2 + (rect_quality - 2) * 2 + 1) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, base_vertex_iterator + index_arc) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, base_vertex_iterator + 4 + (15 + 4 * index_arc) % 16) );

                    index_iterator += 3;
                }

                for( gp_uint8_t index = 1; index != rect_quality; ++index )
                {
                    float t = gp_constant_half_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p0.x + radius + (radius + line_half_thickness) * ct;
                    float y0 = p0.y + radius - (radius + line_half_thickness) * st;



                    float x0_soft = p0.x + radius + (radius + line_half_thickness_soft) * ct;
                    float y0_soft = p0.y + radius - (radius + line_half_thickness_soft) * st;

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, x0, y0) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, x0, y0, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, x0_soft, y0_soft) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, x0_soft, y0_soft, u_offset, v_offset, total_width, total_height) );

                    vertex_iterator += 2;
                }

                for( gp_uint8_t index = 1; index != rect_quality; ++index )
                {
                    float t = 0.f + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p1.x - radius + (radius + line_half_thickness) * ct;
                    float y0 = p1.y + radius - (radius + line_half_thickness) * st;

                    float x0_soft = p1.x - radius + (radius + line_half_thickness_soft) * ct;
                    float y0_soft = p1.y + radius - (radius + line_half_thickness_soft) * st;

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, x0, y0) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, x0, y0, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, x0_soft, y0_soft) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, x0_soft, y0_soft, u_offset, v_offset, total_width, total_height) );

                    vertex_iterator += 2;
                }

                for( gp_uint8_t index = 1; index != rect_quality; ++index )
                {
                    float t = gp_constant_one_and_a_half_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p2.x - radius + (radius + line_half_thickness) * ct;
                    float y0 = p2.y - radius - (radius + line_half_thickness) * st;

                    float x0_soft = p2.x - radius + (radius + line_half_thickness_soft) * ct;
                    float y0_soft = p2.y - radius - (radius + line_half_thickness_soft) * st;

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, x0, y0) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, x0, y0, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, x0_soft, y0_soft) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, x0_soft, y0_soft, u_offset, v_offset, total_width, total_height) );

                    vertex_iterator += 2;
                }

                for( gp_uint8_t index = 1; index != rect_quality; ++index )
                {
                    float t = gp_constant_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p3.x + radius + (radius + line_half_thickness) * ct;
                    float y0 = p3.y - radius - (radius + line_half_thickness) * st;

                    float x0_soft = p3.x + radius + (radius + line_half_thickness_soft) * ct;
                    float y0_soft = p3.y - radius - (radius + line_half_thickness_soft) * st;

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, x0, y0) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, x0, y0, u_offset, v_offset, total_width, total_height) );

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, x0_soft, y0_soft) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, x0_soft, y0_soft, u_offset, v_offset, total_width, total_height) );

                    vertex_iterator += 2;
                }
            }
            else
            {
                for( gp_uint16_t index = 0; index != 4; ++index )
                {
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 2 + 0) % 8) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + (index * 2 + 1) % 8) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, base_vertex_iterator + (index + 0) % 4) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, base_vertex_iterator + (index + 0) % 4) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + (index * 2 + 1) % 8) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, base_vertex_iterator + (index + 1) % 4) );

                    index_iterator += 6;
                }

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p0.x + radius, p0.y - line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p0.x + radius, p0.y - line_half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 1;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p1.x - radius, p1.y - line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p1.x - radius, p1.y - line_half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 1;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p1.x + line_half_thickness, p1.y + radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p1.x + line_half_thickness, p1.y + radius, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 1;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p2.x + line_half_thickness, p2.y - radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p2.x + line_half_thickness, p2.y - radius, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 1;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p2.x - radius, p2.y + line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p2.x - radius, p2.y + line_half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 1;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p3.x + radius, p3.y + line_half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p3.x + radius, p3.y + line_half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 1;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p3.x - line_half_thickness, p3.y - radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p3.x - line_half_thickness, p3.y - radius, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 1;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p0.x - line_half_thickness, p0.y + radius) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p0.x - line_half_thickness, p0.y + radius, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 1;

                for( gp_uint16_t index_arc = 0; index_arc != 4; ++index_arc )
                {
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, base_vertex_iterator + 4 + (2 * index_arc + 0) % 8) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, base_vertex_iterator + index_arc) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + index_arc * (rect_quality - 1)) );

                    index_iterator += 3;

                    for( gp_uint16_t index = 0; index != rect_quality - 2; ++index )
                    {
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index_arc * (rect_quality - 1) + (index + 0)) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, base_vertex_iterator + index_arc) );
                        GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + index_arc * (rect_quality - 1) + (index + 1)) );

                        index_iterator += 3;
                    }

                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + index_arc * (rect_quality - 1) + (rect_quality - 1) - 1) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, base_vertex_iterator + index_arc) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, base_vertex_iterator + 4 + (2 * index_arc + 7) % 8) );

                    index_iterator += 3;
                }

                for( gp_uint8_t index = 1; index != rect_quality; ++index )
                {
                    float t = gp_constant_half_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p0.x + radius + (radius + line_half_thickness) * ct;
                    float y0 = p0.y + radius - (radius + line_half_thickness) * st;

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, x0, y0) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, x0, y0, u_offset, v_offset, total_width, total_height) );

                    vertex_iterator += 1;
                }

                for( gp_uint8_t index = 1; index != rect_quality; ++index )
                {
                    float t = 0.f + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p1.x - radius + (radius + line_half_thickness) * ct;
                    float y0 = p1.y + radius - (radius + line_half_thickness) * st;

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, x0, y0) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, x0, y0, u_offset, v_offset, total_width, total_height) );

                    vertex_iterator += 1;
                }

                for( gp_uint8_t index = 1; index != rect_quality; ++index )
                {
                    float t = gp_constant_one_and_a_half_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p2.x - radius + (radius + line_half_thickness) * ct;
                    float y0 = p2.y - radius - (radius + line_half_thickness) * st;

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, x0, y0) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, x0, y0, u_offset, v_offset, total_width, total_height) );

                    vertex_iterator += 1;
                }

                for( gp_uint8_t index = 1; index != rect_quality; ++index )
                {
                    float t = gp_constant_pi + dt * index;

                    float ct = GP_MATH_COSF( t );
                    float st = GP_MATH_SINF( t );

                    float x0 = p3.x + radius + (radius + line_half_thickness) * ct;
                    float y0 = p3.y - radius - (radius + line_half_thickness) * st;

                    GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, x0, y0) );
                    GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                    GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, x0, y0, u_offset, v_offset, total_width, total_height) );

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

    * _vertex_iterator = vertex_iterator;
    *_index_iterator = index_iterator;

    return GP_SUCCESSFUL;
}