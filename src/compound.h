#ifndef GP_COMPOUND_H_
#define GP_COMPOUND_H_

#include "graphics/graphics.h"
#include "struct.h"

gp_result_t gp_calculate_mesh_compound_fill_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh );
gp_result_t gp_calculate_mesh_compound_size( const gp_compound_fill_t * _compound, gp_mesh_t * _mesh );
gp_result_t gp_render_compound_fill( const gp_compound_fill_t * _compound, const gp_mesh_t * _mesh, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator );

#endif
