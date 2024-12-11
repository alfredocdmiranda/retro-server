#ifndef EMULATION_VIDEO_H
#define EMULATION_VIDEO_H

#include <stdbool.h>

#include <png.h>
#include <SDL2/SDL.h>
#include <EGL/egl.h>

#include "glad.h"
#include "libretro.h"

typedef struct {
    GLuint tex_id;
    GLuint fbo_id;
    GLuint rbo_id;

	GLuint pitch;
	GLint tex_w, tex_h;
	GLuint clip_w, clip_h;

	GLuint pixfmt;
	GLuint pixtype;
	GLuint bpp;

    struct retro_hw_render_callback hw_render;
} RetroVideoSDLInfo;

typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint program;

    GLint i_pos;
    GLint i_coord;
    GLint u_tex;
    GLint u_mvp;

} ShaderInfo;

typedef struct {
    unsigned char *data;
    size_t size;
} PNGData;

extern RetroVideoSDLInfo video_info;
extern ShaderInfo shader_info;

uintptr_t retro_core_get_current_framebuffer(void);
void retro_core_video_refresh(const void *data, unsigned width, unsigned height, size_t pitch);

void video_init(const struct retro_game_geometry *geom);
bool video_set_pixel_format(unsigned format);
bool video_set_geometry(const struct retro_game_geometry *geom);

#endif