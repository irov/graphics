#include "graphics/graphics.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_NO_SIMD
#include "stb/stb_image.h"

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "TextEditor.h"

extern "C"{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>

//////////////////////////////////////////////////////////////////////////
typedef struct gp_script_handle_t
{
    lua_State * L;

    jmp_buf panic_jump;

    gp_canvas_t * canvas;

    gp_result_t result;
    std::string error_msg;
    int error_line;
} gp_script_handle_t;
//////////////////////////////////////////////////////////////////////////
static int __lua_panic( lua_State * L )
{
    gp_script_handle_t * handle = *(gp_script_handle_t **)lua_getextraspace( L );

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
static int __lua_thickness( lua_State * L )
{
    int args_count = 1;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    gp_script_handle_t * handle = *(gp_script_handle_t **)lua_getextraspace( L );

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
static int __lua_penumbra( lua_State * L )
{
    int args_count = 1;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    gp_script_handle_t * handle = *(gp_script_handle_t **)lua_getextraspace( L );

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
static int __lua_color( lua_State * L )
{
    int args_count = 4;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    gp_script_handle_t * handle = *(gp_script_handle_t **)lua_getextraspace( L );

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
static int __lua_begin_fill( lua_State * L )
{
    int args_count = 0;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    gp_script_handle_t * handle = *(gp_script_handle_t **)lua_getextraspace( L );

    if( gp_begin_fill( handle->canvas ) == GP_FAILURE )
    {
        handle->result = GP_FAILURE;
        handle->error_msg = "invalid begin fill";
        handle->error_line = __lua_get_line( L );
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
static int __lua_end_fill( lua_State * L )
{
    int args_count = 0;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    gp_script_handle_t * handle = *(gp_script_handle_t **)lua_getextraspace( L );

    if( gp_end_fill( handle->canvas ) == GP_FAILURE )
    {
        handle->result = GP_FAILURE;
        handle->error_msg = "invalid end fill";
        handle->error_line = __lua_get_line( L );
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
static int __lua_move_to( lua_State * L )
{
    int args_count = 2;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    gp_script_handle_t * handle = *(gp_script_handle_t **)lua_getextraspace( L );

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
static int __lua_line_to( lua_State * L )
{
    int args_count = 2;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    gp_script_handle_t * handle = *(gp_script_handle_t **)lua_getextraspace( L );

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
static int __lua_quadratic_curve_to( lua_State * L )
{
    int args_count = 4;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    gp_script_handle_t * handle = *(gp_script_handle_t **)lua_getextraspace( L );

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
static int __lua_bezier_curve_to( lua_State * L )
{
    int args_count = 6;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    gp_script_handle_t * handle = *(gp_script_handle_t **)lua_getextraspace( L );

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
static int __lua_rect( lua_State * L )
{
    int args_count = 4;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    gp_script_handle_t * handle = *(gp_script_handle_t **)lua_getextraspace( L );

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
static int __lua_rounded_rect( lua_State * L )
{
    int args_count = 5;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    gp_script_handle_t * handle = *(gp_script_handle_t **)lua_getextraspace( L );

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
static int __lua_circle( lua_State * L )
{
    int args_count = 3;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    gp_script_handle_t * handle = *(gp_script_handle_t **)lua_getextraspace( L );

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
static int __lua_ellipse( lua_State * L )
{
    int args_count = 4;

    if( lua_gettop( L ) != args_count )
    {
        return luaL_error( L, "expecting exactly %d arguments", args_count );
    }

    gp_script_handle_t * handle = *(gp_script_handle_t **)lua_getextraspace( L );

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
static int gp_lua_call( gp_script_handle_t * _handle )
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
static const char * identifiers[] =
{
    "thickness",
    "penumbra",
    "color",
    "begin_fill",
    "end_fill",
    "move_to",
    "line_to",
    "quadratic_curve_to",
    "bezier_curve_to",
    "rect",
    "rounded_rect",
    "circle",
    "ellipse"
};
//////////////////////////////////////////////////////////////////////////
static const char * idecls[] =
{
    "set edge thickness",
    "set edge penumbra",
    "begin figure fill",
    "end figure fill",
    "move line plotter"
    "draw line to",
    "draw quadratic curve to",
    "draw bezier curve to",
    "draw rect",
    "draw rounded_rect",
    "draw circle",
    "draw ellipse"
};
//////////////////////////////////////////////////////////////////////////
static const struct luaL_Reg lua_functions[] = {
    {"thickness", &__lua_thickness}
    , {"penumbra", &__lua_penumbra}
    , {"color", &__lua_color}
    , {"begin_fill", &__lua_begin_fill}
    , {"end_fill", &__lua_end_fill}
    , {"move_to", &__lua_move_to}
    , {"line_to", &__lua_line_to}
    , {"quadratic_curve_to", &__lua_quadratic_curve_to}
    , {"bezier_curve_to", &__lua_bezier_curve_to}
    , {"rect", &__lua_rect}
    , {"rounded_rect", &__lua_rounded_rect}
    , {"circle", &__lua_circle}
    , {"ellipse", &__lua_ellipse}

    , {NULL, NULL} /* end of array */
};
//////////////////////////////////////////////////////////////////////////
static void __make_ortho( float _l, float _r, float _t, float _b, float _n, float _f, float _m[16] )
{
    _m[0] = 2.f / (_r - _l);
    _m[5] = 2.f / (_t - _b);
    _m[10] = -2.f / (_f - _n);
    _m[12] = -(_r + _l) / (_r - _l);
    _m[13] = -(_t + _b) / (_t - _b);
    _m[14] = -(_f + _n) / (_f - _n);
    _m[15] = 1.f;

    _m[1] = _m[2] = _m[3] = _m[4] = _m[6] = _m[7] = _m[8] = _m[9] = _m[11] = 0.f;
}
//////////////////////////////////////////////////////////////////////////
static GLuint __make_program( const char * _vertexShaderSource, const char * _fragmentShaderSource )
{
    GLint vertexShaderColor = glCreateShader( GL_VERTEX_SHADER );
    glShaderSource( vertexShaderColor, 1, &_vertexShaderSource, NULL );
    glCompileShader( vertexShaderColor );

    GLint vertexShader_success;
    glGetShaderiv( vertexShaderColor, GL_COMPILE_STATUS, &vertexShader_success );

    if( vertexShader_success == 0 )
    {
        char infoLog[512];
        glGetShaderInfoLog( vertexShaderColor, 512, NULL, infoLog );

        printf( "error: %s\n"
            , infoLog
        );

        return EXIT_FAILURE;
    }

    GLuint fragmentShaderColor = glCreateShader( GL_FRAGMENT_SHADER );
    glShaderSource( fragmentShaderColor, 1, &_fragmentShaderSource, NULL );
    glCompileShader( fragmentShaderColor );

    GLint fragmentShader_success;
    glGetShaderiv( fragmentShaderColor, GL_COMPILE_STATUS, &fragmentShader_success );
    if( fragmentShader_success == 0 )
    {
        char infoLog[512];
        glGetShaderInfoLog( fragmentShaderColor, 512, NULL, infoLog );

        printf( "error: %s\n"
            , infoLog
        );

        return EXIT_FAILURE;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader( shaderProgram, vertexShaderColor );
    glAttachShader( shaderProgram, fragmentShaderColor );
    glLinkProgram( shaderProgram );

    GLint shaderProgram_success;
    glGetProgramiv( shaderProgram, GL_LINK_STATUS, &shaderProgram_success );
    if( shaderProgram_success == 0 )
    {
        char infoLog[512];
        glGetProgramInfoLog( shaderProgram, 512, NULL, infoLog );

        printf( "error: %s\n"
            , infoLog
        );

        return EXIT_FAILURE;
    }

    glDeleteShader( vertexShaderColor );
    glDeleteShader( fragmentShaderColor );

    return shaderProgram;
}
//////////////////////////////////////////////////////////////////////////
static GLuint __make_texture( const char * _path )
{
    int width;
    int height;
    int comp;

    char texture_path[250];
    sprintf( texture_path, "%s/%s"
        , GRAPHICS_CONTENT_PATH
        , _path
    );

    uint8_t * data = stbi_load( texture_path, &width, &height, &comp, STBI_default );

    if( data == GP_NULLPTR )
    {
        return 0;
    }

    GLint internal_format;
    GLenum format;
    switch( comp )
    {
    case 1:
        {
            internal_format = GL_R8;
            format = GL_RED;
        }break;
    case 2:
        {
            internal_format = GL_RG8;
            format = GL_RG;
        }break;
    case 3:
        {
            internal_format = GL_RGB8;
            format = GL_RGB;
        }break;
    case 4:
        {
            internal_format = GL_RGBA8;
            format = GL_RGBA;
        }break;
    default:
        return 0;
    }

    GLuint id;
    glGenTextures( 1, &id );
    glBindTexture( GL_TEXTURE_2D, id );
    glTexImage2D( GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, data );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

    glBindTexture( GL_TEXTURE_2D, 0 );

    stbi_image_free( data );

    return id;
}
//////////////////////////////////////////////////////////////////////////
static void framebuffer_size_callback( GLFWwindow * _window, int _width, int _height )
{
    GP_UNUSED( _window );

    glViewport( 0, 0, _width, _height );
}
//////////////////////////////////////////////////////////////////////////
static const char * vertexShaderColorSource = "#version 330 core\n"
"layout (location = 0) in vec2 inPos;\n"
"layout (location = 1) in vec4 inColor;\n"
"uniform mat4 uWVP;\n"
"out vec4 v2fColor;\n"
"void main()\n"
"{\n"
"   vec4 p = vec4(inPos.x, inPos.y, 0.0, 1.0);\n"
"   gl_Position = uWVP * p;\n"
"   v2fColor = inColor;\n"
"}\0";
//////////////////////////////////////////////////////////////////////////
static const char * fragmentShaderColorSource = "#version 330 core\n"
"in vec4 v2fColor;\n"
"out vec4 oColor;\n"
"void main()\n"
"{\n"
"   oColor = v2fColor;\n"
"}\n\0";
//////////////////////////////////////////////////////////////////////////
static const char * vertexShaderTextureSource = "#version 330 core\n"
"layout (location = 0) in vec2 inPos;\n"
"layout (location = 1) in vec4 inColor;\n"
"layout (location = 2) in vec2 inUV;\n"
"uniform mat4 uWVP;\n"
"out vec4 v2fColor;\n"
"out vec2 v2fUV;\n"
"void main()\n"
"{\n"
"   vec4 p = vec4(inPos.x, inPos.y, 0.0, 1.0);\n"
"   gl_Position = uWVP * p;\n"
"   v2fColor = inColor;\n"
"   v2fUV = inUV;\n"
"}\0";
//////////////////////////////////////////////////////////////////////////
static const char * fragmentShaderTextureSource = "#version 330 core\n"
"uniform sampler2D uTextureRGB;\n"
"in vec4 v2fColor;\n"
"in vec2 v2fUV;\n"
"out vec4 oColor;\n"
"void main()\n"
"{\n"
"   vec4 texColor = texture( uTextureRGB, v2fUV );\n"
"   oColor = texColor * v2fColor;\n"
"}\n\0";
//////////////////////////////////////////////////////////////////////////
static void * gp_malloc( gp_size_t _size, void * _ud )
{
    GP_UNUSED( _ud );

    void * p = malloc( _size );

    return p;
}
//////////////////////////////////////////////////////////////////////////
static void gp_free( void * _ptr, void * _ud )
{
    GP_UNUSED( _ud );

    free( _ptr );
}
//////////////////////////////////////////////////////////////////////////
typedef struct gl_vertex_t
{
    float x;
    float y;
    gp_uint32_t c;
    float u;
    float v;
} gl_vertex_t;
//////////////////////////////////////////////////////////////////////////
typedef gp_uint16_t gl_index_t;
//////////////////////////////////////////////////////////////////////////
static void __draw_figure( gp_canvas_t * _canvas, float _dx, float _dy )
{
    gp_set_color( _canvas, 1.f, 0.5f, 0.25f, 1.f );
    gp_move_to( _canvas, _dx + 0.f, _dy + 0.f );
    gp_set_color( _canvas, 0.5f, 1.0f, 0.25f, 1.f );
    gp_quadratic_curve_to( _canvas, _dx + 50.f, _dy + 100.f, _dx + 100.f, _dy + 100.f );
    gp_set_color( _canvas, 0.5f, 0.5f, 1.f, 1.f );
    gp_bezier_curve_to( _canvas, _dx + 125.f, _dy + 175.f, _dx + 175.f, _dy + 150.f, _dx + 200.f, _dy );
    gp_set_color( _canvas, 0.25f, 1.f, 1.f, 1.f );
    gp_line_to( _canvas, _dx + 250.f, _dy + 100.f );

    gp_set_color( _canvas, 0.2f, 0.6f, 0.9f, 1.f );
    gp_rect( _canvas, _dx + 275.f, _dy + 0.f, 100.f, 50.f );

    gp_set_color( _canvas, 0.7f, 0.3f, 0.4f, 1.f );
    gp_rounded_rect( _canvas, _dx + 275.f, _dy + 75.f, 100.f, 50.f, 10.f );

    gp_set_color( _canvas, 0.1f, 0.8f, 0.2f, 1.f );
    gp_circle( _canvas, _dx + 100.f, _dy + 225.f, 50.f );

    gp_set_color( _canvas, 0.9f, 0.1f, 0.7f, 1.f );
    gp_ellipse( _canvas, _dx + 250.f, _dy + 225.f, 50.f, 25.f );
}
//////////////////////////////////////////////////////////////////////////
int main( int argc, char ** argv )
{
    GP_UNUSED( argc );
    GP_UNUSED( argv );

    if( glfwInit() == 0 )
    {
        return EXIT_FAILURE;
    }

    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

    int window_width = 1024;
    int window_height = 768;

    GLFWwindow * fwWindow = glfwCreateWindow( window_width, window_height, "graphics", 0, 0 );

    if( fwWindow == 0 )
    {
        glfwTerminate();

        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent( fwWindow );
    glfwSetFramebufferSizeCallback( fwWindow, framebuffer_size_callback );

    if( gladLoadGLLoader( (GLADloadproc)glfwGetProcAddress ) == 0 )
    {
        return EXIT_FAILURE;
    }

    glfwSwapInterval( 1 );

    ImGuiContext * context = ImGui::CreateContext();
    (void)context;

    if( context == nullptr )
    {
        return EXIT_FAILURE;
    }

    if( ImGui_ImplGlfw_InitForOpenGL( fwWindow, true ) == false )
    {
        return EXIT_FAILURE;
    }

    const char * glsl_version = "#version 330";

    if( ImGui_ImplOpenGL3_Init( glsl_version ) == false )
    {
        return EXIT_FAILURE;
    }

    if( ImGui_ImplOpenGL3_CreateFontsTexture() == false )
    {
        return EXIT_FAILURE;
    }

    if( ImGui_ImplOpenGL3_CreateDeviceObjects() == false )
    {
        return EXIT_FAILURE;
    }

    lua_State * L = lua_newstate( &__lua_alloc, nullptr );

    lua_atpanic( L, &__lua_panic );

    luaL_openlibs( L );

    lua_getglobal( L, "_G" );
    luaL_setfuncs( L, lua_functions, 0 );

    gp_script_handle_t script_handle;
    script_handle.L = L;
    script_handle.result = GP_SUCCESSFUL;

    *(gp_script_handle_t **)lua_getextraspace( L ) = &script_handle;

    gp_canvas_t * canvas;
    gp_canvas_create( &canvas, &gp_malloc, &gp_free, GP_NULLPTR );

    script_handle.canvas = canvas;

    GLint textureId = __make_texture( "texture.jpg" );

    GLuint shaderColorProgram = __make_program( vertexShaderColorSource, fragmentShaderColorSource );
    GLuint shaderTextureProgram = __make_program( vertexShaderTextureSource, fragmentShaderTextureSource );

    float left = 0.f;
    float right = (float)window_width;
    float bottom = (float)window_height;
    float top = 0.f;
    float zNear = -1.f;
    float zFar = 1.f;

    float projOrtho[16];
    __make_ortho( left, right, top, bottom, zNear, zFar, projOrtho );

    glUseProgram( shaderColorProgram );

    GLint wvpColorLocation = glGetUniformLocation( shaderColorProgram, "uWVP" );

    if( wvpColorLocation >= 0 )
    {
        glUniformMatrix4fv( wvpColorLocation, 1, GL_FALSE, projOrtho );
    }

    glUseProgram( shaderTextureProgram );

    GLint texLocRGB = glGetUniformLocation( shaderTextureProgram, "uTextureRGB" );

    if( texLocRGB >= 0 )
    {
        glUniform1i( texLocRGB, 0 );
    }

    GLint wvpTextureLocation = glGetUniformLocation( shaderTextureProgram, "uWVP" );

    if( wvpTextureLocation >= 0 )
    {
        glUniformMatrix4fv( wvpTextureLocation, 1, GL_FALSE, projOrtho );
    }

    GLuint VAO;
    glGenVertexArrays( 1, &VAO );
    glBindVertexArray( VAO );

    GLuint VBO;
    glGenBuffers( 1, &VBO );


    glBindBuffer( GL_ARRAY_BUFFER, VBO );

    glEnableVertexAttribArray( 0 );
    glEnableVertexAttribArray( 1 );

    glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, sizeof( gl_vertex_t ), (gp_uint8_t *)0 + offsetof( gl_vertex_t, x ) );
    glVertexAttribPointer( 1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( gl_vertex_t ), (gp_uint8_t *)0 + offsetof( gl_vertex_t, c ) );
    glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( gl_vertex_t ), (gp_uint8_t *)0 + offsetof( gl_vertex_t, u ) );

    GLuint IBO;
    glGenBuffers( 1, &IBO );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, IBO );

    glBindBuffer( GL_ARRAY_BUFFER, VBO );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    uint32_t max_vertex_count = 8196;
    uint32_t max_index_count = 32768;

    glBufferData( GL_ARRAY_BUFFER, max_vertex_count * sizeof( gl_vertex_t ), GP_NULLPTR, GL_DYNAMIC_DRAW );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, max_index_count * sizeof( gl_index_t ), GP_NULLPTR, GL_DYNAMIC_DRAW );

    TextEditor editor;
    TextEditor::LanguageDefinition lang = TextEditor::LanguageDefinition::Lua();

    for( size_t i = 0; i != sizeof( identifiers ) / sizeof( identifiers[0] ); ++i )
    {
        TextEditor::Identifier id;
        id.mDeclaration = std::string( idecls[i] );

        lang.mIdentifiers.insert( {std::string( identifiers[i] ), id} );
    }

    editor.SetLanguageDefinition( lang );

    while( glfwWindowShouldClose( fwWindow ) == 0 )
    {
        glfwPollEvents();

        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();

        ImGui::NewFrame();

        static bool wireframe = false;
        static bool texture = false;

        ImGui::Checkbox( "wireframe", &wireframe );
        ImGui::Checkbox( "texture", &texture );

        static bool fill = false;

        ImGui::Checkbox( "fill", &fill );

        static float thickness = gp_get_default_thickness();
        ImGui::DragFloat( "thickness", &thickness, 0.125f, 0.125f, 64.f );

        static float penumbra = gp_get_default_penumbra() / gp_get_default_thickness() * 2.f;
        ImGui::DragFloat( "penumbra", &penumbra, 0.0125f, 0.f, 1.f );

        static int curve_quality = gp_get_default_curve_quality();
        ImGui::SliderInt( "curve quality", &curve_quality, 1, 64 );

        static int ellipse_quality = gp_get_default_ellipse_quality();
        ImGui::SliderInt( "ellipse quality", &ellipse_quality, 8, 128 );

        static int rect_quality = gp_get_default_rect_quality();
        ImGui::SliderInt( "rect quality", &rect_quality, 2, 64 );

        static bool Custom = false;
        ImGui::Checkbox( "Custom", &Custom );

        bool TextEditorCollapsingHeader = ImGui::CollapsingHeader( "text editor" );

        if( TextEditorCollapsingHeader == true )
        {
            editor.Render( "text editor", {400.f, 200.f}, false );

            ImGui::Spacing();
        }

        int call_result = 0;
        int syntax_result = 0;
        std::string call_error;

        static std::string successful_text;

        if( Custom == true )
        {
            script_handle.result = GP_SUCCESSFUL;

            std::string text = editor.GetText();

            syntax_result = luaL_loadbufferx( L, text.c_str(), text.size(), "graphics", nullptr );

            if( syntax_result != 0 )
            {
                call_error = lua_tostring( L, -1 );

                lua_pop( L, 1 );
            }
            else
            {
                call_result = gp_lua_call( &script_handle );

                if( call_result != 0 )
                {
                    call_error = lua_tostring( L, -1 );

                    lua_pop( L, 1 );
                }
            }
        }

        if( TextEditorCollapsingHeader == true )
        {
            ImGui::NewLine();

            if( syntax_result != 0 )
            {
                ImGui::TextColored( ImVec4( 1.f, 0.f, 0.f, 1.f ), "lua syntax error: %s", call_error.c_str() );

                gp_canvas_clear( canvas );

                luaL_loadbufferx( L, successful_text.c_str(), successful_text.size(), "graphics", nullptr );
                gp_lua_call( &script_handle );
            }
            else if( call_result != 0 )
            {
                ImGui::TextColored( ImVec4( 1.f, 0.f, 0.f, 1.f ), "lua call error: %s", call_error.c_str() );

                gp_canvas_clear( canvas );

                luaL_loadbufferx( L, successful_text.c_str(), successful_text.size(), "graphics", nullptr );
                gp_lua_call( &script_handle );
            }
            else if( script_handle.result == GP_FAILURE )
            {
                ImGui::TextColored( ImVec4( 1.f, 0.f, 0.f, 1.f ), "graphics error: %s [line:%d]", script_handle.error_msg.c_str(), script_handle.error_line );

                gp_canvas_clear( canvas );

                luaL_loadbufferx( L, successful_text.c_str(), successful_text.size(), "graphics", nullptr );
                gp_lua_call( &script_handle );
            }
            else
            {
                ImGui::TextColored( ImVec4( 0.f, 1.f, 0.f, 1.f ), "successful" );

                successful_text = editor.GetText();
            }

            ImGui::NewLine();
        }

        ImGui::EndFrame();

        if( wireframe == 1 )
        {
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        }
        else
        {
            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
        }

        if( texture == 1 )
        {
            glActiveTexture( GL_TEXTURE0 );
            glBindTexture( GL_TEXTURE_2D, textureId );

            glUseProgram( shaderTextureProgram );

            glEnableVertexAttribArray( 2 );
        }
        else
        {
            glActiveTexture( GL_TEXTURE0 );
            glBindTexture( GL_TEXTURE_2D, 0 );

            glUseProgram( shaderColorProgram );

            glDisableVertexAttribArray( 2 );
        }

        glClearColor( 0.2f, 0.3f, 0.3f, 1.f );
        glClear( GL_COLOR_BUFFER_BIT );

        if( Custom == false )
        {
            gp_set_thickness( canvas, thickness );
            gp_set_penumbra( canvas, thickness * penumbra * (0.5f - 0.0125f) );

            gp_set_curve_quality( canvas, curve_quality );
            gp_set_ellipse_quality( canvas, ellipse_quality );
            gp_set_rect_quality( canvas, rect_quality );

            if( fill == true )
            {
                gp_begin_fill( canvas );
            }

            __draw_figure( canvas, 300.f, 300.f );

            if( fill == true )
            {
                gp_end_fill( canvas );
            }
        }

        gp_mesh_t mesh;
        gp_calculate_mesh_size( canvas, &mesh );

        if( mesh.vertex_count >= max_vertex_count ||
            mesh.index_count >= max_index_count )
        {
            break;
        }

        glBindBuffer( GL_ARRAY_BUFFER, VBO );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, IBO );

        void * vertices = glMapBuffer( GL_ARRAY_BUFFER, GL_WRITE_ONLY );
        void * indices = glMapBuffer( GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY );

        mesh.color.r = 1.f;
        mesh.color.g = 1.f;
        mesh.color.b = 1.f;
        mesh.color.a = 1.f;

        mesh.positions_buffer = vertices;
        mesh.positions_offset = offsetof( gl_vertex_t, x );
        mesh.positions_stride = sizeof( gl_vertex_t );

        mesh.colors_buffer = vertices;
        mesh.colors_offset = offsetof( gl_vertex_t, c );
        mesh.colors_stride = sizeof( gl_vertex_t );

        mesh.uv_buffer = vertices;
        mesh.uv_offset = offsetof( gl_vertex_t, u );
        mesh.uv_stride = sizeof( gl_vertex_t );

        mesh.indices_buffer = indices;
        mesh.indices_offset = 0;
        mesh.indices_stride = sizeof( gp_uint16_t );

        gp_result_t result = gp_render( canvas, &mesh );

        glUnmapBuffer( GL_ARRAY_BUFFER );
        glUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER );

        if( result == GP_SUCCESSFUL )
        {
            glDrawElements( GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_SHORT, GP_NULLPTR );
        }

        if( gp_canvas_clear( canvas ) == GP_FAILURE )
        {
            break;
        }

        ImGui::Render();

        ImDrawData * imData = ImGui::GetDrawData();

        ImGui_ImplOpenGL3_RenderDrawData( imData );

        glfwSwapBuffers( fwWindow );
    }

    gp_canvas_destroy( canvas );

    glDeleteVertexArrays( 1, &VAO );
    glDeleteBuffers( 1, &VBO );
    glDeleteBuffers( 1, &IBO );

    glDeleteProgram( shaderColorProgram );
    glDeleteProgram( shaderTextureProgram );

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow( fwWindow );
    glfwTerminate();

    return EXIT_SUCCESS;
}