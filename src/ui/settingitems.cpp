#include "core/app.h"
#include "frameui/fontsys.h"
#include "graphics/imagelib.h"
#include "frameui/controlframes.h"
#include "base/utils.h"
#include "dota/colors.h"

#include "settingswnd.h"

#define IDC_RESETWARPATH        100
#define IDC_BROWSEWARPATH       101
#define IDC_RESETREPLAYPATH     102
#define IDC_BROWSEREPLAYPATH    103
#define IDC_APPLYREPLAYPATH     104
#define IDC_COPYFORMATHELP      105
#define IDC_SEL_COL             106
#define IDC_OPENWITHTHIS        107
#define IDC_TIMELINE            108
#define IDC_CHATFONT            109
#define IDC_CHATCOLOR           110
#define IDC_CHATCOLORMODE       111

#define COL_SAVED       0x01
#define COL_SIZE        0x02
#define COL_NAME        0x04
#define COL_RATIO       0x10
#define COL_LENGTH      0x20
#define COL_MODE        0x40

class ClickColor : public WindowFrame
{
  int curSlot;
  int curTime;
  HFONT hFont;
  uint32 onMessage(uint32 message, uint32 wParam, uint32 lParam);
public:
  ClickColor(Frame* parent, int id = 0);
};
ClickColor::ClickColor(Frame* parent, int id)
  : WindowFrame(parent)
{
  curSlot = 0;
  curTime = 5000;
  hFont = FontSys::getFont(cfg::chatFont.get<LOGFONT>());
  create("", WS_CHILD, WS_EX_CLIENTEDGE);
  setId(id);
  SetTimer(hWnd, 2345, 1500, NULL);
}
uint32 ClickColor::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  switch (message)
  {
  case WM_ERASEBKGND:
    {
      HDC hDC = (HDC) wParam;
      RECT rc;
      GetClientRect(hWnd, &rc);
      SetBkColor(hDC, cfg::chatBg);
      ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    }
    break;
  case WM_LBUTTONDOWN:
    SetCapture(hWnd);
    break;
  case WM_LBUTTONUP:
    if (GetCapture() == hWnd)
    {
      ReleaseCapture();
      HWND hParent = GetParent(hWnd);
      if (hParent)
        SendMessage(hParent, WM_COMMAND, MAKELONG(id(), BN_CLICKED), (uint32) hWnd);
    }
    break;
  case WM_SETFONT:
    hFont = (wParam ? (HFONT) wParam : FontSys::getSysFont());
    InvalidateRect(hWnd, NULL, TRUE);
    break;
  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hDC = BeginPaint(hWnd, &ps);

      RECT rc;
      GetClientRect(hWnd, &rc);

      SetBkColor(hDC, cfg::chatBg);
      SetTextColor(hDC, cfg::chatFg);
      SelectObject(hDC, hFont);

      String buf = format_time(curTime);
      DrawText(hDC, buf, buf.length(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      rc.left += 58;

      SIZE sz;
      buf = "[All] ";
      DrawText(hDC, buf, buf.length(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      GetTextExtentPoint32(hDC, buf, buf.length(), &sz);
      rc.left += sz.cx;

      if (cfg::chatColors == 0)
        SetTextColor(hDC, getDefaultColor(curSlot));
      else if (cfg::chatColors == 1)
        SetTextColor(hDC, getSlotColor(curSlot));
      else
        SetTextColor(hDC, getDarkColor(curSlot));
      buf = "d07.RiV: ";
      DrawText(hDC, buf, buf.length(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      GetTextExtentPoint32(hDC, buf, buf.length(), &sz);
      rc.left += sz.cx;

      SetTextColor(hDC, cfg::chatFg);
      buf = "click to change background";
      DrawText(hDC, buf, buf.length(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      GetTextExtentPoint32(hDC, buf, buf.length(), &sz);
      rc.left += sz.cx;

      EndPaint(hWnd, &ps);
    }
    break;
  case WM_TIMER:
    curSlot = (curSlot + 5) % 12;
    curTime = rand();
    InvalidateRect(hWnd, NULL, TRUE);
    break;
  }
  return WindowFrame::onMessage(message, wParam, lParam);
}

void SettingsWindow::addAllItems()
{
  int tab = addTab("General");

  FontStringRegion* tip = tabs[tab]->createFontString("Warcraft III Folder:");
  WindowFrame* item1 = addStringItem(tab, &cfg::warPath);
  item1->setPoint(PT_TOPLEFT, 140, 7);
  item1->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item1, PT_BOTTOMLEFT, -4, -7);
  ButtonFrame* btn1 = new ButtonFrame("Reset", tabs[tab], IDC_RESETWARPATH);
  btn1->setPoint(PT_BOTTOMLEFT, item1, PT_BOTTOMRIGHT, 4, 0);
  btn1->setSize(60, 23);
  ButtonFrame* btn2 = new ButtonFrame("Browse...", tabs[tab], IDC_BROWSEWARPATH);
  btn2->setPoint(PT_BOTTOMLEFT, btn1, PT_BOTTOMRIGHT, 4, 0);
  btn2->setSize(60, 23);

  tip = tabs[tab]->createFontString("Replay folder:");
  WindowFrame* item2 = addStringItem(tab, &cfg::replayPath);
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);
  item2->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item2, PT_BOTTOMLEFT, -4, -7);
  btn1 = new ButtonFrame("Reset", tabs[tab], IDC_RESETREPLAYPATH);
  btn1->setPoint(PT_BOTTOMLEFT, item2, PT_BOTTOMRIGHT, 4, 0);
  btn1->setSize(60, 23);
  btn2 = new ButtonFrame("Browse...", tabs[tab], IDC_BROWSEREPLAYPATH);
  btn2->setPoint(PT_BOTTOMLEFT, btn1, PT_BOTTOMRIGHT, 4, 0);
  btn2->setSize(60, 23);
  btn1 = new ButtonFrame("Apply", tabs[tab], IDC_APPLYREPLAYPATH);
  btn1->setPoint(PT_BOTTOMLEFT, btn2, PT_BOTTOMRIGHT, 4, 0);
  btn1->setSize(60, 23);

  tip = tabs[tab]->createFontString("Maximum number of files:");
  item1 = addIntItem(tab, &cfg::maxFiles);
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item1, PT_BOTTOMLEFT, -4, -7);

  item2 = addBoolItem(tab, &cfg::viewWindow);
  item2->setText("View replays in windowed mode");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);
  item2->setSize(228, 16);

  item1 = addBoolItem(tab, &cfg::autoView);
  item1->setText("Automatically detect new replays");
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(228, 16);

  item2 = addBoolItem(tab, &cfg::autoCopy);
  item2->setText("Automatically copy new replays");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);
  item2->setSize(228, 16);

  tip = tabs[tab]->createFontString("Copy to file:");
  item1 = addStringItem(tab, &cfg::copyFormat);
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(292, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item1, PT_BOTTOMLEFT, -4, -7);
  btn1 = new ButtonFrame("Help", tabs[tab], IDC_COPYFORMATHELP);
  btn1->setPoint(PT_BOTTOMLEFT, item1, PT_BOTTOMRIGHT, 4, 0);
  btn1->setSize(60, 23);

  item2 = addBoolItem(tab, &cfg::showDetails);
  item2->setText("Show details in folder view");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 15);
  item2->setSize(228, 16);

  ButtonFrame* group = new ButtonFrame("Select columns", tabs[tab], IDC_SEL_COL, BS_GROUPBOX);
  group->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, -50, 5);
  group->setSize(440, 90);

  item1 = addBoolItem(tab, &cfg::selColumns, COL_SAVED);
  item1->setText("Date saved");
  item1->setSize(80, 16);
  item1->setPoint(PT_TOPLEFT, group, PT_TOPLEFT, 10, 16);

  item2 = addBoolItem(tab, &cfg::selColumns, COL_SIZE);
  item2->setText("File size");
  item2->setSize(82, 16);
  item2->setPoint(PT_BOTTOMLEFT, item1, PT_BOTTOMRIGHT, 4, 0);

  tip = tabs[tab]->createFontString("Warning: the following options require parsing every file"
    " and may slow down viewing.");
  tip->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 2);

  item1 = addBoolItem(tab, &cfg::selColumns, COL_NAME);
  item1->setText("Game name");
  item1->setSize(80, 16);
  item1->setPoint(PT_TOPLEFT, tip, PT_BOTTOMLEFT, 0, 2);

  item2 = addBoolItem(tab, &cfg::selColumns, COL_RATIO);
  item2->setText("Game ratio");
  item2->setSize(82, 16);
  item2->setPoint(PT_BOTTOMLEFT, item1, PT_BOTTOMRIGHT, 4, 0);

  item1 = addBoolItem(tab, &cfg::selColumns, COL_LENGTH);
  item1->setText("Game length");
  item1->setSize(80, 16);
  item1->setPoint(PT_BOTTOMLEFT, item2, PT_BOTTOMRIGHT, 4, 0);

  item2 = addBoolItem(tab, &cfg::selColumns, COL_MODE);
  item2->setText("Game mode");
  item2->setSize(82, 16);
  item2->setPoint(PT_BOTTOMLEFT, item1, PT_BOTTOMRIGHT, 4, 0);

  item1 = addBoolItem(tab, &cfg::saveCache);
  item1->setText("Save game cache (speeds up parsing file info)");
  item1->setSize(300, 16);
  item1->setPoint(PT_TOPLEFT, tip, PT_BOTTOMLEFT, 0, 22);

  item1 = addBoolItem(tab, &cfg::useTray);
  item1->setText("Minimize to system tray");
  item1->setSize(300, 16);
  item1->setPoint(PT_TOPLEFT, group, PT_BOTTOMLEFT, 50, 12);

  item2 = addBoolItem(tab, &cfg::enableUrl);
  item2->setText("Enable URL in path bar (make sure to include http://)");
  item2->setSize(300, 16);
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);

  item1 = addBoolItem(tab, &cfg::autoUpdate);
  item1->setText("Check for updates automatically (once a day)");
  item1->setSize(300, 16);
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);

  btn1 = new ButtonFrame("Set this program as default for opening replays",
    tabs[tab], IDC_OPENWITHTHIS, BS_AUTOCHECKBOX);
  btn1->setSize(300, 16);
  btn1->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);
  String cmd;
  if (getRegString(HKEY_CLASSES_ROOT, "Warcraft3.Replay\\shell\\open\\command", "", cmd) &&
      cmd == getAppPath(true) + " \"%1\"")
    btn1->setCheck(true);
  else
    btn1->setCheck(false);

  //////////////////////////////////////////////////////////////

  tab = addTab("Replay");

  tip = tabs[tab]->createFontString("Your name(s) in replays:");
  item1 = addStringItem(tab, &cfg::ownNames);
  item1->setPoint(PT_TOPLEFT, 240, 7);
  item1->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item1, PT_BOTTOMLEFT, -4, -7);

  tip = tabs[tab]->createFontString("Repeated action delay for skills (ms):");
  item2 = addIntItem(tab, &cfg::repDelay);
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);
  item2->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item2, PT_BOTTOMLEFT, -4, -7);

  tip = tabs[tab]->createFontString("Repeated action delay for items (ms):");
  item1 = addIntItem(tab, &cfg::repDelayItems);
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item1, PT_BOTTOMLEFT, -4, -7);

  group = new ButtonFrame("Timeline mode", tabs[tab], IDC_TIMELINE, BS_GROUPBOX);
  group->setPoint(PT_TOP, item1, PT_BOTTOM, 0, 10);
  group->setPoint(PT_LEFT, 50, 0);
  group->setSize(430, 152);

  item2 = addBoolItem(tab, &cfg::drawWards);
  item2->setText("Draw wards");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 22);
  item2->setSize(228, 16);

  tip = tabs[tab]->createFontString("Ward lifetime (seconds):");
  item1 = addIntItem(tab, &cfg::wardLife);
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item1, PT_BOTTOMLEFT, -4, -7);

  item2 = addBoolItem(tab, &cfg::drawChat);
  item2->setText("Draw chat");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);
  item2->setSize(228, 16);

  tip = tabs[tab]->createFontString("Chat fade time (seconds):");
  item1 = addIntItem(tab, &cfg::chatStaysOn);
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item1, PT_BOTTOMLEFT, -4, -7);

  item2 = addBoolItem(tab, &cfg::drawPings);
  item2->setText("Draw minimap pings");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);
  item2->setSize(228, 16);

  item1 = addBoolItem(tab, &cfg::drawBuildings);
  item1->setText("Draw buildings");
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(228, 16);

  item1 = addBoolItem(tab, &cfg::showLevels);
  item1->setText("Show skill levels in build view");
  item1->setPoint(PT_TOPLEFT, group, PT_BOTTOMLEFT, 10, 10);
  item1->setSize(180, 16);
  WindowFrame* item3 = item1;

  item2 = addBoolItem(tab, &cfg::skillColors);
  item2->setText("Color skills in build view");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 4);
  item2->setSize(180, 16);

  item1 = addBoolItem(tab, &cfg::smoothGold);
  item1->setText("Smoother gold timeline");
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 4);
  item1->setSize(180, 16);

  item2 = addBoolItem(tab, &cfg::relTime);
  item2->setText("Time relative to creep spawn");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 4);
  item2->setSize(180, 16);

  item1 = addBoolItem(tab, &cfg::showEmptySlots);
  item1->setText("Show empty slots");
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 4);
  item1->setSize(180, 16);

  item2 = addBoolItem(tab, &cfg::showAssemble);
  item2->setText("Show assembled items in itembuild");
  item2->setPoint(PT_BOTTOMLEFT, item3, PT_BOTTOMRIGHT, 0, 0);
  item2->setSize(180, 16);

  item3 = item1;

  item1 = addBoolItem(tab, &cfg::syncSelect);
  item1->setText("Synchronize selection in build view");
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 4);
  item1->setSize(180, 16);

  item2 = addBoolItem(tab, &cfg::chatHeroes);
  item2->setText("Show hero names in chat");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 4);
  item2->setSize(180, 16);

  item1 = addBoolItem(tab, &cfg::chatAssists);
  item1->setText("Show assists in chat");
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 4);
  item1->setSize(180, 16);

  tip = tabs[tab]->createFontString("Chat font:");
  tip->setPoint(PT_TOPLEFT, item3, PT_BOTTOMLEFT, 0, 10);
  btn1 = new ButtonFrame("Font", tabs[tab], IDC_CHATFONT);
  btn1->setPoint(PT_BOTTOMLEFT, tip, PT_BOTTOMRIGHT, 5, 6);
  btn1->setSize(60, 23);

  FontStringRegion* tip2 = tabs[tab]->createFontString("Player colors:");
  tip2->setPoint(PT_BOTTOMLEFT, tip, PT_BOTTOMLEFT, 180, 0);
  chatColorMode = new ComboFrame(tabs[tab], IDC_CHATCOLORMODE);
  chatColorMode->setPoint(PT_BOTTOMLEFT, tip2, PT_BOTTOMRIGHT, 5, 5);
  chatColorMode->setPoint(PT_RIGHT, NULL, PT_LEFT, 468, 0);
  chatColorMode->addString("Default");
  chatColorMode->addString("Adapted");
  chatColorMode->addString("Dark");
  chatColorMode->setCurSel(cfg::chatColors);

  chatColors = new ClickColor(tabs[tab], IDC_CHATCOLOR);
  chatColors->setPoint(PT_LEFT, 60, 0);
  chatColors->setPoint(PT_TOP, btn1, PT_BOTTOM, 0, 5);
  chatColors->setSize(408, 50);

  item1 = addBoolItem(tab, &cfg::useLog);
  item1->setText("Log actions to log.txt");
  item1->setPoint(PT_TOPLEFT, chatColors, PT_BOTTOMLEFT, 0, 8);
  item1->setSize(180, 16);
}

