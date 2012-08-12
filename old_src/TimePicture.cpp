// TimePicture.cpp : implementation file
//

#include "stdafx.h"
#include "DotAReplay.h"
#include "TimePicture.h"

#include "replay.h"
#include "ilib.h"
#include <math.h>

// CTimePicture

static double KX;
static double DX;
static double KY;
static double DY;
static const double MX0 = -7500;
static const double MY0 = -7900;
static const double MX1 = 7400;
static const double MY1 = 7300;
static double rfix = 487.0 / 512.0;

IMPLEMENT_DYNAMIC(CTimePicture, CWnd)
CTimePicture::CTimePicture (CRect const& rc, CWnd* parent)
{
  time = 0;
  gl = NULL;
  w3g = NULL;
  CreateEx (WS_EX_CLIENTEDGE, NULL, "", WS_CHILD, rc, parent, IDC_TIMEPIC);
  ShowWindow (SW_SHOW);

  cursor = LoadCursor (::AfxGetInstanceHandle (), MAKEINTRESOURCE (IDC_ELFHAND));

  // MX0 * KX + DX = 0
  // MX1 * KX + DX = 1
  KX = 1 / (MX1 - MX0);
  DX = -MX0 * KX;
  // MY0 * KY + DY = 1
  // MY1 * KY + DY = 0
  KY = 1 / (MY0 - MY1);
  DY = -MY1 * KY;
}

CTimePicture::~CTimePicture()
{
}


BEGIN_MESSAGE_MAP(CTimePicture, CWnd)
  ON_WM_CREATE()
  ON_WM_PAINT()
  ON_WM_DESTROY()
  ON_WM_MOUSEMOVE()
  ON_WM_SETCURSOR()
  ON_WM_SIZE()
END_MESSAGE_MAP()



// CTimePicture message handlers

int CTimePicture::OnCreate (LPCREATESTRUCT lpCreateStruct)
{
  if (CWnd::OnCreate (lpCreateStruct) == -1)
    return -1;

  gl = new OpenGL (m_hWnd);
  if (!gl->isOk ())
    MessageBox ("Error initializing OpenGL!", "Error", MB_OK | MB_ICONERROR);

  gl->begin ();
  GLImage map ("dota_map");
  glGenTextures (1, &tex);
  glEnable (GL_TEXTURE_2D);
  glBindTexture (GL_TEXTURE_2D, tex);
  glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei (GL_PACK_ALIGNMENT, 1);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexImage2D (GL_TEXTURE_2D, 0, 3, map.width, map.height, 0, GL_RGB, GL_UNSIGNED_BYTE, map.bits);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glBindTexture (GL_TEXTURE_2D, 0);
  glDisable (GL_TEXTURE_2D);
  CRect rc;
  GetClientRect (rc);
  imgw = rc.right;
  imgh = rc.bottom;
  wglMakeCurrent (NULL, NULL);

  return 0;
}
void CTimePicture::OnSize (UINT nType, int cx, int cy)
{
  if (cx > 0 && cy > 0)
  {
    imgw = cx;
    imgh = cy;
    if (gl)
      gl->onSize (cx, cy);
  }
}

int CTimePicture::mapX (float x)
{
  return int ((x * KX + DX) * double (imgw) + 0.5);
}
int CTimePicture::mapY (float y)
{
  return int ((y * KY + DY) * double (imgh) + 0.5);
}

int CTimePicture::unmapX (int x)
{
  return int ((double (x) / double (imgw) - DX) / KX);
}
int CTimePicture::unmapY (int y)
{
  return int ((double (y) / double (imgh) - DY) / KY);
}

extern bool drawChat;
extern bool drawPings;
extern int chatStaysOn;
extern bool drawWards;
extern int wardLife;
extern bool chatHeroes;
extern bool drawBuildings;

