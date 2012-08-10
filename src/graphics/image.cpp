#include <stdlib.h>

#include "base/file.h"

#include "image.h"

//#include "icon.h"

inline int _min(int a, int b)
{
  return a < b ? a : b;
}
inline int _max(int a, int b)
{
  return a > b ? a : b;
}

unsigned char Image::mtable[65536];
bool Image::gtable = false;
void Image::initTable ()
{
  if (!gtable)
  {
    for (int c = 0; c <= 255; c++)
      for (int a = 0; a <= 255; a++)
        mtable[a * 256 + c] = (unsigned char) (c * (255 - a) / 255);
    gtable = true;
  }
}
inline void _qmemset (void* mem, uint32 fill, uint32 count)
{
  _asm
  {
    cld
    mov edi, mem
    mov ecx, count
    mov eax, fill
    rep stosd
  }
}

Image::Image(int width, int height)
{
  //srcDC = NULL;
  //bmp = NULL;
  initTable ();

  _width = width;
  _height = height;
  _bits = new unsigned long[width * height];
  mode = _opaque;
  _qmemset (_bits, 0xFF000000, width * height);
  unowned = false;

  resetClipRect();
}
bool Image::load(File* file)
{
  //srcDC = NULL;
  //bmp = NULL;
  initTable();
  _width = 0;
  _height = 0;
  _bits = NULL;
  mode = _opaque;
  unowned = false;

  if (file)
  {
    bool loaded = false;
    file->seek(0, SEEK_SET); if (!loaded) loaded = loadGIF(file);
    file->seek(0, SEEK_SET); if (!loaded) loaded = loadPNG(file);
    file->seek(0, SEEK_SET); if (!loaded) loaded = loadTGA(file);
    file->seek(0, SEEK_SET); if (!loaded) loaded = loadBIN(file);
    file->seek(0, SEEK_SET); if (!loaded) loaded = loadBLP(file);
    file->seek(0, SEEK_SET); if (!loaded) loaded = loadBLP2(file);
    if (!loaded)
    {
      delete[] _bits;
      _bits = NULL;
      _width = 0;
      _height = 0;
      mode = _opaque;
    }

    if (loaded)
    {
      for (int i = _width * _height - 1; i >= 0 && mode == _opaque; i--)
        if ((_bits[i] & 0xFF000000) != 0xFF000000)
          mode = _alpha;
    }
    resetClipRect();
    return loaded;
  }
  resetClipRect();
  return false;
}
Image::Image(char const* filename)
{
  load(TempFile(File::open(filename, File::READ)));
}
Image::Image(File* file)
{
  load(file);
}
Image::Image(int width, int height, uint32* bits)
{
  initTable ();

  _width = width;
  _height = height;
  _bits = bits;
  mode = _opaque;
  _qmemset (_bits, 0xFF000000, width * height);
  unowned = true;

  resetClipRect ();
}
Image::~Image()
{
  if (!unowned)
    delete[] _bits;
  //delete srcDC;
  //delete bmp;
}

