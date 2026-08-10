#include "ellipse.h"

#include "detail.h"
#include "struct.h"

//////////////////////////////////////////////////////////////////////////
static gp_bool_t __ellipse_arc_has_rounded_caps( const gp_ellipse_arc_t * _arc )
{
    if( _arc->rounded_caps == GP_FALSE )
    {
        return GP_FALSE;
    }

    float angle_delta = _arc->angle_to - _arc->angle_from;
    float angle_delta_abs = angle_delta < 0.f ? -angle_delta : angle_delta;

    return GP_EQUAL_F( angle_delta_abs - gp_constant_two_pi, 0.000001f ) == GP_TRUE
        ? GP_FALSE
        : GP_TRUE;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __calculate_mesh_ellipse_size( const gp_canvas_t * _canvas, gp_uint32_t * _vertex_count, gp_uint32_t * _index_count )
{
    gp_uint32_t vertex_count = 0;
    gp_uint32_t index_count = 0;

    for( const gp_ellipse_t * e = _canvas->ellipses; e != GP_NULLPTR; e = e->next )
    {
        uint8_t ellipse_quality = e->state->ellipse_quality;

        if( e->state->fill == GP_TRUE )
        {
            if( e->state->penumbra > 0.f )
            {
                vertex_count += ellipse_quality;
                index_count += ellipse_quality * 6;
            }

            vertex_count += 1;
            vertex_count += ellipse_quality;
            index_count += ellipse_quality * 3;
        }
        else
        {
            if( e->state->penumbra > 0.f )
            {
                vertex_count += ellipse_quality * 4;
                index_count += ellipse_quality * 18;
            }
            else
            {
                vertex_count += ellipse_quality * 2;
                index_count += ellipse_quality * 6;
            }
        }
    }

    for( const gp_ellipse_arc_t * a = _canvas->ellipse_arcs; a != GP_NULLPTR; a = a->next )
    {
        float angle_delta = a->angle_to - a->angle_from;
        if( GP_EQUAL_F( angle_delta, 0.000001f ) == GP_TRUE )
        {
            continue;
        }

        gp_uint32_t segment_count = gp_ellipse_arc_segment_count( angle_delta, a->state->ellipse_quality );

        float inner_ratio = a->state->inner_radius;
        if( inner_ratio < 0.f )
        {
            inner_ratio = 0.f;
        }
        else if( inner_ratio > 1.f )
        {
            inner_ratio = 1.f;
        }

        if( inner_ratio > 0.0001f )
        {
            vertex_count += (segment_count + 1) * 2;
            index_count += segment_count * 6;
        }
        else
        {
            vertex_count += segment_count + 2;
            index_count += segment_count * 3;
        }

        if( __ellipse_arc_has_rounded_caps( a ) == GP_TRUE )
        {
            gp_uint32_t cap_segment_count = a->state->ellipse_quality / 2;
            vertex_count += cap_segment_count * 2;
            index_count += cap_segment_count * 6;
        }
    }

    *_vertex_count = vertex_count;
    *_index_count = index_count;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_calculate_mesh_ellipse_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh )
{
    gp_uint32_t vertex_count;
    gp_uint32_t index_count;

    GP_CALL( __calculate_mesh_ellipse_size, (_canvas, &vertex_count, &index_count) );

    _mesh->vertex_count += vertex_count;
    _mesh->index_count += index_count;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_render_ellipse( const gp_ellipse_t * _ellipse, const gp_state_t * _state, const gp_mesh_t * _mesh, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator )
{
    gp_uint32_t vertex_iterator = *_vertex_iterator;
    gp_uint32_t index_iterator = *_index_iterator;

    const gp_ellipse_t * e = _ellipse;
    if( e == GP_NULLPTR )
    {
        return GP_SUCCESSFUL;
    }

    gp_color_t total_color;
    gp_color_mul( &total_color, &_mesh->color, &e->state->color );
    gp_argb_t argb = gp_color_argb( &total_color );

    gp_uint8_t ellipse_quality = e->state->ellipse_quality;
    float line_penumbra = e->state->penumbra;

    float thickness = e->state->thickness;
    float half_thickness = thickness * 0.5f;

    float total_width = e->radius_width * 2.f + thickness;
    float total_height = e->radius_height * 2.f + thickness;

    float u_offset = -(e->point.x - e->radius_width - half_thickness);
    float v_offset = -(e->point.y - e->radius_height - half_thickness);

    float ellipse_quality_inv = e->state->ellipse_quality_inv;

    float dt = gp_constant_two_pi * ellipse_quality_inv;

    if( e->state->fill == GP_FALSE )
    {
        if( line_penumbra > 0.f )
        {
            float line_half_thickness_soft = half_thickness - line_penumbra;

            gp_uint32_t ellipse_quality2 = ellipse_quality * 4;

            for( gp_uint16_t index = 0; index != ellipse_quality; ++index )
            {
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 4 + 0) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 1, vertex_iterator + (index * 4 + 1) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 2, vertex_iterator + (index * 4 + 4) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 3, vertex_iterator + (index * 4 + 4) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 4, vertex_iterator + (index * 4 + 1) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 5, vertex_iterator + (index * 4 + 5) % ellipse_quality2) );

                index_iterator += 6;

                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 4 + 1) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 1, vertex_iterator + (index * 4 + 2) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 2, vertex_iterator + (index * 4 + 5) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 3, vertex_iterator + (index * 4 + 5) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 4, vertex_iterator + (index * 4 + 2) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 5, vertex_iterator + (index * 4 + 6) % ellipse_quality2) );

                index_iterator += 6;

                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 4 + 2) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 1, vertex_iterator + (index * 4 + 3) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 2, vertex_iterator + (index * 4 + 6) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 3, vertex_iterator + (index * 4 + 6) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 4, vertex_iterator + (index * 4 + 3) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 5, vertex_iterator + (index * 4 + 7) % ellipse_quality2) );

                index_iterator += 6;
            }

            float t = 0.f;

            for( gp_uint32_t index = 0; index != ellipse_quality; ++index, t += dt )
            {
                float ct = GP_MATH_COSF( t );
                float st = GP_MATH_SINF( t );

                float x0 = e->point.x + (e->radius_width + half_thickness) * ct;
                float y0 = e->point.y + (e->radius_height + half_thickness) * st;

                float x1 = e->point.x + (e->radius_width - half_thickness) * ct;
                float y1 = e->point.y + (e->radius_height - half_thickness) * st;

                float x0_soft = e->point.x + (e->radius_width + line_half_thickness_soft) * ct;
                float y0_soft = e->point.y + (e->radius_height + line_half_thickness_soft) * st;

                float x1_soft = e->point.x + (e->radius_width - line_half_thickness_soft) * ct;
                float y1_soft = e->point.y + (e->radius_height - line_half_thickness_soft) * st;

                GP_CALL( gp_mesh_push_position, (_mesh, vertex_iterator + 0, x0, y0) );
                GP_CALL( gp_mesh_push_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_CALL( gp_mesh_push_uv_map, (_state, _mesh, vertex_iterator + 0, x0, y0, u_offset, v_offset, total_width, total_height) );

                GP_CALL( gp_mesh_push_position, (_mesh, vertex_iterator + 1, x0_soft, y0_soft) );
                GP_CALL( gp_mesh_push_color, (_mesh, vertex_iterator + 1, argb) );
                GP_CALL( gp_mesh_push_uv_map, (_state, _mesh, vertex_iterator + 1, x0_soft, y0_soft, u_offset, v_offset, total_width, total_height) );

                GP_CALL( gp_mesh_push_position, (_mesh, vertex_iterator + 2, x1_soft, y1_soft) );
                GP_CALL( gp_mesh_push_color, (_mesh, vertex_iterator + 2, argb) );
                GP_CALL( gp_mesh_push_uv_map, (_state, _mesh, vertex_iterator + 2, x1_soft, y1_soft, u_offset, v_offset, total_width, total_height) );

                GP_CALL( gp_mesh_push_position, (_mesh, vertex_iterator + 3, x1, y1) );
                GP_CALL( gp_mesh_push_color, (_mesh, vertex_iterator + 3, argb & 0x00ffffff) );
                GP_CALL( gp_mesh_push_uv_map, (_state, _mesh, vertex_iterator + 3, x1, y1, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 4;
            }
        }
        else
        {
            gp_uint32_t ellipse_quality2 = ellipse_quality * 2;

            for( gp_uint16_t index = 0; index != ellipse_quality; ++index )
            {
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 2 + 0) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 1, vertex_iterator + (index * 2 + 1) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 2, vertex_iterator + (index * 2 + 2) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 3, vertex_iterator + (index * 2 + 2) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 4, vertex_iterator + (index * 2 + 1) % ellipse_quality2) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 5, vertex_iterator + (index * 2 + 3) % ellipse_quality2) );

                index_iterator += 6;
            }

            float t = 0.f;

            for( gp_uint32_t index = 0; index != ellipse_quality; ++index, t += dt )
            {
                float ct = GP_MATH_COSF( t );
                float st = GP_MATH_SINF( t );

                float x0 = e->point.x + (e->radius_width + half_thickness) * ct;
                float y0 = e->point.y + (e->radius_height + half_thickness) * st;

                float x1 = e->point.x + (e->radius_width - half_thickness) * ct;
                float y1 = e->point.y + (e->radius_height - half_thickness) * st;

                GP_CALL( gp_mesh_push_position, (_mesh, vertex_iterator + 0, x0, y0) );
                GP_CALL( gp_mesh_push_color, (_mesh, vertex_iterator + 0, argb) );
                GP_CALL( gp_mesh_push_uv_map, (_state, _mesh, vertex_iterator + 0, x0, y0, u_offset, v_offset, total_width, total_height) );

                GP_CALL( gp_mesh_push_position, (_mesh, vertex_iterator + 1, x1, y1) );
                GP_CALL( gp_mesh_push_color, (_mesh, vertex_iterator + 1, argb) );
                GP_CALL( gp_mesh_push_uv_map, (_state, _mesh, vertex_iterator + 1, x1, y1, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;
            }
        }
    }
    else
    {
        if( line_penumbra > 0.f )
        {
            float line_half_thickness_soft = half_thickness - line_penumbra;

            gp_uint32_t ellipse_quality2 = ellipse_quality * 2;

            for( gp_uint16_t index = 0; index != ellipse_quality; ++index )
            {
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 2 + 0) % ellipse_quality2 + 1) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 1, vertex_iterator + (index * 2 + 1) % ellipse_quality2 + 1) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 2, vertex_iterator + (index * 2 + 2) % ellipse_quality2 + 1) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 3, vertex_iterator + (index * 2 + 2) % ellipse_quality2 + 1) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 4, vertex_iterator + (index * 2 + 1) % ellipse_quality2 + 1) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 5, vertex_iterator + (index * 2 + 3) % ellipse_quality2 + 1) );

                index_iterator += 6;
            }

            for( gp_uint16_t index = 0; index != ellipse_quality; ++index )
            {
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 0, vertex_iterator + (index * 2 + 1) % ellipse_quality2 + 1) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 1, vertex_iterator + (index * 2 + 3) % ellipse_quality2 + 1) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 2, vertex_iterator) );

                index_iterator += 3;
            }

            GP_CALL( gp_mesh_push_position, (_mesh, vertex_iterator + 0, e->point.x, e->point.y) );
            GP_CALL( gp_mesh_push_color, (_mesh, vertex_iterator + 0, argb) );
            GP_CALL( gp_mesh_push_uv_map, (_state, _mesh, vertex_iterator + 0, e->point.x, e->point.y, u_offset, v_offset, total_width, total_height) );

            vertex_iterator += 1;

            float t = 0.f;

            for( gp_uint32_t index = 0; index != ellipse_quality; ++index, t += dt )
            {
                float ct = GP_MATH_COSF( t );
                float st = GP_MATH_SINF( t );

                float x = e->point.x + (e->radius_width + half_thickness) * ct;
                float y = e->point.y + (e->radius_height + half_thickness) * st;

                float x_soft = e->point.x + (e->radius_width + line_half_thickness_soft) * ct;
                float y_soft = e->point.y + (e->radius_height + line_half_thickness_soft) * st;

                GP_CALL( gp_mesh_push_position, (_mesh, vertex_iterator + 0, x, y) );
                GP_CALL( gp_mesh_push_color, (_mesh, vertex_iterator + 0, argb & 0x00ffffff) );
                GP_CALL( gp_mesh_push_uv_map, (_state, _mesh, vertex_iterator + 0, x, y, u_offset, v_offset, total_width, total_height) );

                GP_CALL( gp_mesh_push_position, (_mesh, vertex_iterator + 1, x_soft, y_soft) );
                GP_CALL( gp_mesh_push_color, (_mesh, vertex_iterator + 1, argb) );
                GP_CALL( gp_mesh_push_uv_map, (_state, _mesh, vertex_iterator + 1, x_soft, y_soft, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 2;
            }
        }
        else
        {
            for( gp_uint16_t index = 0; index != ellipse_quality; ++index )
            {
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 0, vertex_iterator + (index + 0) % ellipse_quality + 1) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 1, vertex_iterator + (index + 1) % ellipse_quality + 1) );
                GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 2, vertex_iterator) );

                index_iterator += 3;
            }

            GP_CALL( gp_mesh_push_position, (_mesh, vertex_iterator + 0, e->point.x, e->point.y) );
            GP_CALL( gp_mesh_push_color, (_mesh, vertex_iterator + 0, argb) );
            GP_CALL( gp_mesh_push_uv_map, (_state, _mesh, vertex_iterator + 0, e->point.x, e->point.y, u_offset, v_offset, total_width, total_height) );

            vertex_iterator += 1;

            float t = 0.f;

            for( gp_uint32_t index = 0; index != ellipse_quality; ++index, t += dt )
            {
                float ct = GP_MATH_COSF( t );
                float st = GP_MATH_SINF( t );

                float x = e->point.x + (e->radius_width + half_thickness) * ct;
                float y = e->point.y + (e->radius_height + half_thickness) * st;

                GP_CALL( gp_mesh_push_position, (_mesh, vertex_iterator + 0, x, y) );
                GP_CALL( gp_mesh_push_color, (_mesh, vertex_iterator + 0, argb) );
                GP_CALL( gp_mesh_push_uv_map, (_state, _mesh, vertex_iterator + 0, x, y, u_offset, v_offset, total_width, total_height) );

                vertex_iterator += 1;
            }
        }
    }
    * _vertex_iterator = vertex_iterator;
    *_index_iterator = index_iterator;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __ellipse_arc_render_rounded_cap( const gp_ellipse_arc_t * _arc, const gp_state_t * _state, const gp_mesh_t * _mesh, gp_argb_t _color, float _angle, float _progress_direction, float _cap_direction, gp_uint32_t _outer_vertex, gp_uint32_t _inner_vertex, gp_bool_t _reverse, float _u_offset, float _v_offset, float _total_width, float _total_height, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator )
{
    gp_uint32_t cap_segment_count = _arc->state->ellipse_quality / 2;
    gp_uint32_t cap_vertex_begin = *_vertex_iterator;
    float radial_x = GP_MATH_COSF( _angle );
    float radial_y = GP_MATH_SINF( _angle );
    float direction_x = -radial_y * _progress_direction;
    float direction_y = radial_x * _progress_direction;
    float inner_radius = _arc->radius_width * _arc->state->inner_radius;
    float cap_radius = (_arc->radius_width - inner_radius) * 0.5f;
    float middle_radius = (_arc->radius_width + inner_radius) * 0.5f;
    float middle_x = _arc->point.x + radial_x * middle_radius;
    float middle_y = _arc->point.y + radial_y * middle_radius;

    for( gp_uint32_t index = 1; index != cap_segment_count; ++index )
    {
        float cap_angle = gp_constant_pi * (float)index / (float)cap_segment_count;
        float cap_cos = GP_MATH_COSF( cap_angle );
        float cap_sin = GP_MATH_SINF( cap_angle ) * _cap_direction;
        float x = middle_x + (radial_x * cap_cos + direction_x * cap_sin) * cap_radius;
        float y = middle_y + (radial_y * cap_cos + direction_y * cap_sin) * cap_radius;

        GP_CALL( gp_mesh_push_position, (_mesh, *_vertex_iterator, x, y) );
        GP_CALL( gp_mesh_push_color, (_mesh, *_vertex_iterator, _color) );
        GP_CALL( gp_mesh_push_uv_map, (_state, _mesh, *_vertex_iterator, x, y, _u_offset, _v_offset, _total_width, _total_height) );
        ++(*_vertex_iterator);
    }

    gp_uint32_t center_vertex = *_vertex_iterator;
    GP_CALL( gp_mesh_push_position, (_mesh, center_vertex, middle_x, middle_y) );
    GP_CALL( gp_mesh_push_color, (_mesh, center_vertex, _color) );
    GP_CALL( gp_mesh_push_uv_map, (_state, _mesh, center_vertex, middle_x, middle_y, _u_offset, _v_offset, _total_width, _total_height) );
    ++(*_vertex_iterator);

    for( gp_uint32_t index = 0; index != cap_segment_count; ++index )
    {
        gp_uint32_t current_vertex = index == 0
            ? _outer_vertex
            : cap_vertex_begin + index - 1;
        gp_uint32_t next_vertex = index + 1 == cap_segment_count
            ? _inner_vertex
            : cap_vertex_begin + index;

        GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 0, center_vertex) );
        GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 1, _reverse == GP_TRUE ? next_vertex : current_vertex) );
        GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 2, _reverse == GP_TRUE ? current_vertex : next_vertex) );
        *_index_iterator += 3;
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_render_ellipse_arc( const gp_ellipse_arc_t * _ellipse_arc, const gp_state_t * _state, const gp_mesh_t * _mesh, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator )
{
    gp_uint32_t vertex_iterator = *_vertex_iterator;
    gp_uint32_t index_iterator = *_index_iterator;

    const gp_ellipse_arc_t * a = _ellipse_arc;
    if( a == GP_NULLPTR )
    {
        return GP_SUCCESSFUL;
    }

    float angle_delta = a->angle_to - a->angle_from;
    if( GP_EQUAL_F( angle_delta, 0.000001f ) == GP_TRUE )
    {
        return GP_SUCCESSFUL;
    }

    gp_uint32_t segment_count = gp_ellipse_arc_segment_count( angle_delta, a->state->ellipse_quality );

    gp_color_t total_color;
    gp_color_mul( &total_color, &_mesh->color, &a->state->color );
    gp_argb_t argb = gp_color_argb( &total_color );
    gp_uint32_t body_vertex_begin = vertex_iterator;

    float inner_ratio = a->state->inner_radius;
    if( inner_ratio < 0.f )
    {
        inner_ratio = 0.f;
    }
    else if( inner_ratio > 1.f )
    {
        inner_ratio = 1.f;
    }

    float total_width = a->radius_width * 2.f;
    float total_height = a->radius_height * 2.f;
    float u_offset = -(a->point.x - a->radius_width);
    float v_offset = -(a->point.y - a->radius_height);

    if( inner_ratio > 0.0001f )
    {
        for( gp_uint32_t index = 0; index != segment_count; ++index )
        {
            GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 0, vertex_iterator + index * 2 + 0) );
            GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 1, vertex_iterator + index * 2 + 1) );
            GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 2, vertex_iterator + index * 2 + 2) );
            GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 3, vertex_iterator + index * 2 + 2) );
            GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 4, vertex_iterator + index * 2 + 1) );
            GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 5, vertex_iterator + index * 2 + 3) );

            index_iterator += 6;
        }

        for( gp_uint32_t index = 0; index != segment_count + 1; ++index )
        {
            float angle = gp_ellipse_arc_sample_angle( a->angle_from, a->angle_to, a->state->ellipse_quality, index );
            float ct = GP_MATH_COSF( angle );
            float st = GP_MATH_SINF( angle );

            float x0 = a->point.x + a->radius_width * ct;
            float y0 = a->point.y + a->radius_height * st;
            float x1 = a->point.x + a->radius_width * inner_ratio * ct;
            float y1 = a->point.y + a->radius_height * inner_ratio * st;

            GP_CALL( gp_mesh_push_position, (_mesh, vertex_iterator + 0, x0, y0) );
            GP_CALL( gp_mesh_push_color, (_mesh, vertex_iterator + 0, argb) );
            GP_CALL( gp_mesh_push_uv_map, (_state, _mesh, vertex_iterator + 0, x0, y0, u_offset, v_offset, total_width, total_height) );

            GP_CALL( gp_mesh_push_position, (_mesh, vertex_iterator + 1, x1, y1) );
            GP_CALL( gp_mesh_push_color, (_mesh, vertex_iterator + 1, argb) );
            GP_CALL( gp_mesh_push_uv_map, (_state, _mesh, vertex_iterator + 1, x1, y1, u_offset, v_offset, total_width, total_height) );

            vertex_iterator += 2;
        }
    }
    else
    {
        for( gp_uint32_t index = 0; index != segment_count; ++index )
        {
            GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 0, vertex_iterator) );
            GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 1, vertex_iterator + index + 1) );
            GP_CALL( gp_mesh_push_index, (_mesh, index_iterator + 2, vertex_iterator + index + 2) );

            index_iterator += 3;
        }

        GP_CALL( gp_mesh_push_position, (_mesh, vertex_iterator + 0, a->point.x, a->point.y) );
        GP_CALL( gp_mesh_push_color, (_mesh, vertex_iterator + 0, argb) );
        GP_CALL( gp_mesh_push_uv_map, (_state, _mesh, vertex_iterator + 0, a->point.x, a->point.y, u_offset, v_offset, total_width, total_height) );

        vertex_iterator += 1;

        for( gp_uint32_t index = 0; index != segment_count + 1; ++index )
        {
            float angle = gp_ellipse_arc_sample_angle( a->angle_from, a->angle_to, a->state->ellipse_quality, index );
            float x = a->point.x + a->radius_width * GP_MATH_COSF( angle );
            float y = a->point.y + a->radius_height * GP_MATH_SINF( angle );

            GP_CALL( gp_mesh_push_position, (_mesh, vertex_iterator + 0, x, y) );
            GP_CALL( gp_mesh_push_color, (_mesh, vertex_iterator + 0, argb) );
            GP_CALL( gp_mesh_push_uv_map, (_state, _mesh, vertex_iterator + 0, x, y, u_offset, v_offset, total_width, total_height) );

            vertex_iterator += 1;
        }
    }

    if( __ellipse_arc_has_rounded_caps( a ) == GP_TRUE )
    {
        gp_uint32_t start_outer_vertex;
        gp_uint32_t start_inner_vertex;
        gp_uint32_t end_outer_vertex;
        gp_uint32_t end_inner_vertex;
        if( inner_ratio > 0.0001f )
        {
            start_outer_vertex = body_vertex_begin + 0;
            start_inner_vertex = body_vertex_begin + 1;
            end_outer_vertex = body_vertex_begin + segment_count * 2;
            end_inner_vertex = end_outer_vertex + 1;
        }
        else
        {
            start_inner_vertex = body_vertex_begin;
            end_inner_vertex = body_vertex_begin;
            start_outer_vertex = body_vertex_begin + 1;
            end_outer_vertex = body_vertex_begin + segment_count + 1;
        }

        float progress_direction = angle_delta < 0.f ? -1.f : 1.f;
        GP_CALL( __ellipse_arc_render_rounded_cap, (a, _state, _mesh, argb, a->angle_from, progress_direction, -1.f, start_outer_vertex, start_inner_vertex, GP_FALSE, u_offset, v_offset, total_width, total_height, &vertex_iterator, &index_iterator) );
        GP_CALL( __ellipse_arc_render_rounded_cap, (a, _state, _mesh, argb, a->angle_to, progress_direction, 1.f, end_outer_vertex, end_inner_vertex, GP_TRUE, u_offset, v_offset, total_width, total_height, &vertex_iterator, &index_iterator) );
    }

    * _vertex_iterator = vertex_iterator;
    *_index_iterator = index_iterator;

    return GP_SUCCESSFUL;
}
