#include "core.h"
#include "video.h"

#define SCALE 3
// static float SCALE = 3;
RetroVideoSDLInfo video_info = {0};
ShaderInfo shader_info = {0};

static SDL_Window *window = NULL;
static SDL_GLContext *context = NULL;

static const char *g_vshader_src =
    "#version 150\n"
    "in vec2 i_pos;\n"
    "in vec2 i_coord;\n"
    "out vec2 o_coord;\n"
    "uniform mat4 u_mvp;\n"
    "void main() {\n"
        "o_coord = i_coord;\n"
        "gl_Position = vec4(i_pos, 0.0, 1.0) * u_mvp;\n"
    "}";

static const char *g_fshader_src =
    "#version 150\n"
    "in vec2 o_coord;\n"
    "uniform sampler2D u_tex;\n"
    "void main() {\n"
        "gl_FragColor = texture2D(u_tex, o_coord);\n"
    "}";

void write_png_data(png_structp png_ptr, png_bytep data, png_size_t length) {
    PNGData *p = (PNGData *)png_get_io_ptr(png_ptr);
    p->data = (unsigned char *)realloc(p->data, p->size + length);
    if (!p->data) {
        png_error(png_ptr, "Memory allocation error"); 
    }
    memcpy(p->data + p->size, data, length);
    p->size += length;
}

PNGData write_rgba_to_png_memory(unsigned char *rgba_data, int width, int height) {
    PNGData png_data;
    png_data.data = NULL;
    png_data.size = 0;

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        log_message(LOG_LEVEL_ERROR, "Error creating PNG write struct");
        return png_data; 
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        log_message(LOG_LEVEL_ERROR, "Error creating PNG info struct");
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        return png_data; 
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        log_message(LOG_LEVEL_ERROR, "Error during PNG creation");
        png_destroy_write_struct(&png_ptr, &info_ptr);
        free(png_data.data);
        return png_data; 
    }
    png_set_write_fn(png_ptr, &png_data, write_png_data, NULL);
    png_set_IHDR(
        png_ptr, info_ptr, width, height, 8, 
        PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT
    );
    png_write_info(png_ptr, info_ptr);

    // Write image data
    for (int y = 0; y < height; y++) {
        png_write_row(png_ptr, &rgba_data[y * width * 4]);
    }

    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    return png_data; 
}

static void flip_image_vertically(unsigned char* pixels, int width, int height) {
    int row_size = width * 4; // RGBA
    unsigned char* temp_row = (unsigned char*)malloc(row_size);
    if (!temp_row) {
        fprintf(stderr, "Failed to allocate memory for flipping.\n");
        return;
    }

    for (int y = 0; y < height / 2; ++y) {
        int opposite_row = height - y - 1;
        memcpy(temp_row, &pixels[y * row_size], row_size);
        memcpy(&pixels[y * row_size], &pixels[opposite_row * row_size], row_size);
        memcpy(&pixels[opposite_row * row_size], temp_row, row_size);
    }
    free(temp_row);
}

static void broadcast_frame() {
    if (*(core_handler.counter_connections) > 0) {
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        int width = viewport[2];
        int height = viewport[3];

        unsigned char* pixels = (unsigned char*)malloc(width * height * 4); // RGBA
        if (!pixels) {
            log_message(LOG_LEVEL_ERROR, "Failed to allocate memory for pixel data.");
            return;
        }

        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        flip_image_vertically(pixels, width, height);
        PNGData png_data = write_rgba_to_png_memory(pixels, width, height);

        if (png_data.data) {
            for (int i=0;i<MAX_CONN;i++){
                pthread_mutex_lock(&core_handler.connections_mutex[i]);
                if (core_handler.connections[i] != NULL) {
                    send_data(CMD_SEND_VIDEO, png_data.data, png_data.size, core_handler.connections[i]);
                }
                pthread_mutex_unlock(&core_handler.connections_mutex[i]);
            }

            free(png_data.data);
        } else {
            log_message(LOG_LEVEL_ERROR, "Error creating PNG in memory");
        }

        free(pixels);
    }
}

