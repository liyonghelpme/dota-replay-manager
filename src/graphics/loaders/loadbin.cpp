#include <stdlib.h>

#include "base/file.h"
#include "graphics/image.h"
#include "base/utils.h"

bool Image::loadBIN(File* f)
{
  int sig;
  if (f->read(&sig, 4) != 4 || sig != 'BIMG')
    return false;
  if (f->read(&_width, 4) != 4 || f->read(&_height, 4) != 4)
    return false;
  int pos = f->tell();
  f->seek(0, SEEK_END);
  if (f->tell() - pos < _width * _height * 3)
    return false;
  f->seek(pos, SEEK_SET);
  _bits = new uint32[_width * _height];
  for (int i = 0; i < _width * _height; i++)
  {
    sint8 red = f->getc();
    sint8 green = f->getc();
    sint8 blue = f->getc();
    _bits[i] = clr(red, green, blue);
  }
  return true;
}
void Image::writeBIN(File* f)
{
  f->write_int32('BIMG');
  f->write_int32(_width);
  f->write_int32(_height);
  for (int i = 0; i < _width * _height; i++)
  {
    f->putc((_bits[i] >> 16) & 0xFF);
    f->putc((_bits[i] >> 8) & 0xFF);
    f->putc(_bits[i] & 0xFF);
  }
}
