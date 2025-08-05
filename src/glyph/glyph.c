function void font_state_init(Arena *frame)
{
	Arena *arena = arenaAlloc();
	font_state = pushArray(arena, Font_State, 1);
	font_state->arena = arena;
	
	font_state->count = 256;
	font_state->slots = pushArray(arena, Glyph_Slot, font_state->count);
	
	font_state->app_dir = os_getAppDir(arena);
	font_state->font_path = push_str8f(arena, "%.*s/data/delius.ttf");
	FileData font_data = readFile(arena, font_state->font_path);
	
	FT_Init_FreeType(&font_state->library);
	FT_New_Memory_Face(font_state->library, font_data.bytes, font_data.size, 0, &font_state->face);
	font_state->frame = frame;
}

// documenting old stupid bug
// I can't keep strings and use as keys if I am allocating it from transient memory
// Its probably best to keep the hash around and reuse that or use the struct form of keys
// instead of data form for cases where the string is derived from the data

function Glyph *glyph_from_key(u8 cp, int cp_size)
{
	Glyph *out = 0;
	
	u64 data[2] = 
	{
		cp,
		cp_size,
	};
	
	u64 data_size = sizeof(data);
	u64 hash = hash_str8(str8((u8*)data, data_size));
	
	int slot_idx = hash % font_state->count;
	Glyph_Slot *slot = font_state->slots + slot_idx;;
	
	// check if node already exists
	
	for (Glyph *cur = slot->first; cur; cur = cur->hash_next)
	{
		if (cur->hash == hash)
		{
			out = cur;
			break;
		}
	}
	
	// if not, allocate
	if (!out)
	{
		// allocate glyph node
		{
			out = pushArray(font_state->arena, Glyph, 1);
			
			if (!slot->first)
			{
				slot->first = out;
				slot->last = out;
			}
			else
			{
				slot->last->hash_next = out;
				slot->last = out;
			}
		}
		
		// init with key stuff
		{
			out->hash = hash;
			out->cp = cp;
			out->cp_size = cp_size;
		}
		
		// init with freetype stuff
		{
			FT_Set_Char_Size(font_state->face, 0, (cp_size * 64), 0, 96);
			
			out->ascent = font_state->face->ascender >> 6;
			out->descent = font_state->face->descender >> 6;
			
			u32 ch = 0;
			mbstate_t mb_state = {0};
			mbrtoc32(&ch, &cp, 1, &mb_state);
			
			int index = FT_Get_Char_Index(font_state->face, ch);
			
			FT_Load_Glyph(font_state->face, index, FT_LOAD_DEFAULT | FT_LOAD_RENDER);
			
			{
				out->advance_x = font_state->face->glyph->advance.x >> 6;
				out->advance_y = font_state->face->glyph->advance.y >> 6;
				out->offset_x = font_state->face->glyph->bitmap_left;
				out->offset_y = font_state->face->glyph->bitmap_top;
				out->height = font_state->face->size->metrics.height >> 6;
			}
			
			u8 *bytes = font_state->face->glyph->bitmap.buffer;
			
			// no bytes means the glyph has no texture
			// but other values are probably valid
			if (bytes)
			{
				int width = font_state->face->glyph->bitmap.width;
				int height = font_state->face->glyph->bitmap.rows;
				
				int in_size = width * height;
				int out_size = width * height * 4;
				
				u8 *buffer = pushArray(font_state->frame, u8, out_size);
				
				int i = 0;
				int j = 0;
				
				while (i != in_size)
				{
					int a = ((u8*)bytes)[i];
					
					buffer[j + 0] = 255;
					buffer[j + 1] = 255;
					buffer[j + 2] = 255;
					buffer[j + 3] = a;
					
					i += 1;
					j += 4;
				}
				
				out->tex = r_allocTexture(buffer, width, height, false);
			}
		}
	}
	
	return out;
	//print("% %\n", hash, i);
}