#ifndef GP_DETAIL_H_
#define GP_DETAIL_H_

#include "graphics/config.h"

#define GP_NEW(c, t) ((t*)(*c->malloc)(sizeof(t)))
#define GP_FREE(c, o) (*c->free)(o)

#define GP_LIST_PUSHBACK(t, list, e) {t * __l = list; for( ; __l != GP_NULLPTR; __l = __l->next ); if( __l == GP_NULLPTR) list = e; else __l->next = e;}
#define GP_LIST_DESTROY(c, t, list) {for( t * __l = list; __l != GP_NULLPTR; ){ t * __r = __l; __l = __l->next; GP_FREE( c, __r ); } list = GP_NULLPTR;}

#define GP_COLOR_MUL(c, c0, c1) {c.r = c0.r * c1.r; c.g = c0.g * c1.g; c.b = c0.b * c1.b; c.a = c0.a * c1.a;}

#define GP_COLOR_ARGB(argb, c) {const float rgba_255 = 255.5f; uint8_t r8 = (uint8_t)(c.GP_COLOR_R( r ) * rgba_255); uint8_t g8 = (uint8_t)(c.GP_COLOR_G( g ) * rgba_255); uint8_t b8 = (uint8_t)(c.GP_COLOR_B( b ) * rgba_255); uint8_t a8 = (uint8_t)(c.GP_COLOR_A( a ) * rgba_255); argb = (a8 << 24) | (r8 << 16) | (g8 << 8) | (b8 << 0);}

#endif