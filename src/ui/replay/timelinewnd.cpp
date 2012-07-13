#include "core/app.h"
#include "timelinewnd.h"
#include "resource.h"
#include "graphics/glib.h"
#include "graphics/imagelib.h"
#include "dota/colors.h"
#include <math.h>

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
        selPlayer->hero->compute(time);
        selPlayer->inv.compute(time, w3g->getDotaData());
        gl->text(ix + 12, iy - 4, selPlayer->name);
        gl->text(ix + 12, iy + 12, selPlayer->hero->hero->shortName);
        gl->text(ix + 12, iy + 28, String(selPlayer->hero->currentLevel));
        for (int i = 0; i < 6; i++)
          gl->image((i % 2) * 16 + ix + 12,
                    (i / 2) * 16 + iy + 44, imgLib->getImage(
                    selPlayer->inv.computed[i] ? selPlayer->inv.computed[i]->icon : "emptyslot"));
        for (int i = 0; i < 5; i++)
        {
          int px = ix + 52;
          int py = i * 16 + iy + 28;
          Dota::Ability* abil = selPlayer->hero->hero->abilities[i];
          if (abil)
          {
            gl->image(px, py, imgLib->getImage(abil->icon));
            gl->text(px + 18, py, String(selPlayer->hero->skillLevels[i]));
          }
        }
        glDisable(GL_SCISSOR_TEST);
      }
      if (cfg::drawChat || cfg::drawPings)
      {
        int pos = height() - 5;
        int count = 0;
        int delay = cfg::chatStaysOn * 1000;
        if (delay < 2000) delay = 2000;
        for (int line = w3g->getFirstMessage(time);
          line >= 0 && int(time - w3g->getMessage(line).time) < delay; line--)
        {
          W3GMessage& msg = w3g->getMessage(line);
          W3GPlayer* player = w3g->getPlayerById(msg.id);
          if (cfg::drawChat && int(time - msg.time) < cfg::chatStaysOn * 1000)
          {
            int alpha = 255;
            if (int(time - msg.time) > cfg::chatStaysOn * 750)
              alpha = int((4 - double(time - msg.time) / double(cfg::chatStaysOn) / 250) * 255.0);
            if (alpha < 0) alpha = 0;
            if (alpha > 255) alpha = 255;
            String text = "";
            bool isChat = true;
            switch (msg.mode)
            {
            case CHAT_ALL:
              gl->color(getSlotColor(player->slot.color), alpha);
              text.printf("[All] %s: ", formatPlayer(player));
              break;
            case CHAT_ALLIES:
              gl->color(getSlotColor(player->slot.color), alpha);
              text.printf("[Allies] %s: ", formatPlayer(player));
              break;
            case CHAT_OBSERVERS:
              gl->color(getSlotColor(player->slot.color), alpha);
              text.printf("[Observers] %s: ", formatPlayer(player));
              break;
            case CHAT_PRIVATE:
              gl->color(getSlotColor(player->slot.color), alpha);
              text.printf("[Private] %s: ", formatPlayer(player));
              break;
            case CHAT_NOTIFY:
              break;
            default:
              isChat = false;
            }
            if (isChat)
            {
              gl->text(5, pos, text, ALIGN_Y_BOTTOM);
              gl->color(0xFFFFFF, alpha);
              if (msg.mode == CHAT_NOTIFY)
                drawNotify(alpha, 5, pos, msg.text);
              else
                gl->text(5 + gl->getTextWidth(text), pos, msg.text, ALIGN_Y_BOTTOM);
              text += msg.text;
              pos -= gl->getTextHeight(text);
              count++;
            }
          }
          if (cfg::drawPings && msg.mode == CHAT_PING && int(time - msg.time) < 2000)
          {
            int alpha = 255;
            if (int(time - msg.time) > 1000)
              alpha = int((2 - double(time - msg.time) / 1000) * 255.0);
            if (alpha < 0) alpha = 0;
            if (alpha > 255) alpha = 255;
            gl->color(getSlotColor(player->slot.color), alpha);
            double r = double((time - msg.time) % 500) / 10;
            glBegin(GL_LINE_LOOP);
            int x0 = mapx(msg.x);
            int y0 = mapy(msg.y);
            for (int i = 0; i < 20; i++)
              glVertex2i(x0 + int(cos(3.14159265358979 * double(i) / 10) * r + 0.5),
                         y0 + int(sin(3.14159265358979 * double(i) / 10) * r + 0.5));
            glEnd();
          }
        }
      }
    }

    gl->end();
  }
}

//////////////////////////////////////////

#define ID_TIMER            102
#define ID_SPINNER          105
#define ID_PLAY             106
#define ID_SLIDER           107

