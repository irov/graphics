#include "detail.h"

//////////////////////////////////////////////////////////////////////////
void gp_color_mul( gp_color_t * _c, const gp_color_t * _c0, const gp_color_t * _c1 )
{
    _c->r = _c0->r * _c1->r;
    _c->g = _c0->g * _c1->g;
    _c->b = _c0->b * _c1->b;
    _c->a = _c0->a * _c1->a;
}
//////////////////////////////////////////////////////////////////////////
uint32_t gp_color_argb( const gp_color_t * _c )
{
    const float rgba_255 = 255.5f;

    uint8_t r8 = (uint8_t)(_c->GP_COLOR_R( r ) * rgba_255);
    uint8_t g8 = (uint8_t)(_c->GP_COLOR_G( g ) * rgba_255);
    uint8_t b8 = (uint8_t)(_c->GP_COLOR_B( b ) * rgba_255);
    uint8_t a8 = (uint8_t)(_c->GP_COLOR_A( a ) * rgba_255);

    uint32_t argb = (a8 << 24) | (r8 << 16) | (g8 << 8) | (b8 << 0);

    return argb;
}