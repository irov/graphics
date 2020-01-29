#include "ellipse.h"

#include "struct.h"

gp_result_t gp_calculate_mesh_ellipse_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh )
{
    gp_uint32_t vertex_count = 0;
    gp_uint16_t index_count = 0;

    for( const gp_ellipse_t * e = _canvas->ellipses; e != GP_NULLPTR; e = e->next )
    {
        if( e->line_penumbra > 0.f )
        {
            vertex_count += e->quality * 4;
            index_count += e->quality * 18;
        }
        else
        {
            vertex_count += e->quality * 2;
            index_count += e->quality * 6;
        }

        if( e->fill == GP_TRUE )
        {
            vertex_count += 1;
            vertex_count += e->quality;

            index_count += e->quality * 3;
        }
    }

    _mesh->vertex_count += vertex_count;
    _mesh->index_count += index_count;

    return GP_SUCCESSFUL;
}