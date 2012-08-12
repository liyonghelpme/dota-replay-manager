#include "stdafx.h"
#include "image.h"

#pragma pack (push, 1)
struct TGAHeader
{
  uint8 idLength;
  uint8 colormapType;
  uint8 imageType;
  uint16 colormapIndex;
  uint16 colormapLength;
  uint8 colormapEntrySize;
  uint16 xOrigin;
  uint16 yOrigin;
  uint16 width;
  uint16 height;
  uint8 pixelSize;
  uint8 imageDesc;
};
#pragma pack (pop)

bool LoadTGA8 (MPQFILE tga, BLPImage* img, uint32* pal, TGAHeader* hdr)
{
  for (int y = 0; y < img->height; y++)
  {
    uint32* out;
    if (hdr->imageDesc & 0x20)
      out = img->data + y * img->width;
    else
      out = img->data + (img->height - y - 1) * img->height;
    for (int x = 0; x < img->width; x++)
    {
      int c = MPQFileGetc (tga);
      if (MPQError ()) return false;
      if (pal)
        *out++ = pal[c];
      else
        *out++ = RGB (c, c, c) | 0xFF000000;
    }
  }
  return true;
}
bool LoadTGA24 (MPQFILE tga, BLPImage* img, TGAHeader* hdr)
{
  for (int y = 0; y < img->height; y++)
  {
    uint32* out;
    if (hdr->imageDesc & 0x20)
      out = img->data + y * img->width;
    else
      out = img->data + (img->height - y - 1) * img->height;
    for (int x = 0; x < img->width; x++)
    {
      int r = MPQFileGetc (tga);
      int g = MPQFileGetc (tga);
      int b = MPQFileGetc (tga);
      if (MPQError ()) return false;
      *out++ = RGB (r, g, b) | 0xFF000000;
    }
  }
  return true;
}
bool LoadTGA32 (MPQFILE tga, BLPImage* img, TGAHeader* hdr)
{
  for (int y = 0; y < img->height; y++)
  {
    uint32* out;
    if (hdr->imageDesc & 0x20)
      out = img->data + y * img->width;
    else
      out = img->data + (img->height - y - 1) * img->height;
    MPQFileRead (tga, img->width * sizeof (uint32), out);
    if (MPQError ()) return false;
  }
  return true;
}
bool LoadRLE8 (MPQFILE tga, BLPImage* img, uint32* pal, TGAHeader* hdr)
{
  for (int y = 0; y < img->height; y++)
  {
    uint32* out;
    if (hdr->imageDesc & 0x20)
      out = img->data + y * img->width;
    else
      out = img->data + (img->height - y - 1) * img->height;
    for (int x = 0; x < img->width;)
    {
      int pkHdr = MPQFileGetc (tga);
      int pkSize = (pkHdr & 0x7F) + 1;
      if (MPQError ()) return false;
      uint32 color;
      for (int i = 0; i < pkSize; i++)
      {
        if (i == 0 || (pkHdr & 0x80) == 0)
        {
          int val = MPQFileGetc (tga);
          if (MPQError ()) return false;
          if (pal)
            color = pal[val];
          else
            color = RGB (val, val, val) | 0xFF000000;
        }
        *out++ = color;
        if (++x >= img->width)
        {
          x = 0;
          if (++y >= img->height)
            return true;
          if (hdr->imageDesc & 0x20)
            out = img->data + y * img->width;
          else
            out = img->data + (img->height - y - 1) * img->height;
        }
      }
    }
  }
  return true;
}
bool LoadRLE24 (MPQFILE tga, BLPImage* img, TGAHeader* hdr)
{
  for (int y = 0; y < img->height; y++)
  {
    uint32* out;
    if (hdr->imageDesc & 0x20)
      out = img->data + y * img->width;
    else
      out = img->data + (img->height - y - 1) * img->height;
    for (int x = 0; x < img->width;)
    {
      int pkHdr = MPQFileGetc (tga);
      int pkSize = (pkHdr & 0x7F) + 1;
      if (MPQError ()) return false;
      uint32 color;
      for (int i = 0; i < pkSize; i++)
      {
        if (i == 0 || (pkHdr & 0x80) == 0)
        {
          int r = MPQFileGetc (tga);
          int g = MPQFileGetc (tga);
          int b = MPQFileGetc (tga);
          if (MPQError ()) return false;
          color = RGB (r, g, b) | 0xFF000000;
        }
        *out++ = color;
        if (++x >= img->width)
        {
          x = 0;
          if (++y >= img->height)
            return true;
          if (hdr->imageDesc & 0x20)
            out = img->data + y * img->width;
          else
            out = img->data + (img->height - y - 1) * img->height;
        }
      }
    }
  }
  return true;
}
bool LoadRLE32 (MPQFILE tga, BLPImage* img, TGAHeader* hdr)
{
  for (int y = 0; y < img->height; y++)
  {
    uint32* out;
    if (hdr->imageDesc & 0x20)
      out = img->data + y * img->width;
    else
      out = img->data + (img->height - y - 1) * img->height;
    for (int x = 0; x < img->width;)
    {
      int pkHdr = MPQFileGetc (tga);
      int pkSize = (pkHdr & 0x7F) + 1;
      if (MPQError ()) return false;
      uint32 color;
      for (int i = 0; i < pkSize; i++)
      {
        if (i == 0 || (pkHdr & 0x80) == 0)
        {
          MPQFileRead (tga, sizeof color, &color);
          if (MPQError ()) return false;
        }
        *out++ = color;
        if (++x >= img->width)
        {
          x = 0;
          if (++y >= img->height)
            return true;
          if (hdr->imageDesc & 0x20)
            out = img->data + y * img->width;
          else
            out = img->data + (img->height - y - 1) * img->height;
        }
      }
    }
  }
  return true;
}

