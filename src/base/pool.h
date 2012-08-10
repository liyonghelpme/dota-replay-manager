#ifndef __BASE_POOL__
#define __BASE_POOL__

#include "base/types.h"

class MemoryPool
{
  struct MemoryChunk;
  uint32 itemSize;
  uint32 chunkSize;
  MemoryChunk* chunks;
  MemoryChunk* freeChunks;
public:
  MemoryPool(uint32 itemSize, uint32 poolGrow = 65536);
  ~MemoryPool();

  void* alloc();
  void free(void* ptr);
};

#endif // __BASE_POOL__
