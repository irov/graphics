#include "detail.h"

#include "struct.h"

//////////////////////////////////////////////////////////////////////////
void gp_color_mul( gp_color_t * _c, const gp_color_t * _c0, const gp_color_t * _c1 )
{
    _c->r = _c0->r * _c1->r;
    _c->g = _c0->g * _c1->g;
    _c->b = _c0->b * _c1->b;
    _c->a = _c0->a * _c1->a;
}
//////////////////////////////////////////////////////////////////////////
gp_uint32_t gp_color_argb( const gp_color_t * _c )
{
    const float rgba_255 = 255.5f;

    gp_uint8_t r8 = (gp_uint8_t)(_c->GP_COLOR_R( r ) * rgba_255);
    gp_uint8_t g8 = (gp_uint8_t)(_c->GP_COLOR_G( g ) * rgba_255);
    gp_uint8_t b8 = (gp_uint8_t)(_c->GP_COLOR_B( b ) * rgba_255);
    gp_uint8_t a8 = (gp_uint8_t)(_c->GP_COLOR_A( a ) * rgba_255);

    gp_uint32_t argb = (a8 << 24) | (r8 << 16) | (g8 << 8) | (b8 << 0);

    return argb;
}
//////////////////////////////////////////////////////////////////////////
void gp_mesh_index( const gp_mesh_t * _mesh, gp_uint16_t _iterator, gp_uint16_t _index )
{
    *(gp_uint16_t *)((gp_uint8_t *)_mesh->indices_buffer + _mesh->indices_offset + _mesh->indices_stride * _iterator) = _index;
}
//////////////////////////////////////////////////////////////////////////
void gp_mesh_position( const gp_mesh_t * _mesh, gp_uint16_t _iterator, float _x, float _y )
{
    gp_vec2f_t p;
    p.x = _x;
    p.y = _y;

    *(gp_vec2f_t *)((gp_uint8_t *)_mesh->positions_buffer + _mesh->positions_offset + _mesh->positions_stride * _iterator) = p;
}
//////////////////////////////////////////////////////////////////////////
void gp_mesh_color( const gp_mesh_t * _mesh, gp_uint16_t _iterator, gp_uint32_t _c )
{
    *(gp_uint32_t *)((gp_uint8_t *)_mesh->colors_buffer + _mesh->colors_offset + _mesh->colors_stride * _iterator) = _c;
}