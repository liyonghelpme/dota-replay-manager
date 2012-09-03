#include "rmpq.h"
#include "mpqcompress.h"
#include "zlib/zlib.h"
#include "huffman.h"
#include "pklib.h"
#include "wave.h"
#include <string.h>
#include <malloc.h>

#include "base/gzmemory.h"

#include "mpqsync.h"

extern uint32 mpq_error;
static bool _partial = false;

struct CompressionType
{
  uint8 id;
  uint32 (*func) (uint8* in, uint32 in_size, uint8* out, uint32* out_size, gzmemory* gzmem);
};
static CompressionType comp_table[] = {
  {0x40, wave_compress_mono},
  {0x80, wave_compress_stereo},
  {0x01, huff_compress},
  {0x02, zlib_compress},
  {0x08, pkzip_compress}
//  {0x10, bzip2_compress}
};
static CompressionType decomp_table[] = {
//  {0x10, bzip2_decompress},
  {0x08, pkzip_decompress},
  {0x02, zlib_decompress},
  {0x01, huff_decompress},
  {0x80, wave_decompress_stereo},
  {0x40, wave_decompress_mono}
};

uint32 gzdeflate (uint8* in, uint32 in_size, uint8* out, uint32* out_size)
{
  z_stream z;
  memset (&z, 0, sizeof z);
  z.next_in = in;
  z.avail_in = in_size;
  z.total_in = in_size;
  z.next_out = out;
  z.avail_out = *out_size;
  z.total_out = 0;

  memset (out, 0, *out_size);

  int result = deflateInit (&z, Z_DEFAULT_COMPRESSION);
  if (result == Z_OK)
  {
    result = deflate (&z, Z_FINISH);
    *out_size = z.total_out;
    deflateEnd (&z);
  }
  return (result == Z_STREAM_END ? 0 : -1);
}
uint32 zlib_compress (uint8* in, uint32 in_size, uint8* out, uint32* out_size, gzmemory* gzmem)
{
  z_stream z;
  memset (&z, 0, sizeof z);
  z.next_in = in;
  z.avail_in = in_size;
  z.total_in = in_size;
  z.next_out = out;
  z.avail_out = *out_size;
  z.total_out = 0;
  z.zalloc = gzalloc;
  z.zfree = gzfree;
  z.opaque = gzmem;

  memset (out, 0, *out_size);

  int result = deflateInit (&z, Z_DEFAULT_COMPRESSION);
  if (result == Z_OK)
  {
    result = deflate (&z, Z_FINISH);
    *out_size = z.total_out;
    deflateEnd (&z);
  }
  return mpq_error = (result == Z_STREAM_END ? MPQ_OK : MPQ_ERROR_COMPRESS);
}

uint32 gzinflate (uint8* in, uint32 in_size, uint8* out, uint32* out_size)
{
  z_stream z;
  memset (&z, 0, sizeof z);
  z.next_in = in;
  z.avail_in = in_size;
  z.total_in = in_size;
  z.next_out = out;
  z.avail_out = *out_size;
  z.total_out = 0;

  memset (out, 0, *out_size);

  int result = inflateInit (&z);
  if (result == Z_OK)
  {
    result = inflate (&z, _partial ? Z_SYNC_FLUSH : Z_FINISH);
    *out_size = z.total_out;
    inflateEnd (&z);
  }
  return (z.avail_out == 0 ? 0 : -1);
}
uint32 zlib_decompress (uint8* in, uint32 in_size, uint8* out, uint32* out_size, gzmemory* gzmem)
{
  z_stream z;
  memset (&z, 0, sizeof z);
  z.next_in = in;
  z.avail_in = in_size;
  z.total_in = in_size;
  z.next_out = out;
  z.avail_out = *out_size;
  z.total_out = 0;
  z.zalloc = gzalloc;
  z.zfree = gzfree;
  z.opaque = gzmem;

  memset (out, 0, *out_size);

  int result = inflateInit (&z);
  if (result == Z_OK)
  {
    result = inflate (&z, _partial ? Z_SYNC_FLUSH : Z_FINISH);
    *out_size = z.total_out;
    inflateEnd (&z);
  }
  return mpq_error = (z.avail_out == 0 ? MPQ_OK : MPQ_ERROR_DECOMPRESS);
}

struct BUFFERINFO
{
  void* out;
  uint32 outPos;
  uint32 outLen;
  void* in;
  uint32 inPos;
  uint32 inLen;
};

unsigned int FillInput (char* buffer, unsigned int* size, void* param)
{
  BUFFERINFO* bi = (BUFFERINFO*) param;
  uint32 bufSize = *size;
  if (bufSize >= bi->inLen - bi->inPos)
    bufSize = bi->inLen - bi->inPos;
  memcpy (buffer, (char*) bi->in + bi->inPos, bufSize);
  bi->inPos += bufSize;
  return bufSize;
}
void FillOutput (char* buffer, unsigned int* size, void* param)
{
  BUFFERINFO* bi = (BUFFERINFO*) param;
  uint32 bufSize = *size;
  if (bufSize >= bi->outLen - bi->outPos)
    bufSize = bi->outLen - bi->outPos;
  memcpy ((char*) bi->out + bi->outPos, buffer, bufSize);
  bi->outPos += bufSize;
}

