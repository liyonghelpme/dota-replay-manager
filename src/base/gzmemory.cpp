#include "gzmemory.h"
#include "zlib/zlib.h"

void gzmemory::reset()
{
  if (count > size)
  {
    if (buf)
      ::free(buf);
    buf = (char*) malloc(count);
    size = count;
  }
  count = 0;
  pos = 0;
}
void* gzmemory::alloc(int block)
{
  count += block;
  if (pos + block > size)
    return malloc(block);
  char* ptr = buf + pos;
  pos += block;
  return ptr;
}
void gzmemory::free(void* ptr)
{
  if (ptr && (ptr < buf || ptr >= buf + size))
    ::free(ptr);
}
void* gzalloc(void* param, unsigned int items, unsigned int size)
{
  if (param == NULL) return malloc(items * size);
  return ((gzmemory*) param)->alloc(items * size);
}
void gzfree(void* param, void* ptr)
{
  if (param) ((gzmemory*) param)->free (ptr);
  else free(ptr);
}

bool gzinflate(char* old, char* buf, int csize, int usize, gzmemory* mem)
{
  if (mem)
    mem->reset();
  z_stream zs;
  memset(&zs, 0, sizeof zs);

  zs.next_out = (unsigned char*) buf;
  zs.avail_out = usize;
  zs.zalloc = gzalloc;
  zs.zfree = gzfree;
  zs.opaque = mem;

  if (inflateInit2(&zs, -15) != Z_OK)
    return false;
  zs.next_in = (Bytef*) old;
  zs.avail_in = csize;
  int err = inflate(&zs, Z_SYNC_FLUSH);
  inflateEnd(&zs);
  return zs.avail_out == 0;
}
bool gzinflate2(char* old, char* buf, int csize, int usize, gzmemory* mem)
{
  if (mem)
    mem->reset();
  z_stream z;
  memset(&z, 0, sizeof z);
  z.next_in = (Bytef*) old;
  z.avail_in = (uInt) csize;
  z.next_out = (Bytef*) buf;
  z.avail_out = (uInt) usize;
  z.zalloc = gzalloc;
  z.zfree = gzfree;
  z.opaque = mem;

  if (inflateInit(&z) != Z_OK)
    return false;
  inflate(&z, Z_FINISH);
  inflateEnd(&z);
  return usize == z.total_out;
}
