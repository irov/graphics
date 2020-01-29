#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "graphics/graphics.h"

#include <stdlib.h>
#include <stdio.h>

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
static void framebuffer_size_callback( GLFWwindow * _window, int _width, int _height )
{
    GP_UNUSED( _window );

    glViewport( 0, 0, _width, _height );
}
//////////////////////////////////////////////////////////////////////////
const char * vertexShaderSource = "#version 330 core\n"
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
const char * fragmentShaderSource = "#version 330 core\n"
"in vec4 v2fColor;\n"
"out vec4 oColor;\n"
"void main()\n"
"{\n"
"   oColor = v2fColor;\n"
"}\n\0";
//////////////////////////////////////////////////////////////////////////
static void * gp_malloc( gp_size_t _size, void * _ud )
{
    GP_UNUSED( _ud );

    return malloc( _size );
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
} gl_vertex_t;
//////////////////////////////////////////////////////////////////////////
typedef gp_uint16_t gl_index_t;
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

    gp_canvas_t * canvas;
    gp_canvas_create( &canvas, &gp_malloc, &gp_free, 0 );

    gp_set_line_width( canvas, 5.f );
    gp_set_line_penumbra( canvas, 2.f );

    gp_move_to( canvas, 100.f, 100.f );
    gp_quadratic_curve_to( canvas, 150.f, 200.f, 200.f, 200.f );
    gp_bezier_curve_to( canvas, 225.f, 300.f, 275.f, 250.f, 300.f, 100.f );
    gp_line_to( canvas, 400.f, 200.f );

    gp_draw_rect( canvas, 300.f, 300.f, 200.f, 100.f );
    gp_draw_rounded_rect( canvas, 300.f, 500.f, 200.f, 100.f, 20.f );

    gp_draw_circle( canvas, 700.f, 200.f, 100.f );
    gp_draw_ellipse( canvas, 700.f, 400.f, 100.f, 50.f );

    GLint vertexShader = glCreateShader( GL_VERTEX_SHADER );
    glShaderSource( vertexShader, 1, &vertexShaderSource, NULL );
    glCompileShader( vertexShader );

    GLint vertexShader_success;
    glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &vertexShader_success );

    if( vertexShader_success == 0 )
    {
        char infoLog[512];
        glGetShaderInfoLog( vertexShader, 512, NULL, infoLog );

        printf( "error: %s\n"
            , infoLog
        );

        return EXIT_FAILURE;
    }

    GLuint fragmentShader = glCreateShader( GL_FRAGMENT_SHADER );
    glShaderSource( fragmentShader, 1, &fragmentShaderSource, NULL );
    glCompileShader( fragmentShader );

    GLint fragmentShader_success;
    glGetShaderiv( fragmentShader, GL_COMPILE_STATUS, &fragmentShader_success );
    if( fragmentShader_success == 0 )
    {
        char infoLog[512];
        glGetShaderInfoLog( fragmentShader, 512, NULL, infoLog );

        printf( "error: %s\n"
            , infoLog
        );

        return EXIT_FAILURE;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader( shaderProgram, vertexShader );
    glAttachShader( shaderProgram, fragmentShader );
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

    glDeleteShader( vertexShader );
    glDeleteShader( fragmentShader );

    glUseProgram( shaderProgram );

    const float left = 0.f;
    const float right = (float)window_width;
    const float bottom = (float)window_height;
    const float top = 0.f;
    const float zNear = -1.f;
    const float zFar = 1.f;

    float projOrtho[16];
    __make_ortho( left, right, top, bottom, zNear, zFar, projOrtho );

    GLint mvpLoc = glGetUniformLocation( shaderProgram, "uWVP" );

    if( mvpLoc >= 0 )
    {
        glUniformMatrix4fv( mvpLoc, 1, GL_FALSE, projOrtho );
    }

    GLuint VAO;
    glGenVertexArrays( 1, &VAO );
    glBindVertexArray( VAO );

    GLuint VBO;
    glGenBuffers( 1, &VBO );

    glBindBuffer( GL_ARRAY_BUFFER, VBO );

    glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, sizeof( gl_vertex_t ), (void *)0 );
    glVertexAttribPointer( 1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( gl_vertex_t ), (gp_uint8_t *)0 + offsetof( gl_vertex_t, c ) );
    glEnableVertexAttribArray( 0 );
    glEnableVertexAttribArray( 1 );

    GLuint IBO;
    glGenBuffers( 1, &IBO );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, IBO );

    glBindBuffer( GL_ARRAY_BUFFER, VBO );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    gl_vertex_t vertices[4096];
    gl_index_t indices[8096];

    while( glfwWindowShouldClose( fwWindow ) == 0 )
    {
        glfwPollEvents();

        glClearColor( 0.2f, 0.3f, 0.3f, 1.f );
        glClear( GL_COLOR_BUFFER_BIT );

        gp_mesh_t mesh;
        gp_calculate_mesh_size( canvas, &mesh );

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

        mesh.indices_buffer = indices;
        mesh.indices_offset = 0;
        mesh.indices_stride = sizeof( gp_uint16_t );

        gp_render( canvas, &mesh );

        glBufferData( GL_ARRAY_BUFFER, mesh.vertex_count * sizeof( gl_vertex_t ), vertices, GL_STATIC_DRAW );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, mesh.index_count * sizeof( gl_index_t ), indices, GL_STATIC_DRAW );

        glDrawElements( GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_SHORT, NULL );

        glfwSwapBuffers( fwWindow );
    }

    glDeleteVertexArrays( 1, &VAO );
    glDeleteBuffers( 1, &VBO );
    glDeleteBuffers( 1, &IBO );
    glDeleteProgram( shaderProgram );

    gp_canvas_destroy( canvas );

    glfwDestroyWindow( fwWindow );
    glfwTerminate();

    return EXIT_SUCCESS;
}