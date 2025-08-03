/* date = July 29th 2025 1:02 am */

#ifndef BASE_ARENA_H
#define BASE_ARENA_H

typedef struct Arena Arena;
struct Arena
{
	Arena *next;
	Arena *last;
	u64 used;
	u64 align;
	u64 cmt;
	u64 res;
	b32 chain;
};

function b32 isPow2(size_t addr);

#define ARENA_COMMIT_SIZE KB(64)
#define ARENA_RESERVE_SIZE MB(64)
#define ARENA_HEADER_SIZE 128
#define ARENA_ARR_LEN(arena, type) (arena->used / sizeof(type))

#ifndef min
#define min(A,B) (((A)<(B))?(A):(B))
#define max(A,B) (((A)>(B))?(A):(B))
#endif

typedef struct ArenaTemp ArenaTemp;
struct ArenaTemp
{
	Arena *arena;
	u64 pos;
};

#define pushArray(arena,type,count) (type*)arenaPushImpl(arena, sizeof(type) * count)
#define pushArray_zero(arena,type,count) (type*)(memset(arenaPushImpl(arena, sizeof(type) * count), 0, sizeof(type) * count))

function void* arenaPushImpl(Arena* arena, size_t size);
function ArenaTemp arenaTempBegin(Arena *arena);
function void arenaTempEnd(ArenaTemp *temp);
function Arena *arenaAllocSized(u64 cmt, u64 res, b32 chain);
function Arena *arenaAlloc();
function void arenaFree(Arena *arena);
#endif //BASE_ARENA_H