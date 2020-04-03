#pragma once

#include "graphics/graphics.hpp"

#include <string>

extern "C"{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>

typedef struct example_script_handle_t
{
    lua_State * L;

    jmp_buf panic_jump;

    gp_canvas_t * canvas;

    gp_result_t result;
    std::string error_msg;
    int error_line;
} example_script_handle_t;

bool initialize_script( example_script_handle_t ** _handle, gp_canvas_t * _canvas );
void finalize_script( example_script_handle_t * _handle );

int call_script( example_script_handle_t * _handle );