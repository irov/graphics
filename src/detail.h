#ifndef GP_DETAIL_H_
#define GP_DETAIL_H_

#include "graphics/config.h"
#include "graphics/graphics.h"

#define GP_NEW(c, t) ((t*)(*c->malloc)(sizeof(t), c->ud))
#define GP_FREE(c, o) (*c->free)(o, c->ud)

#if defined(GP_DEBUG)
#   define GP_DEBUG_CALL(m, args) if( m args == GP_FAILURE) return GP_FAILURE
#else
#   define GP_DEBUG_CALL(m, args) m args
#endif

#define GP_LIST_FOREACH(t, list, e) for( t * e = list; e != GP_NULLPTR; e = e->next )
#define GP_LIST_PUSHBACK(t, list, e) {if( list == GP_NULLPTR ){list = e; list->prev = e;}else{e->prev = list->prev; e->next = GP_NULLPTR; list->prev->next = e; list->prev = e;}}
#define GP_LIST_BACK(list) (list->prev)
#define GP_LIST_DESTROY(c, t, list) {for( t * __l = list; __l != GP_NULLPTR; ){ t * __r = __l; __l = __l->next; GP_FREE( c, __r ); } list = GP_NULLPTR;}

#define GP_EQUAL_F(f, eps) (((f) >= -(eps)) && ((f) <= (eps)))

static const float gp_constant_pi = 3.1415926535897932384626433832795f;
static const float gp_constant_half_pi = 1.5707963267948966192313216916398f;
static const float gp_constant_one_and_a_half_pi = 4.7123889803846898576939650749193f;
static const float gp_constant_two_pi = 6.283185307179586476925286766559f;

void gp_color_mul( gp_color_t * _c, const gp_color_t * _c0, const gp_color_t * _c1 );
gp_uint32_t gp_color_argb( const gp_color_t * _c );

gp_result_t gp_mesh_index( const gp_mesh_t * _mesh, gp_uint16_t _iterator, gp_uint16_t _index );
gp_result_t gp_mesh_position( const gp_mesh_t * _mesh, gp_uint16_t _iterator, float _x, float _y );
gp_result_t gp_mesh_color( const gp_mesh_t * _mesh, gp_uint16_t _iterator, gp_argb_t _c );
gp_result_t gp_mesh_uv( const gp_canvas_t * _canvas, const gp_mesh_t * _mesh, gp_uint16_t _iterator, float _u, float _v );
gp_result_t gp_mesh_uv_map( const gp_canvas_t * _canvas, const gp_mesh_t * _mesh, gp_uint16_t _iterator, float _x, float _y, float _ox, float _oy, float _w, float _h );

#endif