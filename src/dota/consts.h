#ifndef __DOTA_CONSTS_H__
#define __DOTA_CONSTS_H__

#include "base/types.h"
#include "base/string.h"

#define COLOR_RED             0
#define COLOR_BLUE            1
#define COLOR_TEAL            2
#define COLOR_PURPLE          3
#define COLOR_YELLOW          4
#define COLOR_ORANGE          5
#define COLOR_GREEN           6
#define COLOR_PINK            7
#define COLOR_GRAY            8
#define COLOR_LIGHTBLUE       9
#define COLOR_DARKGREEN       10
#define COLOR_BROWN           11
#define COLOR_OBSERVER        12

#define LANE_ROAMING          0
#define LANE_TOP              1
#define LANE_MIDDLE           2
#define LANE_BOTTOM           3
#define LANE_AFK              4
String getLaneName(int lane);

#define MODE_AP         0x00000001
#define MODE_AR         0x00000002
#define MODE_LM         0x00000004
#define MODE_MM         0x00000008
#define MODE_TR         0x00000010
#define MODE_DM         0x00000020
#define MODE_MR         0x00000040
#define MODE_SP         0x00000080
#define MODE_AA         0x00000100
#define MODE_AI         0x00000200

#define MODE_AS         0x00000400
#define MODE_ID         0x00000800
#define MODE_NP         0x00001000
#define MODE_SC         0x00002000
#define MODE_EM         0x00004000
#define MODE_DU         0x00008000
#define MODE_SH         0x00010000
#define MODE_VR         0x00020000
#define MODE_RV         0x00040000
#define MODE_RD         0x00080000

#define MODE_OM         0x00100000
#define MODE_XL         0x00200000
#define MODE_NM         0x00400000
#define MODE_NT         0x00800000
#define MODE_NB         0x01000000
#define MODE_NS         0x02000000
#define MODE_NR         0x04000000
#define MODE_TS         0x08000000
#define MODE_SD         0x10000000
#define MODE_CD         0x20000000

#define MODE_PM         0x40000000
#define MODE_OI         0x80000000
#define MODE_MI         0x10000000
#define MODE_CM       0x0200000000LL
#define MODE_FR       0x0400000000LL
#define MODE_MO       0x0800000000LL
#define MODE_RO       0x1000000000LL
#define MODE_ER       0x2000000000LL
#define MODE_RS       0x4000000000LL
#define MODE_SO       0x8000000000LL

#define MODE_WTF    0x800000000000LL

int getModeTime (uint64 mode);
uint64 parseMode(String mode, String* parsed = NULL);
String formatMode(uint64 mode);

class Image;
class File;
void addMapIcons(Image* image, File* desc);

#endif // __DOTA_CONSTS_H__
