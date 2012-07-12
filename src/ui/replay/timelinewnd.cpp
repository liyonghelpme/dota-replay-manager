#include "core/app.h"
#include "timelinewnd.h"
#include "resource.h"
#include "graphics/glib.h"
#include "graphics/imagelib.h"
#include "dota/colors.h"

///////////////////// TimePicture ////////////////////////

static float KX;
static float DX;
static float KY;
static float DY;
static const float MX0 = -7500;
static const float MY0 = -7900;
static const float MX1 = 7400;
static const float MY1 = 7300;
static float rfix = 487.0 / 512.0;

TimePicture::TimePicture(Frame* parent)
  : WindowFrame(parent)
{
  KX = 1 / (MX1 - MX0);
  DX = -MX0 * KX;
  KY = 1 / (MY0 - MY1);
  DY = -MY1 * KY;

  time = 0;
  gl = NULL;
  w3g = NULL;
  if (WNDCLASSEX* wcx = createclass("TimeLineWnd"))
  {
    wcx->style |= CS_OWNDC;
    RegisterClassEx(wcx);
  }
  create("", WS_CHILD, WS_EX_CLIENTEDGE);

  cursor = LoadCursor(getApp()->getInstanceHandle(), MAKEINTRESOURCE(IDC_ELFHAND));
}
TimePicture::~TimePicture()
{
  delete gl;
}

int TimePicture::mapx(float x)
{
  if (width() < height())
    return int((x * KX + DX) * float(width()) + 0.5);
  else
    return int(float(width() - height()) / 2 + (x * KX + DX) * float(height()) + 0.5);
}
int TimePicture::mapy(float y)
{
  if (height() < width())
    return int((y * KY + DY) * float(height()) + 0.5);
  else
    return int(float(height() - width()) / 2 + (y * KY + DY) * float(width()) + 0.5);
}
float TimePicture::unmapx(int x)
{
  if (width() < height())
    return (float(x) / float(width()) - DX) / KX;
  else
    return ((float(x) - float(width() - height()) / 2) / float(height()) - DX) / KX;
}
float TimePicture::unmapy(int y)
{
  if (height() < width())
    return (float(y) / float(height()) - DY) / KY;
  else
    return ((float(y) - float(height() - width()) / 2) / float(width()) - DY) / KY;
}

void TimePicture::setReplay(W3GReplay* replay)
{
  w3g = replay;
  time = 0;
  InvalidateRect(hWnd, NULL, FALSE);
}
void TimePicture::setTime(uint32 t)
{
  time = t;
  InvalidateRect(hWnd, NULL, FALSE);
}

uint32 TimePicture::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  switch (message)
  {
  case WM_POSTCREATE:
    gl = new OpenGL(hWnd);
    if (!gl->isOk())
      MessageBox(hWnd, "Error initializing OpenGL!", "Error", MB_OK | MB_ICONERROR);
    else
      tex = gl->genTexture(getApp()->getImageLibrary()->getImage("dota_map"));
    break;
  case WM_SIZE:
    gl->onSize(width(), height());
    break;
  case WM_MOUSEMOVE:
    InvalidateRect(hWnd, NULL, FALSE);
    break;
  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hDC = BeginPaint(hWnd, &ps);
      paint();
      EndPaint(hWnd, &ps);
    }
    return 0;
  }
  return WindowFrame::onMessage(message, wParam, lParam);
}

