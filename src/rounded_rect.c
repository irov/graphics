#include "rounded_rect.h"

#include "struct.h"

gp_result_t gp_calculate_mesh_rounded_rect_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh )
{
    uint32_t vertex_count = 0;
    uint16_t index_count = 0;

    for( const gp_rounded_rect_t * r = _canvas->rounded_rects; r != GP_NULLPTR; r = r->next )
    {
        if( r->line_penumbra > 0.f )
        {
            vertex_count += 32;
            index_count += 72;

            for( uint32_t index = 0; index != 4; ++index )
            {
                vertex_count += (r->quality + 1) * 4;
                index_count += r->quality * 6 * 3;
            }
        }
        else
        {
            vertex_count += 16;
            index_count += 24;

            for( uint32_t index = 0; index != 4; ++index )
            {
                vertex_count += (r->quality + 1) * 2;
                index_count += r->quality * 6;
            }
        }

        if( r->fill == GP_TRUE )
        {
            for( uint32_t index = 0; index != 4; ++index )
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