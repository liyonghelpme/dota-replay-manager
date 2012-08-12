#include "stdafx.h"
#include "rmpq.h"
#include "dota.h"
#include "utils.h"

void parseTXT (MPQLOADER mpq, char const* path)
{
  MPQFILE txt = MPQLoadFile (mpq, path);
  if (txt)
  {
    int curid = 0;
    char line[1024];
    while (MPQFileGets (txt, sizeof line, line))
    {
      int shift = 0;
      while (line[shift] == ' ') shift++;
      int i = 0;
      while (line[i] = line[i + shift]) i++;
      while (i && (line[i - 1] == ' ' || line[i - 1] == '\r' || line[i - 1] == '\t' || line[i - 1] == '\n'))
        line[--i] = 0;
      if (line[0] != '[')
      {
        int i = 0;
        while (line[i] && line[i] != '"')
          i++;
        if (line[i] == '"')
        {
          int len = (int) strlen (line + i);
          memmove (line + i, line + i + 1, len);
          while (line[i] && line[i] != '"')
            i++;
          line[i] = 0;
        }
      }
      if (line[0] == '[')
        curid = makeID (line + 1);
      else if (curid && !strncmp (line, "Name=", 5))
        setEquiv (line + 5, curid);
    }
    MPQCloseFile (txt);
  }
}