String TimePicture::formatPlayer(W3GPlayer* player)
{
  if (player->hero && cfg::chatHeroes)
    return String::format("%s (%s)", player->name, player->hero->hero->shortName);
  else
    return player->name;
}
void TimePicture::drawNotify(int alpha, int x, int y, String text)
{
  String buf = "";
  for (int i = 0; i < text.length(); i++)
  {
    if (text[i] == '@')
    {
      i++;
      if (text[i] == 'm')
      {
        switch (text[i + 1])
        {
        case '3':
          buf += "is on a ";
          break;
        case '4':
        case '6':
        case '7':
        case '9':
        case '0':
          buf += "is ";
          break;
        case '5':
        case '8':
          buf += "has a ";
          break;
        }
      }
      if (text[i] == 'k')
      {
        switch (text[i + 1])
        {
        case '2':
        case '3':
        case '4':
          buf += "just got a ";
          break;
        case '5':
          buf += "is on a ";
          break;
        }
      }
      if (buf.length())
      {
        gl->color(0xFFFFFF, alpha);
        gl->text(x, y, buf, ALIGN_Y_BOTTOM);
        x += gl->getTextWidth(buf);
      }
      buf = "";
      switch (text[i])
      {
      case 's':
        gl->color(getSlotColor(0), alpha);
        buf = "The Sentinel";
        break;
      case 'u':
        gl->color(getSlotColor(6), alpha);
        buf = "The Scourge";
        break;
      case 'n':
        gl->color(getSlotColor(12), alpha);
        buf = "Neutral Creeps";
        break;
      case 'r':
        switch (text[i + 1])
        {
        case '1':
          gl->color(getExtraColor(15), alpha);
          buf = "Haste";
          break;
        case '2':
          gl->color(getExtraColor(16), alpha);
          buf = "Regeneration";
          break;
        case '3':
          gl->color(getExtraColor(17), alpha);
          buf = "Double Damage";
          break;
        case '4':
          gl->color(getExtraColor(18), alpha);
          buf = "Illusion";
          break;
        case '5':
          gl->color(getExtraColor(19), alpha);
          buf = "Invisibility";
          break;
        }
        break;
      case 'm':
        switch (text[i + 1])
        {
        case '3':
          gl->color(getExtraColor(20), alpha);
          buf = "killing spree";
          break;
        case '4':
          gl->color(getExtraColor(21), alpha);
          buf = "dominating";
          break;
        case '5':
          gl->color(getExtraColor(22), alpha);
          buf = "mega kill";
          break;
        case '6':
          gl->color(getExtraColor(23), alpha);
          buf = "unstoppable";
          break;
        case '7':
          gl->color(getExtraColor(24), alpha);
          buf = "wicked sick";
          break;
        case '8':
          gl->color(getExtraColor(25), alpha);
          buf = "monster kill";
          break;
        case '9':
          gl->color(getExtraColor(26), alpha);
          buf = "GODLIKE";
          break;
        case '0':
          gl->color(getExtraColor(27), alpha);
          buf = "beyond GODLIKE";
          break;
        }
        break;
      case 'k':
        switch (text[i + 1])
        {
        case '2':
          gl->color(getExtraColor(28), alpha);
          buf = "Double Kill";
          break;
        case '3':
          gl->color(getExtraColor(29), alpha);
          buf = "Triple Kill";
          break;
        case '4':
          gl->color(getExtraColor(30), alpha);
          buf = "Ultra Kill";
          break;
        case '5':
          gl->color(getExtraColor(31), alpha);
          buf = "Rampage";
          break;
        }
        break;
      default:
        {
          int id = 0;
          int pos = i;
          while (text[pos] >= '0' && text[pos] <= '9')
            id = id * 10 + int(text[pos++] - '0');
          int color = w3g->getPlayerById(id)->slot.color;
          if (text[pos] == '|')
          {
            color = 0;
            pos++;
            while (text[pos] >= '0' && text[pos] <= '9')
              color = color * 10 + int(text[pos++] - '0');
          }
          gl->color(getSlotColor(color), alpha);
          buf = formatPlayer(w3g->getPlayerById(id));
        }
        break;
      }
      if (buf.length())
      {
        gl->text(x, y, buf, ALIGN_Y_BOTTOM);
        x += gl->getTextWidth(buf);
      }
      buf = "";
      if (text[i] == 'm')
      {
        switch (text[i + 1])
        {
        case '3':
        case '4':
        case '5':
          buf = "!";
          break;
        case '6':
        case '7':
        case '8':
          buf = "!!";
          break;
        case '9':
          buf = "!!!";
          break;
        case '0':
          buf = ". Someone KILL HIM!!!!!!";
          break;
        }
      }
      if (text[i] == 'k')
      {
        switch(text[i + 1])
        {
        case '2':
          buf = "!";
          break;
        case '3':
        case '4':
        case '5':
          buf = "!!!";
          break;
        }
      }
      while (text[i] != '@')
        i++;
    }
    else
      buf += text[i];
  }
  if (buf.length())
  {
    gl->color(0xFFFFFF, alpha);
    gl->text(x, y, buf, ALIGN_Y_BOTTOM);
  }
}

