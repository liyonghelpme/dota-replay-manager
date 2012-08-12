#ifndef __MAP_DATA_H__
#define __MAP_DATA_H__

#include "datafile.h"
#include "rmpq.h"

class MapData
{
  CImageList iList;
  GameData data;
  MPQARCHIVE map;
  typedef char imgname[256];
  imgname* images;
  int maxImages;
  int numImages;
public:
  MapData (char const* path);
  ~MapData ();

  bool isLoaded () const
  {
    return map != 0;
  }

  CImageList* getImageList ()
  {
    return &iList;
  }
  ObjectData* getData ();
  int getIconIndex (char const* path);
};

#endif // __MAP_DATA_H__