uint32 pkzip_compress (uint8* in, uint32 in_size, uint8* out, uint32* out_size, gzmemory* gzmem)
{
  void* buf = gzmem->alloc (CMP_BUFFER_SIZE);

  BUFFERINFO bi;
  bi.in = in;
  bi.inPos = 0;
  bi.inLen = in_size;
  bi.out = out;
  bi.outPos = 0;
  bi.outLen = *out_size;

  unsigned int compType = CMP_BINARY;
  unsigned int dictSize;
  if (in_size >= 0xC00)
    dictSize = 0x1000;
  else if (in_size < 0x600)
    dictSize = 0x400;
  else
    dictSize = 0x800;

  implode (FillInput, FillOutput, (char*) buf, &bi, &compType, &dictSize);
  *out_size = bi.outPos;
  gzmem->free (buf);
  return mpq_error = MPQ_OK;
}

uint32 pkzip_decompress (uint8* in, uint32 in_size, uint8* out, uint32* out_size, gzmemory* gzmem)
{
  void* buf = gzmem->alloc (CMP_BUFFER_SIZE);

  BUFFERINFO bi;
  bi.in = in;
  bi.inPos = 0;
  bi.inLen = in_size;
  bi.out = out;
  bi.outPos = 0;
  bi.outLen = *out_size;

  explode (FillInput, FillOutput, (char*) buf, &bi);
  *out_size = bi.outPos;
  gzmem->free (buf);
  return mpq_error = MPQ_OK;
}

uint32 wave_compress_mono (uint8* in, uint32 in_size, uint8* out, uint32* out_size, gzmemory* gzmem)
{
  *out_size = CompressWave (out, *out_size, (short*) in, in_size, 1, 5);
  return mpq_error = MPQ_OK;
}

uint32 wave_decompress_mono (uint8* in, uint32 in_size, uint8* out, uint32* out_size, gzmemory* gzmem)
{
  *out_size = DecompressWave (out, *out_size, in, in_size, 1);
  return mpq_error = MPQ_OK;
}

uint32 wave_compress_stereo (uint8* in, uint32 in_size, uint8* out, uint32* out_size, gzmemory* gzmem)
{
  *out_size = CompressWave (out, *out_size, (short*) in, in_size, 2, 5);
  return mpq_error = MPQ_OK;
}

uint32 wave_decompress_stereo (uint8* in, uint32 in_size, uint8* out, uint32* out_size, gzmemory* gzmem)
{
  *out_size = DecompressWave (out, *out_size, in, in_size, 2);
  return mpq_error = MPQ_OK;
}

uint32 huff_compress (uint8* in, uint32 in_size, uint8* out, uint32* out_size, gzmemory* gzmem)
{
  THuffmannTree* ht = (THuffmannTree*) gzmem->alloc (sizeof(THuffmannTree));

  TOutputStream os;
  os.pbOutBuffer = (uint8*) out;
  os.dwOutSize = *out_size;
  os.pbOutPos = (uint8*) out;
  os.dwBitBuff = 0;
  os.nBits = 0;

  for (int i = 0; i < 0x203; i++)
    ht->items0008[i].ClearItemLinks();
  ht->pItem3050 = NULL;
  ht->pItem3054 = PTR_PTR (&ht->pItem3054);
  ht->pItem3058 = PTR_NOT (&ht->pItem3054);
  ht->pItem305C = NULL;
  ht->pFirst = PTR_PTR (&ht->pFirst);
  ht->pLast = PTR_NOT (&ht->pFirst);
  ht->offs0004 = 1;
  ht->nItems = 0;

  *out_size = ht->DoCompression (&os, (unsigned char*) in, in_size, 0);
  gzmem->free (ht);

  return mpq_error = MPQ_OK;
}

uint32 huff_decompress (uint8* in, uint32 in_size, uint8* out, uint32* out_size, gzmemory* gzmem)
{
  THuffmannTree* ht = (THuffmannTree*) gzmem->alloc (sizeof(THuffmannTree));

  TInputStream is;
  is.pbInBuffer = (uint8*) in;
  is.dwBitBuff = * (uint32*) in;
  is.pbInBuffer += sizeof (uint32);
  is.nBits = 32;

  for (int i = 0; i < 0x203; i++)
    ht->items0008[i].ClearItemLinks();
  ht->pItem3050 = NULL;
  ht->pItem3054 = PTR_PTR (&ht->pItem3054);
  ht->pItem3058 = PTR_NOT (&ht->pItem3054);
  ht->pItem305C = NULL;
  ht->pFirst = PTR_PTR (&ht->pFirst);
  ht->pLast = PTR_NOT (&ht->pFirst);
  ht->offs0004 = 1;
  ht->nItems = 0;

  for (int i = 0; i < 128; i++)
    ht->qd3474[i].offs00 = 0;

  *out_size = ht->DoDecompression ((uint8*) out, *out_size, &is);
  return mpq_error = MPQ_OK;
}

