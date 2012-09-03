#include <stdlib.h>

#include "base/file.h"
#include "graphics/image.h"
#include "base/utils.h"

bool Image::loadBIN(File* f)
{
  int sig;
  if (f->read(&sig, 4) != 4)
    return false;
  if (f->read(&_width, 4) != 4 || f->read(&_height, 4) != 4)
    return false;
  int pos = f->tell();
  f->seek(0, SEEK_END);
  int length = f->tell() - pos;
  f->seek(pos, SEEK_SET);
  if (sig == 'BIMG')
  {
    if (length < _width * _height * 3)
      return false;
    _bits = new uint32[_width * _height];
    for (int i = 0; i < _width * _height; i++)
    {
      sint8 red = f->getc();
      sint8 green = f->getc();
      sint8 blue = f->getc();
      _bits[i] = clr(red, green, blue);
    }
  }
  else if (sig == 'BIM2')
  {
    if (length < _width * _height * sizeof(uint32))
      return false;
    f->read(_bits, _width * _height * sizeof(uint32));
    for (int i = 0; i < _width * _height; i++)
      _bits[i] = clr_rgba_noflip(_bits[i]);
  }
  else if (sig == 'BIM3')
  {
    if (length < 4 + _width * _height * sizeof(uint32))
      return false;
    _bits = new uint32[_width * _height];
    f->read(&_flags, 4);
    _flags &= _bgcolor;
    f->read(_bits, _width * _height * sizeof(uint32));
  }
  else
    return false;
  return true;
}
void Image::writeBIN(File* f)
{
  updateAlpha();
  if (_flags & _premult)
  {
    f->write_int32('BIM3');
    f->write_int32(_width);
    f->write_int32(_height);
    f->write_int32(_flags & _bgcolor);
    f->write(_bits, _width * _height * sizeof(uint32));
  }
  else
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
}
