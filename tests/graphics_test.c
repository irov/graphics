#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "graphics/graphics.h"

#include <stdlib.h>

int main( int argc, char ** argv )
{
    (void)argc;
    (void)argv;

    if( glfwInit() == 0 )
    {
        return EXIT_FAILURE;
    }

    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

    GLFWwindow * fwWindow = glfwCreateWindow( 1024, 768, "graphics", 0, 0 );

    if( fwWindow == 0 )
    {
        glfwTerminate();

        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent( fwWindow );

    gladLoadGLLoader( (GLADloadproc)glfwGetProcAddress );
    glfwSwapInterval( 1 );

    while( glfwWindowShouldClose( fwWindow ) == 0 )
    {
        glfwPollEvents();

        glClearColor( 0.f, 0.f, 0.f, 1.f );
        glClear( GL_COLOR_BUFFER_BIT );

        glfwSwapBuffers( fwWindow );
    }

    glfwDestroyWindow( fwWindow );
    glfwTerminate();

    return EXIT_SUCCESS;
}