#ifndef GP_DETAIL_H_
#define GP_DETAIL_H_

#include "graphics/config.h"

#define GP_NEW(c, t) ((t*)(*c->malloc)(sizeof(t)))
#define GP_FREE(c, o) (*c->free)(o)

#define GP_LIST_PUSHBACK(t, list, e) {if( list == GP_NULLPTR ){list = e; list->prev = e;}else{e->prev = list->prev; e->next = GP_NULLPTR; list->prev->next = e; list->prev = e;}}
#define GP_LIST_BACK(list) (list->prev)
#define GP_LIST_DESTROY(c, t, list) {for( t * __l = list; __l != GP_NULLPTR; ){ t * __r = __l; __l = __l->next; GP_FREE( c, __r ); } list = GP_NULLPTR;}

#define GP_EQUAL_F(f, eps) (((f) >= -(eps)) && ((f) <= (eps)))

void gp_color_mul( gp_color_t * _c, const gp_color_t * _c0, const gp_color_t * _c1 );
uint32_t gp_color_argb( const gp_color_t * _c );

#endif