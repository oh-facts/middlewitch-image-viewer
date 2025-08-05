/* date = August 1st 2025 4:54 am */

#ifndef VIEWER_H
#define VIEWER_H

typedef enum Viewer_FileKind Viewer_FileKind;
enum Viewer_FileKind
{
	Viewer_FileKind_Other,
	Viewer_FileKind_Tex,
	Viewer_FileKind_Dir,
	Viewer_FileKind_Count,
};

typedef struct Viewer_Texture Viewer_Texture;
struct Viewer_Texture
{
	R_Texture *v;
	u64 hash;
	
	Viewer_Texture *free_next;
	
	Viewer_Texture *hash_next;
	Viewer_Texture *hash_prev;
	
	u64 last_touched_tick;
};

typedef struct Viewer_TextureSlot Viewer_TextureSlot;
struct Viewer_TextureSlot
{
	Viewer_Texture *first;
	Viewer_Texture *last;
};

typedef struct Viewer_File Viewer_File;
struct Viewer_File
{
	// Tree links
	Viewer_File *first;
	Viewer_File *last;
	Viewer_File *next;
	Viewer_File *prev;
	Viewer_File *parent;
	
	int children_count;
	
	// hash links
	Viewer_File *hash_next;
	Viewer_File *hash_prev;
	
	// file data
	Str8 path;
	
	Viewer_FileKind kind;
	
	b32 children_loaded;
	
	// comes from path
	u64 hash;
	
	// img data
	V2F current_offset;
	V2F target_offset;
	
	f32 current_zoom;
	f32 target_zoom;
	b32 filtering;
	
	// thumbnail data
	V2F thumbnail_current_offset;
	V2F thumbnail_target_offset;
	
	// 
	u64 last_drawn_tick;
};

typedef struct Viewer_FileSlot Viewer_FileSlot;
struct Viewer_FileSlot
{
	Viewer_File *first;
	Viewer_File *last;
};

typedef struct Viewer Viewer;
struct Viewer
{
	Arena *perm;
	Arena *frame;
	
	Viewer_FileSlot *file_slots;
	int file_slots_count;
	
	Viewer_TextureSlot *tex_slots;
	int tex_slots_count;
	
	Viewer_Texture *free_tex;
	
	u64 tex_memory;
	u64 memory;
	
	u64 ticks;
};

global Viewer *viewer;

// djb2
function unsigned long hash_str8(Str8 str);
function V2F textureSizeFromWidth(R_Texture *tex, f32 width);

function Viewer_File *viewer_fileAlloc(Arena *arena, Viewer_FileKind kind, Str8 path_);

function Viewer_File *viewer_fileFromPath(Str8 path, Viewer_FileKind kind, bool fill_children_if_dir);
function void viewer_equipFileWithParent(Viewer_File *p, Viewer_File *vf);
function void dir_enumerate(Viewer_File *root, char *dirname);
function R_Texture *loadTextureFromPath(Str8 path);
function Str8 dirFromFile(Arena *arena, Str8 path);
function void viewer_filePrint(Viewer_File *file);

function Viewer_Texture *viewer_textureAlloc(Arena *arena, Str8 path);
function void viewer_textureFree(Viewer_Texture *vt);
function R_Texture *viewer_textureFromPath(Str8 path);

function void viewer_init();
function void viewer_update();
#endif //VIEWER_H
