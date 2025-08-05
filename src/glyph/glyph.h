/* date = August 4th 2025 7:05 am */

#ifndef GLYPH_H
#define GLYPH_H

typedef struct Glyph Glyph;
struct Glyph
{
	Glyph *hash_next;
	
	f32 advance_x;
	f32 advance_y;
	f32 offset_x;
	f32 offset_y;
	f32 ascent;
	f32 descent;
	int font_ascent;
	
	R_Texture *tex;
	f32 height;
	
	// key data
	u32 cp;
	int cp_size;
	
	u64 hash;
};

typedef struct Glyph_Slot Glyph_Slot;
struct Glyph_Slot
{
	Glyph *first;
	Glyph *last;
};

typedef struct Font_State Font_State;
struct Font_State
{
	Arena *arena;
	Arena *frame;
	
	Glyph_Slot *slots;
	int count;
	FT_Library library;
	FT_Face face;
	
	Str8 app_dir;
	Str8 font_path;
};

global Font_State *font_state;

function void font_state_init(Arena *frame);
function Glyph *glyph_from_key(u8 cp, int size);
#endif //GLYPH_H