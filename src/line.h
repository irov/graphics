#ifndef GP_LINE_H_
#define GP_LINE_H_

#include "graphics/graphics.h"

gp_result_t gp_calculate_mesh_line_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh );
gp_result_t gp_render_line( const gp_canvas_t * _canvas, const gp_mesh_t * _mesh, uint16_t * _vertex_iterator, uint16_t * _index_iterator );

#endif