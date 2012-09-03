#include <stdlib.h>

#include "base/file.h"
#include "graphics/image.h"

namespace _blp2
{

struct BLP2Header
{
  uint32 id;
  uint32 type;
  uint8 encoding;
  uint8 alphaDepth;
  uint8 alphaEncoding;
  uint8 hasMips;
  uint32 width;
  uint32 height;
  uint32 offsets[16];
  uint32 lengths[16];
  uint32 pal[256];
};

bool load_raw(uint8* src, uint32 length, uint32* data, BLP2Header const& hdr)
{
  uint32 alphaBits = 0;
  uint32 dim = hdr.width * hdr.height;
  if (hdr.alphaDepth == 1)
    alphaBits = (dim + 7) / 8;
  else if (hdr.alphaDepth == 8)
    alphaBits = dim;
  if (length != dim + alphaBits)
    return false;
  uint32* cur = data;
  for (uint32 i = 0; i < dim; i++)
    *cur++ = hdr.pal[*src++] & 0x00FFFFFF;
  if (hdr.alphaDepth == 0)
  {
    for (uint32 i = 0; i < dim; i++)
      *data++ |= 0xFF000000;
  }
  else if (hdr.alphaDepth == 1)
  {
    for (uint32 i = 0; i < dim; i += 8)
    {
      uint8 alpha = *src++; 
      for (uint32 b = 0; b < 8 && i + b < dim; b++)
      {
        if (alpha & (1 << b))
          *data |= 0xFF000000;
        else
          *data = 0;
        data++;
      }
    }
  }
  else if (hdr.alphaDepth == 8)
  {
    for (uint32 i = 0; i < dim; i++)
      *data = Image::clr_noflip(*data, *src++);
  }
  return true;
}

bool load_dxt1(uint8* src, uint32 length, uint32* data, BLP2Header const& hdr)
{
  if (hdr.width < 4 || hdr.height < 4)
    return false;
  if (length != hdr.width * hdr.height / 2)
    return false;
  for (uint32 y = 0; y < hdr.height; y += 4)
  {
    for (uint32 x = 0; x < hdr.width; x += 4)
    {
      uint16 p = *(uint16*) src;
      uint16 q = *(uint16*) (src + 2);
      src += 4;
      uint32 pred = ((p >> 11) & 0x1F) * 255 / 31;
      uint32 pgreen = ((p >> 5) & 0x3F) * 255 / 63;
      uint32 pblue = (p & 0x1F) * 255 / 31;
      uint32 qred = ((q >> 11) & 0x1F) * 255 / 31;
      uint32 qgreen = ((q >> 5) & 0x3F) * 255 / 63;
      uint32 qblue = (q & 0x1F) * 255 / 31;
      uint32 c[4] = {Image::clr(pred, pgreen, pblue),
                     Image::clr(qred, qgreen, qblue)};
      if (p > q)
      {
        c[2] = Image::clr((pred * 2 + qred) / 3,
          (pgreen * 2 + qgreen) / 3, (pblue * 2 + qblue) / 3);
        c[3] = Image::clr((pred + qred * 2) / 3,
          (pgreen + qgreen * 2) / 3, (pblue + qblue * 2) / 3);
      }
      else
      {
        c[2] = Image::clr((pred + qred) / 2,
          (pgreen + qgreen) / 2, (pblue + qblue) / 2);
        c[3] = 0;
      }
      uint32 lookup = *(uint32*) src;
      src += 4;
      for (uint32 cy = y; cy < y + 4; cy++)
      {
        uint32* dst = data + cy * hdr.width + x;
        for (uint32 cx = 0; cx < 4; cx++)
        {
          uint32 i = lookup & 3;
          lookup >>= 2;
          *dst++ = c[i];
        }
      }
    }
  }
  return true;
}

bool load_dxt3(uint8* src, uint32 length, uint32* data, BLP2Header const& hdr)
{
  if (hdr.width < 4 || hdr.height < 4)
    return false;
  if (length != hdr.width * hdr.height)
    return false;
  for (uint32 y = 0; y < hdr.height; y += 4)
  {
    for (uint32 x = 0; x < hdr.width; x += 4)
    {
      uint64 alpha = *(uint64*) src;
      src += 8;
      uint16 p = *(uint16*) src;
      uint16 q = *(uint16*) (src + 2);
      src += 4;
      uint32 pred = ((p >> 11) & 0x1F) * 255 / 31;
      uint32 pgreen = ((p >> 5) & 0x3F) * 255 / 63;
      uint32 pblue = (p & 0x1F) * 255 / 31;
      uint32 qred = ((q >> 11) & 0x1F) * 255 / 31;
      uint32 qgreen = ((q >> 5) & 0x3F) * 255 / 63;
      uint32 qblue = (q & 0x1F) * 255 / 31;
      uint32 c[4] = {Image::clr(pred, pgreen, pblue),
                     Image::clr(qred, qgreen, qblue),
                     Image::clr((pred * 2 + qred) / 3,
                       (pgreen * 2 + qgreen) / 3, (pblue * 2 + qblue) / 3),
                     Image::clr((pred + qred * 2) / 3,
                       (pgreen + qgreen * 2) / 3, (pblue + qblue * 2) / 3)};
      uint32 lookup = *(uint32*) src;
      src += 4;
      for (uint32 cy = y; cy < y + 4; cy++)
      {
        uint32* dst = data + cy * hdr.width + x;
        for (uint32 cx = 0; cx < 4; cx++)
        {
          uint32 i = lookup & 3;
          *dst++ = Image::clr_noflip(c[i], uint32(alpha & 0x0F) * 255 / 15);
          lookup >>= 2;
          alpha >>= 4;
        }
      }
    }
  }
  return true;
}

bool load_dxt5(uint8* src, uint32 length, uint32* data, BLP2Header const& hdr)
{
  if (hdr.width < 4 || hdr.height < 4)
    return false;
  if (length != hdr.width * hdr.height)
    return false;
  for (uint32 y = 0; y < hdr.height; y += 4)
  {
    for (uint32 x = 0; x < hdr.width; x += 4)
    {
      uint32 a0 = *src++;
      uint32 a1 = *src++;
      uint64 alpha = (uint64(*(uint16*) src)) |
                     (uint64(*(uint32*) (src + 2)) << 16);
      src += 6;
      uint16 p = *(uint16*) src;
      uint16 q = *(uint16*) (src + 2);
      src += 4;
      uint32 pred = ((p >> 11) & 0x1F) * 255 / 31;
      uint32 pgreen = ((p >> 5) & 0x3F) * 255 / 63;
      uint32 pblue = (p & 0x1F) * 255 / 31;
      uint32 qred = ((q >> 11) & 0x1F) * 255 / 31;
      uint32 qgreen = ((q >> 5) & 0x3F) * 255 / 63;
      uint32 qblue = (q & 0x1F) * 255 / 31;
      uint32 c[4] = {Image::clr(pred, pgreen, pblue),
                     Image::clr(qred, qgreen, qblue),
                     Image::clr((pred * 2 + qred) / 3,
                       (pgreen * 2 + qgreen) / 3, (pblue * 2 + qblue) / 3),
                     Image::clr((pred + qred * 2) / 3,
                       (pgreen + qgreen * 2) / 3, (pblue + qblue * 2) / 3)};
      uint32 a[8] = {a0, a1};
      if (a0 > a1)
      {
        a[2] = (a0 * 6 + a1) / 7;
        a[3] = (a0 * 5 + a1 * 2) / 7;
        a[4] = (a0 * 4 + a1 * 3) / 7;
        a[5] = (a0 * 3 + a1 * 4) / 7;
        a[6] = (a0 * 2 + a1 * 5) / 7;
        a[7] = (a0 + a1 * 6) / 7;
      }
      else
      {
        a[2] = (a0 * 4 + a1) / 5;
        a[3] = (a0 * 3 + a1 * 2) / 5;
        a[4] = (a0 * 2 + a1 * 3) / 5;
        a[5] = (a0 + a1 * 4) / 5;
        a[6] = 0;
        a[7] = 255;
      }
      uint32 lookup = *(uint32*) src;
      src += 4;
      for (uint32 cy = y; cy < y + 4; cy++)
      {
        uint32* dst = data + cy * hdr.width + x;
        for (uint32 cx = 0; cx < 4; cx++)
        {
          uint32 i = lookup & 3;
          *dst++ = Image::clr_noflip(c[i], a[alpha & 7]);
          lookup >>= 2;
          alpha >>= 3;
        }
      }
    }
  }
  return true;
}

}

