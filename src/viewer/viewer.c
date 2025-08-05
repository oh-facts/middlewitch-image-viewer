
// djb2
function unsigned long hash_str8(Str8 str)
{
	unsigned long hash = 5381;
	int c;
	
	for(u32 i = 0; i < str.len; i++)
	{
		c = str.c[i];
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}
	
	return hash;
}

function V2F textureSizeFromWidth(R_Texture *tex, f32 width)
{
	f32 tex_w = tex->size.x;
	f32 tex_h = tex->size.y;
	
	f32 aspect = tex_w / tex_h;
	
	V2F size = {0};
	
	size.y = width;
	size.x = width * aspect;
	
	return size;
}

function Viewer_File *viewer_fileAlloc(Arena *arena, Viewer_FileKind kind, Str8 path_)
{
	Viewer_File *vf = pushArray(arena, Viewer_File, 1);
	vf->kind = kind;
	vf->current_zoom = 0.5f;
	vf->target_zoom = 0.5f;
	vf->target_offset.y += 64;
	vf->current_offset.y += 64;
	
	Str8 path = push_str8f(arena, "%.*s", str8_varg(path_));
	
	vf->path = path;
	
	return vf;
}

function Viewer_File *viewer_fileFromPath(Str8 path, Viewer_FileKind kind, bool fill_children_if_dir)
{
	u64 hash = hash_str8(path);
	u64 slot_idx = hash % viewer->file_slots_count;
	Viewer_FileSlot *slot = viewer->file_slots + slot_idx;
	Viewer_File *out = 0;
	
	// check file cache
	for (Viewer_File *it = slot->first; it; it = it->hash_next)
	{
		if (it->hash == hash)
		{
			out = it;
			break;
		}
	}
	
	// if not found, allocate, add to cache and init with sdl 
	if (!out)
	{
		out = viewer_fileAlloc(viewer->perm, Viewer_FileKind_Dir, path);
		
		if (slot->last)
		{
			out->hash_prev = slot->last;
			slot->last->hash_next = out;
			slot->last = out;
		}
		else
		{
			slot->first = out;
			slot->last = out;
		}
		
		out->kind = kind;
		out->hash = hash;
	}
	
	if ((kind == Viewer_FileKind_Dir) && fill_children_if_dir)
	{
		out->children_loaded = true;	
		dir_enumerate(out, path.c);
	}
	
	return out;
}

function void viewer_equipFileWithParent(Viewer_File *p, Viewer_File *vf)
{
	if (p->last)
	{
		vf->prev = p->last;
		p->last->next = vf;
		p->last = vf;
	}
	else
	{
		p->first = vf;
		p->last = vf;
	}
	vf->parent = p;
	p->children_count += 1;
}

function void dir_enumerate(Viewer_File *root, char *dirname)
{
	DIR *dir = opendir(dirname);
	if (!dir) return;
	
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_name[0] == '.') continue;
		
		Viewer_File *vf = 0;
		
		char fullpath[1024];
		snprintf(fullpath, sizeof(fullpath), "%s/%s", dirname, entry->d_name);
		
		char real_path_cstr[256] = {0};
		realpath(fullpath, real_path_cstr);
		Str8 path = push_str8f(viewer->frame, "%s", real_path_cstr);
		//printf("%.*s\n\r", str8_varg(path));
		
		struct stat st;
		if (stat(fullpath, &st) == 0) 
		{
			if (S_ISDIR(st.st_mode)) 
			{
				vf = viewer_fileFromPath(path, Viewer_FileKind_Dir, 0);
			} 
			else if (S_ISREG(st.st_mode)) 
			{
				int x, y, channels;
				if (stbi_info(path.c, &x, &y, &channels))
				{
					vf = viewer_fileFromPath(path, Viewer_FileKind_Tex, 0);
				}
			}
		}
		
		if (vf)
		{
			viewer_equipFileWithParent(root, vf);
		}
		
	}
	
	closedir(dir);
}

function R_Texture *loadTextureFromPath(Str8 path)
{
	int w;
	int h;
	int n;
	
	u8 *bytes = stbi_load(path.c, &w, &h, &n, 4);
	
	R_Texture *out = r_allocTexture(bytes, w, h, 0);
	
	return out;
}

function Str8 dirFromFile(Arena *arena, Str8 path)
{
	Str8 out = {0};
	
	for (int i = 0; i < path.len; i++)
	{
		char c = path.c[path.len - i - 1];
		
		if ((c == '\\') || (c == '/'))
		{
			Str8 temp = str8(path.c, path.len - i);
			out = push_str8f(arena, "%.*s", str8_varg(temp));
			break;
		}
	}
	
	return out;
}

function void viewer_filePrint(Viewer_File *file)
{
	for (Viewer_File *cur = file->first; cur; cur = cur->next)
	{
		printf("%.*s\n", str8_varg(cur->path));
	}
}

function Viewer_Texture *viewer_textureAlloc(Arena *arena, Str8 path)
{
	Viewer_Texture *out = viewer->free_tex;
	
	if (out)
	{
		viewer->free_tex = viewer->free_tex->free_next;
		*out = (Viewer_Texture){0};
	}
	else
	{
		out = pushArray(arena, Viewer_Texture, 1);
	}
	
	out->v = loadTextureFromPath(path);
	viewer->tex_memory += out->v->size.x * out->v->size.y * 4;
	
	return out;
}

function void viewer_textureFree(Viewer_Texture *vt)
{
	r_freeTexture(vt->v);
	viewer->tex_memory -= vt->v->size.x * vt->v->size.y * 4;
	vt->free_next = viewer->free_tex;
	viewer->free_tex = vt;
}

function R_Texture *viewer_textureFromPath(Str8 path)
{
	u64 hash = hash_str8(path);
	u64 slot_idx = hash % viewer->tex_slots_count;
	Viewer_TextureSlot *slot = viewer->tex_slots + slot_idx;
	
	Viewer_Texture *out = 0;
	
	for (Viewer_Texture *it = slot->first; it; it = it->hash_next)
	{
		if (it->hash == hash)
		{
			out = it;
			break;
		}
	}
	
	if (!out)
	{
		out = viewer_textureAlloc(viewer->perm, path);
		
		if (slot->last)
		{
			out->hash_prev = slot->last;
			slot->last->hash_next = out;
			slot->last = out;
		}
		else
		{
			slot->first = out;
			slot->last = out;
		}
		
		out->hash = hash;
	}
	
	out->last_touched_tick = viewer->ticks;
	
	return out->v;
}