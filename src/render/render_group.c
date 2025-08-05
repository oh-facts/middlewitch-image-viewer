function Render_Cmds begin_render_cmds(Arena *arena, R_Texture *white_square, V4F clear_color, int w, int h)
{
	Render_Cmds out = {0};
	out.arena = arena;
	out.clear_color = clear_color;
	out.w = w;
	out.h = h;
	out.vertices = pushArray(arena, R_Vertex, MAX_VERTICES);
	out.white_square = white_square;
	return out;
}

function void end_render_cmds(Render_Cmds *cmds)
{
	if (cmds->requested_vertices > MAX_VERTICES)
	{
		// I usually use a chained linked list like in the render group 
		//Even reallocing more space works.
		// but just allocating even more space almost always works unless max count is unsafe to predict
		printf("Allocate more space for the cmd buffer\n %d vertices needed", cmds->requested_vertices);
	}
	
	// push render groups to free list
	cmds->last->next = cmds->free;
	cmds->free = cmds->first;
	cmds->free_group_count += cmds->group_count;
	
	cmds->first = 0;
	cmds->last = 0;
	cmds->frames += 1;
	cmds->group_count = 0;
	cmds->current_vertices = 0;
	cmds->requested_vertices = 0;
	cmds->arena = 0;
}

function Rect_Group *push_render_group(Render_Cmds *cmds)
{
	// check free list. if not present, allocate new
	Rect_Group *out = cmds->free;
	
	if (out)
	{
		cmds->free = cmds->free->next;
		cmds->free_group_count -= 1;
	}
	else
	{
		out = pushArray(cmds->arena, Rect_Group, 1);
		//printf("new group\n");
	}
	
	*out = (Rect_Group){0};
	
	if (cmds->last)
	{
		cmds->last->next = out;
		cmds->last = out;
	}
	else
	{
		cmds->last = out;
		cmds->first = out;
	}
	
	cmds->group_count += 1;    
	return out;
}

function void push_cool_texture(Render_Cmds *cmds, R_Texture *texture, V2F pos, V2F size, RectF src, V4F color_0, V4F color_1, V4F color_2, V4F color_3)
{
	if (cmds->current_vertices < MAX_VERTICES)
	{
		Rect_Group *group = cmds->last;
		
		src.pos.x /= texture->size.x;
		src.pos.y /= texture->size.y;
		
		src.size.x /= texture->size.x;
		src.size.y /= texture->size.y;
		
		if (group)
		{
			if (texture->ogl_id != group->texture->ogl_id)
			{
				group = push_render_group(cmds);
				group->texture = texture;
			}
		}
		else
		{
			group = push_render_group(cmds);
			group->texture = texture;
		}
		
		group->count += 6;
		
		R_Vertex *vertices = cmds->vertices + cmds->current_vertices;
		
		// Top-left
		vertices[0].pos.x = pos.x;
		vertices[0].pos.y = pos.y;
		vertices[0].uv.x = src.pos.x;
		vertices[0].uv.y = src.pos.y;
		vertices[0].color = color_0;
		
		// Bottom-left
		vertices[1].pos.x = pos.x;
		vertices[1].pos.y = pos.y + size.y;
		vertices[1].uv.x = src.pos.x;
		vertices[1].uv.y = src.pos.y + src.size.y;
		vertices[1].color = color_1;
		
		// Top-right
		vertices[2].pos.x = pos.x + size.x;
		vertices[2].pos.y = pos.y;
		vertices[2].uv.x = src.pos.x + src.size.x;
		vertices[2].uv.y = src.pos.y;
		vertices[2].color = color_2;
		
		// Top-right
		vertices[3].pos.x = pos.x + size.x;
		vertices[3].pos.y = pos.y;
		vertices[3].uv.x = src.pos.x + src.size.x;
		vertices[3].uv.y = src.pos.y;
		vertices[3].color = color_2;
		
		// Bottom-left
		vertices[4].pos.x = pos.x;
		vertices[4].pos.y = pos.y + size.y;
		vertices[4].uv.x = src.pos.x;
		vertices[4].uv.y = src.pos.y + src.size.y;
		vertices[4].color = color_1;
		
		// Bottom-right
		vertices[5].pos.x = pos.x + size.x;
		vertices[5].pos.y = pos.y + size.y;
		vertices[5].uv.x = src.pos.x + src.size.x;
		vertices[5].uv.y = src.pos.y + src.size.y;
		vertices[5].color = color_3;
		
		cmds->current_vertices += 6;
	}
}

function inline void push_rect(Render_Cmds *cmds, V2F pos, V2F size, V4F color)
{
	push_cool_texture(cmds, cmds->white_square, pos, size, (RectF){(V2F){0}, (V2F){1,1}}, color, color, color, color);
}

function inline void push_texture(Render_Cmds *cmds, R_Texture *texture, V2F pos, V2F size)
{
	push_cool_texture(cmds, texture, pos, size, (RectF){(V2F){0}, texture->size}, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE);
}

function inline void push_texture_section(Render_Cmds *cmds, R_Texture *texture, V2F pos, V2F size, RectF src)
{
	push_cool_texture(cmds, texture, pos, size, src, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE);
}

function void push_glyphs(Render_Cmds *cmds, Str8 text, V2F pos, int pt_size, V4F color)
{
	f32 x = pos.x;
	f32 y = pos.y;
	
	for (int i = 0; i < text.len; i+=1)
	{
		Glyph *g = glyph_from_key(text.c[i], pt_size);
		
		if (g->tex)
		{
			V2F size = g->tex->size;
			f32 pos_y = g->offset_y;
			V2F dp = {x + g->offset_x, y - pos_y};
			push_cool_texture(cmds, g->tex, dp, size, (RectF){(V2F){0}, g->tex->size}, color, color, color, color);
		}
		
		x += g->advance_x;
		
	}
}