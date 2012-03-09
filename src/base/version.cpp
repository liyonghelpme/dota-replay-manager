#include "version.h"

uint32 makeVersion(int major, int minor, int build)
{
  return (major * 100 + minor) * 26 + build;
}
String formatVersion(uint32 version)
{
  if (version % 26)
    return String::format("%d.%02d%c", version / 2600, (version / 26) % 100, char('a' + (version % 26)));
  else
    return String::format("%d.%02d", version / 2600, (version / 26) % 100);
}
uint32 parseVersion(String version)
{
  Array<String> sub;
  if (version.rfind("(\\d+)\\.(\\d+)([b-z])", 0, &sub) < 0)
    return 0;
  int major = sub[1].toInt();
  int minor = sub[2].toInt();
  int build = 0;
  if (!sub[3].isEmpty())
    build = int(sub[3][0] - 'a');
  return makeVersion(major, minor, build);
}