uint32 SettingsWindow::handleExtra(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_COMMAND && lParam)
  {
    int id = LOWORD(wParam);
    int code = HIWORD(wParam);
    Registry* reg = getApp()->getRegistry();
    switch (id)
    {
    case IDC_RESETWARPATH:
      if (code == BN_CLICKED)
      {
        String warPath = "";
        getRegString(HKEY_CURRENT_USER, "Software\\Blizzard Entertainment\\Warcraft III",
          "InstallPath", warPath);
        cfg::warPath = warPath;
        updateKey(&cfg::warPath);
      }
      break;
    case IDC_BROWSEWARPATH:
      if (code == BN_CLICKED)
      {
        String warPath;
        if (browseForFolder("Select Warcraft III folder", warPath))
        {
          cfg::warPath = warPath;
          updateKey(&cfg::warPath);
        }
      }
      break;
    case IDC_RESETREPLAYPATH:
      if (code == BN_CLICKED)
      {
        if (String(cfg::warPath).isEmpty())
          cfg::replayPath = String::fixPath("");
        else
          cfg::replayPath = String::buildFullName(cfg::warPath, "Replay");
        updateKey(&cfg::replayPath);

        HWND hParent = GetParent(hWnd);
        if (hParent)
          SendMessage(hParent, WM_UPDATEPATH, 0, 0);
      }
      break;
    case IDC_BROWSEREPLAYPATH:
      if (code == BN_CLICKED)
      {
        String replayPath;
        if (browseForFolder("Select replay folder", replayPath))
        {
          cfg::replayPath = replayPath;
          updateKey(&cfg::replayPath);

          HWND hParent = GetParent(hWnd);
          if (hParent)
            SendMessage(hParent, WM_UPDATEPATH, 0, 0);
        }
      }
      break;
    case IDC_APPLYREPLAYPATH:
      if (code == BN_CLICKED)
      {
        HWND hParent = GetParent(hWnd);
        if (hParent)
          SendMessage(hParent, WM_UPDATEPATH, 0, 0);
      }
      break;
    case IDC_COPYFORMATHELP:
      if (code == BN_CLICKED)
      {
      }
      break;
    case IDC_OPENWITHTHIS:
      if (code == BN_CLICKED)
      {
        String cmd = "";
        if (IsDlgButtonChecked(hWnd, IDC_OPENWITHTHIS))
          cmd = getAppPath(true) + " \"%1\"";
        else if (!String(cfg::warPath).isEmpty())
          cmd.printf("\"%s\" -loadfile \"%%1\"", String::buildFullName(cfg::warPath, "War3.exe"));
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_CLASSES_ROOT, "Warcraft3.Replay\\shell\\open\\command",
          0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
        {
          RegSetValueEx(hKey, "", 0, REG_SZ, (LPBYTE) cmd.c_str(), cmd.length() + 1);
          RegCloseKey(hKey);
        }
        else if (RegCreateKeyEx(HKEY_CLASSES_ROOT, ".w3g", 0, NULL, REG_OPTION_NON_VOLATILE,
          KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS)
        {
          RegSetValueEx(hKey, "", 0, REG_SZ, (LPBYTE) "Warcraft3.Replay", strlen("Warcraft3.Replay") + 1);
          RegCloseKey(hKey);
          if (RegCreateKeyEx(HKEY_CLASSES_ROOT, "Warcraft3.Replay\\shell\\open\\command",
            0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS)
          {
            RegSetValueEx(hKey, "", 0, REG_SZ, (LPBYTE) cmd.c_str(), cmd.length() + 1);
            RegCloseKey(hKey);
          }
          if (RegCreateKeyEx(HKEY_CLASSES_ROOT, "Warcraft3.Replay\\DefaultIcon",
            0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS)
          {
            String warIcon;
            if (String(cfg::warPath).isEmpty())
              warIcon = String::buildFullName(cfg::warPath, "replays.ico");
            else
              warIcon = getAppPath(true) + ",2";
            RegSetValueEx(hKey, "", 0, REG_SZ, (LPBYTE) warIcon.c_str(), warIcon.length() + 1);
            RegCloseKey(hKey);
          }
        }
      }
      break;
    case IDC_CHATFONT:
      if (code == BN_CLICKED)
      {
        LOGFONT lf;
        memcpy(&lf, cfg::chatFont.data(), sizeof lf);
        CHOOSEFONT cf;
        memset(&cf, 0, sizeof cf);
        cf.lStructSize = sizeof cf;
        cf.hwndOwner = hWnd;
        cf.lpLogFont = &lf;
        cf.Flags = CF_EFFECTS | CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL | CF_SCREENFONTS;
        cf.rgbColors = cfg::chatFg;
        if (ChooseFont(&cf))
        {
          cfg::chatFont.set((uint8*) &lf, sizeof lf);
          cfg::chatFg = cf.rgbColors;
          chatColors->setFont(FontSys::getFont(lf));
        }
      }
      break;
    case IDC_CHATCOLOR:
      if (code == BN_CLICKED)
      {
        static COLORREF custColors[16];
        for (int i = 0; i < 16; i++)
          custColors[i] = 0xFFFFFF;

        CHOOSECOLOR cc;
        memset(&cc, 0, sizeof cc);
        cc.lStructSize = sizeof cc;
        cc.hwndOwner = hWnd;
        cc.rgbResult = cfg::chatBg;
        cc.Flags = CC_FULLOPEN | CC_RGBINIT;
        cc.lpCustColors = custColors;
        if (ChooseColor(&cc))
        {
          cfg::chatBg = cc.rgbResult;
          InvalidateRect(chatColors->getHandle(), NULL, TRUE);
        }
      }
      break;
    case IDC_CHATCOLORMODE:
      if (code == CBN_SELCHANGE)
      {
        cfg::chatColors = chatColorMode->getCurSel();
        InvalidateRect(chatColors->getHandle(), NULL, TRUE);
      }
      break;
    }
  }
  return 0;
}