void Image::blt(int x, int y, Image const* src)
{
  if (src->_bits == NULL || src == this) return;
  blt(x, y, src, 0, 0, src->_width, src->_height);
}
void Image::make_premult()
{
  unsigned char* ptr = (unsigned char*) _bits;
  for (int i = _width * _height - 1; i >= 0; i--)
  {
    unsigned char* alpha = mtable + (255 - ptr[3]) * 256; 
    *ptr = alpha[*ptr]; ptr++;
    *ptr = alpha[*ptr]; ptr++;
    *ptr = alpha[*ptr]; ptr++;
    ptr++;
  }
  mode = _premult;
}
void Image::blt(int x, int y, Image const* src, int srcX, int srcY, int srcW, int srcH)
{
  if (src == NULL || src->_bits == NULL || src == this) return;
  int startY = _max(0, clipRect.top - y);
  int endY = _min(clipRect.bottom - y, srcH);
  int startX = _max(0, clipRect.left - x);
  int endX = _min(clipRect.right - x, srcW);
  startY += srcY;
  endY += srcY;
  startX += srcX;
  endX += srcX;
  x -= srcX;
  y -= srcY;
  if (mode != _opaque && src->mode != _opaque)
    make_premult();
  for (int cy = startY; cy < endY; cy++)
  {
    unsigned char* srcBits = (unsigned char*) (src->_bits + src->_width * cy + startX);
    unsigned char* dstBits = (unsigned char*) (_bits + _width * (cy + y) + (startX + x));
    if (src->mode == _opaque)
      memcpy(dstBits, srcBits, (endX - startX) * 4);
    else if (mode == _opaque)
    {
      if (src->mode == _alpha)
      {
        for (int cx = startX; cx < endX; cx++)
        {
          unsigned char* tSrc = mtable + (255 - srcBits[3]) * 256;
          unsigned char* tDst = mtable + srcBits[3] * 256;
          *dstBits = tDst[*dstBits] + tSrc[*srcBits++]; dstBits++;
          *dstBits = tDst[*dstBits] + tSrc[*srcBits++]; dstBits++;
          *dstBits = tDst[*dstBits] + tSrc[*srcBits++]; dstBits++;
          dstBits++, srcBits++;
        }
      }
      else
      {
        for (int cx = startX; cx < endX; cx++)
        {
          unsigned char* tDst = mtable + srcBits[3] * 256;
          *dstBits = tDst[*dstBits] + *srcBits++; dstBits++;
          *dstBits = tDst[*dstBits] + *srcBits++; dstBits++;
          *dstBits = tDst[*dstBits] + *srcBits++; dstBits++;
          dstBits++, srcBits++;
        }
      }
    }
    else
    {
      for (int cx = startX; cx < endX; cx++)
      {
        unsigned char* tSrc = mtable + (src->mode == _alpha ? (255 - srcBits[3]) * 256 : 0);
        unsigned char* tDst = mtable + srcBits[3] * 256;
        *dstBits = tDst[*dstBits] + tSrc[*srcBits++]; dstBits++;
        *dstBits = tDst[*dstBits] + tSrc[*srcBits++]; dstBits++;
        *dstBits = tDst[*dstBits] + tSrc[*srcBits++]; dstBits++;
        *dstBits = 255 - tDst[255 - *dstBits];
        dstBits++, srcBits++;
      }
    }
  }
}
void Image::fill(unsigned int color)
{
  _qmemset(_bits, color, _width * _height);
  mode = ((color & 0xFF000000) == 0xFF000000 ? _opaque : _alpha);
}
void Image::fill(unsigned int color, int x, int y, int width, int height)
{
  if (y < clipRect.top) {height -= clipRect.top - y; y = clipRect.top;}
  if (x < clipRect.left) {width -= clipRect.left - x; x = clipRect.left;}
  if (y + height > clipRect.bottom) height = clipRect.bottom - y;
  if (x + width > clipRect.right) width = clipRect.right - x;
  if ((x != 0 || y != 0 || width != _width || height != _height) && mode == _premult)
  {
    unsigned char* buf = (unsigned char*) &color;
    unsigned char* map = mtable + (255 - buf[3]) * 256;
    buf[0] = map[buf[0]];
    buf[1] = map[buf[1]];
    buf[2] = map[buf[2]];
  }
  for (int cy = y; cy < y + height; cy++)
    _qmemset(_bits + _width * cy + x, color, width);
  if (x == 0 && y == 0 && width == _width && height == _height)
    mode = ((color & 0xFF000000) == 0xFF000000 ? _opaque : _alpha);
  else if (width && height && mode == _opaque && (color & 0xFF000000) != 0xFF000000)
    mode = _alpha;
}