using namespace _blp2;

bool Image::loadBLP2(File* f)
{
  BLP2Header hdr;
  f->seek(0, SEEK_SET);
  if (f->read(&hdr, sizeof hdr) != sizeof hdr)
    return false;

  if (hdr.id != '2PLB')
    return false;
  if (hdr.type != 1)
    return false; // do not support JPEG compression
  if (hdr.encoding != 1 && hdr.encoding != 2)
    return false;
  if (hdr.alphaDepth != 0 && hdr.alphaDepth != 1 && hdr.alphaDepth != 8)
    return false;
  if (hdr.alphaEncoding != 0 && hdr.alphaEncoding != 1 &&
    hdr.alphaEncoding != 7 && hdr.alphaEncoding != 8)
    return false;
  if ((hdr.width & (hdr.width - 1)) != 0)
    return false;
  if ((hdr.height & (hdr.height - 1)) != 0)
    return false;

  uint8* src = new uint8[hdr.lengths[0]];
  f->seek(hdr.offsets[0], SEEK_SET);
  if (f->read(src, hdr.lengths[0]) != hdr.lengths[0])
    return false;

  _width = hdr.width;
  _height = hdr.height;
  _bits = new uint32[_width * _height];

  bool result = false;
  if (hdr.encoding == 1)
    result = load_raw(src, hdr.lengths[0], _bits, hdr);
  else if (hdr.alphaDepth <= 1)
    result = load_dxt1(src, hdr.lengths[0], _bits, hdr);
  else if (hdr.alphaEncoding != 7)
    result = load_dxt3(src, hdr.lengths[0], _bits, hdr);
  else
    result = load_dxt5(src, hdr.lengths[0], _bits, hdr);

  if (!result)
  {
    delete[] _bits;
    _bits = NULL;
  }
  delete[] src;

  return result;
}
