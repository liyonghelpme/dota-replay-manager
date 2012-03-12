#include "imagelib.h"

ImageLibrary::ImageInfo::ImageInfo()
{
  image = NULL;
  hBitmap = NULL;
  listIndex = -1;
}
ImageLibrary::ImageInfo::~ImageInfo()
{
  delete image;
  DeleteObject(hBitmap);
}
Image* ImageLibrary::ImageInfo::getImage(MPQArchive* mpq)
{
  if (!image)
  {
    MPQFile* file = mpq->openFile(filename, MPQFILE_READ);
    if (file)
    {
      image = new Image(file);
      if (image->bits() == NULL)
      {
        delete image;
        image = NULL;
      }
      delete file;
    }
  }
  return image;
}

ImageLibrary::ImageLibrary(MPQArchive* _mpq)
  : images(mapAlNumNoCase)
{
  mpq = _mpq;
  list = ImageList_Create(16, 16, ILC_COLOR24, 16, 16);
  
  for (uint32 i = 0; i < mpq->getHashSize(); i++)
  {
    char const* name = mpq->getFileName(i);
    if (name && !strnicmp(name, "images\\", 7))
      loadImage(name);
  }
}
ImageLibrary::~ImageLibrary()
{
  ImageList_Destroy(list);
}

void ImageLibrary::loadImage(String name)
{
  ImageInfo& info = images.create(String::getFileTitle(name));
  info.filename = name;
}

String ImageLibrary::getTooltip(char const* name)
{
  return images.get(name).tooltip;
}
void ImageLibrary::setTooltip(char const* name, String tooltip)
{
  images.get(name).tooltip = tooltip;
}
Image* ImageLibrary::getImage(char const* name)
{
  ImageInfo& info = images.get(name);
  return info.getImage(mpq);
}
HBITMAP ImageLibrary::getBitmap(char const* name)
{
  ImageInfo& info = images.get(name);
  if (!info.hBitmap)
    info.hBitmap = info.getImage(mpq)->createBitmap();
  return info.hBitmap;
}
int ImageLibrary::getListIndex(char const* name)
{
  if (!images.has(name))
    name = "Empty";
  ImageInfo& info = images.get(name);
  if (info.listIndex < 0)
  {
    Image* image = info.getImage(mpq);
    if (image->width() == 16 && image->height() == 16)
    {
      if (!info.hBitmap)
        info.hBitmap = image->createBitmap();
      info.listIndex = ImageList_Add(list, info.hBitmap, NULL);
    }
  }
  return info.listIndex;
}

void ImageLibrary::addImage(char const* name, Image* image, bool big)
{
  if (images.has(name))
    return;

  String name16 = String::format("images\\%s.bin", name);
  File* file16 = mpq->openFile(name16, File::REWRITE);
  if (file16)
  {
    Image i16(16, 16);
    BLTInfo blt16(image);
    blt16.setDstSize(16, 16);
    i16.blt(blt16);
    i16.modBrightness(1.16f);
    i16.sharpen(0.16f);
    i16.writeBIN(file16);
    delete file16;
    loadImage(name16);
  }

  String name32 = String::format("images\\big%s.bin", name);
  File* file32 = mpq->openFile(name32, File::REWRITE);
  if (file32)
  {
    Image i32(32, 32);
    BLTInfo blt32(image);
    blt32.setDstSize(32, 32);
    i32.blt(blt32);
    i32.modBrightness(1.16f);
    i32.sharpen(0.08f);
    i32.writeBIN(file32);
    delete file32;
    loadImage(name32);
  }
}