HBITMAP Image::createBitmap(HDC hDC)
{
  bool nullDC = (hDC == NULL);
  if (nullDC)
    hDC = GetDC(NULL);
  HBITMAP hBitmap = CreateCompatibleBitmap(hDC, _width, _height);
  fillBitmap(hBitmap, hDC);
  if (nullDC)
    ReleaseDC(NULL, hDC);
  return hBitmap;
}
void Image::fillBitmap(HBITMAP hBitmap, HDC hDC)
{
  BITMAPINFOHEADER bi;
  bi.biSize = sizeof bi;
  bi.biWidth = _width;
  bi.biHeight = -_height;
  bi.biPlanes = 1;
  bi.biBitCount = 32;
  bi.biCompression = BI_RGB;
  bi.biSizeImage = 0;
  bi.biXPelsPerMeter = 1;
  bi.biYPelsPerMeter = 1;
  bi.biClrUsed = 0;
  bi.biClrImportant = 0;
  SetDIBits(hDC, hBitmap, 0, _height, _bits, (BITMAPINFO*) &bi, DIB_RGB_COLORS);
}

//void Image4D::render (CDC* dc, int x, int y, bool opaque)
//{
//  if (srcDC == NULL)
//  {
//    srcDC = new CDC;
//    srcDC->CreateCompatibleDC (dc);
//    bmp = new CBitmap;
//    bmp->CreateCompatibleBitmap (dc, width, height);
//    srcDC->SelectObject (bmp);
//  }
//  if (!opaque && mode == _alpha)
//    make_premult ();
//  fillBitmap (bmp, dc);
//  RECT clip;
//  dc->GetClipBox (&clip);
//  int startX = 0;
//  int startY = 0;
//  int endX = width;
//  int endY = height;
//  if (x + startX < clip.left) startX = clip.left - x;
//  if (y + startY < clip.top) startY = clip.top - y;
//  if (x + endX > clip.right) endX = clip.right - x;
//  if (y + endY > clip.bottom) endY = clip.bottom - y;
//  if (endX > startX && endY > startY)
//  {
//    if (opaque || mode == _opaque)
//      dc->BitBlt (x + startX, y + startY, endX - startX, endY - startY, srcDC, startX, startY, SRCCOPY);
//    else
//    {
//      BLENDFUNCTION bf;
//      bf.BlendOp = AC_SRC_OVER;
//      bf.BlendFlags = 0;
//      bf.SourceConstantAlpha = 255;
//      bf.AlphaFormat = AC_SRC_ALPHA;
//      dc->AlphaBlend (x + startX, y + startY, endX - startX, endY - startY, srcDC,
//                      startX, startY, endX - startX, endY - startY, bf);
//    }
//  }
//}
//void Image4D::eraseBkgnd (CDC* dc, int x, int y, unsigned long color)
//{
//  CWnd* wnd = dc->GetWindow ();
//  CRect rc;
//  wnd->GetClientRect (rc);
//  eraseBkgnd (dc, x, y, rc, color);
//}
//void Image4D::eraseBkgnd (CDC* dc, int x, int y, CRect const& rc, unsigned long color)
//{
//  if (x > 0)
//    dc->FillSolidRect (0, 0, x, rc.bottom, color);
//  if (x + width < rc.right)
//    dc->FillSolidRect (x + width, 0, rc.right, rc.bottom, color);
//  if (y > 0)
//    dc->FillSolidRect (x, 0, x + width, y, color);
//  if (y + height < rc.bottom)
//    dc->FillSolidRect (x, y + height, x + width, rc.bottom, color);
//}
//void Image4D::setClipRect (CDC* dc, int x, int y)
//{
//  resetClipRect ();
//  RECT clip;
//  dc->GetClipBox (&clip);
//  if (x + clipRect.left < clip.left) clipRect.left = clip.left - x;
//  if (y + clipRect.top < clip.top) clipRect.top = clip.top - y;
//  if (x + clipRect.right > clip.right) clipRect.right = clip.right - x;
//  if (y + clipRect.bottom > clip.bottom) clipRect.bottom = clip.bottom - y;
//}
static inline unsigned char _add(unsigned char x, unsigned char y)
{
  if (x > 255 - y)
    return 255;
  else
    return x + y;
}
static inline unsigned char _clip(__int64 x)
{
  if (x > 255)
    return 255;
  else
    return (unsigned char) x;
}
void Image::blt(BLTInfo& info)
{
  if (info.src == NULL || info.src->_bits == NULL || info.src == this)
    return;
  if (info.dstW == 0 || info.dstH == 0)
    return;
  if (info.srcW == 0 || info.srcH == 0)
    return;
  if (info.dstW < 0)
  {
    info.x += info.dstW;
    info.dstW = -info.dstW;
    info.flipX = !info.flipX;
  }
  if (info.srcW < 0)
  {
    info.srcX += info.srcW;
    info.srcW = -info.srcW;
    info.flipX = !info.flipX;
  }
  if (info.dstH < 0)
  {
    info.y += info.dstH;
    info.dstH = -info.dstH;
    info.flipX = !info.flipX;
  }
  if (info.srcH < 0)
  {
    info.srcY += info.srcH;
    info.srcH = -info.srcH;
    info.flipY = !info.flipY;
  }
  bool mod = ((info.modulate & 0x00FFFFFF) != 0x00FFFFFF);
  unsigned char* modb = mtable + (255 - (info.modulate & 0xFF)) * 256;
  unsigned char* modg = mtable + (255 - ((info.modulate >> 8) & 0xFF)) * 256;
  unsigned char* modr = mtable + (255 - ((info.modulate >> 16) & 0xFF)) * 256;
  if (mode != _opaque && info.src->mode != _opaque)
    make_premult ();
  if (info.srcW == info.dstW && info.srcH == info.dstH)
  {
    int cyStart = 0;
    int cyEnd = info.dstH;
    if (info.y + cyStart < clipRect.top) cyStart = clipRect.top - info.y;
    if (info.y + cyEnd > clipRect.bottom) cyEnd = clipRect.bottom - info.y;
    if (info.flipY)
    {
      if (info.srcY + info.srcH - cyStart > info.src->_height)
        cyStart = info.srcY + info.srcH - info.src->_height;
      if (info.srcY + info.srcH - cyEnd < 0)
        cyEnd = info.srcY + info.srcH;
    }
    else
    {
      if (info.srcY + cyStart < 0) cyStart = -info.srcY;
      if (info.srcY + cyEnd > info.src->_height) cyEnd = info.src->_height - info.srcY;
    }
    int cxStart = 0;
    int cxEnd = info.dstW;
    if (info.x + cxStart < clipRect.left) cxStart = clipRect.left - info.x;
    if (info.x + cxEnd > clipRect.right) cxEnd = clipRect.right - info.x;
    if (info.flipX)
    {
      if (info.srcX + info.srcW - cxStart > info.src->_width)
        cxStart = info.srcX + info.srcW - info.src->_width;
      if (info.srcX + info.srcW - cxEnd < 0)
        cxEnd = info.srcX + info.srcW;
    }
    else
    {
      if (info.srcX + cxStart < 0) cxStart = -info.srcX;
      if (info.srcX + cxEnd > info.src->_width) cxEnd = info.src->_width - info.srcX;
    }
    int dstOY = info.y;
    int dstOX = info.x;
    int srcOY = info.flipY ? info.srcY + info.srcH - 1 : info.srcY;
    int srcOX = info.flipX ? info.srcX + info.srcW - 1 : info.srcX;
    int srcDY = info.flipY ? -1 : 1;
    int srcDX = info.flipX ? -1 : 1;
    for (int cy = cyStart; cy < cyEnd && cxEnd > cxStart; cy++)
    {
      int dstY = dstOY + cy;
      int srcY = srcOY + cy * srcDY;
      unsigned char* srcBits = (unsigned char*) (info.src->_bits + info.src->_width * srcY + srcOX + cxStart);
      unsigned char* dstBits = (unsigned char*) (_bits + _width * dstY + dstOX + cxStart);
      if (info.src->mode == _opaque && !info.flipX && !info.desaturate && info.alphaMode == BLTInfo::Blend)
        memcpy(dstBits, srcBits, (cxEnd - cxStart) * 4);
      else
      {
        for (int cx = cxStart; cx < cxEnd; cx++)
        {
          unsigned char* tSrc = mtable + (info.src->mode == _alpha ? (255 - srcBits[3]) * 256 : 0);
          unsigned char* tMid = mtable + srcBits[3] * 256;
          unsigned char* tDst = (mode == _alpha ? mtable + (255 - tMid[dstBits[3]]) * 256 : tMid);
          unsigned char srcb = srcBits[0];
          unsigned char srcg = srcBits[1];
          unsigned char srcr = srcBits[2];
          if (mod)
          {
            srcb = modb[srcb];
            srcg = modg[srcg];
            srcr = modr[srcr];
          }
          if (info.desaturate)
          {
            unsigned char sat = brightness (srcr, srcg, srcb);
            srcb = sat;
            srcg = sat;
            srcr = sat;
          }
          if (info.alphaMode == BLTInfo::Blend)
          {
            dstBits[0] = tDst[dstBits[0]] + tSrc[srcb];
            dstBits[1] = tDst[dstBits[1]] + tSrc[srcg];
            dstBits[2] = tDst[dstBits[2]] + tSrc[srcr];
            dstBits[3] = 255 - tMid[255 - dstBits[3]];
          }
          else
          {
            tDst = mtable + (mode == _alpha ? (255 - dstBits[3]) * 256 : 0);
            dstBits[0] = _add (tDst[dstBits[0]], tSrc[srcb]);
            dstBits[1] = _add (tDst[dstBits[1]], tSrc[srcg]);
            dstBits[2] = _add (tDst[dstBits[2]], tSrc[srcr]);
          }
          dstBits += 4;
          srcBits += 4 * srcDX;
        }
      }
    }
  }
  else
  {
    int cxStart = 0;
    int cxEnd = info.dstW;
    if (info.x + cxStart < clipRect.left) cxStart = clipRect.left - info.x;
    if (info.x + cxEnd > clipRect.right) cxEnd = clipRect.right - info.x;
    int cyStart = 0;
    int cyEnd = info.dstH;
    if (info.y + cyStart < clipRect.top) cyStart = clipRect.top - info.y;
    if (info.y + cyEnd > clipRect.bottom) cyEnd = clipRect.bottom - info.y;
    for (int cx = cxStart; cx < cxEnd; cx++)
    {
      for (int cy = cyStart; cy < cyEnd; cy++)
      {
        int dstX = info.x + cx;
        int dstY = info.y + cy;
        int srcX, srcY;
        if (info.flipX)
          srcX = (info.srcX * info.srcW) * info.dstW - (cx + 1) * info.srcW;
        else
          srcX = info.srcX * info.dstW + cx * info.srcW;
        if (info.flipY)
          srcY = (info.srcY * info.srcH) * info.dstH - (cy + 1) * info.srcH;
        else
          srcY = info.srcY * info.dstH + cy * info.srcH;
        __int64 cb = 0;
        __int64 cg = 0;
        __int64 cr = 0;
        __int64 ca = 0;
        __int64 area = 0;
        int curX = srcX;
        int curY = srcY;
        while (curX < srcX + info.srcW)
        {
          int px = curX / info.dstW;
          int endX = (px + 1) * info.dstW;
          if (endX > srcX + info.srcW)
            endX = srcX + info.srcW;
          if (px >= 0 && px < info.src->_width)
          {
            while (curY < srcY + info.srcH)
            {
              int py = curY / info.dstH;
              int endY = (py + 1) * info.dstH;
              if (endY > srcY + info.srcH)
                endY = srcY + info.srcH;
              if (py >= 0 && py < info.src->_height)
              {
                unsigned char* src = (unsigned char*) (info.src->_bits + info.src->_width * py + px);
                __int64 da = (endX - curX) * (endY - curY);
                __int64 alpha = src[3];
                __int64 salpha = (info.src->mode == _alpha ? alpha : 255);
                if (info.desaturate)
                {
                  __int64 sat = __int64(brightness (modr[src[2]], modg[src[1]], modb[src[0]])) * salpha * da;
                  cb += sat;
                  cg += sat;
                  cr += sat;
                }
                else
                {
                  cb += __int64(modr[src[0]]) * salpha * da;
                  cg += __int64(modg[src[1]]) * salpha * da;
                  cr += __int64(modb[src[2]]) * salpha * da;
                }
                ca += __int64 (255 - alpha) * da;
                area += da;
              }
              curY = endY;
            }
          }
          curX = endX;
        }
        if (area)
        {
          unsigned char* dst = (unsigned char*) (_bits + _width * dstY + dstX);
          unsigned char* tDst = mtable + (mode == _alpha ? (255 - dst[3]) * 256 : 0);
          if (info.alphaMode == BLTInfo::Blend)
          {
            dst[0] = (unsigned char) ((__int64(tDst[dst[0]]) * ca + cb) / area / 255);
            dst[1] = (unsigned char) ((__int64(tDst[dst[1]]) * ca + cg) / area / 255);
            dst[2] = (unsigned char) ((__int64(tDst[dst[2]]) * ca + cr) / area / 255);
            dst[3] = 255 - (unsigned char) (__int64(255 - dst[3]) * ca / area / 255);
          }
          else
          {
            dst[0] = _clip (__int64(tDst[dst[0]]) + cb / area / 255);
            dst[1] = _clip (__int64(tDst[dst[1]]) + cg / area / 255);
            dst[2] = _clip (__int64(tDst[dst[2]]) + cr / area / 255);
          }
        }
      }
    }
  }
  if (mode != _opaque)
    mode = _premult;
}

