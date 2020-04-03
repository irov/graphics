#include "graphics/graphics.hpp"

#include "graphics_lua.h"
#include "graphics_opengl.h"

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "TextEditor.h"

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>

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
    "set color",

    "begin figure fill",
    "end figure fill",

    "move line plotter",
    "draw line to",
    "draw quadratic curve to",
    "draw bezier curve to",

    "draw rect",
    "draw rounded_rect",
    "draw circle",
    "draw ellipse"
};
//////////////////////////////////////////////////////////////////////////
static void framebuffer_size_callback( GLFWwindow * _window, int _width, int _height )
{
    GP_UNUSED( _window );

    glViewport( 0, 0, _width, _height );
}
//////////////////////////////////////////////////////////////////////////
static void * gp_malloc( gp_size_t _size, void * _ud )
{
    GP_UNUSED( _ud );

    void * p = malloc( _size );

    return p;
}
//////////////////////////////////////////////////////////////////////////
static void * gp_realloc( void * _ptr, gp_size_t _size, void * _ud )
{
    GP_UNUSED( _ud );

    void * p = realloc( _ptr, _size );

    return p;
}
//////////////////////////////////////////////////////////////////////////
static void gp_free( void * _ptr, void * _ud )
{
    GP_UNUSED( _ud );

    free( _ptr );
}
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

    gp_canvas_t * canvas;
    gp_canvas_create( &canvas, &gp_malloc, &gp_realloc, &gp_free, GP_NULLPTR );

    example_script_handle_t * script_handle;
    initialize_script( &script_handle, canvas );

    uint32_t max_vertex_count = 8196 * 2;
    uint32_t max_index_count = 32768;

    example_opengl_handle_t * opengl_handle;
    initialize_opengl( &opengl_handle, (float)window_width, (float)window_height, max_vertex_count, max_index_count );


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
            script_handle->result = GP_SUCCESSFUL;

            std::string text = editor.GetText();

            syntax_result = luaL_loadbufferx( script_handle->L, text.c_str(), text.size(), "graphics", nullptr );

            if( syntax_result != 0 )
            {
                call_error = lua_tostring( script_handle->L, -1 );

                lua_pop( script_handle->L, 1 );
            }
            else
            {
                call_result = call_script( script_handle );

                if( call_result != 0 )
                {
                    call_error = lua_tostring( script_handle->L, -1 );

                    lua_pop( script_handle->L, 1 );
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

                luaL_loadbufferx( script_handle->L, successful_text.c_str(), successful_text.size(), "graphics", nullptr );
                call_script( script_handle );
            }
            else if( call_result != 0 )
            {
                ImGui::TextColored( ImVec4( 1.f, 0.f, 0.f, 1.f ), "lua call error: %s", call_error.c_str() );

                gp_canvas_clear( canvas );

                luaL_loadbufferx( script_handle->L, successful_text.c_str(), successful_text.size(), "graphics", nullptr );
                call_script( script_handle );
            }
            else if( script_handle->result == GP_FAILURE )
            {
                ImGui::TextColored( ImVec4( 1.f, 0.f, 0.f, 1.f ), "graphics error: %s [line:%d]", script_handle->error_msg.c_str(), script_handle->error_line );

                gp_canvas_clear( canvas );

                luaL_loadbufferx( script_handle->L, successful_text.c_str(), successful_text.size(), "graphics", nullptr );
                call_script( script_handle );
            }
            else
            {
                ImGui::TextColored( ImVec4( 0.f, 1.f, 0.f, 1.f ), "successful" );

                successful_text = editor.GetText();
            }

            ImGui::NewLine();
        }

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
            glBindTexture( GL_TEXTURE_2D, opengl_handle->textureId );

            glUseProgram( opengl_handle->shaderTextureProgram );

            glEnableVertexAttribArray( 2 );
        }
        else
        {
            glActiveTexture( GL_TEXTURE0 );
            glBindTexture( GL_TEXTURE_2D, 0 );

            glUseProgram( opengl_handle->shaderColorProgram );

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

        glBindBuffer( GL_ARRAY_BUFFER, opengl_handle->VBO );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, opengl_handle->IBO );

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

        if( TextEditorCollapsingHeader == true )
        {
            if( result == GP_FAILURE )
            {
                ImGui::TextColored( {1.f, 0.f, 0.f, 1.f}, "invalid render" );
            }
        }

        ImGui::EndFrame();

        if( gp_canvas_clear( canvas ) == GP_FAILURE )
        {
            break;
        }

        ImGui::Render();

        ImDrawData * imData = ImGui::GetDrawData();

        ImGui_ImplOpenGL3_RenderDrawData( imData );

        glfwSwapBuffers( fwWindow );
    }

    finalize_script( script_handle );
    finalize_opengl( opengl_handle );

    gp_canvas_destroy( canvas );

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow( fwWindow );
    glfwTerminate();

    return EXIT_SUCCESS;
}