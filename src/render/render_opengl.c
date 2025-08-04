global GLuint rect_shader;
global GLuint rect_vao;
global GLuint rect_vbo;
global GLuint u_screen_size;
global Arena *gl_arena;
global R_Texture *gl_free_textures;

function void r_backend_init(Arena *scratch)
{
	gl_arena = arenaAlloc();
	gladLoadGL();
	
	glGenVertexArrays(1, &rect_vao);  
	
	glBindVertexArray(rect_vao);
	
	glGenBuffers(1, &rect_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, rect_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(R_Vertex) * MAX_VERTICES, 0, GL_DYNAMIC_DRAW);
	
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(R_Vertex), (void*)0);
	glEnableVertexAttribArray(0);  
	
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(R_Vertex), (void*)(sizeof(V2F)));
	glEnableVertexAttribArray(1);
	
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(R_Vertex), (void*)(sizeof(V2F) * 2));
	glEnableVertexAttribArray(2);
	
	Str8 app_dir = os_getAppDir(scratch);
	
	Str8 full_vert_path = str8_join(scratch, app_dir, str8_lit("src/shaders/rect.vert"));
	pushArray(scratch, u8, 1);
	Str8 full_frag_path = str8_join(scratch, app_dir, str8_lit("src/shaders/rect.frag"));
	pushArray(scratch, u8, 1);
	
	FileData vert_data = readFile(scratch, full_vert_path);
	pushArray(scratch, u8, 1);
	FileData frag_data = readFile(scratch, full_frag_path);
	pushArray(scratch, u8, 1);
	
	rect_shader = make_shader_program(vert_data.bytes, frag_data.bytes);
	u_screen_size = glGetUniformLocation(rect_shader, "u_screen_size");
}

function GLuint make_shader_program(char *vertexShaderSource, char *fragmentShaderSource)
{
	GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert_shader, 1, (const char *const *)&vertexShaderSource, 0);
	glCompileShader(vert_shader);
	checkCompileErrors(vert_shader, "vertex shader");
	
	GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag_shader, 1, (const char *const *)&fragmentShaderSource, 0);
	glCompileShader(frag_shader);
	checkCompileErrors(frag_shader, "fragment shader");
	
	GLuint shader_prog = glCreateProgram();
	glAttachShader(shader_prog, vert_shader);
	glAttachShader(shader_prog, frag_shader);
	
	glLinkProgram(shader_prog);
	checkLinkErrors(shader_prog, "vert/frag shader");
	
	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
	
	return shader_prog;
}

function void checkCompileErrors(GLuint shader, const char *type)
{
	int success;
	char infoLog[1024];
	
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader, 1024, 0, infoLog);
		printf("\n%s compilation error:\n%s\n", type, infoLog);
	}
}

function void checkLinkErrors(GLuint shader, const char *type)
{
	int success;
	char infoLog[1024];
	glGetProgramiv(shader, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shader, 1024, 0, infoLog);
		printf("\n%s linking error:\n%s\n", type, infoLog);
	}
}

function R_Texture *r_allocTexture(void *bytes, int w, int h, b32 filtering)
{
	R_Texture *out = gl_free_textures;
	
	if (out)
	{
		gl_free_textures = gl_free_textures->next;
		*out = (R_Texture){0};
	}
	else
	{
		out = pushArray(gl_arena, R_Texture, 1);
	}
	
	out->size.x = w;
	out->size.y = h;
	
	unsigned int texture;
	glGenTextures(1, &texture);
	
	glBindTexture(GL_TEXTURE_2D, texture);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	if (filtering)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
	
	if (filtering)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	
	out->ogl_id = texture;
	
	return out;
}

function void r_freeTexture(R_Texture *tex)
{
	glDeleteTextures(1, &tex->ogl_id);
	tex->next = gl_free_textures;
	gl_free_textures = tex;
}

function void r_submit(SDL_Window *win, Render_Cmds cmds)
{
	int w, h;
	SDL_GetWindowSize(win, &w, &h);
	
	glViewport(0, 0, w, h);
	glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glUseProgram(rect_shader);
	glBindVertexArray(rect_vao);
	
	glBindBuffer(GL_ARRAY_BUFFER, rect_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(R_Vertex) * cmds.current_vertices, cmds.vertices);
	
	glUniform2f(u_screen_size, w, h);
	
	int start = 0;
	
	for (Rect_Group *cur = cmds.first; cur; cur = cur->next)
	{
		glBindTexture(GL_TEXTURE_2D, cur->texture->ogl_id);
		glDrawArrays(GL_TRIANGLES, start, cur->count);
		start += cur->count;
	}
	
	SDL_GL_SwapWindow(win);
}