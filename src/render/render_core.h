/* date = July 30th 2025 3:56 am */

#ifndef RENDER_CORE_H
#define RENDER_CORE_H

#define COLOR_WHITE ((V4F){1, 1, 1, 1})
#define COLOR_BLACK ((V4F){0})
#define COLOR_RED ((V4F){1, 0, 0, 1})

// Needs to match shader
typedef struct R_Vertex R_Vertex;
struct R_Vertex
{
	V2F pos;
	V2F uv;
	V4F color;
};

typedef struct R_Texture R_Texture;
struct R_Texture
{
	R_Texture *next;
	u32 ogl_id;
	V2F size;
};

// Implemented per backend
function R_Texture *r_allocTexture(void *bytes, int w, int h, b32 filtering);
function void r_freeTexture(R_Texture *tex);
function void r_setTextureFiltering(R_Texture *tex, b32 filtering);

#endif //RENDER_CORE_H