void Image::desaturate()
{
  unsigned char* data = (unsigned char*) _bits;
  for (int i = _width * _height; i > 0; i--)
  {
    unsigned char sat = brightness(data[0], data[1], data[2]);
    data[0] = sat;
    data[1] = sat;
    data[2] = sat;
    data += 4;
  }
}

bool Image::getRect(int& left, int& top, int& right, int& bottom) const
{
  left = _width;
  top = _height;
  right = 0;
  bottom = 0;
  unsigned long* data = _bits;
  for (int y = 0; y < _height; y++)
  {
    for (int x = 0; x < _width; x++, data++)
    {
      if ((*data) & 0xFF000000)
      {
        if (x < left) left = x;
        if (x >= right) right = x + 1;
        if (y < top) top = y;
        if (y >= bottom) bottom = y + 1;
      }
    }
  }
  if (left >= right)
    return false;
  return true;
}

//ImageList::ImageList (int _width, int _height, int _background)
//{
//  width = _width;
//  height = _height;
//  background = _background | 0xFF000000;
//  list = new CImageList;
//  img = new Image4D (width, height);
//  bmp = new CBitmap;
//  list->Create (width, height, ILC_COLOR24, 16, 16);
//
//  CDC* dc = CDC::FromHandle (GetDC (NULL));
//  bmp->CreateCompatibleBitmap (dc, width, height);
//  ReleaseDC (NULL, dc->m_hDC);
//}
//ImageList::~ImageList ()
//{
//  delete img;
//  delete bmp;
//  delete list;
//}
//void ImageList::reset ()
//{
//  list->DeleteImageList ();
//  list->Create (width, height, ILC_COLOR24, 16, 16);
//  images.clear ();
//}
//int ImageList::getImagePos (Image4D* image)
//{
//  if (image == NULL) return getBlankPos ();
//  if (images.has ((uint32) image))
//    return images.get ((uint32) image);
//
//  img->fill (background);
//  BLTInfo blt (image);
//  blt.setDstSize (width, height);
//  img->blt (blt);
//
//  CDC* dc = CDC::FromHandle (GetDC (NULL));
//  img->fillBitmap (bmp, dc);
//  int pos = list->Add (bmp, (CBitmap*) NULL);
//  ReleaseDC (NULL, dc->m_hDC);
//
//  images.set ((uint32) image, pos);
//  return pos;
//}
//int ImageList::getIconPos (Icon* icon, int size)
//{
//  if (icon == NULL) return getBlankPos ();
//  if (images.has ((uint32) icon))
//    return images.get ((uint32) icon);
//
//  img->fill (background);
//  Image4D* image = icon->getImage (size).getImage ();
//  BLTInfo blt (image);
//  blt.setDstSize (width, height);
//  img->blt (blt);
//
//  CDC* dc = CDC::FromHandle (GetDC (NULL));
//  img->fillBitmap (bmp, dc);
//  int pos = list->Add (bmp, (CBitmap*) NULL);
//  ReleaseDC (NULL, dc->m_hDC);
//
//  images.set ((uint32) icon, pos);
//  return pos;
//}
//int ImageList::getBlankPos ()
//{
//  if (images.has (0))
//    return images.get (0);
//
//  img->fill (background);
//
//  CDC* dc = CDC::FromHandle (GetDC (NULL));
//  img->fillBitmap (bmp, dc);
//  int pos = list->Add (bmp, (CBitmap*) NULL);
//  ReleaseDC (NULL, dc->m_hDC);
//
//  images.set (0, pos);
//  return pos;
//}

