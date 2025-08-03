/* date = July 29th 2025 1:06 am */

#ifndef BASE_FILE_H
#define BASE_FILE_H
typedef struct FileData FileData;
struct FileData
{
	u8 *bytes;
	u64 size;
};

function FileData readFile(Arena *arena, Str8 filepath);
function Str8 fileNameFromPath(Arena *arena, Str8 path);
#endif //BASE_FILE_H