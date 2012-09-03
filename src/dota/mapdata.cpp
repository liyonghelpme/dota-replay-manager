#include "core/app.h"

#include "mapdata.h"
#include "graphics/image.h"

MapData::MapData(char const* path)
  : images(DictionaryMap::alNumNoCase)
{
  imgList = NULL;
  map = MPQArchive::open(path, File::READ);
  if (map == NULL)
    map = MPQArchive::open(String::buildFullName(cfg.warPath, path), File::READ);
  if (map)
  {
    imgList = ImageList_Create(16, 16, ILC_COLOR24, 16, 16);

    MPQLoader loader(*getApp()->getWarLoader());
    loader.addArchive(map);
    LoadGameData(data, &loader, WC3_LOAD_UNITS | WC3_LOAD_ITEMS |
      WC3_LOAD_ABILITIES | WC3_LOAD_UPGRADES | WC3_LOAD_MERGED | WC3_LOAD_NO_WEONLY);

    Image blank(16, 16);
    blank.fill(Image::clr(255, 255, 255));
    HBITMAP hBitmap = blank.createBitmap();
    ImageList_Add(imgList, hBitmap, NULL);
    DeleteObject(hBitmap);

    images.set("", 0);
  }
}
MapData::~MapData()
{
  delete map;
  if (imgList)
    ImageList_Destroy(imgList);
}

int MapData::getImageIndex(String name)
{
  String title = String::getFileTitle(name);
  if (images.has(title))
    return images.get(title);

  MPQLoader loader(*getApp()->getWarLoader());
  loader.addArchive(map);
  File* file = loader.load(name);
  if (file == NULL)
  {
    String::setExtension(name, ".blp");
    file = loader.load(name);
  }

  Image image(file);
  delete file;

  if (image.bits())
  {
    Image i16(16, 16);
    BLTInfo blt16(&image);
    blt16.setDstSize(16, 16);
    i16.blt(blt16);
    i16.modBrightness(1.16f);
    i16.sharpen(0.08f);

    HBITMAP hBitmap = i16.createBitmap();
    int pos = ImageList_Add(imgList, hBitmap, NULL);
    DeleteObject(hBitmap);

    images.set(title, pos);
    return pos;
  }

  return 0;
}