ReplayTimelineTab::ReplayTimelineTab(FrameWindow* parent)
  : ReplayTab(parent)
{
  speedBox = new EditFrame(this, 0, ES_RIGHT | ES_READONLY);
  updownBox = new UpDownFrame(this, ID_SPINNER);
  playBox = new ButtonFrame("", this, ID_PLAY, BS_AUTOCHECKBOX | BS_PUSHLIKE | BS_ICON);
  slider = new SliderFrame(this, ID_SLIDER);
  timeBox = new EditFrame(this, 0, ES_RIGHT | ES_READONLY);
  picture = new TimePicture(this);

  speedBox->setPoint(PT_BOTTOMLEFT, 40, -20);
  speedBox->setSize(35, 23);
  updownBox->setPoint(PT_TOPLEFT, speedBox, PT_TOPRIGHT, -1, 0);
  updownBox->setSize(15, 23);
  playBox->setPoint(PT_LEFT, updownBox, PT_RIGHT, 8, 0);
  playBox->setSize(22, 21);
  timeBox->setPoint(PT_BOTTOMRIGHT, -40, -20);
  timeBox->setSize(70, 23);
  slider->setPoint(PT_LEFT, playBox, PT_RIGHT, 8, 0);
  slider->setPoint(PT_RIGHT, timeBox, PT_LEFT, -8, 0);
  slider->setHeight(39);

  picture->setPoint(PT_TOPLEFT, 40, 20);
  picture->setPoint(PT_TOPRIGHT, -40, 20);
  picture->setPoint(PT_BOTTOM, slider, PT_TOP, 0, -10);

  SendMessage(playBox->getHandle(), BM_SETIMAGE, IMAGE_ICON,
    (LPARAM) LoadIcon(getInstance(), MAKEINTRESOURCE(IDI_PLAY)));
  speedBox->setText("1x");
  speed = 1;
  lastTime = 0;
  lastUpdate = 0;

  SetTimer(hWnd, ID_TIMER, 100, NULL);
}
ReplayTimelineTab::~ReplayTimelineTab()
{
}

void ReplayTimelineTab::onSetReplay()
{
  speed = 1;
  speedBox->setText("1x");
  playBox->setCheck(false);
  timeBox->setText("");
  picture->setReplay(w3g);
  updownBox->enable(w3g != NULL);
  playBox->enable(w3g != NULL);
  slider->enable(w3g != NULL);
  if (w3g)
  {
    slider->setLineSize(60000);
    slider->setPageSize(600000);
    slider->setPos(0);
    uint32 gameTime = w3g->getLength(false);
    slider->setRange(0, gameTime);
    if (gameTime < 20000) slider->setTicFreq(1000);
    else if (gameTime < 40000) slider->setTicFreq(2000);
    else if (gameTime < 100000) slider->setTicFreq(5000);
    else if (gameTime < 200000) slider->setTicFreq(10000);
    else if (gameTime < 600000) slider->setTicFreq(30000);
    else if (gameTime < 1200000) slider->setTicFreq(60000);
    else if (gameTime < 2400000) slider->setTicFreq(120000);
    else if (gameTime < 6000000) slider->setTicFreq(300000);
    else if (gameTime < 12000000) slider->setTicFreq(600000);
    else slider->setTicFreq(900000);
  }
}

void ReplayTimelineTab::onMove()
{
  if (!visible())
    playBox->setCheck(false);
}
void ReplayTimelineTab::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_TIMER && wParam == ID_TIMER)
  {
    if (w3g)
    {
      int time = slider->getPos();
      if (playBox->checked())
      {
        time += int(GetTickCount() - lastUpdate) * speed;
        if (time < 0)
          time = 0;
        else if (time > (int) w3g->getLength(false))
          time = w3g->getLength(false);
        lastUpdate = GetTickCount();
        slider->setPos(time);
      }
      if (time != lastTime)
      {
        picture->setTime(time);
        lastTime = time;
      }
      if (playBox->checked() || (GetTickCount() % 1400) > 400)
        timeBox->setText(w3g->formatTime(time, TIME_HOURS | TIME_SECONDS));
      else
        timeBox->setText("");
    }
  }
  else if (message == WM_COMMAND && HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == ID_PLAY)
  {
    lastUpdate = GetTickCount();
  }
  else if (message == WM_NOTIFY)
  {
    NMUPDOWN* nmud = (NMUPDOWN*) lParam;
    if (nmud->hdr.code == UDN_DELTAPOS)
    {
      speed -= nmud->iDelta;
      if (speed == 0)
        speed -= nmud->iDelta;
      if (speed < -99) speed = -99;
      if (speed > 99) speed = 99;
      speedBox->setText(String::format("%dx", speed));
      SetFocus(hWnd);
    }
  }
}