static GLuint compile_shader(unsigned type, unsigned count, const char **strings) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, count, strings, NULL);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE) {
        char buffer[4096];
        glGetShaderInfoLog(shader, sizeof(buffer), NULL, buffer);
        log_message(LOG_LEVEL_ERROR, "Failed to compile %s shader: %s", type == GL_VERTEX_SHADER ? "vertex" : "fragment", buffer);
    }

    return shader;
}

static void ortho2d(float m[4][4], float left, float right, float bottom, float top) {
    m[0][0] = 1; m[0][1] = 0; m[0][2] = 0; m[0][3] = 0;
    m[1][0] = 0; m[1][1] = 1; m[1][2] = 0; m[1][3] = 0;
    m[2][0] = 0; m[2][1] = 0; m[2][2] = 1; m[2][3] = 0;
    m[3][0] = 0; m[3][1] = 0; m[3][2] = 0; m[3][3] = 1;

    m[0][0] = 2.0f / (right - left);
    m[1][1] = 2.0f / (top - bottom);
    m[2][2] = -1.0f;
    m[3][0] = -(right + left) / (right - left);
    m[3][1] = -(top + bottom) / (top - bottom);
}

static void refresh_vertex_data() {
    SDL_assert(video_info.tex_w);
    SDL_assert(video_info.tex_h);
    SDL_assert(video_info.clip_w);
    SDL_assert(video_info.clip_h);

    float bottom = (float)video_info.clip_h / video_info.tex_h;
    float right  = (float)video_info.clip_w / video_info.tex_w;

    float vertex_data[] = {
        // pos, coord
        -1.0f, -1.0f, 0.0f,  bottom, // left-bottom
        -1.0f,  1.0f, 0.0f,  0.0f,   // left-top
         1.0f, -1.0f, right,  bottom,// right-bottom
         1.0f,  1.0f, right,  0.0f,  // right-top
    };

    glBindVertexArray(shader_info.vao);

    glBindBuffer(GL_ARRAY_BUFFER, shader_info.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STREAM_DRAW);

    glEnableVertexAttribArray(shader_info.i_pos);
    glEnableVertexAttribArray(shader_info.i_coord);
    glVertexAttribPointer(shader_info.i_pos, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, 0);
    glVertexAttribPointer(shader_info.i_coord, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void resize_cb(int width, int height) {
	glViewport(0, 0, width, height);
}

static void resize_to_aspect(double ratio, int src_width, int src_height, int *dest_width, int *dest_height) {
	*dest_width = src_width;
	*dest_height = src_height;

	if (ratio <= 0) {
        ratio = (double)src_width / src_height;
    }

	if ((float)src_width / src_height < 1) {
        *dest_width = *dest_height * ratio;
    } else {
        *dest_height = *dest_width / ratio;
    }
}

static void init_shaders() {
    GLuint vshader = compile_shader(GL_VERTEX_SHADER, 1, &g_vshader_src);
    GLuint fshader = compile_shader(GL_FRAGMENT_SHADER, 1, &g_fshader_src);
    GLuint program = glCreateProgram();

    SDL_assert(program);

    glAttachShader(program, vshader);
    glAttachShader(program, fshader);
    glLinkProgram(program);

    glDeleteShader(vshader);
    glDeleteShader(fshader);

    glValidateProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if(status == GL_FALSE) {
        char buffer[4096];
        glGetProgramInfoLog(program, sizeof(buffer), NULL, buffer);
        log_message(LOG_LEVEL_ERROR, "Failed to link shader program: %s", buffer);
    }

    shader_info.program = program;
    shader_info.i_pos   = glGetAttribLocation(program,  "i_pos");
    shader_info.i_coord = glGetAttribLocation(program,  "i_coord");
    shader_info.u_tex   = glGetUniformLocation(program, "u_tex");
    shader_info.u_mvp   = glGetUniformLocation(program, "u_mvp");

    glGenVertexArrays(1, &shader_info.vao);
    glGenBuffers(1, &shader_info.vbo);

    glUseProgram(shader_info.program);

    glUniform1i(shader_info.u_tex, 0);

    float m[4][4];
    if (video_info.hw_render.bottom_left_origin) {
        ortho2d(m, -1, 1, 1, -1);
    } else {
        ortho2d(m, -1, 1, -1, 1);
    }

    glUniformMatrix4fv(shader_info.u_mvp, 1, GL_FALSE, (float*)m);

    glUseProgram(0);
}

static void video_create_window(int width, int height) {
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    if (video_info.hw_render.context_type == RETRO_HW_CONTEXT_OPENGL_CORE || 
        video_info.hw_render.version_major >= 3) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, video_info.hw_render.version_major);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, video_info.hw_render.version_minor);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    }

    switch (video_info.hw_render.context_type) {
        case RETRO_HW_CONTEXT_OPENGL_CORE:
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            break;
        case RETRO_HW_CONTEXT_OPENGLES2:
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
            break;
        case RETRO_HW_CONTEXT_OPENGL:
            if (video_info.hw_render.version_major >= 3) {
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
            }
            break;
        default:
            log_message(LOG_LEVEL_ERROR, "Unsupported HW Render context %i. (only OPENGL, OPENGL_CORE and OPENGLES2 supported)", video_info.hw_render.context_type);
            exit(EXIT_FAILURE);
    }

    window = SDL_CreateWindow(
        "EGL with SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN
    );

	if (!window){
        log_message(LOG_LEVEL_ERROR, "Failed to create window: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    context = SDL_GL_CreateContext(window);

    SDL_GL_MakeCurrent(window, context);

    if (!context) {
        log_message(LOG_LEVEL_ERROR, "Failed to create OpenGL context: %s", SDL_GetError());
    }

    if (video_info.hw_render.context_type == RETRO_HW_CONTEXT_OPENGLES2) {
        if (!gladLoadGLES2Loader((GLADloadproc)SDL_GL_GetProcAddress)){
            log_message(LOG_LEVEL_ERROR, "Failed to initialize glad.");
        }
    } else {
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            log_message(LOG_LEVEL_ERROR, "Failed to initialize glad.");
        }
    }

    fprintf(stderr, "GL_SHADING_LANGUAGE_VERSION: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    fprintf(stderr, "GL_VERSION: %s\n", glGetString(GL_VERSION));


    init_shaders();

    SDL_GL_SetSwapInterval(1);
    SDL_GL_SwapWindow(window); // make apitrace output nicer

    resize_cb(width, height);
}

static void init_framebuffer(int width, int height) {
    glGenFramebuffers(1, &video_info.fbo_id);
    glBindFramebuffer(GL_FRAMEBUFFER, video_info.fbo_id);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, video_info.tex_id, 0);

    if (video_info.hw_render.depth && video_info.hw_render.stencil) {
        glGenRenderbuffers(1, &video_info.rbo_id);
        glBindRenderbuffer(GL_RENDERBUFFER, video_info.rbo_id);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, video_info.rbo_id);
    } else if (video_info.hw_render.depth) {
        glGenRenderbuffers(1, &video_info.rbo_id);
        glBindRenderbuffer(GL_RENDERBUFFER, video_info.rbo_id);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, video_info.rbo_id);
    }

    if (video_info.hw_render.depth || video_info.hw_render.stencil)
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    SDL_assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Functions that will be called be the cores
/**
 * Render a frame.
 *
 * @param data A pointer to the frame buffer data with a pixel format of 15-bit \c 0RGB1555 native endian, unless changed with \c RETRO_ENVIRONMENT_SET_PIXEL_FORMAT.
 * @param width The width of the frame buffer, in pixels.
 * @param height The height frame buffer, in pixels.
 * @param pitch The width of the frame buffer, in bytes.
 */