BLPImage* LoadTGA (MPQFILE tga)
{
  TGAHeader hdr;
  MPQFileRead (tga, sizeof hdr, &hdr);
  if (MPQError ()) return NULL;
  if (hdr.imageType != 0 && hdr.imageType != 1 && hdr.imageType != 2 &&
      hdr.imageType != 3 && hdr.imageType != 9 && hdr.imageType != 10 &&
      hdr.imageType != 11)
    return NULL;
  if (hdr.pixelSize != 32 && hdr.pixelSize != 24 && hdr.pixelSize != 16 &&
      hdr.pixelSize != 15 && hdr.pixelSize != 8)
    return NULL;
  int width = hdr.width;
  int height = hdr.height;
  int offs = (sizeof hdr) + hdr.idLength;
  MPQFileSeek (tga, offs, MPQSEEK_SET);
  BLPImage* img = new BLPImage;
  img->width = width;
  img->height = height;
  img->data = new uint32[width * height];
  uint32* pal = NULL;
  if (hdr.imageType == 1 || hdr.imageType == 9)
  {
    pal = new uint32[hdr.colormapLength];
    if (hdr.colormapEntrySize == 15 || hdr.colormapEntrySize == 16)
    {
      for (int i = 0; i < hdr.colormapLength; i++)
      {
        int a = MPQFileGetc (tga);
        int b = MPQFileGetc (tga);
        pal[i] = RGB (((b & 0x7C) >> 2) << 16,
                      (((b & 0x03) << 3) | ((a & 0xE0) >> 5)) << 8,
                      a & 0x1F) | 0xFF000000;
      }
    }
    else if (hdr.colormapEntrySize == 24)
    {
      for (int i = 0; i < hdr.colormapLength; i++)
      {
        int r = MPQFileGetc (tga);
        int g = MPQFileGetc (tga);
        int b = MPQFileGetc (tga);
        pal[i] = RGB (r, g, b) | 0xFF000000;
      }
    }
    else if (hdr.colormapEntrySize == 32)
      MPQFileRead (tga, hdr.colormapLength * sizeof (uint32), pal);
  }
  if (MPQError ())
  {
    delete img;
    delete[] pal;
    return NULL;
  }
  bool err;
  if (hdr.imageType >= 1 && hdr.imageType <= 3)
  {
    if (hdr.pixelSize == 8)
      err = LoadTGA8 (tga, img, pal, &hdr);
    else if (hdr.pixelSize == 24)
      err = LoadTGA24 (tga, img, &hdr);
    else
      err = LoadTGA32 (tga, img, &hdr);
  }
  else
  {
    if (hdr.pixelSize == 8)
      err = LoadRLE8 (tga, img, pal, &hdr);
    else if (hdr.pixelSize == 24)
      err = LoadRLE24 (tga, img, &hdr);
    else
      err = LoadRLE32 (tga, img, &hdr);
  }
  delete[] pal;
  if (!err)
  {
    delete img;
    return NULL;
  }
  return img;
}
