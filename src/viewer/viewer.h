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

typedef struct Viewer_File Viewer_File;
struct Viewer_File
{
	// Tree links
	Viewer_File *first;
	Viewer_File *last;
	Viewer_File *next;
	Viewer_File *prev;
	Viewer_File *parent;
	
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
	R_Texture *tex;
	V2F current_offset;
	V2F target_offset;
	
	f32 current_zoom;
	f32 target_zoom;
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
};

global Viewer *viewer;

function void dir_enumerate(Viewer_File *root, char *dirname);
function R_Texture *loadTextureFromPath(Str8 path);
function Str8 dirFromFile(Arena *arena, Str8 path);

#endif //VIEWER_H
