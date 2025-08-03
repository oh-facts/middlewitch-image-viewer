function b32 isPow2(size_t addr)
{
	return (addr & (addr-1)) == 0;
}

function void* arenaPushImpl(Arena* arena_first, size_t size)
{
    Arena *arena = arena_first->last;
    
	u64 pos_mem = alignPow2(arena->used, arena->align);
	u64 pos_new = pos_mem + size;
	
	if((arena->res < pos_new))
	{
		assert(arena->chain && "arena marked as don't chain but grew");
        
        u64 memory_required = alignPow2(size + ARENA_HEADER_SIZE, arena->align);
        u64 res = arena->res;
        u64 cmt = arena->cmt;
        if (memory_required > res)
        {
            res = memory_required;
            cmt = memory_required;
        }
        
        Arena *new_block = arenaAllocSized(cmt, res, arena->chain);
        
        arena->last->next = new_block;
        arena->last = new_block;
        
        arena = new_block;
    }
	
	if(arena->cmt < pos_new)
	{
		u64 cmt_new_aligned, cmt_new_clamped, cmt_new_size;
		
		cmt_new_aligned = alignPow2(pos_new, ARENA_COMMIT_SIZE);
		cmt_new_clamped = clampTop(cmt_new_aligned, arena->res);
		cmt_new_size    = cmt_new_clamped - arena->cmt;
		os_commit((u8*)arena + arena->cmt, cmt_new_size);
		arena->cmt = cmt_new_clamped;
	}
	
	void *memory = 0;
	
	if (arena->cmt >= pos_new)
	{
		memory = (u8*)arena + pos_mem;
		arena->used = pos_new;
	}
	
	return memory;
}

function ArenaTemp arenaTempBegin(Arena *arena)
{
	assert(!arena->chain && "chaining not supported for temp arenas");
    ArenaTemp out = {
		.arena = arena,
		.pos = arena->used,
	};
	return out;
}

// TODO(mizu): Can be very expensive
function void arenaTempEnd(ArenaTemp *temp)
{
	assert(!temp->arena->chain && "chaining not supported for temp arenas");
    //memset((u8*)temp->arena + temp->pos, 0, temp->arena->used - temp->pos);
	
	temp->arena->used = temp->pos;
}

function Arena *arenaAllocSized(u64 cmt, u64 res, b32 chain)
{
	Arena *arena = 0;
	
	u64 page_size = os_getPageSize();
	res = alignPow2(res, page_size);
	cmt = alignPow2(cmt, page_size);
	
	void *memory = os_reserve(res);
	os_commit(memory, cmt);
	
	arena = (Arena*)memory;
	arena->used = ARENA_HEADER_SIZE;
	arena->align = DEFAULT_ALIGN;
	
	arena->cmt = cmt;
	arena->res = res;
	arena->last = arena;
	arena->chain = chain;
    
	return arena;
}

function Arena *arenaAlloc()
{
	return arenaAllocSized(ARENA_COMMIT_SIZE, ARENA_RESERVE_SIZE, 1);
}

function void arenaFree(Arena *arena)
{
	os_free(arena, arena->res);
}