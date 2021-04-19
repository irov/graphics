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

    gp_uint8_t r8 = (gp_uint8_t)(GP_COLOR_R( _c ) * rgba_255);
    gp_uint8_t g8 = (gp_uint8_t)(GP_COLOR_G( _c ) * rgba_255);
    gp_uint8_t b8 = (gp_uint8_t)(GP_COLOR_B( _c ) * rgba_255);
    gp_uint8_t a8 = (gp_uint8_t)(GP_COLOR_A( _c ) * rgba_255);

    gp_uint32_t argb = (a8 << 24) | (r8 << 16) | (g8 << 8) | (b8 << 0);

    return argb;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_mesh_push_index( const gp_mesh_t * _mesh, gp_uint16_t _iterator, gp_uint16_t _index )
{
    if( _mesh->indices_buffer == GP_NULLPTR )
    {
        return GP_SUCCESSFUL;
    }

#if defined(GP_DEBUG)
    if( _mesh->index_count <= _iterator )
    {
        return GP_FAILURE;
    }
#endif

    * (gp_uint16_t *)((gp_uint8_t *)_mesh->indices_buffer + _mesh->indices_offset + _mesh->indices_stride * _iterator) = _index;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_mesh_push_position( const gp_mesh_t * _mesh, gp_uint16_t _iterator, float _x, float _y )
{
    if( _mesh->positions_buffer == GP_NULLPTR )
    {
        return GP_SUCCESSFUL;
    }

#if defined(GP_DEBUG)
    if( _mesh->vertex_count <= _iterator )
    {
        return GP_FAILURE;
    }
#endif

    gp_vec2f_t p;
    p.x = _x;
    p.y = _y;

    *(gp_vec2f_t *)((gp_uint8_t *)_mesh->positions_buffer + _mesh->positions_offset + _mesh->positions_stride * _iterator) = p;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_mesh_push_color( const gp_mesh_t * _mesh, gp_uint16_t _iterator, gp_argb_t _c )
{
    if( _mesh->colors_buffer == GP_NULLPTR )
    {
        return GP_SUCCESSFUL;
    }

#if defined(GP_DEBUG)
    if( _mesh->vertex_count <= _iterator )
    {
        return GP_FAILURE;
    }
#endif

    * (gp_argb_t *)((gp_uint8_t *)_mesh->colors_buffer + _mesh->colors_offset + _mesh->colors_stride * _iterator) = _c;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_mesh_push_uv( const gp_canvas_t * _canvas, const gp_mesh_t * _mesh, gp_uint16_t _iterator, float _u, float _v )
{
    if( _mesh->uv_buffer == GP_NULLPTR )
    {
        return GP_SUCCESSFUL;
    }

#if defined(GP_DEBUG)
    if( _mesh->vertex_count <= _iterator )
    {
        return GP_FAILURE;
    }
#endif

    const gp_state_t * state = GP_GET_STATE( _canvas );

    gp_vec2f_t uv;
    uv.x = state->uv_ou + _u * state->uv_su;
    uv.y = state->uv_ov + _v * state->uv_sv;

    *(gp_vec2f_t *)((gp_uint8_t *)_mesh->uv_buffer + _mesh->uv_offset + _mesh->uv_stride * _iterator) = uv;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_mesh_push_uv_map( const gp_canvas_t * _canvas, const gp_mesh_t * _mesh, gp_uint16_t _iterator, float _x, float _y, float _ox, float _oy, float _w, float _h )
{
    float u = (_x + _ox) / _w;
    float v = (_y + _oy) / _h;

    gp_result_t result = gp_mesh_push_uv( _canvas, _mesh, _iterator, u, v );

    return result;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_mesh_get_position( const gp_mesh_t * _mesh, gp_uint16_t _iterator, gp_vec2f_t * const _pos )
{
#if defined(GP_DEBUG)
    if( _mesh->vertex_count <= _iterator )
    {
        return GP_FAILURE;
    }
#endif

    const gp_vec2f_t * pos = (const gp_vec2f_t *)((gp_uint8_t *)_mesh->positions_buffer + _mesh->positions_offset + _mesh->positions_stride * _iterator);

    *_pos = *pos;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////