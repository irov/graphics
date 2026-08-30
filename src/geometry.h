#ifndef GP_GEOMETRY_H_
#define GP_GEOMETRY_H_

#include "graphics/graphics.h"

#include "struct.h"

gp_result_t gp_geometry_write( gp_mesh_writer_t * _writer, const gp_geometry_t * _geometry );
gp_result_t gp_textured_rects_write( gp_mesh_writer_t * _writer, const gp_textured_rects_t * _rects );

#endif
