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

#define GP_COLOR_R(X) r
#define GP_COLOR_G(X) g
#define GP_COLOR_B(X) b
#define GP_COLOR_A(X) a

#ifndef GP_LINE_POINTS_MAX
#define GP_LINE_POINTS_MAX 1024
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