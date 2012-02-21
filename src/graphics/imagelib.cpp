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
  String title = String::getFileTitle(name).toLower();
  ImageInfo& info = images.create(title);
  info.filename = name;
}

String ImageLibrary::getTooltip(String name)
{
  return images.get(name.toLower()).tooltip;
}
void ImageLibrary::setTooltip(String name, String tooltip)
{
  images.get(name.toLower()).tooltip = tooltip;
}
Image* ImageLibrary::getImage(String name)
{
  ImageInfo& info = images.get(name.toLower());
  return info.getImage(mpq);
}
HBITMAP ImageLibrary::getBitmap(String name)
{
  ImageInfo& info = images.get(name.toLower());
  if (!info.hBitmap)
    info.hBitmap = info.getImage(mpq)->createBitmap();
  return info.hBitmap;
}
int ImageLibrary::getListIndex(String name)
{
  name.toLower();
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
