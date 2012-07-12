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

  String getTooltip(char const* name);
  void setTooltip(char const* name, String tooltip);
  Image* getImage(char const* name);
  HBITMAP getBitmap(char const* name);
  int getListIndex(char const* name, char const* def = NULL);

  void addImage(char const* name, Image* image, bool big = false);
};

#endif // __GRAPHICS_IMAGELIB_H__