wchar_t const* fmt_pname (W3GPlayer* p)
{
  if (!chatHeroes)
    return p->uname;
  DotaHero* hero = NULL;
  if (p->hero)
    hero = getHero (p->hero->id);
  if (!hero)
    return p->uname;
  static wchar_t buf[256];
  wcscpy (buf, p->uname);
  int len = (int) wcslen (buf);
  buf[len++] = ' ';
  buf[len++] = '(';
  for (int i = 0; hero->abbr[i]; i++)
    buf[len++] = hero->abbr[i];
  buf[len++] = ')';
  buf[len] = 0;
  return buf;
}
void drawNotify (OpenGL* gl, W3GReplay* w3g, double alpha, int x, int y, wchar_t const* text)
{
  static wchar_t buf[1024];
  int len = 0;
  for (int i = 0; text[i];)
  {
    if (text[i] == '@')
    {
      if (text[i + 1] == 'm' || text[i + 1] == 'k')
      {
        char const* aptr = "";
        if (text[i + 1] == 'm')
        {
          char d = text[i + 2];
          if (d == '3')
            aptr = "is on a ";
          else if (d == '4')
            aptr = "is ";
          else if (d == '5')
            aptr = "has a ";
          else if (d == '6')
            aptr = "is ";
          else if (d == '7')
            aptr = "is ";
          else if (d == '8')
            aptr = "has a ";
          else if (d == '9')
            aptr = "is ";
          else if (d == '0')
            aptr = "is ";
        }
        else
        {
          char d = text[i + 2];
          if (d >= '2' && d <= '4')
            aptr = "just got a ";
          else if (d == '5')
            aptr = "is on a ";
        }
        for (int p = 0; aptr[p]; p++)
          buf[len++] = aptr[p];
      }
      if (len)
      {
        glColor4d (1, 1, 1, alpha);
        buf[len] = 0;
        gl->drawText (x, y, buf, ALIGN_Y_BOTTOM);
        x += gl->getTextWidth (buf);
        len = 0;
      }
      if (text[i + 1] == 's' || text[i + 1] == 'u' || text[i + 1] == 'n')
      {
        i++;
        if (text[i] == 's')
        {
          glColor (Color (getSlotColor (0), alpha));
          gl->drawText (x, y, "The Sentinel", ALIGN_Y_BOTTOM);
          x += gl->getTextWidth ("The Sentinel");
        }
        else if (text[i] == 'u')
        {
          glColor (Color (getSlotColor (6), alpha));
          gl->drawText (x, y, "The Scourge", ALIGN_Y_BOTTOM);
          x += gl->getTextWidth ("The Scourge");
        }
        else if (text[i] == 'n')
        {
          glColor (Color (getSlotColor (12), alpha));
          gl->drawText (x, y, "Neutral Creeps", ALIGN_Y_BOTTOM);
          x += gl->getTextWidth ("Neutral Creeps");
        }
        i += 2;
      }
      else if (text[i + 1] == 'r')
      {
        i += 2;
        if (text[i] == '1')
        {
          glColor (Color (getExtraColor (15), alpha));
          gl->drawText (x, y, "Haste", ALIGN_Y_BOTTOM);
          x += gl->getTextWidth ("Haste");
        }
        else if (text[i] == '2')
        {
          glColor (Color (getExtraColor (16), alpha));
          gl->drawText (x, y, "Regeneration", ALIGN_Y_BOTTOM);
          x += gl->getTextWidth ("Regeneration");
        }
        else if (text[i] == '3')
        {
          glColor (Color (getExtraColor (17), alpha));
          gl->drawText (x, y, "Double Damage", ALIGN_Y_BOTTOM);
          x += gl->getTextWidth ("Double Damage");
        }
        else if (text[i] == '4')
        {
          glColor (Color (getExtraColor (18), alpha));
          gl->drawText (x, y, "Illusion", ALIGN_Y_BOTTOM);
          x += gl->getTextWidth ("Illusion");
        }
        else if (text[i] == '5')
        {
          glColor (Color (getExtraColor (19), alpha));
          gl->drawText (x, y, "Invisibility", ALIGN_Y_BOTTOM);
          x += gl->getTextWidth ("Invisibility");
        }
        i += 2;
      }
      else if (text[i + 1] == 'm')
      {
        i += 2;
        int streak = text[i] - '0';
        i += 2;
        if (streak == 0)
          streak = 10;
        int clr = 17 + streak;
        char const* prnt = "";
        if (streak == 3)
          prnt = "killing spree";
        else if (streak == 4)
          prnt = "dominating";
        else if (streak == 5)
          prnt = "mega kill";
        else if (streak == 6)
          prnt = "unstoppable";
        else if (streak == 7)
          prnt = "wicked sick";
        else if (streak == 8)
          prnt = "monster kill";
        else if (streak == 9)
          prnt = "GODLIKE";
        else if (streak == 10)
          prnt = "beyond GODLIKE";
        if (streak <= 5)
          wcscpy (buf, L"!");
        else if (streak <= 8)
          wcscpy (buf, L"!!");
        else if (streak == 9)
          wcscpy (buf, L"!!!");
        else if (streak == 10)
          wcscpy (buf, L". Someone KILL HIM!!!!!!");
        len = (int) wcslen (buf);

        glColor (Color (getExtraColor (clr), alpha));
        gl->drawText (x, y, prnt, ALIGN_Y_BOTTOM);
        x += gl->getTextWidth (prnt);
      }
      else if (text[i + 1] == 'k')
      {
        i += 2;
        int streak = text[i] - '0';
        i += 2;
        int clr = 26 + streak;
        char const* prnt = "";
        if (streak == 2)
          prnt = "Double Kill";
        else if (streak == 3)
          prnt = "Triple Kill";
        else if (streak == 4)
          prnt = "Ultra Kill";
        else if (streak == 5)
          prnt = "Rampage";
        if (streak == 2)
          wcscpy (buf, L"!");
        else
          wcscpy (buf, L"!!!");
        len = (int) wcslen (buf);

        glColor (Color (getExtraColor (clr), alpha));
        gl->drawText (x, y, prnt, ALIGN_Y_BOTTOM);
        x += gl->getTextWidth (prnt);
      }
      else
      {
        int id = 0;
        for (i++; text[i] != '@' && text[i] != '|'; i++)
          id = id * 10 + text[i] - '0';
        int clr = (id >= 0 && id <= 255 && w3g->players[id].name[0])
          ? w3g->players[id].slot.color
          : 0;
        if (text[i] == '|')
        {
          clr = 0;
          for (i++; text[i] != '@'; i++)
            clr = clr * 10 + text[i] - '0';
        }
        for (i++; text[i] && text[i] != ' ' && text[i] != '\t' && text[i] != '/'; i++)
          buf[len++] = text[i];
        if (clr)
        {
          DotaHero* hero = NULL;
          if (w3g->players[id].hero)
            hero = getHero (w3g->players[id].hero->id);
          if (chatHeroes && hero)
          {
            buf[len++] = ' ';
            buf[len++] = '(';
            for (int i = 0; hero->abbr[i]; i++)
              buf[len++] = hero->abbr[i];
            buf[len++] = ')';
          }

          glColor (Color (getSlotColor (clr), alpha));
          buf[len] = 0;
          gl->drawText (x, y, buf, ALIGN_Y_BOTTOM);
          x += gl->getTextWidth (buf);
          len = 0;
        }
      }
    }
    else
      buf[len++] = text[i++];
  }
  if (len)
  {
    glColor4d (1, 1, 1, alpha);
    buf[len] = 0;
    gl->drawText (x, y, buf, ALIGN_Y_BOTTOM);
  }
}
void CTimePicture::OnPaint ()
{
  CPoint mouse;
  GetCursorPos (&mouse);
  ScreenToClient (&mouse);
  CPaintDC dc (this);
  if (gl->isOk ())
  {
    gl->begin ();

    if (w3g && w3g->dota.isDota)
    {
      glEnable (GL_TEXTURE_2D);
      glBindTexture (GL_TEXTURE_2D, tex);
      glBegin (GL_QUADS);
      glTexCoord2f (0, (1 + rfix) / 2);
      glVertex2i (0, 0);
      glTexCoord2f (1, (1 + rfix) / 2);
      glVertex2i (imgw, 0);
      glTexCoord2f (1, (1 - rfix) / 2);
      glVertex2i (imgw, imgh);
      glTexCoord2f (0, (1 - rfix) / 2);
      glVertex2i (0, imgh);
      glEnd ();
      glBindTexture (GL_TEXTURE_2D, 0);
      glDisable (GL_TEXTURE_2D);
//      gl->drawRect (0, 0, GLImage ("dota_map"));
      int selPlayer = -1;
      if (drawBuildings)
      {
        DotaBuilding* bld = getBuildings ();
        for (int i = NUM_BUILDINGS - 1; i >= 0; i--)
        {
          if (w3g->dota.bdTime[i] == 0 || w3g->dota.bdTime[i] > time)
          {
            int ix = mapX (bld[i].x);
            int iy = mapY (bld[i].y);
            glColor (getSlotColor (i >= BUILDINGS_SCOURGE ? 6 : 0));
            gl->drawRect (ix, iy, GLImage (bld[i].icon), ALIGN_X_CENTER | ALIGN_Y_CENTER);
          }
        }
      }
      if (drawWards)
      {
        for (int cur = w3g->getWardPos (time); cur >= 0 && w3g->wards[cur].time + wardLife * 1000 > time; cur--)
        {
          int ix = mapX (w3g->wards[cur].x);
          int iy = mapY (w3g->wards[cur].y);
          glColor (getSlotColor (w3g->players[w3g->wards[cur].id].slot.color));
          gl->drawRect (ix, iy, GLImage (12, 12), ALIGN_X_CENTER | ALIGN_Y_CENTER);
          gl->drawRect (ix, iy, GLImage ("ward"), 1, 1, 10, 10, ALIGN_X_CENTER | ALIGN_Y_CENTER);
        }
      }
      for (int i = 0; i < w3g->numPlayers; i++)
      {
        int id = w3g->pindex[i];
        float x, y, dx, dy;
        if (w3g->players[id].hero && w3g->getPlayerPos (id, time, x, y, dx, dy))
        {
          int ix = mapX (x);
          int iy = mapY (y);
          glColor (getSlotColor (w3g->players[id].slot.color));
          gl->drawRect (ix, iy, GLImage (16, 16), ALIGN_X_CENTER | ALIGN_Y_CENTER);
          gl->drawRect (ix, iy, GLImage (getHero (w3g->players[id].hero->id)->imgTag),
            1, 1, 14, 14, ALIGN_X_CENTER | ALIGN_Y_CENTER);

          if (mouse.x > ix - 8 && mouse.x < ix + 8 &&
              mouse.y > iy - 8 && mouse.y < iy + 8)
            selPlayer = id;
        }
      }
      if (selPlayer >= 0)
      {
        int id = selPlayer;
        float x, y, dx, dy;
        if (w3g->players[id].hero && w3g->getPlayerPos (id, time, x, y, dx, dy))
        {
          int ix = mapX (x);
          int iy = mapY (y);
          glColor (getSlotColor (w3g->players[id].slot.color));
          glLine (ix, iy, mapX (dx), mapY (dy));
          if (w3g->players[id].hero->compute (time, w3g->dota) &&
              w3g->players[id].inv.compute (time, w3g->dota))
          {
            if (ix + 88 > gl->wd)
              ix -= 96;
            if (iy + 92 > gl->ht)
              iy -= 116;
            glColor (Color (0, 0, 0));
            glFillRect (ix + 8, iy - 8, ix + 88, iy + 108);
            glColor (getSlotColor (w3g->players[id].slot.color));
            gl->drawText (ix + 12, iy - 4, w3g->players[id].uname);
            gl->drawText (ix + 12, iy + 12, getHero (w3g->players[id].hero->id)->abbr);
            gl->drawText (ix + 12, iy + 28, mprintf ("%d", w3g->players[id].hero->cur_lvl));
            for (int i = 0; i < 6; i++)
            {
              int px = (i % 2) * 16 + ix + 12;
              int py = (i / 2) * 16 + iy + 44;
              gl->drawRect (px, py, GLImage (getItemIcon (w3g->players[id].inv.inv[i])));
            }
            for (int i = 0; i < 5; i++)
            {
              int px = ix + 52;
              int py = i * 16 + iy + 28;
              DotaAbility* abil = getHeroAbility (w3g->players[id].hero->id, i);
              if (abil)
              {
                gl->drawRect (px, py, GLImage (getHeroAbility (
                  w3g->players[id].hero->id, i)->imgTag));
                gl->drawText (px + 18, py, mprintf ("%d", w3g->players[id].hero->levels[i]));
              }
            }
          }
        }
      }
      if (drawChat || drawPings)
      {
        int pos = gl->ht - 5;
        int line = w3g->getChatPos (time);
        int cnt = 0;
        int delay = chatStaysOn * 1000;
        if (delay < 2000) delay = 2000;
        while (line >= 0 && int (time - w3g->chat[line].time) < delay)
        {
          if (drawChat && cnt < 10 && int (time - w3g->chat[line].time) < chatStaysOn * 1000)
          {
            wchar_t buf[1024];
            double alpha = 1;
            if (int (time - w3g->chat[line].time) > chatStaysOn * 750)
              alpha = 4 - double (time - w3g->chat[line].time) / double (chatStaysOn) / 250;
            bool isChat = true;
            switch (w3g->chat[line].mode)
            {
            case CHAT_ALL:
              glColor (Color (getSlotColor (w3g->players[w3g->chat[line].id].slot.color), alpha));
              swprintf (buf, L"[All] %s: ", fmt_pname (&w3g->players[w3g->chat[line].id]));
              break;
            case CHAT_ALLIES:
              glColor (Color (getSlotColor (w3g->players[w3g->chat[line].id].slot.color), alpha));
              swprintf (buf, L"[Allies] %s: ", fmt_pname (&w3g->players[w3g->chat[line].id]));
              break;
            case CHAT_OBSERVERS:
              glColor (Color (getSlotColor (w3g->players[w3g->chat[line].id].slot.color), alpha));
              swprintf (buf, L"[Observers] %s: ", fmt_pname (&w3g->players[w3g->chat[line].id]));
              break;
            case CHAT_PRIVATE:
              glColor (Color (getSlotColor (w3g->players[w3g->chat[line].id].slot.color), alpha));
              swprintf (buf, L"[Private] %s: ", fmt_pname (&w3g->players[w3g->chat[line].id]));
              break;
            case CHAT_NOTIFY:
              buf[0] = 0;
              break;
            default:
              isChat = false;
            }
            if (isChat)
            {
              gl->drawText (5, pos, buf, ALIGN_Y_BOTTOM);
              glColor4d (1, 1, 1, alpha);
              if (w3g->chat[line].mode == CHAT_NOTIFY)
                drawNotify (gl, w3g, alpha, 5, pos, w3g->chat[line].utext);
              else
                gl->drawText (5 + gl->getTextWidth (buf), pos, w3g->chat[line].utext, ALIGN_Y_BOTTOM);
              wcscat (buf, w3g->chat[line].utext);
              pos -= gl->getTextHeight (buf);
              cnt++;
            }
          }
          if (drawPings && w3g->chat[line].mode == CHAT_PING && int (time - w3g->chat[line].time) < 2000)
          {
            double alpha = 1;
            if (int (time - w3g->chat[line].time) > 1000)
              alpha = 2 - double (time - w3g->chat[line].time) / 1000;
            glColor (Color (getSlotColor (w3g->players[w3g->chat[line].id].slot.color), alpha));
            double r = double ((time - w3g->chat[line].time) % 500) / 10;
            glBegin (GL_LINE_LOOP);
            int x0 = mapX (w3g->chat[line].x);
            int y0 = mapY (w3g->chat[line].y);
            for (int i = 0; i < 20; i++)
              glVertex2i (x0 + int (cos (3.14159265358979 * double (i) / 10) * r + 0.5),
                          y0 + int (sin (3.14159265358979 * double (i) / 10) * r + 0.5));
            glEnd ();
          }
          line--;
        }
      }
      if (0 && GetAsyncKeyState (VK_CONTROL))
      {
        int vx = unmapX (mouse.x);
        int vy = unmapY (mouse.y);
        glColor3d (1, 1, 1);
        gl->drawText (5, 5, mprintf ("X: %d, Y: %d", vx, vy), ALIGN_X_LEFT | ALIGN_Y_TOP);
      }
    }

    gl->end ();
  }
}

void CTimePicture::OnDestroy ()
{
  delete gl;
  gl = NULL;
}

void CTimePicture::setReplay (W3GReplay* replay)
{
  w3g = replay;
  time = 0;
  InvalidateRect (NULL, FALSE);
}

void CTimePicture::setTime (unsigned long theTime)
{
  time = theTime;
  InvalidateRect (NULL, FALSE);
}
void CTimePicture::OnMouseMove (UINT nFlags, CPoint point)
{
  InvalidateRect (NULL, FALSE);
}

BOOL CTimePicture::OnSetCursor (CWnd* pWnd, UINT nHitTest, UINT message)
{
  SetCursor (cursor);
  return TRUE;
}
