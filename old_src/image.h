#ifndef __IMAGE_H__
#define __IMAGE_H__

#include "rmpq.h"

struct BLPImage
{
  int width;
  int height;
  uint32* data;
  BLPImage ()
  {
    data = NULL;
  }
  ~BLPImage ()
  {
    delete[] data;
  }
};

#define IMAGE_UNIT        5
#define IMAGE_BUFF        6
int GetImage (CImageList* list, MPQLOADER mpq, char const* path, int type);
void deleteImages ();
BLPImage* LoadBLP (MPQFILE blp);
BLPImage* LoadTGA (MPQFILE tga);
BLPImage* MPQLoadImage (MPQLOADER mpq, char const* path);
BLPImage* MPQLoadImageA (MPQARCHIVE mpq, char const* path);
void BLPAddIcons (BLPImage* img, MPQFILE desc);
void BLPPrepare (BLPImage* img, uint32 bk, int wd, int ht);

inline int clamp (int x, int a, int b)
{
  if (x < a) x = a;
  if (x > b) x = b;
  return x;
}

#endif
