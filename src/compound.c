#include "compound.h"

#include "detail.h"

//////////////////////////////////////////////////////////////////////////
typedef struct gp_compound_point_t
{
    gp_vec2f_t position;
    gp_argb_t color;
} gp_compound_point_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_compound_contour_t
{
    gp_compound_point_t * points;
    gp_uint32_t count;
    float area;
    gp_uint32_t parent;
    gp_uint32_t depth;
    int winding_before;
    int winding_inside;
    gp_bool_t outer;
    gp_bool_t hole;
} gp_compound_contour_t;
//////////////////////////////////////////////////////////////////////////
typedef struct gp_compound_data_t
{
    gp_compound_contour_t * contours;
    gp_uint32_t contour_count;
    gp_realloc_t realloc;
    gp_free_t free;
    void * ud;
} gp_compound_data_t;
//////////////////////////////////////////////////////////////////////////
static float __compound_lerp( float _a, float _b, float _t )
{
    return _a * (1.f - _t) + _b * _t;
}
//////////////////////////////////////////////////////////////////////////
static void __compound_lerp_color( gp_color_t * _result, const gp_color_t * _a, const gp_color_t * _b, float _t )
{
    _result->r = __compound_lerp( _a->r, _b->r, _t );
    _result->g = __compound_lerp( _a->g, _b->g, _t );
    _result->b = __compound_lerp( _a->b, _b->b, _t );
    _result->a = __compound_lerp( _a->a, _b->a, _t );
}
//////////////////////////////////////////////////////////////////////////
static void __compound_bezier( gp_vec2f_t * _result, const gp_vec2f_t * _from, const gp_vec2f_t * _to, const gp_vec2f_t * _controls, gp_uint32_t _control_count, float _t )
{
    float mt = 1.f - _t;
    if( _control_count == 1 )
    {
        _result->x = mt * mt * _from->x + 2.f * mt * _t * _controls[0].x + _t * _t * _to->x;
        _result->y = mt * mt * _from->y + 2.f * mt * _t * _controls[0].y + _t * _t * _to->y;
    }
    else
    {
        _result->x = mt * mt * mt * _from->x + 3.f * mt * mt * _t * _controls[0].x + 3.f * mt * _t * _t * _controls[1].x + _t * _t * _t * _to->x;
        _result->y = mt * mt * mt * _from->y + 3.f * mt * mt * _t * _controls[0].y + 3.f * mt * _t * _t * _controls[1].y + _t * _t * _t * _to->y;
    }
}
//////////////////////////////////////////////////////////////////////////
static gp_uint32_t __compound_line_point_count( const gp_line_t * _line )
{
    gp_uint32_t count = 1;
    gp_uint8_t quality = _line->state->curve_quality;
    for( const gp_line_edge_t * edge = _line->edges; edge != GP_NULLPTR; edge = edge->next )
    {
        if( edge->controls == 0 )
        {
            count += 1;
        }
        else if( edge->controls == 3 )
        {
            float angle_delta = edge->p[2].y - edge->p[2].x;
            count += gp_ellipse_arc_segment_count( angle_delta, _line->state->ellipse_quality );
        }
        else
        {
            count += quality;
        }
    }

    return count;
}
//////////////////////////////////////////////////////////////////////////
static void __compound_write_color( gp_compound_point_t * _point, const gp_mesh_t * _mesh, const gp_color_t * _color )
{
    if( _mesh == GP_NULLPTR )
    {
        _point->color = 0xffffffffU;
        return;
    }

    gp_color_t color;
    gp_color_mul( &color, &_mesh->color, _color );
    _point->color = gp_color_argb( &color );
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __compound_flatten_line( const gp_line_t * _line, const gp_mesh_t * _mesh, gp_compound_contour_t * _contour, gp_realloc_t _realloc, void * _ud )
{
    _contour->count = __compound_line_point_count( _line );
    _contour->points = (gp_compound_point_t *)(*_realloc)(GP_NULLPTR, sizeof( gp_compound_point_t ) * _contour->count, _ud);
    if( _contour->points == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    gp_uint32_t write = 0;
    const gp_line_point_t * point = _line->points;
    gp_uint8_t quality = _line->state->curve_quality;
    float quality_inv = _line->state->curve_quality_inv;

    for( const gp_line_edge_t * edge = _line->edges; edge != GP_NULLPTR; edge = edge->next )
    {
        const gp_line_point_t * from = point;
        point = point->next;
        const gp_line_point_t * to = point;

        if( edge->controls == 0 )
        {
            _contour->points[write].position = from->p;
            __compound_write_color( _contour->points + write, _mesh, &from->state->color );
            ++write;
            continue;
        }

        if( edge->controls == 3 )
        {
            float angle_delta = edge->p[2].y - edge->p[2].x;
            gp_uint32_t segment_count = gp_ellipse_arc_segment_count( angle_delta, _line->state->ellipse_quality );

            for( gp_uint32_t index = 0; index != segment_count; ++index )
            {
                float t = gp_ellipse_arc_sample_t( angle_delta, _line->state->ellipse_quality, index );
                float angle = gp_ellipse_arc_sample_angle( edge->p[2].x, edge->p[2].y, _line->state->ellipse_quality, index );
                _contour->points[write].position.x = edge->p[0].x + GP_MATH_COSF( angle ) * edge->p[1].x;
                _contour->points[write].position.y = edge->p[0].y + GP_MATH_SINF( angle ) * edge->p[1].y;

                gp_color_t color;
                __compound_lerp_color( &color, &from->state->color, &to->state->color, t );
                __compound_write_color( _contour->points + write, _mesh, &color );
                ++write;
            }

            continue;
        }

        for( gp_uint8_t index = 0; index != quality; ++index )
        {
            float t = (float)index * quality_inv;
            __compound_bezier( &_contour->points[write].position, &from->p, &to->p, edge->p, edge->controls, t );

            gp_color_t color;
            __compound_lerp_color( &color, &from->state->color, &to->state->color, t );
            __compound_write_color( _contour->points + write, _mesh, &color );
            ++write;
        }
    }

    _contour->points[write].position = point->p;
    __compound_write_color( _contour->points + write, _mesh, &point->state->color );

    _contour->area = 0.f;
    for( gp_uint32_t index = 0; index != _contour->count; ++index )
    {
        const gp_vec2f_t * p0 = &_contour->points[index].position;
        const gp_vec2f_t * p1 = &_contour->points[(index + 1) % _contour->count].position;
        _contour->area += p0->x * p1->y - p0->y * p1->x;
    }
    _contour->area *= 0.5f;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static gp_bool_t __compound_contains( const gp_compound_contour_t * _contour, const gp_vec2f_t * _point )
{
    gp_bool_t inside = GP_FALSE;
    for( gp_uint32_t index = 0, previous = _contour->count - 1; index != _contour->count; previous = index++ )
    {
        const gp_vec2f_t * p0 = &_contour->points[index].position;
        const gp_vec2f_t * p1 = &_contour->points[previous].position;
        gp_bool_t crosses = ((p0->y > _point->y) != (p1->y > _point->y)) ? GP_TRUE : GP_FALSE;
        if( crosses == GP_TRUE )
        {
            float x = (p1->x - p0->x) * (_point->y - p0->y) / (p1->y - p0->y) + p0->x;
            if( _point->x < x )
            {
                inside = inside == GP_TRUE ? GP_FALSE : GP_TRUE;
            }
        }
    }

    return inside;
}
//////////////////////////////////////////////////////////////////////////
static void __compound_classify( gp_compound_data_t * _data, gp_fill_rule_t _rule )
{
    for( gp_uint32_t index = 0; index != _data->contour_count; ++index )
    {
        gp_compound_contour_t * contour = _data->contours + index;
        contour->parent = UINT32_MAX;
        float contour_area = contour->area < 0.f ? -contour->area : contour->area;
        float parent_area = 0.f;

        for( gp_uint32_t candidate_index = 0; candidate_index != _data->contour_count; ++candidate_index )
        {
            if( candidate_index == index )
            {
                continue;
            }

            gp_compound_contour_t * candidate = _data->contours + candidate_index;
            float candidate_area = candidate->area < 0.f ? -candidate->area : candidate->area;
            if( candidate_area <= contour_area )
            {
                continue;
            }

            if( __compound_contains( candidate, &contour->points[0].position ) == GP_FALSE )
            {
                continue;
            }

            if( contour->parent == UINT32_MAX || candidate_area < parent_area )
            {
                contour->parent = candidate_index;
                parent_area = candidate_area;
            }
        }
    }

    for( gp_uint32_t index = 0; index != _data->contour_count; ++index )
    {
        gp_compound_contour_t * contour = _data->contours + index;
        contour->depth = 0;
        contour->winding_before = 0;

        gp_uint32_t parent = contour->parent;
        while( parent != UINT32_MAX )
        {
            const gp_compound_contour_t * parent_contour = _data->contours + parent;
            ++contour->depth;
            contour->winding_before += parent_contour->area >= 0.f ? 1 : -1;
            parent = parent_contour->parent;
        }

        contour->winding_inside = contour->winding_before + (contour->area >= 0.f ? 1 : -1);
        if( _rule == GP_FILL_RULE_EVEN_ODD )
        {
            contour->outer = contour->depth % 2 == 0 ? GP_TRUE : GP_FALSE;
            contour->hole = contour->depth % 2 == 1 ? GP_TRUE : GP_FALSE;
        }
        else
        {
            contour->outer = contour->winding_before == 0 && contour->winding_inside != 0 ? GP_TRUE : GP_FALSE;
            contour->hole = contour->winding_before != 0 && contour->winding_inside == 0 ? GP_TRUE : GP_FALSE;
        }
    }
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __compound_build( const gp_compound_fill_t * _compound, const gp_mesh_t * _mesh, gp_compound_data_t * _data )
{
    const gp_line_t * first = _compound->first_line;
    _data->realloc = first->realloc;
    _data->free = first->free;
    _data->ud = first->ud;
    _data->contour_count = 0;

    for( const gp_line_t * line = first; line != GP_NULLPTR; line = line->next )
    {
        if( line->compound_fill == _compound )
        {
            ++_data->contour_count;
        }
        if( line == _compound->last_line )
        {
            break;
        }
    }

    _data->contours = (gp_compound_contour_t *)(*_data->realloc)(GP_NULLPTR, sizeof( gp_compound_contour_t ) * _data->contour_count, _data->ud);
    if( _data->contours == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    gp_uint32_t index = 0;
    for( const gp_line_t * line = first; line != GP_NULLPTR; line = line->next )
    {
        if( line->compound_fill == _compound )
        {
#if defined(GP_DEBUG)
            if( line->edges == GP_NULLPTR )
            {
                return GP_FAILURE;
            }
#endif
            GP_CALL( __compound_flatten_line, (line, _mesh, _data->contours + index, _data->realloc, _data->ud) );
            ++index;
        }
        if( line == _compound->last_line )
        {
            break;
        }
    }

    __compound_classify( _data, _compound->fill_rule );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static void __compound_destroy( gp_compound_data_t * _data )
{
    for( gp_uint32_t index = 0; index != _data->contour_count; ++index )
    {
        (*_data->free)(_data->contours[index].points, _data->ud);
    }
    (*_data->free)(_data->contours, _data->ud);
}
//////////////////////////////////////////////////////////////////////////
static gp_uint32_t __compound_owner_outer( const gp_compound_data_t * _data, gp_uint32_t _hole_index )
{
    gp_uint32_t parent = _data->contours[_hole_index].parent;
    while( parent != UINT32_MAX )
    {
        if( _data->contours[parent].outer == GP_TRUE )
        {
            return parent;
        }
        parent = _data->contours[parent].parent;
    }

    return UINT32_MAX;
}
//////////////////////////////////////////////////////////////////////////
static gp_uint32_t __compound_order_count( const gp_compound_data_t * _data, gp_uint32_t _outer_index )
{
    gp_uint32_t count = _data->contours[_outer_index].count;
    for( gp_uint32_t index = 0; index != _data->contour_count; ++index )
    {
        if( _data->contours[index].hole == GP_TRUE && __compound_owner_outer( _data, index ) == _outer_index )
        {
            count += _data->contours[index].count + 2;
        }
    }

    return count;
}
//////////////////////////////////////////////////////////////////////////
static gp_uint32_t __compound_global_offset( const gp_compound_data_t * _data, gp_uint32_t _contour_index )
{
    gp_uint32_t offset = 0;
    for( gp_uint32_t index = 0; index != _contour_index; ++index )
    {
        offset += _data->contours[index].count;
    }

    return offset;
}
//////////////////////////////////////////////////////////////////////////
static const gp_vec2f_t * __compound_get_position( const gp_compound_data_t * _data, gp_uint32_t _global_index )
{
    gp_uint32_t offset = 0;
    for( gp_uint32_t index = 0; index != _data->contour_count; ++index )
    {
        const gp_compound_contour_t * contour = _data->contours + index;
        if( _global_index < offset + contour->count )
        {
            return &contour->points[_global_index - offset].position;
        }
        offset += contour->count;
    }

    return GP_NULLPTR;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __compound_make_order( const gp_compound_data_t * _data, gp_uint32_t _outer_index, gp_uint32_t ** _order, gp_uint32_t * _order_count )
{
    gp_uint32_t capacity = __compound_order_count( _data, _outer_index );
    gp_uint32_t * order = (gp_uint32_t *)(*_data->realloc)(GP_NULLPTR, sizeof( gp_uint32_t ) * capacity, _data->ud);
    if( order == GP_NULLPTR )
    {
        return GP_FAILURE;
    }

    const gp_compound_contour_t * outer = _data->contours + _outer_index;
    gp_uint32_t outer_offset = __compound_global_offset( _data, _outer_index );
    gp_uint32_t count = outer->count;
    for( gp_uint32_t index = 0; index != count; ++index )
    {
        order[index] = outer_offset + index;
    }

    for( gp_uint32_t hole_index = 0; hole_index != _data->contour_count; ++hole_index )
    {
        const gp_compound_contour_t * hole = _data->contours + hole_index;
        if( hole->hole == GP_FALSE || __compound_owner_outer( _data, hole_index ) != _outer_index )
        {
            continue;
        }

        gp_uint32_t hole_vertex = 0;
        for( gp_uint32_t index = 1; index != hole->count; ++index )
        {
            const gp_vec2f_t * candidate = &hole->points[index].position;
            const gp_vec2f_t * current = &hole->points[hole_vertex].position;
            if( candidate->x > current->x || (candidate->x == current->x && candidate->y < current->y) )
            {
                hole_vertex = index;
            }
        }

        const gp_vec2f_t * hole_position = &hole->points[hole_vertex].position;
        gp_uint32_t bridge_position = 0;
        float bridge_distance = 0.f;
        for( gp_uint32_t index = 0; index != count; ++index )
        {
            const gp_vec2f_t * candidate = __compound_get_position( _data, order[index] );
            float dx = candidate->x - hole_position->x;
            float dy = candidate->y - hole_position->y;
            float distance = dx * dx + dy * dy;
            if( index == 0 || distance < bridge_distance )
            {
                bridge_position = index;
                bridge_distance = distance;
            }
        }

        gp_uint32_t insert_count = hole->count + 2;
        for( gp_uint32_t index = count; index > bridge_position + 1; --index )
        {
            order[index + insert_count - 1] = order[index - 1];
        }

        gp_uint32_t hole_offset = __compound_global_offset( _data, hole_index );
        gp_bool_t reverse = (outer->area >= 0.f) == (hole->area >= 0.f) ? GP_TRUE : GP_FALSE;
        gp_uint32_t write = bridge_position + 1;
        for( gp_uint32_t index = 0; index != hole->count; ++index )
        {
            gp_uint32_t local;
            if( reverse == GP_TRUE )
            {
                local = (hole_vertex + hole->count - index) % hole->count;
            }
            else
            {
                local = (hole_vertex + index) % hole->count;
            }
            order[write++] = hole_offset + local;
        }
        order[write++] = hole_offset + hole_vertex;
        order[write++] = order[bridge_position];
        count += insert_count;
    }

    *_order_count = count;
    *_order = order;

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static float __compound_triangle_area( const gp_vec2f_t * _a, const gp_vec2f_t * _b, const gp_vec2f_t * _c )
{
    return (_b->x - _a->x) * (_c->y - _a->y) - (_b->y - _a->y) * (_c->x - _a->x);
}
//////////////////////////////////////////////////////////////////////////
static gp_bool_t __compound_same_point( const gp_vec2f_t * _a, const gp_vec2f_t * _b )
{
    return GP_EQUAL_F( _a->x - _b->x, 0.00001f ) && GP_EQUAL_F( _a->y - _b->y, 0.00001f );
}
//////////////////////////////////////////////////////////////////////////
static gp_bool_t __compound_point_in_triangle( const gp_vec2f_t * _point, const gp_vec2f_t * _a, const gp_vec2f_t * _b, const gp_vec2f_t * _c )
{
    if( __compound_same_point( _point, _a ) == GP_TRUE || __compound_same_point( _point, _b ) == GP_TRUE || __compound_same_point( _point, _c ) == GP_TRUE )
    {
        return GP_FALSE;
    }

    float c0 = __compound_triangle_area( _a, _b, _point );
    float c1 = __compound_triangle_area( _b, _c, _point );
    float c2 = __compound_triangle_area( _c, _a, _point );
    gp_bool_t negative = c0 < -0.00001f || c1 < -0.00001f || c2 < -0.00001f ? GP_TRUE : GP_FALSE;
    gp_bool_t positive = c0 > 0.00001f || c1 > 0.00001f || c2 > 0.00001f ? GP_TRUE : GP_FALSE;

    return negative == GP_TRUE && positive == GP_TRUE ? GP_FALSE : GP_TRUE;
}
//////////////////////////////////////////////////////////////////////////
static float __compound_order_area( const gp_compound_data_t * _data, const gp_uint32_t * _order, gp_uint32_t _count )
{
    float area = 0.f;
    for( gp_uint32_t index = 0; index != _count; ++index )
    {
        const gp_vec2f_t * p0 = __compound_get_position( _data, _order[index] );
        const gp_vec2f_t * p1 = __compound_get_position( _data, _order[(index + 1) % _count] );
        area += p0->x * p1->y - p0->y * p1->x;
    }

    return area;
}
//////////////////////////////////////////////////////////////////////////
static gp_result_t __compound_triangulate( const gp_compound_data_t * _data, gp_uint32_t * _order, gp_uint32_t _count, const gp_mesh_t * _mesh, gp_uint32_t _vertex_begin, gp_uint32_t * _index_iterator )
{
    gp_uint32_t remaining = _count;
    gp_bool_t ccw = __compound_order_area( _data, _order, _count ) >= 0.f ? GP_TRUE : GP_FALSE;

    while( remaining > 2 )
    {
        gp_bool_t clipped = GP_FALSE;
        for( gp_uint32_t index = 0; index != remaining; ++index )
        {
            gp_uint32_t i0 = _order[(index + remaining - 1) % remaining];
            gp_uint32_t i1 = _order[index];
            gp_uint32_t i2 = _order[(index + 1) % remaining];
            const gp_vec2f_t * p0 = __compound_get_position( _data, i0 );
            const gp_vec2f_t * p1 = __compound_get_position( _data, i1 );
            const gp_vec2f_t * p2 = __compound_get_position( _data, i2 );
            float area = __compound_triangle_area( p0, p1, p2 );
            if( (ccw == GP_TRUE && area <= 0.00001f) || (ccw == GP_FALSE && area >= -0.00001f) )
            {
                continue;
            }

            gp_bool_t contains = GP_FALSE;
            for( gp_uint32_t test = 0; test != remaining; ++test )
            {
                gp_uint32_t point_index = _order[test];
                if( point_index == i0 || point_index == i1 || point_index == i2 )
                {
                    continue;
                }
                if( __compound_point_in_triangle( __compound_get_position( _data, point_index ), p0, p1, p2 ) == GP_TRUE )
                {
                    contains = GP_TRUE;
                    break;
                }
            }
            if( contains == GP_TRUE )
            {
                continue;
            }

            GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 0, _vertex_begin + (ccw == GP_TRUE ? i0 : i2)) );
            GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 1, _vertex_begin + i1) );
            GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 2, _vertex_begin + (ccw == GP_TRUE ? i2 : i0)) );
            *_index_iterator += 3;

            for( gp_uint32_t move = index; move + 1 < remaining; ++move )
            {
                _order[move] = _order[move + 1];
            }
            --remaining;
            clipped = GP_TRUE;
            break;
        }

        if( clipped == GP_FALSE )
        {
            for( gp_uint32_t index = 1; index + 1 < remaining; ++index )
            {
                GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 0, _vertex_begin + _order[0]) );
                GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 1, _vertex_begin + _order[index]) );
                GP_CALL( gp_mesh_push_index, (_mesh, *_index_iterator + 2, _vertex_begin + _order[index + 1]) );
                *_index_iterator += 3;
            }
            break;
        }
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_calculate_mesh_compound_fill_size( const gp_canvas_t * _canvas, gp_mesh_t * _mesh )
{
    for( const gp_compound_fill_t * compound = _canvas->compound_fills; compound != GP_NULLPTR; compound = compound->next )
    {
        GP_CALL( gp_calculate_mesh_compound_size, (compound, _mesh) );
    }

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_calculate_mesh_compound_size( const gp_compound_fill_t * _compound, gp_mesh_t * _mesh )
{
    gp_compound_data_t data;
    GP_CALL( __compound_build, (_compound, GP_NULLPTR, &data) );

    for( gp_uint32_t index = 0; index != data.contour_count; ++index )
    {
        _mesh->vertex_count += data.contours[index].count;
        if( data.contours[index].outer == GP_TRUE )
        {
            gp_uint32_t order_count = __compound_order_count( &data, index );
            if( order_count >= 3 )
            {
                _mesh->index_count += (order_count - 2) * 3;
            }
        }
    }

    __compound_destroy( &data );

    return GP_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
gp_result_t gp_render_compound_fill( const gp_compound_fill_t * _compound, const gp_mesh_t * _mesh, gp_uint32_t * _vertex_iterator, gp_uint32_t * _index_iterator )
{
    gp_compound_data_t data;
    GP_CALL( __compound_build, (_compound, _mesh, &data) );

    gp_vec2f_t minimum = data.contours[0].points[0].position;
    gp_vec2f_t maximum = minimum;
    for( gp_uint32_t contour_index = 0; contour_index != data.contour_count; ++contour_index )
    {
        const gp_compound_contour_t * contour = data.contours + contour_index;
        for( gp_uint32_t index = 0; index != contour->count; ++index )
        {
            const gp_vec2f_t * position = &contour->points[index].position;
            minimum.x = position->x < minimum.x ? position->x : minimum.x;
            minimum.y = position->y < minimum.y ? position->y : minimum.y;
            maximum.x = position->x > maximum.x ? position->x : maximum.x;
            maximum.y = position->y > maximum.y ? position->y : maximum.y;
        }
    }
    float width = maximum.x - minimum.x;
    float height = maximum.y - minimum.y;

    gp_uint32_t vertex_begin = *_vertex_iterator;
    for( gp_uint32_t contour_index = 0; contour_index != data.contour_count; ++contour_index )
    {
        const gp_compound_contour_t * contour = data.contours + contour_index;
        for( gp_uint32_t index = 0; index != contour->count; ++index )
        {
            const gp_compound_point_t * point = contour->points + index;
            GP_CALL( gp_mesh_push_position, (_mesh, *_vertex_iterator, point->position.x, point->position.y) );
            GP_CALL( gp_mesh_push_color, (_mesh, *_vertex_iterator, point->color) );
            float u = width > 0.f ? (point->position.x - minimum.x) / width : 0.f;
            float v = height > 0.f ? (point->position.y - minimum.y) / height : 0.f;
            GP_CALL( gp_mesh_push_uv, (_compound->first_line->state, _mesh, *_vertex_iterator, u, v) );
            ++(*_vertex_iterator);
        }
    }

    for( gp_uint32_t contour_index = 0; contour_index != data.contour_count; ++contour_index )
    {
        if( data.contours[contour_index].outer == GP_FALSE )
        {
            continue;
        }

        gp_uint32_t order_count;
        gp_uint32_t * order;
        GP_CALL( __compound_make_order, (&data, contour_index, &order, &order_count) );
        GP_CALL( __compound_triangulate, (&data, order, order_count, _mesh, vertex_begin, _index_iterator) );
        (*data.free)(order, data.ud);
    }

    __compound_destroy( &data );

    return GP_SUCCESSFUL;
}
