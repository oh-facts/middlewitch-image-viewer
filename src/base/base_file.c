#if defined(OS_WIN32)
#define fileOpenImpl(file, filepath, mode) fopen_s(file, filepath, mode)
#elif defined (OS_LINUX) || defined (OS_APPLE)
#define fileOpenImpl(file, filepath, mode) *file = fopen(filepath, mode)
#endif

function FileData readFile(Arena *arena, Str8 filepath)
{
	FileData out = {0};
	FILE *file;
	
	fileOpenImpl(&file, filepath.c, "rb");
	
	fseek(file, 0, SEEK_END);
	
	out.size = ftell(file);
	
	fseek(file, 0, SEEK_SET);
	
	out.bytes = pushArray(arena, u8, out.size);
	fread(out.bytes, sizeof(u8), out.size, file);
	
	fclose(file);
	
	return out;
}

function Str8 fileNameFromPath(Arena *arena, Str8 path)
{
	char *cur = (char*)&path.c[path.len - 1];
	u32 count = 0;
	
	//NOTE(mizu): pig
	while(*cur != '/' && *cur != '\\' && *cur != '\0')
	{
		cur--;
		count++;
	}
	
	Str8 file_name_cstr = {0};
	file_name_cstr.c = pushArray(arena, u8, count + 1);
	file_name_cstr.len = count + 1;
	memcpy(file_name_cstr.c, cur + 1, count);
	file_name_cstr.c[count] = '\0';
	
	return file_name_cstr;
}