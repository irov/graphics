#include "rect.h"

#include "struct.h"

gp_result_t gp_calculate_mesh_rect_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh )
{
    uint32_t vertex_count = 0;
    uint16_t index_count = 0;

    for( const gp_rect_t * r = _canvas->rects; r != GP_NULLPTR; r = r->next )
    {
        if( r->line_penumbra > 0.f )
        {
            vertex_count += 16;
            index_count += 72;
        }
        else
        {
            vertex_count += 8;
            index_count += 24;
        }

        if( r->fill == GP_TRUE )
        {
            vertex_count += 4;
            index_count += 6;
        }
    }

    _mesh->vertex_count += vertex_count;
    _mesh->index_count += index_count;

    return GP_SUCCESSFUL;
}