void retro_core_video_refresh(const void *data, unsigned width, unsigned height, size_t pitch) {
    if (video_info.clip_w != width || video_info.clip_h != height){
		video_info.clip_h = height;
		video_info.clip_w = width;

		refresh_vertex_data();
	}

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, video_info.tex_id);

	if (pitch != video_info.pitch)
		video_info.pitch = pitch;

    if (data && data != RETRO_HW_FRAME_BUFFER_VALID) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, video_info.pitch / video_info.bpp);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
						video_info.pixtype, video_info.pixfmt, data);
	}

    int w = 0, h = 0;
    SDL_GetWindowSize(window, &w, &h);
    glViewport(0, 0, w, h);

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_info.program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, video_info.tex_id);


    glBindVertexArray(shader_info.vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glUseProgram(0);

    SDL_GL_SwapWindow(window);
    broadcast_frame();
}

uintptr_t retro_core_get_current_framebuffer() {
    return video_info.fbo_id;
}

// Public functions
void video_init(const struct retro_game_geometry *geom) {
	int nwidth, nheight;

	resize_to_aspect(geom->aspect_ratio, geom->base_width * 1, geom->base_height * 1, &nwidth, &nheight);

	nwidth *= SCALE;
	nheight *= SCALE;

	if (!window){
        video_create_window(nwidth, nheight);
    }

	if (video_info.tex_id){
        glDeleteTextures(1, &video_info.tex_id);
    }

	video_info.tex_id = 0;
	if (!video_info.pixfmt){
        video_info.pixfmt = GL_UNSIGNED_SHORT_5_5_5_1;
    }

    SDL_SetWindowSize(window, nwidth, nheight);
	glGenTextures(1, &video_info.tex_id);

	if (!video_info.tex_id){
        log_message(LOG_LEVEL_ERROR, "Failed to create the video texture");
        exit(EXIT_FAILURE);
    }

	video_info.pitch = geom->max_width * video_info.bpp;

	glBindTexture(GL_TEXTURE_2D, video_info.tex_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, geom->max_width, geom->max_height, 0, video_info.pixtype, video_info.pixfmt, NULL
    );

	glBindTexture(GL_TEXTURE_2D, 0);

    init_framebuffer(geom->max_width, geom->max_height);

	video_info.tex_w = geom->max_width;
	video_info.tex_h = geom->max_height;
	video_info.clip_w = geom->base_width;
	video_info.clip_h = geom->base_height;

	refresh_vertex_data();

    video_info.hw_render.context_reset();
}

