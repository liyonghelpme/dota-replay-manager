#ifndef __GRAPHICS_IMAGELIB_H__
#define __GRAPHICS_IMAGELIB_H__

#include "core/app.h"
#include "base/mpqfile.h"
#include "base/dictionary.h"
#include "graphics/image.h"

class ImageLibrary
{
  HIMAGELIST list;
  struct ImageInfo
  {
    String filename;
    String tooltip;
    Image* image;
    HBITMAP hBitmap;
    int listIndex;
    ImageInfo();
    ~ImageInfo();
    Image* getImage(MPQArchive* mpq);
  };
  Dictionary<ImageInfo> images;
  MPQArchive* mpq;

  void loadImage(String name);
public:
  ImageLibrary(MPQArchive* mpq);
  ~ImageLibrary();

  HIMAGELIST getImageList() const
  {
    return list;
  }

  String getTooltip(String name);
  void setTooltip(String name, String tooltip);
  Image* getImage(String name);
  HBITMAP getBitmap(String name);
  int getListIndex(String name);
};

#endif // __GRAPHICS_IMAGELIB_H__
