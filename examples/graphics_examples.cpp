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
#include <math.h>
#include <vector>

//////////////////////////////////////////////////////////////////////////
static const char * identifiers[] =
{
    "thickness",
    "penumbra",
    "outline_width",
    "color",
    "outline_color",

    "begin_fill",
    "end_fill",

    "move_to",
    "line_to",
    "quadratic_curve_to",
    "bezier_curve_to",

    "rect",
    "rounded_rect",
    "circle",
    "ellipse",
    "arc",
    "ellipse_arc"
};
//////////////////////////////////////////////////////////////////////////
static const char * idecls[] =
{
    "set edge thickness",
    "set edge penumbra",
    "set outline width",
    "set color",
    "set outline color",

    "begin figure fill",
    "end figure fill",

    "move line plotter",
    "draw line to",
    "draw quadratic curve to",
    "draw bezier curve to",

    "draw rect",
    "draw rounded_rect",
    "draw circle",
    "draw ellipse",
    "draw arc",
    "draw ellipse arc"
};
//////////////////////////////////////////////////////////////////////////
float camera_scale = 1.f;
float camera_scale_min = 0.125f;
float camera_scale_max = 16.f;
float camera_scale_step = 0.125f;
float camera_offset_x = 0.f;
float camera_offset_y = 0.f;
//////////////////////////////////////////////////////////////////////////
float mouse_pos_x = 0.f;
float mouse_pos_y = 0.f;
//////////////////////////////////////////////////////////////////////////
static void glfw_framebufferSizeCallback( GLFWwindow * _window, int _width, int _height )
{
    GP_UNUSED( _window );

    glViewport( 0, 0, _width, _height );
}
//////////////////////////////////////////////////////////////////////////
static void glfw_scrollCallback( GLFWwindow * _window, double _x, double _y )
{
    GP_UNUSED( _x );

    if( glfwGetKey( _window, GLFW_KEY_SPACE ) != GLFW_PRESS )
    {
        return;
    }

    camera_offset_x -= mouse_pos_x / camera_scale;
    camera_offset_y -= mouse_pos_y / camera_scale;

    float scroll = (float)_y * camera_scale_step;

    if( camera_scale + scroll > camera_scale_max )
    {
        camera_scale = camera_scale_max;
    }
    else if( camera_scale + scroll < camera_scale_min )
    {
        camera_scale = camera_scale_min;
    }
    else
    {
        camera_scale += scroll;
    }

    camera_offset_x += mouse_pos_x / camera_scale;
    camera_offset_y += mouse_pos_y / camera_scale;
}
//////////////////////////////////////////////////////////////////////////
static void glfw_cursorPosCallback( GLFWwindow * _window, double _x, double _y )
{    
    if( glfwGetKey( _window, GLFW_KEY_SPACE ) == GLFW_PRESS &&
        glfwGetMouseButton( _window, GLFW_MOUSE_BUTTON_LEFT ) == GLFW_PRESS )
    {
        const float dx = (float)_x - mouse_pos_x;
        const float dy = (float)_y - mouse_pos_y;

        camera_offset_x += dx / camera_scale;
        camera_offset_y += dy / camera_scale;
    }

    mouse_pos_x = (float)_x;
    mouse_pos_y = (float)_y;
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
typedef struct example_path_point_t
{
    float x;
    float y;
    float width;
    gp_color_t color;
} example_path_point_t;
//////////////////////////////////////////////////////////////////////////
static void __draw_card( gp_canvas_t * _canvas, float _x, float _y, float _width, float _height )
{
    gp_push_state( _canvas );
    gp_set_penumbra( _canvas, 1.f );
    gp_set_color( _canvas, 0.055f, 0.075f, 0.145f, 0.98f );
    gp_begin_fill( _canvas );
    gp_rounded_rect( _canvas, _x, _y, _width, _height, 22.f );
    gp_end_fill( _canvas );

    gp_set_thickness( _canvas, 1.5f );
    gp_set_color( _canvas, 0.18f, 0.25f, 0.45f, 0.72f );
    gp_rounded_rect( _canvas, _x, _y, _width, _height, 22.f );
    gp_pop_state( _canvas );
}
//////////////////////////////////////////////////////////////////////////
static void __draw_radial_progress( gp_canvas_t * _canvas, float _progress )
{
    const float pi = 3.14159265358979323846f;
    const float angle_from = -pi * 0.5f;
    const float angle_to = angle_from + _progress * pi * 2.f;
    const float x = 475.f;
    const float y = 226.f;

    gp_push_state( _canvas );
    gp_set_outline_width( _canvas, 0.f );
    gp_set_penumbra( _canvas, 0.f );
    gp_set_color( _canvas, 0.18f, 0.86f, 1.f, 1.f );
    gp_rounded_ring( _canvas, x, y, 68.f, 84.f, angle_from, angle_to );

    gp_set_penumbra( _canvas, 0.f );
    gp_set_color( _canvas, 0.52f, 0.38f, 1.f, 0.95f );
    gp_begin_fill( _canvas );
    gp_star( _canvas, x, y, 10.f, 22.f, 5, angle_from );
    gp_end_fill( _canvas );
    gp_pop_state( _canvas );
}
//////////////////////////////////////////////////////////////////////////
static void __draw_segmented_cooldown( gp_canvas_t * _canvas, float _progress, float _time )
{
    const float pi = 3.14159265358979323846f;
    const float angle_from = -pi * 0.5f;
    const float angle_to = angle_from + _progress * pi * 2.f;
    const float x = 808.f;
    const float y = 226.f;
    const float dash[] = {8.f, 24.f};

    gp_push_state( _canvas );
    gp_set_outline_width( _canvas, 0.f );
    gp_set_penumbra( _canvas, 2.f );
    gp_set_line_cap( _canvas, GP_LINE_CAP_ROUND );
    gp_set_thickness( _canvas, 16.f );
    gp_set_color( _canvas, 0.13f, 0.17f, 0.3f, 1.f );
    gp_arc( _canvas, x, y, 76.f, angle_from, angle_from + pi * 2.f );

    gp_set_line_dash( _canvas, dash, 2, _time * 18.f );
    gp_set_color( _canvas, 1.f, 0.39f, 0.67f, 1.f );
    gp_arc( _canvas, x, y, 76.f, angle_from, angle_to );
    gp_clear_line_dash( _canvas );

    gp_set_penumbra( _canvas, 0.f );
    gp_set_thickness( _canvas, 4.f );
    gp_set_color( _canvas, 1.f, 0.68f, 0.25f, 0.9f );
    gp_arc( _canvas, x, y, 49.f, angle_from, angle_to );

    gp_set_color( _canvas, 1.f, 0.68f, 0.25f, 0.12f );
    gp_ring( _canvas, x, y, 35.f, 44.f, angle_from, angle_to );
    gp_pop_state( _canvas );
}
//////////////////////////////////////////////////////////////////////////
static void __draw_rich_paths( gp_canvas_t * _canvas, float _progress, float _time )
{
    const gp_path_layout_t layout = {
        offsetof( example_path_point_t, width ),
        offsetof( example_path_point_t, color ),
        GP_PATH_ATTRIBUTE_NONE
    };
    const example_path_point_t trail[] = {
        {336.f, 470.f, 4.f, {0.20f, 0.92f, 1.f, 0.22f}},
        {421.f, 426.f, 9.f, {0.18f, 0.82f, 1.f, 0.90f}},
        {518.f, 482.f, 24.f, {0.48f, 0.42f, 1.f, 1.f}},
        {628.f, 421.f, 17.f, {0.92f, 0.28f, 0.82f, 1.f}},
        {743.f, 481.f, 10.f, {1.f, 0.39f, 0.58f, 0.96f}},
        {862.f, 432.f, 3.f, {1.f, 0.72f, 0.26f, 0.20f}}
    };

    gp_push_state( _canvas );
    gp_set_outline_width( _canvas, 0.f );
    gp_set_penumbra( _canvas, 6.f );
    gp_set_line_cap( _canvas, GP_LINE_CAP_ROUND );
    gp_set_line_join( _canvas, GP_LINE_JOIN_ROUND );
    gp_path_t * stable_path;
    gp_path_create( &stable_path, &gp_malloc, &gp_realloc, &gp_free, GP_NULLPTR, gp_get_default_curve_quality(), gp_get_default_ellipse_quality() );
    gp_path_catmull_rom( stable_path, trail, sizeof( trail ) / sizeof( trail[0] ), sizeof( trail[0] ), &layout, GP_FALSE );
    gp_set_path_trim( _canvas, 0.f, _progress );
    gp_draw_path( _canvas, stable_path );
    gp_path_sample_t caret;
    gp_path_sample( stable_path, _progress, GP_PATH_SAMPLE_NORMALIZED, &caret );
    gp_set_path_trim( _canvas, 0.f, 1.f );
    gp_set_penumbra( _canvas, 1.f );
    gp_set_color( _canvas, 1.f, 0.95f, 0.65f, 1.f );
    gp_begin_fill( _canvas );
    gp_circle( _canvas, caret.x, caret.y, 5.f );
    gp_end_fill( _canvas );
    gp_path_destroy( stable_path );

    const example_path_point_t route[] = {
        {355.f, 606.f, 5.f, {0.23f, 0.96f, 0.65f, 0.40f}},
        {475.f, 564.f, 8.f, {0.31f, 0.94f, 0.57f, 0.92f}},
        {592.f, 625.f, 13.f, {0.79f, 0.94f, 0.27f, 1.f}},
        {719.f, 567.f, 9.f, {1.f, 0.68f, 0.22f, 1.f}},
        {862.f, 616.f, 4.f, {1.f, 0.36f, 0.34f, 1.f}}
    };

    gp_set_penumbra( _canvas, 3.f );
    gp_set_line_join( _canvas, GP_LINE_JOIN_BEVEL );
    const gp_path_marker_t end_marker = {GP_PATH_MARKER_FILLED_ARROW, 24.f, 26.f, 0.f};
    gp_set_path_markers( _canvas, GP_NULLPTR, &end_marker );
    gp_set_uv_mode( _canvas, GP_UV_MODE_PATH );
    const float dash[] = {28.f, 12.f, 5.f};
    gp_set_line_dash( _canvas, dash, 3, _time * 16.f );
    gp_path_t * route_path;
    gp_path_create( &route_path, &gp_malloc, &gp_realloc, &gp_free, GP_NULLPTR, gp_get_default_curve_quality(), gp_get_default_ellipse_quality() );
    gp_path_catmull_rom( route_path, route + 1, sizeof( route ) / sizeof( route[0] ) - 1, sizeof( route[0] ), &layout, GP_FALSE );
    gp_draw_path( _canvas, route_path );
    gp_path_destroy( route_path );
    gp_clear_line_dash( _canvas );
    gp_set_path_markers( _canvas, GP_NULLPTR, GP_NULLPTR );
    gp_set_uv_mode( _canvas, GP_UV_MODE_BOUNDS );

    gp_set_penumbra( _canvas, 1.f );
    gp_set_color( _canvas, 0.28f, 0.88f, 1.f, 0.9f );
    gp_begin_fill( _canvas );
    gp_capsule( _canvas, 350.f, 670.f, 126.f, 28.f );
    gp_set_color( _canvas, 0.68f, 0.41f, 1.f, 0.9f );
    gp_rounded_rect_varying( _canvas, 500.f, 656.f, 126.f, 42.f, 20.f, 6.f, 20.f, 6.f );
    gp_set_color( _canvas, 1.f, 0.42f, 0.65f, 0.9f );
    gp_superellipse( _canvas, 688.f, 674.f, 30.f, 22.f, 4.5f, _time * 0.35f );
    gp_end_fill( _canvas );

    gp_set_color( _canvas, 1.f, 0.7f, 0.24f, 0.95f );
    gp_set_thickness( _canvas, 5.f );
    gp_set_line_cap( _canvas, GP_LINE_CAP_ROUND );
    gp_path_t * path;
    gp_path_create( &path, &gp_malloc, &gp_realloc, &gp_free, GP_NULLPTR, gp_get_default_curve_quality(), gp_get_default_ellipse_quality() );
    gp_path_move_to( path, 757.f, 687.f );
    gp_path_tangent_arc_to( path, 812.f, 644.f, 882.f, 682.f, 28.f );
    gp_path_line_to( path, 901.f, 668.f );
    gp_draw_path( _canvas, path );
    gp_path_destroy( path );
    gp_pop_state( _canvas );
}
//////////////////////////////////////////////////////////////////////////
static void __draw_showcase( gp_canvas_t * _canvas, float _progress, float _time )
{
    __draw_card( _canvas, 304.f, 70.f, 318.f, 308.f );
    __draw_card( _canvas, 640.f, 70.f, 318.f, 308.f );
    __draw_card( _canvas, 304.f, 396.f, 654.f, 330.f );

    __draw_radial_progress( _canvas, _progress );
    __draw_segmented_cooldown( _canvas, _progress, _time );
    __draw_rich_paths( _canvas, _progress, _time );
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
    glfwSetFramebufferSizeCallback( fwWindow, &glfw_framebufferSizeCallback );
    glfwSetScrollCallback( fwWindow, &glfw_scrollCallback );
    glfwSetCursorPosCallback( fwWindow, &glfw_cursorPosCallback );

    double cursorPosX;
    double cursorPosY;
    glfwGetCursorPos( fwWindow, &cursorPosX, &cursorPosY );

    mouse_pos_x = (float)cursorPosX;
    mouse_pos_y = (float)cursorPosY;

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

    ImGui::GetIO().IniFilename = nullptr;

    ImGuiStyle & style = ImGui::GetStyle();
    style.WindowRounding = 12.f;
    style.ChildRounding = 8.f;
    style.FrameRounding = 7.f;
    style.GrabRounding = 7.f;
    style.WindowPadding = ImVec2( 16.f, 16.f );
    style.ItemSpacing = ImVec2( 8.f, 9.f );
    style.Colors[ImGuiCol_WindowBg] = ImVec4( 0.035f, 0.045f, 0.085f, 0.96f );
    style.Colors[ImGuiCol_Border] = ImVec4( 0.18f, 0.25f, 0.45f, 0.7f );
    style.Colors[ImGuiCol_FrameBg] = ImVec4( 0.08f, 0.11f, 0.20f, 1.f );
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4( 0.12f, 0.17f, 0.30f, 1.f );
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4( 0.16f, 0.23f, 0.40f, 1.f );
    style.Colors[ImGuiCol_CheckMark] = ImVec4( 0.25f, 0.86f, 1.f, 1.f );
    style.Colors[ImGuiCol_SliderGrab] = ImVec4( 0.52f, 0.38f, 1.f, 1.f );
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4( 0.95f, 0.30f, 0.75f, 1.f );
    style.Colors[ImGuiCol_Header] = ImVec4( 0.10f, 0.14f, 0.26f, 1.f );
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4( 0.16f, 0.22f, 0.39f, 1.f );
    style.Colors[ImGuiCol_Button] = ImVec4( 0.16f, 0.22f, 0.39f, 1.f );
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4( 0.24f, 0.34f, 0.58f, 1.f );
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4( 0.25f, 0.86f, 1.f, 1.f );

    if( ImGui_ImplGlfw_InitForOpenGL( fwWindow, true ) == false )
    {
        return EXIT_FAILURE;
    }

    const char * glsl_version = "#version 330";

    if( ImGui_ImplOpenGL3_Init( glsl_version ) == false )
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

    uint32_t max_vertex_count = 262144;
    uint32_t max_index_count = 524288;

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
        static bool animate = true;
        static bool Custom = false;
        static float progress = 0.68f;
        static float animation_speed = 1.f;
        static float showcase_time = 5.66f;
        static double previous_time = glfwGetTime();

        double current_time = glfwGetTime();
        float frame_time = (float)(current_time - previous_time);
        previous_time = current_time;

        if( animate == true )
        {
            showcase_time += frame_time * animation_speed;
            progress = fmodf( showcase_time * 0.12f, 1.f );
        }

        ImGui::SetNextWindowPos( ImVec2( 16.f, 16.f ), ImGuiCond_Once );
        ImGui::SetNextWindowSize( ImVec2( 268.f, 736.f ), ImGuiCond_Once );
        ImGui::Begin( "Graphics playground" );

        ImGui::TextColored( ImVec4( 0.25f, 0.86f, 1.f, 1.f ), "PHASE 3 SHOWCASE" );
        ImGui::TextWrapped( "Reusable paths, stable trim and VFX trails." );
        ImGui::Spacing();
        ImGui::Checkbox( "Animate", &animate );
        float progress_percent = progress * 100.f;
        if( ImGui::SliderFloat( "Progress", &progress_percent, 0.f, 100.f, "%.0f%%" ) == true )
        {
            progress = progress_percent * 0.01f;
            animate = false;
        }
        ImGui::SliderFloat( "Speed", &animation_speed, 0.1f, 2.5f, "%.1fx" );
        ImGui::ProgressBar( progress, ImVec2( -1.f, 0.f ) );

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text( "On canvas" );
        ImGui::BulletText( "rounded radial ring" );
        ImGui::BulletText( "stable trim + sampled caret" );
        ImGui::BulletText( "dashed rich spline" );
        ImGui::BulletText( "rich Catmull-Rom + marker" );
        ImGui::BulletText( "superellipse and arcTo" );

        if( ImGui::CollapsingHeader( "Diagnostics" ) == true )
        {
            ImGui::SliderFloat( "Scale", &camera_scale, camera_scale_min, camera_scale_max );

            if( ImGui::Button( "Reset camera" ) )
            {
                camera_scale = 1.f;
                camera_offset_x = 0.f;
                camera_offset_y = 0.f;
            }

            ImGui::Checkbox( "Wireframe", &wireframe );
            ImGui::Checkbox( "Texture", &texture );
            ImGui::TextDisabled( "Hold Space: drag to pan, wheel to zoom" );
        }

        static int curve_quality = gp_get_default_curve_quality();
        static int ellipse_quality = gp_get_default_ellipse_quality();
        static int rect_quality = gp_get_default_rect_quality();
        if( ImGui::CollapsingHeader( "Tessellation" ) == true )
        {
            ImGui::SliderInt( "Curve", &curve_quality, 1, 64 );
            ImGui::SliderInt( "Ellipse", &ellipse_quality, 8, 128 );
            ImGui::SliderInt( "Rect", &rect_quality, 2, 64 );
        }

        ImGui::Spacing();
        ImGui::Checkbox( "Custom Lua", &Custom );

        bool TextEditorCollapsingHeader = Custom == true && ImGui::CollapsingHeader( "Text editor" );

        if( TextEditorCollapsingHeader == true )
        {
            editor.Render( "text editor", {235.f, 220.f}, false );

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

        glEnableVertexAttribArray( 2 );
        opengl_use_color_program( opengl_handle );
        opengl_set_camera( opengl_handle, camera_offset_x, camera_offset_y, camera_scale );

        glClearColor( 0.018f, 0.024f, 0.055f, 1.f );
        glClear( GL_COLOR_BUFFER_BIT );

        if( Custom == false )
        {
            gp_set_curve_quality( canvas, curve_quality );
            gp_set_ellipse_quality( canvas, ellipse_quality );
            gp_set_rect_quality( canvas, rect_quality );

            __draw_showcase( canvas, progress, showcase_time );
        }

        gp_mesh_t mesh;
        gp_calculate_mesh_size( canvas, &mesh );

        if( mesh.vertex_count > max_vertex_count ||
            mesh.index_count > max_index_count )
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

        std::vector<gp_render_batch_t> batches( mesh.batch_count );
        mesh.batches_buffer = batches.data();
        mesh.batches_offset = 0;
        mesh.batches_stride = sizeof( gp_render_batch_t );

        gp_result_t result = gp_render( canvas, &mesh );

        glUnmapBuffer( GL_ARRAY_BUFFER );
        glUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER );

        if( result == GP_SUCCESSFUL )
        {
            for( const gp_render_batch_t & batch : batches )
            {
                opengl_use_batch_program( opengl_handle, texture );
                opengl_set_camera( opengl_handle, camera_offset_x, camera_offset_y, camera_scale );
                const void * index_offset = (const void *)(uintptr_t)(batch.index_offset * sizeof( gp_uint16_t ));
                glDrawElementsBaseVertex( GL_TRIANGLES, batch.index_count, GL_UNSIGNED_SHORT, index_offset, (GLint)batch.vertex_offset );
            }
        }

        if( TextEditorCollapsingHeader == true )
        {
            if( result == GP_FAILURE )
            {
                ImGui::TextColored( {1.f, 0.f, 0.f, 1.f}, "invalid render" );
            }
        }

        if( Custom == false && camera_scale == 1.f && camera_offset_x == 0.f && camera_offset_y == 0.f )
        {
            ImDrawList * foreground = ImGui::GetForegroundDrawList();
            ImU32 title_color = IM_COL32( 180, 201, 255, 255 );
            ImU32 caption_color = IM_COL32( 112, 136, 184, 255 );
            char progress_text[32];
            snprintf( progress_text, sizeof( progress_text ), "%d%%", (int)(progress * 100.f + 0.5f) );

            foreground->AddText( ImVec2( 328.f, 91.f ), title_color, "RADIAL PROGRESS" );
            foreground->AddText( ImVec2( 740.f, 91.f ), title_color, "SEGMENTED COOLDOWN" );
            foreground->AddText( ImVec2( 462.f, 216.f ), IM_COL32( 228, 238, 255, 255 ), progress_text );
            foreground->AddText( ImVec2( 795.f, 216.f ), IM_COL32( 228, 238, 255, 255 ), progress_text );
            foreground->AddText( ImVec2( 328.f, 416.f ), title_color, "STABLE TRIM + VFX PATHS" );
            foreground->AddText( ImVec2( 328.f, 702.f ), caption_color, "CAPSULE" );
            foreground->AddText( ImVec2( 500.f, 702.f ), caption_color, "4 RADII" );
            foreground->AddText( ImVec2( 646.f, 702.f ), caption_color, "SUPERELLIPSE" );
            foreground->AddText( ImVec2( 790.f, 702.f ), caption_color, "arcTo" );
        }

        ImGui::End();

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
