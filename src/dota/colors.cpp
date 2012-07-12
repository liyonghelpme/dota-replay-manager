#include "colors.h"

uint32 getDefaultColor(int clr)
{
  switch (clr)
  {
  case 0:  return 0x0202FF;
  case 1:  return 0xFF4100;
  case 2:  return 0xB8E51B;
  case 3:  return 0x800053;
  case 4:  return 0x00FCFF;
  case 5:  return 0x0D89FE;
  case 6:  return 0x00BF1F;
  case 7:  return 0xAF5AE4;
  case 8:  return 0x969594;
  case 9:  return 0xF1BE7D;
  case 10: return 0x45610F;
  case 11: return 0x03294D;
  default: return 0xFFFFFF;
  }
}
uint32 getSlotColor(int clr)
{
  switch (clr)
  {
  case 0:  return 0x0202FF;
  case 1:  return 0xFF4100;
  case 2:  return 0xB8E51B;
  case 3:  return 0xFF00A6;
  case 4:  return 0x00D2D6;
  case 5:  return 0x0D89FE;
  case 6:  return 0x00BF1F;
  case 7:  return 0xAF5AE4;
  case 8:  return 0x969594;
  case 9:  return 0xF1BE7D;
  case 10: return 0x45610F;
  case 11: return 0x054D8D;
  default: return 0xFFFFFF;
  }
}
uint32 getLightColor(int clr)
{
  uint32 c = getSlotColor(clr);
  for (int i = 0; i < 24; i += 8)
    c = (c & (~(0xFF << i))) | ((((((c >> i) & 0xFF) + 510) / 3) & 0xFF) << i);
  return c;
}
uint32 getDarkColor(int clr)
{
  uint32 c = getSlotColor(clr);
  if ((c & 0xFF) + ((c >> 8) & 0xFF) + ((c >> 16) & 0xFF) > 383)
  {
    for (int i = 0; i < 24; i += 8)
      c = (c & (~(0xFF << i))) | (((((c >> i) & 0xFF) * 2 / 3) & 0xFF) << i);
  }
  return c;
}
uint32 getFlipColor(int clr)
{
  uint32 c = getSlotColor(clr);
  return ((c >> 16) & 0xFF) | (c & 0x00FF00) | ((c & 0xFF) << 16);
}
uint32 getExtraColor(int clr)
{
  switch (clr)
  {
  case 15: return 0x0000FF; // Haste
  case 16: return 0x00FF00; // Regeneration
  case 17: return 0xFF0000; // Double Damage
  case 18: return 0x00AFAF; // Illusion
  case 19: return 0xC12D65; // Invisibility

  case 20: return 0x40FF00; // Killing Spree
  case 21: return 0x800040; // Dominating
  case 22: return 0x8000FF; // Mega Kill
  case 23: return 0x0080FF; // Unstoppable
  case 24: return 0x008080; // Wicked Sick
  case 25: return 0xFF80FF; // Monster Kill
  case 26: return 0x0000FF; // GODLIKE
  case 27: return 0x0080FF; // Beyond GODLIKE

  case 28: return 0xFF0000; // Double Kill
  case 29: return 0x40FF00; // Triple Kill
  case 30: return 0xFFFF00; // Ultra Kill
  case 31: return 0xFFAA00; // Rampage

  default: return 0xFFFFFF;
  }
}
