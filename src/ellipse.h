#ifndef GP_ELLIPSE_H_
#define GP_ELLIPSE_H_

#include "graphics/graphics.h"
#include "struct.h"

gp_result_t gp_calculate_mesh_ellipse_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh );
gp_result_t gp_render_ellipse( const gp_ellipse_t * _ellipse, const gp_state_t * _state, const gp_mesh_t * _mesh, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator );
gp_result_t gp_render_ellipse_arc( const gp_ellipse_arc_t * _ellipse_arc, const gp_state_t * _state, const gp_mesh_t * _mesh, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator );

#endif