void TimePicture::paint()
{
  POINT cursor;
  GetCursorPos(&cursor);
  ScreenToClient(hWnd, &cursor);

  if (gl->isOk())
  {
    gl->begin();

    if (w3g && w3g->getDotaInfo())
    {
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, tex);
      glBegin(GL_QUADS);
      glTexCoord2f(0, (1 - rfix) / 2);
      glVertex2i(mapx(MX0) + 5, mapy(MY1) + 5);
      glTexCoord2f(1, (1 - rfix) / 2);
      glVertex2i(mapx(MX1) - 5, mapy(MY1) + 5);
      glTexCoord2f(1, (1 + rfix) / 2);
      glVertex2i(mapx(MX1) - 5, mapy(MY0) - 5);
      glTexCoord2f(0, (1 + rfix) / 2);
      glVertex2i(mapx(MX0) + 5, mapy(MY0) - 5);
      glEnd();
      glBindTexture(GL_TEXTURE_2D, 0);
      glDisable(GL_TEXTURE_2D);

      DotaInfo const* dota = w3g->getDotaInfo();
      ImageLibrary* imgLib = getApp()->getImageLibrary();

      if (cfg::drawBuildings)
      {
        DotaBuilding* bld = getBuildings();
        for (int i = NUM_BUILDINGS - 1; i >= 0; i--)
        {
          if (dota->bdTime[i] == 0 || dota->bdTime[i] > time)
            gl->image(mapx(bld[i].x), mapy(bld[i].y), imgLib->getImage(bld[i].icon),
              ALIGN_X_CENTER | ALIGN_Y_CENTER);
        }
      }
      if (cfg::drawWards)
      {
        Image* img = imgLib->getImage("ward");
        for (int cur = w3g->getFirstWard(time); cur >= 0; cur--)
        {
          W3GWard& ward = w3g->getWard(cur);
          if (ward.time + 1000 * cfg::wardLife <= time)
            break;
          int x = mapx(ward.x);
          int y = mapy(ward.y);
          gl->color(getSlotColor(ward.player->slot.color), 0xFF);
          gl->image(x, y, NULL, 0, 0, 12, 12, ALIGN_X_CENTER | ALIGN_Y_CENTER);
          gl->image(x, y, img, 1, 1, 10, 10, ALIGN_X_CENTER | ALIGN_Y_CENTER);
        }
      }
      W3GPlayer* selPlayer = NULL;
      for (int i = 0; i < w3g->getNumPlayers(); i++)
      {
        W3GPlayer* player = w3g->getPlayer(i);
        float x, y;
        if (player->hero && player->actions.getPosition(time, x, y) == STATE_ALIVE)
        {
          int ix = mapx(x);
          int iy = mapy(y);
          gl->color(getSlotColor(player->slot.color), 0xFF);
          gl->image(ix, iy, NULL, 0, 0, 16, 16, ALIGN_X_CENTER | ALIGN_Y_CENTER);
          gl->image(ix, iy, imgLib->getImage(player->hero->hero->icon),
            1, 1, 14, 14, ALIGN_X_CENTER | ALIGN_Y_CENTER);

          if (cursor.x > ix - 8 && cursor.x < ix + 8 &&
              cursor.y > iy - 8 && cursor.y < iy + 8)
            selPlayer = player;
        }
      }
      if (selPlayer)
      {
        float x, y, dx, dy;
        selPlayer->actions.getPosition(time, x, y, &dx, &dy);
        int ix = mapx(x);
        int iy = mapy(y);
        gl->color(getSlotColor(selPlayer->slot.color), 0xFF);
        gl->line(ix, iy, mapx(dx), mapy(dy));

        if (ix + 88 > width()) ix -= 96;
        if (iy + 110 > height()) iy -= 102;
        gl->color(0xFF000000);
        gl->fillrect(ix + 8, iy - 8, ix + 88, iy + 110);
        gl->color(getSlotColor(selPlayer->slot.color), 0xFF);
        gl->rect(ix + 8, iy - 8, ix + 88, iy + 110);

        glEnable(GL_SCISSOR_TEST);
        glScissor(ix + 8, height() - (iy + 110), 80, 118);
        gl->text(ix + 12, iy - 4, selPlayer->name);
        gl->text(ix + 12, iy + 12, selPlayer->hero->hero->shortName);
        gl->text(ix + 12, iy + 28, String(selPlayer->hero->level));
        for (int i = 0; i < 6; i++)
          gl->image((i % 2) * 16 + ix + 12,
                    (i / 2) * 16 + iy + 44, imgLib->getImage(
                    selPlayer->inv.final[i] ? selPlayer->inv.final[i]->icon : "emptyslot"));
        for (int i = 0; i < 5; i++)
        {
          int px = ix + 52;
          int py = i * 16 + iy + 28;
          Dota::Ability* abil = selPlayer->hero->hero->abilities[i];
          if (abil)
          {
            gl->image(px, py, imgLib->getImage(abil->icon));
            gl->text(px + 18, py, String(4));
          }
        }
        glDisable(GL_SCISSOR_TEST);
      }
    }

    gl->end();
  }
}

//////////////////////////////////////////

ReplayTimelineTab::ReplayTimelineTab(FrameWindow* parent)
  : ReplayTab(parent)
{
  picture = new TimePicture(this);
  picture->setPoint(PT_TOPLEFT, 10, 10);
  picture->setPoint(PT_BOTTOMRIGHT, -10, -10);
}
ReplayTimelineTab::~ReplayTimelineTab()
{
}

void ReplayTimelineTab::onSetReplay()
{
  picture->setReplay(w3g);
}