uint32 mpq_compress (uint8* in, uint32 in_size, uint8* out, uint32* out_size, uint8* temp, uint32 methods)
{
  gzmemory gzmem;
  gzmem.reset();
  if (in_size <= 32)
  {
    if (*out_size < in_size)
      return mpq_error = MPQ_ERROR_COMPRESS;
    memcpy (out, in, in_size);
    *out_size = in_size;
    return mpq_error = MPQ_OK;
  }
  uint8 method = 0;
  uint32 cur_size = in_size;
  uint8* cur_in = in;
  uint8* cur_out = temp;
  int numMethods = sizeof comp_table / sizeof comp_table[0];
  for (int i = 0; i < numMethods; i++)
  {
    if (methods & comp_table[i].id)
    {
      uint32 size = *out_size - 1;
      uint32 res = comp_table[i].func (cur_in, cur_size, cur_out, &size, &gzmem);
      if (res == MPQ_OK && size < cur_size)
      {
        cur_size = size;
        cur_in = cur_out;
        if (cur_in == temp)
          cur_out = out + 1;
        else
          cur_out = temp;
        method |= comp_table[i].id;
      }
    }
  }
  if (*out_size - 1 <= cur_size)
  {
    if (*out_size == in_size)
    {
      memcpy (out, in, in_size);
      return mpq_error = MPQ_OK;
    }
    else
      return mpq_error = MPQ_ERROR_COMPRESS;
  }
  if (cur_in == temp)
    memcpy (out + 1, temp, cur_size);
  out[0] = method;
  *out_size = cur_size + 1;
  return mpq_error = MPQ_OK;
}

uint32 mpq_decompress (uint8* in, uint32 in_size, uint8* out, uint32* out_size)
{
  gzmemory gzmem;
  gzmem.reset();
  if (in_size == *out_size)
  {
    if (in != out)
      memcpy (out, in, in_size);
    return MPQ_OK;
  }
  int numMethods = sizeof decomp_table / sizeof decomp_table[0];
  uint8 method = *in++;
  uint8 remain = method;
  in_size--;
  int count = 0;
  for (int i = 0; i < numMethods; i++)
  {
    if (method & decomp_table[i].id)
    {
      count++;
      remain &= ~decomp_table[i].id;
    }
  }
  if (remain)
    return MPQ_ERROR_DECOMPRESS;
  if (count == 0)
  {
    if (in_size > *out_size)
      return MPQ_ERROR_DECOMPRESS;
    if (in != out)
      memcpy (out, in, in_size);
    *out_size = in_size;
    return MPQ_OK;
  }
  uint8* temp = NULL;
  if (count > 1)
    temp = (uint8*) gzmem.alloc (*out_size);
  for (int i = 0; i < numMethods; i++)
  {
    if (method & decomp_table[i].id)
    {
      uint8* work = (count-- & 1) ? out : temp;
      uint32 work_size = *out_size;
      uint32 res;
      if (res = decomp_table[i].func (in, in_size, work, &work_size, &gzmem))
      {
        gzmem.free (temp);
        return res;
      }
      in = work;
      in_size = work_size;
    }
  }
  if (in != out)
    memcpy (out, in, in_size);
  *out_size = in_size;
  gzmem.free (temp);
  return MPQ_OK;
}
uint32 mpq_part_decompress (uint8* in, uint32 in_size, uint8* out, uint32* out_size)
{
  gzmemory gzmem;
  gzmem.reset();
  int numMethods = sizeof decomp_table / sizeof decomp_table[0];
  uint8 method = *in++;
  uint8 remain = method;
  in_size--;
  int count = 0;
  for (int i = 0; i < numMethods; i++)
  {
    if (method & decomp_table[i].id)
    {
      count++;
      remain &= ~decomp_table[i].id;
    }
  }
  if (remain)
    return MPQ_ERROR_DECOMPRESS;
  if (count == 0)
  {
    if (in_size < *out_size)
      *out_size = in_size;
    if (in != out)
      memcpy (out, in, *out_size);
    return MPQ_OK;
  }
  uint8* temp = NULL;
  if (count > 1)
    temp = (uint8*) gzmem.alloc (*out_size);
  _partial = true;
  for (int i = 0; i < numMethods; i++)
  {
    if (method & decomp_table[i].id)
    {
      uint8* work = (count-- & 1) ? out : temp;
      uint32 work_size = *out_size;
      uint32 res;
      if (res = decomp_table[i].func (in, in_size, work, &work_size, &gzmem))
      {
        _partial = false;
        gzmem.free (temp);
        return res;
      }
      in = work;
      in_size = work_size;
    }
  }
  _partial = false;
  if (in != out)
    memcpy (out, in, in_size);
  *out_size = in_size;
  gzmem.free (temp);
  return MPQ_OK;
}

void MPQC_INIT ()
{
}
void MPQC_CLEANUP ()
{
}
