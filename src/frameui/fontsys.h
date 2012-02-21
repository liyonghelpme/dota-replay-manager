#ifndef __FRAMEUI_SYSFONT_H__
#define __FRAMEUI_SYSFONT_H__

#include <windows.h>

#include "base/string.h"
#include "graphics/rect.h"

#define FONT_BOLD       0x0001
#define FONT_ITALIC     0x0002
#define FONT_UNDERLINE  0x0004
#define FONT_STRIKEOUT  0x0008

class FontSys
{
  struct FontStruct
  {
    HFONT font;
    String face;
    String name;
    int size;
    int flags;
  };
  FontStruct** fonts;
  int numFonts;
  int maxFonts;
  int logPixelsY;
  HDC hDC;
  FontSys();
  static FontSys instance;
  HFONT _getFont(int height, String face, int flags = 0);
public:
  ~FontSys();

  static HFONT getSysFont();
  static HFONT getFont(int size, String face, int flags = 0);
  static HFONT getFont(int size, int flags = 0);
  static HFONT changeSize(int size, HFONT oldFont = NULL);
  static HFONT changeFlags(int flags, HFONT oldFont = NULL);
  static void setFontName(HFONT hFont, String name);
  static HFONT getFontByName(String name);

  static HFONT getFont(LOGFONT const& lf);

  static int getFlags(HFONT font = NULL);

  static Point getTextSize(HFONT font, String text);
  static int getMTextHeight(HFONT font, int width, String text);
};

#endif // __FRAMEUI_SYSFONT_H__
