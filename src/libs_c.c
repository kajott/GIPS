#ifndef _WIN32
    #define GL_GLEXT_PROTOTYPES
    #include <GL/glcorearb.h>
#endif

#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
