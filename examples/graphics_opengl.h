#pragma once

#include "graphics/graphics.hpp"

#include "glad/glad.h"

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
typedef struct example_opengl_handle_t
{
    GLuint textureId;

    GLuint VAO;
    GLuint VBO;
    GLuint IBO;

    GLuint shaderColorProgram;
    GLuint shaderTextureProgram;
}example_opengl_handle_t;
//////////////////////////////////////////////////////////////////////////
bool initialize_opengl( example_opengl_handle_t ** _handle, float _width, float _height, int _max_vertex_count, int _max_index_count );
void finalize_opengl( example_opengl_handle_t * _handle );