#ifndef __DOTA_MISC_H__
#define __DOTA_MISC_H__

#include "base/types.h"
#include "base/string.h"

uint32 getDefaultColor(int clr);
uint32 getSlotColor(int clr);
uint32 getLightColor(int clr);
uint32 getDarkColor(int clr);
uint32 getFlipColor(int clr);
String getExtraColors();
uint32 getExtraColor(int clr);

#endif // __DOTA_MISC_H__
