#ifndef GP_DETAIL_H_
#define GP_DETAIL_H_

#include "graphics/config.h"
#include "graphics/graphics.h"

#include "struct.h"

#define GP_NEW(c, t) ((t*)gp_canvas_recording_allocate(c, sizeof(t)))
#define GP_REALLOC(c, p, t, s) ((t*)gp_canvas_scratch_reallocate_array(c, p, sizeof(t), (s)))
#define GP_FREE(c, p) gp_canvas_scratch_free(c, p)

#define GP_CALL(m, args) do { if( m args == GP_FAILURE ) return GP_FAILURE; } while( 0 )

#define GP_LIST_FOREACH(t, list, e) for( t * e = list; e != GP_NULLPTR; e = e->next )
#define GP_LIST_PUSHBACK(t, list, e) {if( list == GP_NULLPTR ){list = e; list->prev = e;}else{e->prev = list->prev; e->next = GP_NULLPTR; list->prev->next = e; list->prev = e;}}
#define GP_LIST_BACK(list) (list->prev)
#define GP_LIST_DESTROY(c, t, list) {for( t * __l = list; __l != GP_NULLPTR; ){ t * __r = __l; __l = __l->next; GP_FREE( c, __r ); } list = GP_NULLPTR;}

#define GP_EQUAL_F(f, eps) (((f) >= -(eps)) && ((f) <= (eps)))

#define GP_GET_STATE(c) (c->state_cook + c->state_stack)

static const float gp_constant_pi = 3.1415926535897932384626433832795f;
static const float gp_constant_half_pi = 1.5707963267948966192313216916398f;
static const float gp_constant_one_and_a_half_pi = 4.7123889803846898576939650749193f;
static const float gp_constant_two_pi = 6.283185307179586476925286766559f;

void gp_color_mul( gp_color_t * _c, const gp_color_t * _c0, const gp_color_t * _c1 );
gp_uint32_t gp_color_argb( const gp_color_t * _c );

gp_uint32_t gp_ellipse_arc_segment_count( float _angle_delta, gp_uint8_t _quality );
float gp_ellipse_arc_sample_t( float _angle_delta, gp_uint8_t _quality, gp_uint32_t _index );
float gp_ellipse_arc_sample_angle( float _angle_from, float _angle_to, gp_uint8_t _quality, gp_uint32_t _index );

gp_result_t gp_mesh_push_index( const gp_mesh_t * _mesh, gp_uint32_t _iterator, gp_uint32_t _index );
gp_result_t gp_mesh_push_position( const gp_mesh_t * _mesh, gp_uint32_t _iterator, float _x, float _y );
gp_result_t gp_mesh_push_color( const gp_mesh_t * _mesh, gp_uint32_t _iterator, gp_argb_t _c );
gp_result_t gp_mesh_push_uv( const gp_state_t * _state, const gp_mesh_t * _mesh, gp_uint32_t _iterator, float _u, float _v );
gp_result_t gp_mesh_push_uv_map( const gp_state_t * _state, const gp_mesh_t * _mesh, gp_uint32_t _iterator, float _x, float _y, float _ox, float _oy, float _w, float _h );

gp_result_t gp_mesh_get_position( const gp_mesh_t * _mesh, gp_uint32_t _iterator, gp_vec2f_t * const _pos );

void * gp_canvas_recording_allocate( gp_canvas_t * _canvas, gp_size_t _size );
void * gp_canvas_scratch_reallocate( gp_canvas_t * _canvas, void * _pointer, gp_size_t _size );
void * gp_canvas_scratch_reallocate_array( gp_canvas_t * _canvas, void * _pointer, gp_size_t _element_size, gp_size_t _count );
void gp_canvas_scratch_free( gp_canvas_t * _canvas, void * _pointer );
void * gp_canvas_scratch_malloc_callback( gp_size_t _size, void * _ud );
void * gp_canvas_scratch_realloc_callback( void * _pointer, gp_size_t _size, void * _ud );
void gp_canvas_scratch_free_callback( void * _pointer, void * _ud );
gp_size_t gp_canvas_scratch_mark( const gp_canvas_t * _canvas );
void gp_canvas_scratch_reset( gp_canvas_t * _canvas, gp_size_t _mark );

#endif