void video_deinit() {
    if (video_info.fbo_id){
        glDeleteFramebuffers(1, &video_info.fbo_id);
    }

	if (video_info.tex_id) {
        glDeleteTextures(1, &video_info.tex_id);
    }

    if (shader_info.vao) {
        glDeleteVertexArrays(1, &shader_info.vao);
    }

    if (shader_info.vbo) {
        glDeleteBuffers(1, &shader_info.vbo);
    }
        
    if (shader_info.program) {
        glDeleteProgram(shader_info.program);
    }

    video_info.fbo_id = 0;
	video_info.tex_id = 0;
    shader_info.vao = 0;
    shader_info.vbo = 0;
    shader_info.program = 0;

    SDL_GL_MakeCurrent(window, context);
    SDL_GL_DeleteContext(context);

    context = NULL;

    SDL_DestroyWindow(window);
}

bool video_set_geometry(const struct retro_game_geometry *geom){
    video_info.clip_w = geom->base_width;
    video_info.clip_h = geom->base_height;

    if (window) {
        refresh_vertex_data();

        int ow = 0, oh = 0;
        resize_to_aspect(geom->aspect_ratio, geom->base_width, geom->base_height, &ow, &oh);

        ow *= SCALE;
        oh *= SCALE;

        SDL_SetWindowSize(window, ow, oh);
    }

    return true;
}

bool video_set_pixel_format(unsigned format) {
	switch (format) {
        case RETRO_PIXEL_FORMAT_0RGB1555:
            video_info.pixfmt = GL_UNSIGNED_SHORT_5_5_5_1;
            video_info.pixtype = GL_BGRA;
            video_info.bpp = sizeof(uint16_t);
            break;
        case RETRO_PIXEL_FORMAT_XRGB8888:
            video_info.pixfmt = GL_UNSIGNED_INT_8_8_8_8_REV;
            video_info.pixtype = GL_BGRA;
            video_info.bpp = sizeof(uint32_t);
            break;
        case RETRO_PIXEL_FORMAT_RGB565:
            video_info.pixfmt  = GL_UNSIGNED_SHORT_5_6_5;
            video_info.pixtype = GL_RGB;
            video_info.bpp = sizeof(uint16_t);
            break;
        default:
            return false;
	}

	return true;
}