#include "pool.h"
#include <stdlib.h>

struct MemoryPool::MemoryChunk
{
  MemoryChunk* next;
  MemoryChunk* nextFree;

  uint8* begin;
  uint8* end;
  uint8* firstFree;
  uint8* firstUnused;
  uint32 itemSize;

  MemoryChunk(uint32 size, uint32 count);
  ~MemoryChunk();

  bool hasSpace()
  {
    return firstFree || firstUnused < end;
  }
  bool contains(uint8* ptr)
  {
    return ptr >= begin && ptr < end;
  }
  uint8* alloc();
  void free(uint8* ptr);
};

MemoryPool::MemoryChunk::MemoryChunk(uint32 size, uint32 count)
{
  next = NULL;
  nextFree = NULL;

  itemSize = size;

  begin = new uint8[size * count];
  end = begin + size * count;
  firstFree = NULL;
  firstUnused = begin;
}
MemoryPool::MemoryChunk::~MemoryChunk()
{
  delete[] begin;
}

uint8* MemoryPool::MemoryChunk::alloc()
{
  if (firstFree)
  {
    uint8* result = firstFree;
    firstFree = *(uint8**) firstFree;
    return result;
  }
  else if (firstUnused < end)
  {
    uint8* result = firstUnused;
    firstUnused += itemSize;
    return result;
  }
  return NULL;
}
void MemoryPool::MemoryChunk::free(uint8* ptr)
{
  *(uint8**) ptr = firstFree;
  firstFree = ptr;
}

MemoryPool::MemoryPool(uint32 itemSize, uint32 poolGrow)
{
  this->itemSize = itemSize;
  chunkSize = poolGrow / itemSize;
  chunks = NULL;
  freeChunks = NULL;
}
MemoryPool::~MemoryPool()
{
  while (chunks)
  {
    MemoryChunk* next = chunks->next;
    delete chunks;
    chunks = next;
  }
}

void* MemoryPool::alloc()
{
  if (freeChunks == NULL)
  {
    freeChunks = new MemoryChunk(itemSize, chunkSize);
    freeChunks->next = chunks;
    chunks = freeChunks;
  }
  void* ptr = freeChunks->alloc();
  if (!freeChunks->hasSpace())
    freeChunks = freeChunks->nextFree;
  return ptr;
}
void MemoryPool::free(void* ptr)
{
  for (MemoryChunk* chunk = chunks; chunk; chunk = chunk->next)
  {
    if (chunk->contains((uint8*) ptr))
    {
      if (!chunk->hasSpace())
      {
        chunk->nextFree = freeChunks;
        freeChunks = chunk;
      }
      chunk->free((uint8*) ptr);
      return;
    }
  }
}
