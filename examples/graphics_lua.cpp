#include "graphics_lua.h"

//////////////////////////////////////////////////////////////////////////
static int __lua_panic( lua_State * L )
{
    example_script_handle_t * handle = *(example_script_handle_t **)lua_getextraspace( L );

    longjmp( handle->panic_jump, 1 );
}
//////////////////////////////////////////////////////////////////////////
static void * __lua_alloc( void * ud, void * ptr, size_t osize, size_t nsize )
{
    if( ptr == nullptr )
    {
        osize = 0;
    }

    if( nsize == 0 )
    {
        free( ptr );

        return nullptr;
    }

    void * nptr = realloc( ptr, nsize );

    return nptr;
}
//////////////////////////////////////////////////////////////////////////
static int __lua_get_line( lua_State * L )
{
    lua_Debug ar;
    lua_getstack( L, 1, &ar );
    lua_getinfo( L, "nSl", &ar );
    int line = ar.currentline;

    return line;
}
//////////////////////////////////////////////////////////////////////////
static int __lua_f_thickness( lua_State * L )
{
    int args_count = 1;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    example_script_handle_t * handle = *(example_script_handle_t **)lua_getextraspace( L );

    lua_Number thickness = lua_tonumber( L, 1 );

    if( gp_set_thickness( handle->canvas, (float)thickness ) == GP_FAILURE )
    {
        handle->result = GP_FAILURE;
        handle->error_msg = "invalid thickness";
        handle->error_line = __lua_get_line( L );
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
static int __lua_f_penumbra( lua_State * L )
{
    int args_count = 1;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    example_script_handle_t * handle = *(example_script_handle_t **)lua_getextraspace( L );

    lua_Number penumbra = lua_tonumber( L, 1 );

    if( gp_set_penumbra( handle->canvas, (float)penumbra ) == GP_FAILURE )
    {
        handle->result = GP_FAILURE;
        handle->error_msg = "invalid penumbra";
        handle->error_line = __lua_get_line( L );
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
static int __lua_f_outline_width( lua_State * L )
{
    int args_count = 1;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    example_script_handle_t * handle = *(example_script_handle_t **)lua_getextraspace( L );

    lua_Number width = lua_tonumber( L, 1 );

    if( gp_set_outline_width( handle->canvas, (float)width ) == GP_FAILURE )
    {
        handle->result = GP_FAILURE;
        handle->error_msg = "invalid outline width";
        handle->error_line = __lua_get_line( L );
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
static int __lua_f_color( lua_State * L )
{
    int args_count = 4;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    example_script_handle_t * handle = *(example_script_handle_t **)lua_getextraspace( L );

    lua_Number r = lua_tonumber( L, 1 );
    lua_Number g = lua_tonumber( L, 2 );
    lua_Number b = lua_tonumber( L, 3 );
    lua_Number a = lua_tonumber( L, 4 );

    if( gp_set_color( handle->canvas, (float)r, (float)g, (float)b, (float)a ) == GP_FAILURE )
    {
        handle->result = GP_FAILURE;
        handle->error_msg = "invalid color";
        handle->error_line = __lua_get_line( L );
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
static int __lua_f_outline_color( lua_State * L )
{
    int args_count = 4;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    example_script_handle_t * handle = *(example_script_handle_t **)lua_getextraspace( L );

    lua_Number r = lua_tonumber( L, 1 );
    lua_Number g = lua_tonumber( L, 2 );
    lua_Number b = lua_tonumber( L, 3 );
    lua_Number a = lua_tonumber( L, 4 );

    if( gp_set_outline_color( handle->canvas, (float)r, (float)g, (float)b, (float)a ) == GP_FAILURE )
    {
        handle->result = GP_FAILURE;
        handle->error_msg = "invalid outline color";
        handle->error_line = __lua_get_line( L );
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
static int __lua_f_begin_fill( lua_State * L )
{
    int args_count = 0;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    example_script_handle_t * handle = *(example_script_handle_t **)lua_getextraspace( L );

    if( gp_begin_fill( handle->canvas ) == GP_FAILURE )
    {
        handle->result = GP_FAILURE;
        handle->error_msg = "invalid begin fill";
        handle->error_line = __lua_get_line( L );
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
static int __lua_f_end_fill( lua_State * L )
{
    int args_count = 0;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    example_script_handle_t * handle = *(example_script_handle_t **)lua_getextraspace( L );

    if( gp_end_fill( handle->canvas ) == GP_FAILURE )
    {
        handle->result = GP_FAILURE;
        handle->error_msg = "invalid end fill";
        handle->error_line = __lua_get_line( L );
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
static int __lua_f_move_to( lua_State * L )
{
    int args_count = 2;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    example_script_handle_t * handle = *(example_script_handle_t **)lua_getextraspace( L );

    lua_Number x = lua_tonumber( L, 1 );
    lua_Number y = lua_tonumber( L, 2 );

    if( gp_move_to( handle->canvas, (float)x, (float)y ) == GP_FAILURE )
    {
        handle->result = GP_FAILURE;
        handle->error_msg = "invalid move to";
        handle->error_line = __lua_get_line( L );
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
static int __lua_f_line_to( lua_State * L )
{
    int args_count = 2;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    example_script_handle_t * handle = *(example_script_handle_t **)lua_getextraspace( L );

    lua_Number x = lua_tonumber( L, 1 );
    lua_Number y = lua_tonumber( L, 2 );

    if( gp_line_to( handle->canvas, (float)x, (float)y ) == GP_FAILURE )
    {
        handle->result = GP_FAILURE;
        handle->error_msg = "invalid line to";
        handle->error_line = __lua_get_line( L );
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
static int __lua_f_quadratic_curve_to( lua_State * L )
{
    int args_count = 4;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    example_script_handle_t * handle = *(example_script_handle_t **)lua_getextraspace( L );

    lua_Number p0x = lua_tonumber( L, 1 );
    lua_Number p0y = lua_tonumber( L, 2 );
    lua_Number x = lua_tonumber( L, 3 );
    lua_Number y = lua_tonumber( L, 4 );

    if( gp_quadratic_curve_to( handle->canvas, (float)p0x, (float)p0y, (float)x, (float)y ) == GP_FAILURE )
    {
        handle->result = GP_FAILURE;
        handle->error_msg = "invalid quadratic curve to";
        handle->error_line = __lua_get_line( L );
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
static int __lua_f_bezier_curve_to( lua_State * L )
{
    int args_count = 6;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    example_script_handle_t * handle = *(example_script_handle_t **)lua_getextraspace( L );

    lua_Number p0x = lua_tonumber( L, 1 );
    lua_Number p0y = lua_tonumber( L, 2 );
    lua_Number p1x = lua_tonumber( L, 3 );
    lua_Number p1y = lua_tonumber( L, 4 );
    lua_Number x = lua_tonumber( L, 5 );
    lua_Number y = lua_tonumber( L, 6 );

    if( gp_bezier_curve_to( handle->canvas, (float)p0x, (float)p0y, (float)p1x, (float)p1y, (float)x, (float)y ) == GP_FAILURE )
    {
        handle->result = GP_FAILURE;
        handle->error_msg = "invalid bezier curve to";
        handle->error_line = __lua_get_line( L );
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
static int __lua_f_rect( lua_State * L )
{
    int args_count = 4;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    example_script_handle_t * handle = *(example_script_handle_t **)lua_getextraspace( L );

    lua_Number x = lua_tonumber( L, 1 );
    lua_Number y = lua_tonumber( L, 2 );
    lua_Number w = lua_tonumber( L, 3 );
    lua_Number h = lua_tonumber( L, 4 );

    if( gp_rect( handle->canvas, (float)x, (float)y, (float)w, (float)h ) == GP_FAILURE )
    {
        handle->result = GP_FAILURE;
        handle->error_msg = "invalid rect to";
        handle->error_line = __lua_get_line( L );
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
static int __lua_f_rounded_rect( lua_State * L )
{
    int args_count = 5;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    example_script_handle_t * handle = *(example_script_handle_t **)lua_getextraspace( L );

    lua_Number x = lua_tonumber( L, 1 );
    lua_Number y = lua_tonumber( L, 2 );
    lua_Number w = lua_tonumber( L, 3 );
    lua_Number h = lua_tonumber( L, 4 );
    lua_Number r = lua_tonumber( L, 5 );

    if( gp_rounded_rect( handle->canvas, (float)x, (float)y, (float)w, (float)h, (float)r ) == GP_FAILURE )
    {
        handle->result = GP_FAILURE;
        handle->error_msg = "invalid rounded rect to";
        handle->error_line = __lua_get_line( L );
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
static int __lua_f_circle( lua_State * L )
{
    int args_count = 3;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    example_script_handle_t * handle = *(example_script_handle_t **)lua_getextraspace( L );

    lua_Number x = lua_tonumber( L, 1 );
    lua_Number y = lua_tonumber( L, 2 );
    lua_Number r = lua_tonumber( L, 3 );

    if( gp_circle( handle->canvas, (float)x, (float)y, (float)r ) == GP_FAILURE )
    {
        handle->result = GP_FAILURE;
        handle->error_msg = "invalid circle to";
        handle->error_line = __lua_get_line( L );
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
static int __lua_f_ellipse( lua_State * L )
{
    int args_count = 4;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    example_script_handle_t * handle = *(example_script_handle_t **)lua_getextraspace( L );

    lua_Number x = lua_tonumber( L, 1 );
    lua_Number y = lua_tonumber( L, 2 );
    lua_Number w = lua_tonumber( L, 3 );
    lua_Number h = lua_tonumber( L, 4 );

    if( gp_ellipse( handle->canvas, (float)x, (float)y, (float)w, (float)h ) == GP_FAILURE )
    {
        handle->result = GP_FAILURE;
        handle->error_msg = "invalid ellipse to";
        handle->error_line = __lua_get_line( L );
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
static int __lua_call( example_script_handle_t * _handle )
{
    if( setjmp( _handle->panic_jump ) == 1 )
    {
        /* recovered from panic. log and return */

        return -1;
    }

    int result = lua_pcallk( _handle->L, 0, 0, 0, 0, nullptr );

    return result;
}
//////////////////////////////////////////////////////////////////////////
static const struct luaL_Reg lua_functions[] = {
    {"thickness", &__lua_f_thickness}
    , {"penumbra", &__lua_f_penumbra}
    , {"outline_width", &__lua_f_outline_width}
    , {"color", &__lua_f_color}
    , {"outline_color", &__lua_f_outline_color}
    , {"begin_fill", &__lua_f_begin_fill}
    , {"end_fill", &__lua_f_end_fill}
    , {"move_to", &__lua_f_move_to}
    , {"line_to", &__lua_f_line_to}
    , {"quadratic_curve_to", &__lua_f_quadratic_curve_to}
    , {"bezier_curve_to", &__lua_f_bezier_curve_to}
    , {"rect", &__lua_f_rect}
    , {"rounded_rect", &__lua_f_rounded_rect}
    , {"circle", &__lua_f_circle}
    , {"ellipse", &__lua_f_ellipse}

    , {NULL, NULL} /* end of array */
};
//////////////////////////////////////////////////////////////////////////
bool initialize_script( example_script_handle_t ** _handle, gp_canvas_t * _canvas )
{
    lua_State * L = lua_newstate( &__lua_alloc, nullptr );

    lua_atpanic( L, &__lua_panic );

    luaL_openlibs( L );

    lua_getglobal( L, "_G" );
    luaL_setfuncs( L, lua_functions, 0 );

    example_script_handle_t * script_handle = new example_script_handle_t;
    script_handle->L = L;
    script_handle->result = GP_SUCCESSFUL;

    *(example_script_handle_t **)lua_getextraspace( L ) = script_handle;

    script_handle->canvas = _canvas;

    *_handle = script_handle;

    return true;
}
//////////////////////////////////////////////////////////////////////////
void finalize_script( example_script_handle_t * _handle )
{
    lua_close( _handle->L );

    delete _handle;
}
//////////////////////////////////////////////////////////////////////////
int call_script( example_script_handle_t * _handle )
{
    int result = __lua_call( _handle );

    return result;
}