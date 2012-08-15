#ifndef __BASE_POOL__
#define __BASE_POOL__

#include "base/types.h"

class FixedMemoryPool
{
  struct MemoryChunk;
  uint32 itemSize;
  uint32 chunkSize;
  MemoryChunk* chunks;
  MemoryChunk* freeChunks;
public:
  FixedMemoryPool(uint32 itemSize, uint32 poolGrow = 65536);
  ~FixedMemoryPool();

  void* alloc();
  void free(void* ptr);
  void clear();
};

class MemoryPool
{
  struct MemoryChunk;
  uint32 chunkSize;
  MemoryChunk* chunks;
public:
  MemoryPool(uint32 chunk = 65536);
  ~MemoryPool();

  void* alloc(uint32 size);
  void clear();
};

#endif // __BASE_POOL__
