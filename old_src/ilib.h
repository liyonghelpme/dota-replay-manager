#ifndef __I_LIB_H__
#define __I_LIB_H__

#include "image.h"

struct RImageInfo
{
  int width;
  int height;
  CBitmap* bmp;
};

bool loadImages ();
void unloadImages ();
CBitmap* cpyImage (char const* image, BLPImage* blp, RImageInfo* info = NULL);
CBitmap* addImage (char const* image, BLPImage* blp, RImageInfo* info = NULL);

CImageList* getImageList ();

int getImageIndex (char const* image, char const* imgDefault = NULL);
void setImageTip (char const* image, char const* tip);
char const* getImageTip (char const* image);
char const* getImageTip (int image);
CBitmap* getImageBitmap (char const* image, RImageInfo* info = NULL);

struct GLImage
{
  int width;
  int height;
  unsigned char* bits;

  GLImage (char const* image);
  GLImage (int wd, int ht)
  {
    width = wd;
    height = ht;
    bits = NULL;
  }
};

struct ResFile
{
  int size;
  unsigned char* data;

  ResFile (char const* file);
};

void addNewImage (char const* path, bool big);

struct ImgCompressor
{
  int* col[3];
  int* out[3];
  int* cnt;
  ImgCompressor (int wd, int ht);
  ~ImgCompressor ();
};
void sharpenImage (ImgCompressor& cmp, int size, float coeff);

#endif // __I_LIB_H__
