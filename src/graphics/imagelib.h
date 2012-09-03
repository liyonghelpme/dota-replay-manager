#ifndef __GRAPHICS_IMAGELIB_H__
#define __GRAPHICS_IMAGELIB_H__

#include "core/app.h"
#include "base/mpqfile.h"
#include "base/dictionary.h"
#include "graphics/image.h"

class ImageLibrary
{
  CRITICAL_SECTION lock;
  HIMAGELIST list;
  HDC hImageDC;
  struct ImageInfo
  {
    String filename;
    String tooltip;
    Image* image;
    HBITMAP hBitmap;
    HBITMAP hAlphaBitmap;
    int listIndex;
    ImageInfo();
    ~ImageInfo();
    Image* getImage(MPQArchive* mpq);
  };
  Dictionary<ImageInfo> images;
  Array<ImageInfo*> ilist;
  MPQArchive* mpq;

  void loadImage(String name);
public:
  ImageLibrary(MPQArchive* mpq);
  ~ImageLibrary();

  HIMAGELIST getImageList() const
  {
    return list;
  }

  String getTooltip(int index);
  String getTooltip(char const* name);
  void setTooltip(char const* name, String tooltip);
  Image* getImage(char const* name);
  HBITMAP getBitmap(char const* name);
  int getListIndex(char const* name, char const* def = NULL);

  void drawAlpha(HDC hDC, int index, int x, int y, int width, int height);

  bool hasImage(char const* name)
  {
    return images.has(name);
  }
  void addImage(char const* name, Image* image, bool big = false);
};

#endif // __GRAPHICS_IMAGELIB_H__
