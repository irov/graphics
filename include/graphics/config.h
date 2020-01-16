#ifndef GP_CONFIG_H_
#define GP_CONFIG_H_

#include <stdint.h>

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

typedef struct gp_color_t
{
    float r;
    float g;
    float b;
    float a;
} gp_color_t;

#ifndef GP_UNUSED
#define GP_UNUSED(X) ((void)(X))
#endif GP_UNUSED

#define GP_NULLPTR (0)

#define GP_COLOR_R(X) r
#define GP_COLOR_G(X) g
#define GP_COLOR_B(X) b
#define GP_COLOR_A(X) a

#endif