#ifndef __DOTA_MAPDATA__
#define __DOTA_MAPDATA__

#include "dota/load/datafile.h"
#include "base/mpqfile.h"

class MapData
{
  HIMAGELIST imgList;
  Dictionary<int> images;
  GameData data;
  MPQArchive* map;
public:
  MapData(char const* path);
  ~MapData();

  bool isLoaded() const
  {
    return map != NULL;
  }

  HIMAGELIST getImageList() const
  {
    return imgList;
  }
  ObjectData* getData() const
  {
    return data.merged;
  }

  int getImageIndex(String name);
};

#endif // __DOTA_MAPDATA__
