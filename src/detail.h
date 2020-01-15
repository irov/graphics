#ifndef GP_DETAIL_H_
#define GP_DETAIL_H_

#include "graphics/config.h"

#define GP_NEW(c, t) ((t*)(*c->malloc)(sizeof(t)))
#define GP_FREE(c, o) (*c->free)(o)

#define GP_LIST_PUSHBACK(t, list, e) {t * __l = list; for( ; __l != GP_NULLPTR; __l = __l->next ); if( __l == GP_NULLPTR) list = e; else __l->next = e;}

#endif