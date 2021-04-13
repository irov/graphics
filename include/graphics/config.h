#ifndef GP_CONFIG_H_
#define GP_CONFIG_H_

#include <stdint.h>
#include <stddef.h>

typedef enum gp_result_e
{
    GP_SUCCESSFUL,
    GP_FAILURE
} gp_result_e;

typedef enum gp_boolean_e
{
    GP_FALSE,
    GP_TRUE
} gp_boolean_e;

typedef gp_result_e gp_result_t;
typedef uint32_t gp_bool_t;
typedef uint8_t gp_uint8_t;
typedef uint16_t gp_uint16_t;
typedef uint32_t gp_uint32_t;
typedef size_t gp_size_t;

typedef gp_uint32_t gp_argb_t;

typedef struct gp_color_t
{
    float r;
    float g;
    float b;
    float a;
} gp_color_t;

#ifndef NDEBUG
#define GP_DEBUG
#endif

#ifndef GP_UNUSED
#define GP_UNUSED(X) ((void)(X))
#endif

#define GP_NULLPTR (0)

#ifndef GP_COLOR_R
#define GP_COLOR_R(Ñ) (_c->r)
#endif

#ifndef GP_COLOR_G
#define GP_COLOR_G(Ñ) (_c->g)
#endif

#ifndef GP_COLOR_B
#define GP_COLOR_B(Ñ) (_c->b)
#endif

#ifndef GP_COLOR_A
#define GP_COLOR_A(Ñ) (_c->a)
#endif

#ifndef GP_LINE_POINTS_MAX
#define GP_LINE_POINTS_MAX 1024
#endif

#ifndef GP_STATE_STACK_MAX
#define GP_STATE_STACK_MAX 8
#endif

#ifndef GP_ENABLE_EXTRA_LINE_POINTS
#define GP_DISABLE_EXTRA_LINE_POINTS
#endif

#ifndef GP_MATH_SQRTF
#include <math.h>
#define GP_MATH_SQRTF (sqrtf)
#endif

#ifndef GP_MATH_COSF
#include <math.h>
#define GP_MATH_COSF (cosf)
#endif

#ifndef GP_MATH_SINF
#include <math.h>
#define GP_MATH_SINF (sinf)
#endif

#endif