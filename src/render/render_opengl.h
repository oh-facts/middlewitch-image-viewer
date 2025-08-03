/* date = July 29th 2025 4:18 am */

#ifndef RENDER_OPENGL_H
#define RENDER_OPENGL_H

function void r_backend_init(Arena *scratch);

function GLuint make_shader_program(char *vertexShaderSource, char *fragmentShaderSource);
function void checkCompileErrors(GLuint shader, const char *type);
function void checkLinkErrors(GLuint shader, const char *type);

#endif //RENDER_OPENGL_H
