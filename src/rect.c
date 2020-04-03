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
        if( r->state->fill == GP_FALSE )
        {
            if( r->state->penumbra > 0.f )
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
            if( r->state->penumbra > 0.f )
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

    GP_DEBUG_CALL( __calculate_mesh_rect_size, (_canvas, &vertex_count, &index_count) );

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
        gp_color_mul( &line_color, &_mesh->color, &r->state->color );
        gp_argb_t argb = gp_color_argb( &line_color );

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

        float penumbra = r->state->penumbra;
        float thickness = r->state->thickness;
        float half_thickness = thickness * 0.5f;
        float half_thickness_soft = half_thickness - penumbra;

        float total_width = r->width + thickness;
        float total_height = r->height + thickness;

        float u_offset = -(p0.x - half_thickness);
        float v_offset = -(p0.y - half_thickness);

        if( r->state->fill == GP_FALSE )
        {
            if( penumbra > 0.f )
            {
                for( gp_uint16_t index = 0; index != 4; ++index )
                {
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 4 + 0) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + (index * 4 + 1) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + (index * 4 + 4) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + (index * 4 + 4) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + (index * 4 + 1) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + (index * 4 + 5) % 16) );

                    index_iterator += 6;

                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 4 + 1) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + (index * 4 + 2) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + (index * 4 + 5) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + (index * 4 + 5) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + (index * 4 + 2) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + (index * 4 + 6) % 16) );

                    index_iterator += 6;

                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 4 + 2) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + (index * 4 + 3) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + (index * 4 + 6) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + (index * 4 + 6) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + (index * 4 + 3) % 16) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + (index * 4 + 7) % 16) );

                    index_iterator += 6;
                }

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p0.x - half_thickness, p0.y - half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p0.x - half_thickness, p0.y - half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p0.x - half_thickness_soft, p0.y - half_thickness_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p0.x - half_thickness_soft, p0.y - half_thickness_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p0.x + half_thickness_soft, p0.y + half_thickness_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, p0.x + half_thickness_soft, p0.y + half_thickness_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, p0.x + half_thickness, p0.y + half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 3, p0.x + half_thickness, p0.y + half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 4;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p1.x + half_thickness, p1.y - half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p1.x + half_thickness, p1.y - half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p1.x + half_thickness_soft, p1.y - half_thickness_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p1.x + half_thickness_soft, p1.y - half_thickness_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p1.x - half_thickness_soft, p1.y + half_thickness_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, p1.x - half_thickness_soft, p1.y + half_thickness_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, p1.x - half_thickness, p1.y + half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 3, p1.x - half_thickness, p1.y + half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 4;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p2.x + half_thickness, p2.y + half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p2.x + half_thickness, p2.y + half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p2.x + half_thickness_soft, p2.y + half_thickness_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p2.x + half_thickness_soft, p2.y + half_thickness_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p2.x - half_thickness_soft, p2.y - half_thickness_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, p2.x - half_thickness_soft, p2.y - half_thickness_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, p2.x - half_thickness, p2.y - half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 3, p2.x - half_thickness, p2.y - half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 4;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p3.x - half_thickness, p3.y + half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p3.x - half_thickness, p3.y + half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p3.x - half_thickness_soft, p3.y + half_thickness_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p3.x - half_thickness_soft, p3.y + half_thickness_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p3.x + half_thickness_soft, p3.y - half_thickness_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, p3.x + half_thickness_soft, p3.y - half_thickness_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 3, p3.x + half_thickness, p3.y - half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 3, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 3, p3.x + half_thickness, p3.y - half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 4;
            }
            else
            {
                for( gp_uint16_t index = 0; index != 4; ++index )
                {
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 2 + 0) % 8) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + (index * 2 + 1) % 8) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + (index * 2 + 2) % 8) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + (index * 2 + 2) % 8) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + (index * 2 + 1) % 8) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + (index * 2 + 3) % 8) );

                    index_iterator += 6;
                }

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p0.x - half_thickness, p0.y - half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p0.x - half_thickness, p0.y - half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p0.x + half_thickness, p0.y + half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p0.x + half_thickness, p0.y + half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p1.x + half_thickness, p1.y - half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p1.x + half_thickness, p1.y - half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p1.x - half_thickness, p1.y + half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p1.x - half_thickness, p1.y + half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p2.x + half_thickness, p2.y + half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p2.x + half_thickness, p2.y + half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p2.x - half_thickness, p2.y - half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p2.x - half_thickness, p2.y - half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p3.x - half_thickness, p3.y + half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p3.x - half_thickness, p3.y + half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p3.x + half_thickness, p3.y - half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p3.x + half_thickness, p3.y - half_thickness, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;
            }
        }
        else
        {
            if( penumbra > 0.f )
            {
                for( gp_uint16_t index = 0; index != 4; ++index )
                {
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 3 + 0) % 12) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + (index * 3 + 1) % 12) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + (index * 3 + 3) % 12) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + (index * 3 + 3) % 12) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + (index * 3 + 1) % 12) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + (index * 3 + 4) % 12) );

                    index_iterator += 6;

                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 3 + 1) % 12) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + (index * 3 + 2) % 12) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + (index * 3 + 4) % 12) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + (index * 3 + 4) % 12) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + (index * 3 + 2) % 12) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + (index * 3 + 5) % 12) );

                    index_iterator += 6;
                }

                GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + 2) );
                GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + 5) );
                GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + 11) );
                GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + 11) );
                GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + 5) );
                GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + 8) );

                index_iterator += 6;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p0.x - half_thickness, p0.y - half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p0.x - half_thickness, p0.y - half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p0.x - half_thickness_soft, p0.y - half_thickness_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p0.x - half_thickness_soft, p0.y - half_thickness_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p0.x, p0.y) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, p0.x, p0.y, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 3;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p1.x + half_thickness, p1.y - half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p1.x + half_thickness, p1.y - half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p1.x + half_thickness_soft, p1.y - half_thickness_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p1.x + half_thickness_soft, p1.y - half_thickness_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p1.x, p1.y) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, p1.x, p1.y, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 3;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p2.x + half_thickness, p2.y + half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p2.x + half_thickness, p2.y + half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p2.x + half_thickness_soft, p2.y + half_thickness_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p2.x + half_thickness_soft, p2.y + half_thickness_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p2.x, p2.y) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, p2.x, p2.y, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 3;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p3.x - half_thickness, p3.y + half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p3.x - half_thickness, p3.y + half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p3.x - half_thickness_soft, p3.y + half_thickness_soft) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p3.x - half_thickness_soft, p3.y + half_thickness_soft, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 2, p3.x, p3.y) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 2, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 2, p3.x, p3.y, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 3;
            }
            else
            {
                for( gp_uint16_t index = 0; index != 4; ++index )
                {
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 2 + 0) % 8) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + (index * 2 + 1) % 8) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + (index * 2 + 2) % 8) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + (index * 2 + 2) % 8) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + (index * 2 + 1) % 8) );
                    GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + (index * 2 + 3) % 8) );

                    index_iterator += 6;
                }

                GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 0, vertex_iterator + 1) );
                GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 1, vertex_iterator + 3) );
                GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 2, vertex_iterator + 7) );
                GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 3, vertex_iterator + 7) );
                GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 4, vertex_iterator + 3) );
                GP_DEBUG_CALL( gp_mesh_index, (_mesh, index_iterator + 5, vertex_iterator + 5) );

                index_iterator += 6;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p0.x - half_thickness, p0.y - half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p0.x - half_thickness, p0.y - half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p0.x, p0.y) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p0.x, p0.y, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p1.x + half_thickness, p1.y - half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p1.x + half_thickness, p1.y - half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p1.x, p1.y) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p1.x, p1.y, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p2.x + half_thickness, p2.y + half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p2.x + half_thickness, p2.y + half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p2.x, p2.y) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p2.x, p2.y, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 0, p3.x - half_thickness, p3.y + half_thickness) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 0, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 0, p3.x - half_thickness, p3.y + half_thickness, u_offset, v_offset, total_width, total_height) );

                GP_DEBUG_CALL( gp_mesh_position, (_mesh, vertex_iterator + 1, p3.x, p3.y) );
                GP_DEBUG_CALL( gp_mesh_color, (_mesh, vertex_iterator + 1, argb) );
                GP_DEBUG_CALL( gp_mesh_uv_map, (_canvas, _mesh, vertex_iterator + 1, p3.x, p3.y, u_offset, v_offset, total_width, total_height) );

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