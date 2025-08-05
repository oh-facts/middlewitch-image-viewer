/*
Have different views
Single image views
One, where every image is visible one after the other.
option to change # of rows / col
Like in a file explorer
[x] Even draw folders
[x] left, right key to go left / right
[x] up / down to go up down folders

styling
image border
background
tint

[ ] Add file / folder navitation first.
text rendering
try freetype
copy code over

Panels

// side bar tree
// tab to toggle its rendering

// use hashmapfor image nav ?
// maybe something like the one you had in your asset system
// use keys only
 
// option to see clipboard / screenshots

// texture streaming
// draw file tree on the side
// inotify

[] glyph map
[] text rendering
[] ui layer
[] stuff

[] leave platform and layer set up here and app stuff inside viewer.c

[] folder nav animation
*/

#include <dirent.h>
#include <sys/stat.h>
#include <uchar.h>

#include <stdint.h>
#define STB_SPRINTF_IMPLEMENTATION
#include <stb/stb_sprintf.c>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.c>

#include <glad/glad.h>
#include <glad/glad.c>
#include <stdio.h>
#include <math.h>

#include <SDL3/SDL.h>

#include <base/base_core.h>
#include <os/os_core.h>
#include <base/base_arena.h>
#include <base/base_string.h>
#include <base/base_file.h>
#include <base/base_math.h>
#include <render/render_core.h>
#include <render/render_group.h>
#include <render/render_opengl.h>
#include <viewer/viewer.h>

#include <base/base_arena.c>
#include <base/base_string.c>
#include <base/base_file.c>

#include <freetype/freetype.h>
#include <glyph/glyph.h>

#if defined(OS_WIN32)
#undef function
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define function static
#include <os/os_win32.c>
#elif defined(OS_LINUX)
#undef function
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#include <time.h>
#define function static
#include <os/os_unix.c>
#elif defined(OS_APPLE)
#undef function
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#include <time.h>
#define function static
#include <os/os_unix.c>
#else
#error platform not supported
#endif

#include <glyph/glyph.c>
#include <render/render_group.c>
#include <render/render_opengl.c>
#include <viewer/viewer.c>