static inline int swork(int org, int acc, int count, float coeff)
{
  int res = int(float(org) * (1 + count * coeff) - float(acc) * coeff + 0.5f);
  if (res < 0) res = 0;
  if (res > 255) res = 255;
  return res;
}
static inline int sadd(uint32 pixel, int* acc)
{
  acc[0] += int(pixel & 0xFF);
  acc[1] += int((pixel >> 8) & 0xFF);
  acc[2] += int((pixel >> 16) & 0xFF);
  acc[3] += int((pixel >> 24) & 0xFF);
  return 1;
}
void Image::sharpen(float coeff)
{
  static int ox[4] = {-1, 0, 0, 1};
  static int oy[4] = {0, -1, 1, 0};
  uint32* extra = new uint32[_width];
  for (int y = 0; y < _height; y++)
  {
    uint32 prev;
    for (int x = 0; x < _width; x++)
    {
      int pos = x + y * _width;
      int acc[4] = {0, 0, 0, 0};
      int count = 0;
      if (x > 0)
        count += sadd(prev, acc);
      if (y > 0)
        count += sadd(extra[x], acc);
      if (x < _width - 1)
        count += sadd(_bits[pos + 1], acc);
      if (y < _height - 1)
        count += sadd(_bits[pos + _width], acc);
      prev = _bits[pos];
      extra[x] = _bits[pos];
      _bits[pos] = swork(prev & 0xFF, acc[0], count, coeff) |
                  (swork((prev >> 8) & 0xFF, acc[0], count, coeff) << 8) |
                  (swork((prev >> 16) & 0xFF, acc[0], count, coeff) << 16) |
                  (swork((prev >> 24) & 0xFF, acc[0], count, coeff) << 24);
    }
  }
  delete[] extra;
}
static inline int bwork(int org, float coeff)
{
  int res = int(float(org) * coeff + 0.5f);
  if (res < 0) res = 0;
  if (res > 255) res = 255;
  return res;
}
void Image::modBrightness(float coeff)
{
  for (int i = _width * _height - 1; i >= 0; i--)
    _bits[i] = bwork(_bits[i] & 0xFF, coeff) |
              (bwork((_bits[i] >> 8) & 0xFF, coeff) << 8) |
              (bwork((_bits[i] >> 16) & 0xFF, coeff) << 16) |
              (_bits[i] & 0xFF000000);
}
