/* date = July 30th 2025 3:55 am */

#ifndef RENDER_GROUP_H
#define RENDER_GROUP_H

typedef struct Rect_Group Rect_Group;
struct Rect_Group
{
	Rect_Group *next;
	R_Texture *texture;
	int count;
};

#define MAX_RECTS 100000
#define MAX_VERTICES (MAX_RECTS * 4)

typedef struct Render_Cmds Render_Cmds;
struct Render_Cmds
{
	V4F clear_color;
	Rect_Group *first;
	Rect_Group *last;
	int group_count;
	
	Rect_Group *free;
	int free_group_count;
	R_Vertex *vertices;
	int current_vertices;
	int requested_vertices;
	
	int w;
	int h;
	R_Texture *white_square;
	int frames;
	
	// user arena
	Arena *arena;
};

function void push_rect(Render_Cmds *cmds, V2F pos, V2F size, V4F color);
function void push_texture(Render_Cmds *cmds, R_Texture *texture, V2F pos, V2F size);
function void push_texture_section(Render_Cmds *cmds, R_Texture *texture, V2F pos, V2F size, RectF src);
#endif //RENDER_GROUP_H