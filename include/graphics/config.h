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

#ifndef GP_UNUSED
#define GP_UNUSED(X) ((void)(X))
#endif GP_UNUSED

#define GP_NULLPTR (0)

#endif