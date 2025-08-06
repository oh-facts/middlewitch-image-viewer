/* date = July 29th 2025 1:08 am */

#ifndef BASE_STRING_H
#define BASE_STRING_H
#define str8_varg(S) (int)((S).len), ((S).c)

typedef struct Str8 Str8;
struct Str8
{
	u8 *c;
	u64 len;
};

#define str8_lit(c) (Str8){(u8*)c, sizeof(c) - 1}

function u64 cstr8Len(char *c);
function Str8 str8(u8 *c, u64 len);
function Str8 str8_cpy(Arena *arena, Str8 src);
function void str8_memcpy(Str8 *dst, Str8 src);
function Str8 push_str8fv(Arena *arena, char *fmt, va_list args);
function Str8 push_str8f(Arena *arena, char *fmt, ...);
function b32 str8_equals(Str8 a, Str8 b);
function Str8 str8_join(Arena *arena, Str8 a, Str8 b);
#endif //BASE_STRING_H
