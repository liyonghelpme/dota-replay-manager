#ifndef __BASE_GZMEMORY__
#define __BASE_GZMEMORY__

#include <stdlib.h>
#include <memory.h>

struct gzmemory
{
  int count;
  int pos;
  char* buf;
  int size;
  void reset();
  void* alloc(int block);
  void free(void* ptr);
  gzmemory()
  {
    buf = NULL;
    size = 0;
    count = 65536;
  }
  ~gzmemory()
  {
    if (buf)
      ::free(buf);
  }
};

void* gzalloc(void* param, unsigned int items, unsigned int size);
void gzfree(void* param, void* ptr);

bool gzinflate(char* old, char* buf, int csize, int usize, gzmemory* mem);
bool gzinflate2(char* old, char* buf, int csize, int usize, gzmemory* mem = NULL);

#endif // __BASE_GZMEMORY__
