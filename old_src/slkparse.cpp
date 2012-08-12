#include "stdafx.h"
#include "rmpq.h"
#include "dota.h"
#include "utils.h"

struct SLKEntry
{
  char type;
  char val[256];
};
char* SLKReadEntry (char* line, SLKEntry& e)
{
  e.type = *line++;
  int len = 0;
  if (*line == '"')
  {
    line++;
    while (*line != '"' && *line)
      e.val[len++] = *line++;
    while (*line != ';' && *line)
      line++;
  }
  else
  {
    while (*line != ';' && *line)
      e.val[len++] = *line++;
  }
  e.val[len] = 0;
  if (*line == ';')
    line++;
  return line;
}

void parseSLK (MPQLOADER mpq, char const* path)
{
  MPQFILE slk = MPQLoadFile (mpq, path);
  if (slk)
  {
    zero_abils ();
    int curid = 0;
    char abils[256] = "";
    int y = 0;
    int x = 0;
    int xID = 0;
    int xAbil = 0;
    char lineb[256];
    while (MPQFileGets (slk, sizeof lineb, lineb))
    {
      char* line = lineb;
      while (*line == ' ')
        line++;
      int i = 0;
      while (line[i]) i++;
      while (i && (line[i - 1] == ' ' || line[i - 1] == '\r' || line[i - 1] == '\t' || line[i - 1] == '\n'))
        line[--i] = 0;
      SLKEntry s;
      line = SLKReadEntry (line, s);
      if (s.type == 'C')
      {
        line = SLKReadEntry (line, s);
        while (s.type == 'X' || s.type == 'Y')
        {
          if (s.type == 'X')
            x = atoi (s.val);
          else
          {
            y = atoi (s.val);
            curid = 0;
            abils[0] = 0;
          }
          line = SLKReadEntry (line, s);
        }
        if (x && s.type == 'K')
        {
          if (y == 1)
          {
            if (!strcmp (s.val, "unitAbilID"))
              xID = x;
            else if (!strcmp (s.val, "heroAbilList"))
              xAbil = x;
          }
          else if (x == xID)
          {
            curid = makeID (s.val);
            if (abils[0])
            {
              set_abils (curid, abils);
              curid = 0;
              abils[0] = 0;
            }
          }
          else if (x == xAbil)
          {
            strcpy (abils, s.val);
            if (curid)
            {
              set_abils (curid, abils);
              curid = 0;
              abils[0] = 0;
            }
          }
        }
      }
    }
    MPQCloseFile (slk);
  }
}