int main(int argc, char *argv[])
{
	if (argc == 2)
	{
		int file_exists = SDL_GetPathInfo(argv[1], 0);
		
		if (file_exists)
		{
			SDL_Init(SDL_INIT_VIDEO);
			SDL_Window *win = SDL_CreateWindow("Viewer", 960, 540, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
			
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			
			SDL_GLContext glctx = SDL_GL_CreateContext(win);
			
			Arena *perm = arenaAllocSized(MB(1), MB(8), 1);
			Arena *frame = arenaAllocSized(MB(32), MB(32), 0);
			r_backend_init(frame);
			font_state_init(frame);
			
			u64 start = os_getPerfCounter();
			u64 freq = os_getPerfFreq();
			
			f64 time_elapsed = 0;
			f64 delta = 0;
			
			b32 run = 1;
			
			viewer = pushArray(perm, Viewer, 1);
			viewer->perm = perm;
			viewer->frame = frame;
			viewer->file_slots_count = 256;
			viewer->file_slots = pushArray(perm, Viewer_FileSlot, viewer->file_slots_count);
			viewer->tex_slots_count = 256;
			viewer->tex_slots = pushArray(perm, Viewer_TextureSlot, viewer->tex_slots_count);
			
			Str8 app_dir = os_getAppDir(frame);
			
			u32 pix = 0xFFFFFFFF;
			R_Texture *white_square = r_allocTexture(&pix, 1, 1, 0);
			
			u32 color1_u32 = 0xFF111111;
			u32 color2_u32 = 0xFF333333;
			
			u32 tex_data[4] = {color1_u32, color2_u32, color2_u32, color1_u32};
			
			R_Texture *uv_bg = r_allocTexture(tex_data, 2, 2, 0);
			R_Texture *folder_icon = loadTextureFromPath(push_str8f(frame, "%.*s/data/folder_icon.png", str8_varg(app_dir)));
			
			b32 dragging = 0;
			
			f32 x_l = 0;
			f32 x_y = 0;
			
			f32 counter = 0;
			
			char real_path_cstr[256] = {0};
			realpath(argv[1], real_path_cstr);
			Str8 real_path = push_str8f(frame, "%s", real_path_cstr);
			
			Str8 workspace_dir = dirFromFile(perm, real_path);
			
			Viewer_File *current_folder = viewer_fileFromPath(workspace_dir, Viewer_FileKind_Dir, 1);
			Viewer_File *current_vf = viewer_fileFromPath(real_path, Viewer_FileKind_Dir, 0);
			
			for (Str8 parent_parent_path = push_str8f(frame, "%.*s", str8_varg(current_folder->path));
					 1;
					 parent_parent_path = push_str8f(frame, "%.*s/..", str8_varg(parent_parent_path)))
			{
				pushArray_zero(frame, u8, 1);
				
				char real_path_cstr[256] = {0};
				realpath(parent_parent_path.c, real_path_cstr);
				Str8 real_path = push_str8f(frame, "%s", real_path_cstr);
				
				Viewer_File *f = viewer_fileFromPath(real_path, Viewer_FileKind_Dir, 1);
				
				if (real_path.len == 1)
				{
					break;
				}
			}
			
			for(;run;)
			{
				ArenaTemp temp = arenaTempBegin(frame);
				
				f64 time_since_last = time_elapsed;
				
				Str8 win_title = push_str8f(frame, "%.*s | %.2f MB | %.2f MB | %.2f KB", str8_varg(current_vf->path), To_MB(viewer->tex_memory), To_MB(perm->used), To_KB(frame->used));
				pushArray_zero(frame, u8, 1);
				
				SDL_SetWindowTitle(win, win_title.c);
				
				f32 x, y;
				SDL_GetMouseState(&x, &y);
				
				s32 w, h;
				SDL_GetWindowSize(win, &w, &h);
				
				f32 window_aspect = (f32)w / h;
				
				bool filter_changed = false;
				
				SDL_Event event;
				while (SDL_PollEvent(&event)) 
				{
					if (event.type == SDL_EVENT_QUIT) 
					{
						run = false;
					}
					else if (event.type == SDL_EVENT_KEY_DOWN)
					{
						//					if (!event.key.repeat)
						{
							switch (event.key.key)
							{
								case SDLK_ESCAPE:
								{
									run = false;
								}break;
								case SDLK_LEFT:
								{
									if (current_vf->prev)
									{
										current_vf = current_vf->prev;
									}
									else
									{
										current_vf = current_vf->parent->last;
									}
									
								}break;
								case SDLK_RIGHT:
								{
									if (current_vf->next)
									{
										current_vf = current_vf->next;
									}
									else
									{
										current_vf = current_vf->parent->first;
									}
								}break;
								case SDLK_DOWN:
								{
									if (current_vf->kind == Viewer_FileKind_Dir)
									{
										if (!current_vf->children_loaded)
										{
											current_vf = viewer_fileFromPath(current_vf->path, Viewer_FileKind_Dir, 1);
										}
										
										if (current_vf->first)
										{
											current_vf = current_vf->first;
										}
									}
									
								}break;
								case SDLK_UP:
								{
									if (!str8_equals(current_vf->parent->path, str8_lit("/")))
									{
										current_vf = current_vf->parent;
									}
								}break;
								case SDLK_R:
								{
									current_vf->target_offset = (V2F){0};
									current_vf->target_zoom = 1;
								}break;
								
								case SDLK_SPACE:
								{
									current_vf->filtering = !current_vf->filtering;
									filter_changed = true;
								}break;
							}
						}
					}
					else if (event.type == SDL_EVENT_KEY_UP)
					{
						
					}
					else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
					{
						if (event.button.button == SDL_BUTTON_LEFT)
						{
							dragging = 1;
						}
					}
					else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
					{
						if (event.button.button == SDL_BUTTON_LEFT)
						{
							dragging = 0;
						}
					}
					else if (event.type == SDL_EVENT_MOUSE_WHEEL)
					{
						f32 zoom_factor = 1.1f;
						
						f32 prev_zoom = current_vf->target_zoom;
						
						f32 img_x = (x - w / 2.0f - current_vf->target_offset.x) / prev_zoom;
						f32 img_y = (y - h / 2.0f - current_vf->target_offset.y) / prev_zoom;
						
						current_vf->target_zoom *= powf(zoom_factor, event.wheel.y);
						current_vf->target_zoom = max(current_vf->target_zoom, 0.1f);
						
						current_vf->target_offset.x = x - w / 2.0f - img_x * current_vf->target_zoom;
						current_vf->target_offset.y = y - h / 2.0f - img_y * current_vf->target_zoom;
					}
				}
				
				counter += delta;
				
				if (dragging)
				{
					current_vf->target_offset.x += x - x_l;
					current_vf->target_offset.y += y - x_y;
				}
				
				x_l = x;
				x_y = y;
				
				Render_Cmds cmds = begin_render_cmds(frame, white_square, (V4F){1, 1, 1, 1}, w, h);
				
				// draw bg
				{
					V2F tile_size = {16, 16};
					V2F src_size = {w, h};
					src_size.x /= tile_size.x;
					src_size.y /= tile_size.y;
					
					push_texture_section(&cmds, uv_bg, (V2F){0}, (V2F){w, h}, (RectF){(V2F){0}, src_size});
				}
				
				// draw main tex
				switch (current_vf->kind)
				{
					case Viewer_FileKind_Tex:
					{
						R_Texture *tex = viewer_textureFromPath(current_vf->path);
						
						if (filter_changed)
						{
							r_setTextureFiltering(tex, current_vf->filtering);
						}
						
						f32 smoothing = 0.1f;
						current_vf->current_zoom += (current_vf->target_zoom - current_vf->current_zoom) * smoothing;
						
						f32 img_aspect_ratio = tex->size.x / tex->size.y;
						
						current_vf->current_offset.x += (current_vf->target_offset.x - current_vf->current_offset.x) * smoothing;
						current_vf->current_offset.y += (current_vf->target_offset.y - current_vf->current_offset.y) * smoothing;
						
						f32 scale = current_vf->current_zoom;
						
						V2F size = {0};
						if (window_aspect > img_aspect_ratio) 
						{
							size.y = h * scale;
							size.x = size.y * img_aspect_ratio;
						} else 
						{
							size.x = w * scale;
							size.y = size.x / img_aspect_ratio;
						}
						
						V2F pos = {w / 2 - size.x / 2, h / 2 - size.y / 2};
						pos.x += current_vf->current_offset.x;
						pos.y += current_vf->current_offset.y;
						
						push_texture(&cmds, tex, pos, size);
					}break;
					
					case Viewer_FileKind_Dir:
					{
						f32 img_aspect_ratio = 1.f;
						
						V2F size = {0};
						if (window_aspect > img_aspect_ratio) 
						{
							size.y = h;
							size.x = size.y * img_aspect_ratio;
						}
						else 
						{
							size.x = w;
							size.y = size.x / img_aspect_ratio;
						}
						
						V2F pos = {w / 2 - size.x / 2, h / 2 - size.y / 2};
						
						push_texture(&cmds, folder_icon, pos, size);
					}break;
					
					case Viewer_FileKind_Other:
					case Viewer_FileKind_Count:
					default:
					{
						assert(false && "Control shouldn't be here");
					}break;
				}
				
				push_glyphs(&cmds, win_title, (V2F){0, 64}, 32, COLOR_WHITE);
				
				r_submit(win, cmds);
				end_render_cmds(&cmds);
				
				if (((viewer->ticks % 64) == 0) && (viewer->tex_memory > GB(1)))
				{
					for (int i = 0; i < viewer->tex_slots_count; i+=1)
					{
						Viewer_TextureSlot *slot = viewer->tex_slots + i;
						for (Viewer_Texture *cur = slot->first; cur; cur = cur->hash_next)
						{
							if (cur->last_touched_tick + 32 < viewer->ticks)
							{
								printf("freed %lu\n", cur->hash);
								
								if (cur->hash_prev)
								{
									cur->hash_prev->hash_next = cur->hash_next;
								}
								else
								{
									slot->first = cur->hash_next;
								}
								
								if (cur->hash_next)
								{
									cur->hash_next->hash_prev = cur->hash_prev;
								}
								else
								{
									slot->last = cur->hash_prev;
								}
								
								viewer_textureFree(cur);
								// free texture
								// remove from map
								// etc. etc.
							}
						}
					}
				}
				
				viewer->ticks += 1;
				
				arenaTempEnd(&temp);
				
				u64 end = os_getPerfCounter();
				time_elapsed = (end - start) / (freq * 1.f);
				delta = time_elapsed - time_since_last;
			}
		}
		else
		{
			printf("file not found\n");
		}
	}
	else
	{
		printf("Correct Usage: ./mw_viewer path\n");
	}
	
	printf("Quit\n");
}