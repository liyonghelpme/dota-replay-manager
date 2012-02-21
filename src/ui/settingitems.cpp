#include "core/app.h"
#include "frameui/fontsys.h"
#include "graphics/imagelib.h"
#include "frameui/controlframes.h"
#include "base/utils.h"
#include "dota/misc.h"

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
  LOGFONT lf;
  memset (&lf, 0, sizeof lf);
  HDC hDC = GetDC(NULL);
  strcpy(lf.lfFaceName, "Georgia");
  lf.lfHeight = -MulDiv(10, GetDeviceCaps(hDC, LOGPIXELSY), 72);
  ReleaseDC(NULL, hDC);
  getApp()->getRegistry()->readBinary("chatFont", &lf, sizeof lf);

  curSlot = 0;
  curTime = 5000;
  hFont = FontSys::getFont(lf);
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
      SetBkColor(hDC, getApp()->getRegistry()->readInt("chatBg", 0x000000));
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

      uint32 bg = getApp()->getRegistry()->readInt("chatBg", 0x000000);
      uint32 fg = getApp()->getRegistry()->readInt("chatFg", 0xFFFFFF);

      RECT rc;
      GetClientRect(hWnd, &rc);

      SetBkColor(hDC, bg);
      SetTextColor(hDC, fg);
      SelectObject(hDC, hFont);

      String buf = format_time(curTime);
      DrawText(hDC, buf, buf.length(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      rc.left += 58;

      SIZE sz;
      buf = "[All] ";
      DrawText(hDC, buf, buf.length(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      GetTextExtentPoint32(hDC, buf, buf.length(), &sz);
      rc.left += sz.cx;

      int chatColors = getApp()->getRegistry()->readInt("chatColors");
      if (chatColors == 0)
        SetTextColor(hDC, getDefaultColor(curSlot));
      else if (chatColors == 1)
        SetTextColor(hDC, getSlotColor(curSlot));
      else
        SetTextColor(hDC, getDarkColor(curSlot));
      buf = "d07.RiV: ";
      DrawText(hDC, buf, buf.length(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      GetTextExtentPoint32(hDC, buf, buf.length(), &sz);
      rc.left += sz.cx;

      SetTextColor(hDC, fg);
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
  Registry* reg = getApp()->getRegistry();
  String warPath = "";
  getRegString(HKEY_CURRENT_USER, "Software\\Blizzard Entertainment\\Warcraft III",
    "InstallPath", warPath);
  warPath = reg->readString("warPath", warPath);

  String replayPath;
  if (warPath.isEmpty())
    replayPath = String::fixPath("");
  else
    replayPath = String::buildFullName(warPath, "Replay");
  replayPath = reg->readString("replayPath", replayPath);

  reg->writeString("warPath", warPath);
  reg->writeString("replayPath", replayPath);

  int tab = addTab("General");

  FontStringRegion* tip = tabs[tab]->createFontString("Warcraft III Folder:");
  WindowFrame* item1 = addStringItem(tab, "warPath");
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
  WindowFrame* item2 = addStringItem(tab, "replayPath");
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
  item1 = addIntItem(tab, "maxFiles", 2000);
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item1, PT_BOTTOMLEFT, -4, -7);

  item2 = addBoolItem(tab, "viewWindow", 1, 0);
  item2->setText("View replays in windowed mode");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);
  item2->setSize(228, 16);

  item1 = addBoolItem(tab, "autoView", 1, 1);
  item1->setText("Automatically detect new replays");
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(228, 16);

  item2 = addBoolItem(tab, "autoCopy", 1, 0);
  item2->setText("Automatically copy new replays");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);
  item2->setSize(228, 16);

  tip = tabs[tab]->createFontString("Copy to file:");
  item1 = addStringItem(tab, "copyFormat", "Autocopy\\replay<n>");
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(292, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item1, PT_BOTTOMLEFT, -4, -7);
  btn1 = new ButtonFrame("Help", tabs[tab], IDC_COPYFORMATHELP);
  btn1->setPoint(PT_BOTTOMLEFT, item1, PT_BOTTOMRIGHT, 4, 0);
  btn1->setSize(60, 23);

  item2 = addBoolItem(tab, "showDetails", 1, 0);
  item2->setText("Show details in folder view");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 15);
  item2->setSize(228, 16);

  ButtonFrame* group = new ButtonFrame("Select columns", tabs[tab], IDC_SEL_COL, BS_GROUPBOX);
  group->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, -50, 5);
  group->setSize(440, 90);

  item1 = addBoolItem(tab, "selColumns", COL_SAVED);
  item1->setText("Date saved");
  item1->setSize(80, 16);
  item1->setPoint(PT_TOPLEFT, group, PT_TOPLEFT, 10, 16);

  item2 = addBoolItem(tab, "selColumns", COL_SIZE);
  item2->setText("File size");
  item2->setSize(82, 16);
  item2->setPoint(PT_BOTTOMLEFT, item1, PT_BOTTOMRIGHT, 4, 0);

  tip = tabs[tab]->createFontString("Warning: the following options require parsing every file"
    " and may slow down viewing.");
  tip->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 2);

  item1 = addBoolItem(tab, "selColumns", COL_NAME);
  item1->setText("Game name");
  item1->setSize(80, 16);
  item1->setPoint(PT_TOPLEFT, tip, PT_BOTTOMLEFT, 0, 2);

  item2 = addBoolItem(tab, "selColumns", COL_RATIO);
  item2->setText("Game ratio");
  item2->setSize(82, 16);
  item2->setPoint(PT_BOTTOMLEFT, item1, PT_BOTTOMRIGHT, 4, 0);

  item1 = addBoolItem(tab, "selColumns", COL_LENGTH);
  item1->setText("Game length");
  item1->setSize(80, 16);
  item1->setPoint(PT_BOTTOMLEFT, item2, PT_BOTTOMRIGHT, 4, 0);

  item2 = addBoolItem(tab, "selColumns", COL_MODE);
  item2->setText("Game mode");
  item2->setSize(82, 16);
  item2->setPoint(PT_BOTTOMLEFT, item1, PT_BOTTOMRIGHT, 4, 0);

  item1 = addBoolItem(tab, "saveCache", 1, 1);
  item1->setText("Save game cache (speeds up parsing file info)");
  item1->setSize(300, 16);
  item1->setPoint(PT_TOPLEFT, tip, PT_BOTTOMLEFT, 0, 22);

  item1 = addBoolItem(tab, "useTray", 1, 1);
  item1->setText("Minimize to system tray");
  item1->setSize(300, 16);
  item1->setPoint(PT_TOPLEFT, group, PT_BOTTOMLEFT, 50, 12);

  item2 = addBoolItem(tab, "enableUrl", 1, 1);
  item2->setText("Enable URL in path bar (make sure to include http://)");
  item2->setSize(300, 16);
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);

  item1 = addBoolItem(tab, "autoUpdate", 1, 1);
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
  item1 = addStringItem(tab, "ownNames");
  item1->setPoint(PT_TOPLEFT, 240, 7);
  item1->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item1, PT_BOTTOMLEFT, -4, -7);

  tip = tabs[tab]->createFontString("Repeated action delay for skills (ms):");
  item2 = addIntItem(tab, "repDelay", 3000);
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);
  item2->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item2, PT_BOTTOMLEFT, -4, -7);

  tip = tabs[tab]->createFontString("Repeated action delay for items (ms):");
  item1 = addIntItem(tab, "repDelayItems", 1000);
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item1, PT_BOTTOMLEFT, -4, -7);

  group = new ButtonFrame("Timeline mode", tabs[tab], IDC_TIMELINE, BS_GROUPBOX);
  group->setPoint(PT_TOP, item1, PT_BOTTOM, 0, 10);
  group->setPoint(PT_LEFT, 50, 0);
  group->setSize(430, 152);

  item2 = addBoolItem(tab, "drawWards", 1, 1);
  item2->setText("Draw wards");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 22);
  item2->setSize(228, 16);

  tip = tabs[tab]->createFontString("Ward lifetime (seconds):");
  item1 = addIntItem(tab, "wardLife", 360);
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item1, PT_BOTTOMLEFT, -4, -7);

  item2 = addBoolItem(tab, "drawChat", 1, 1);
  item2->setText("Draw chat");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);
  item2->setSize(228, 16);

  tip = tabs[tab]->createFontString("Chat fade time (seconds):");
  item1 = addIntItem(tab, "chatStaysOn", 16);
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item1, PT_BOTTOMLEFT, -4, -7);

  item2 = addBoolItem(tab, "drawPings", 1, 1);
  item2->setText("Draw minimap pings");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);
  item2->setSize(228, 16);

  item1 = addBoolItem(tab, "drawBuildings", 1, 1);
  item1->setText("Draw buildings");
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(228, 16);

  item1 = addBoolItem(tab, "showLevels", 1, 1);
  item1->setText("Show skill levels in build view");
  item1->setPoint(PT_TOPLEFT, group, PT_BOTTOMLEFT, 10, 10);
  item1->setSize(180, 16);
  WindowFrame* item3 = item1;

  item2 = addBoolItem(tab, "skillColors", 1, 1);
  item2->setText("Color skills in build view");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 4);
  item2->setSize(180, 16);

  item1 = addBoolItem(tab, "smoothGold", 1, 1);
  item1->setText("Smoother gold timeline");
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 4);
  item1->setSize(180, 16);

  item2 = addBoolItem(tab, "relTime", 1, 1);
  item2->setText("Time relative to creep spawn");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 4);
  item2->setSize(180, 16);

  item1 = addBoolItem(tab, "showEmptySlots", 1, 1);
  item1->setText("Show empty slots");
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 4);
  item1->setSize(180, 16);

  item2 = addBoolItem(tab, "showAssemble", 1, 1);
  item2->setText("Show assembled items in itembuild");
  item2->setPoint(PT_BOTTOMLEFT, item3, PT_BOTTOMRIGHT, 0, 0);
  item2->setSize(180, 16);

  item3 = item1;

  item1 = addBoolItem(tab, "syncSelect", 1, 1);
  item1->setText("Synchronize selection in build view");
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 4);
  item1->setSize(180, 16);

  item2 = addBoolItem(tab, "chatHeroes", 1, 1);
  item2->setText("Show hero names in chat");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 4);
  item2->setSize(180, 16);

  item1 = addBoolItem(tab, "chatAssists", 1, 1);
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
  chatColorMode->setCurSel(reg->readInt("chatColors", 1));

  chatColors = new ClickColor(tabs[tab], IDC_CHATCOLOR);
  chatColors->setPoint(PT_LEFT, 60, 0);
  chatColors->setPoint(PT_TOP, btn1, PT_BOTTOM, 0, 5);
  chatColors->setSize(408, 50);

  //FontStringRegion* text = tabs[0]->createFontString("Hello World!");
  //text->setPoint(PT_TOPLEFT, 10, 10);

  //Image* texture = getApp()->getImageLibrary()->getImage("dota_map");
  //TextureRegion* txt = tabs[1]->createTexture(texture);
  //txt->setAllPoints();
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
        reg->writeString("warPath", warPath);
        updateKey("warPath");
      }
      break;
    case IDC_BROWSEWARPATH:
      if (code == BN_CLICKED)
      {
        String warPath;
        if (browseForFolder("Select Warcraft III folder", warPath))
        {
          reg->writeString ("warPath", warPath);
          updateKey("warPath");
        }
      }
      break;
    case IDC_RESETREPLAYPATH:
      if (code == BN_CLICKED)
      {
        String warPath = reg->readString("warPath");
        String replayPath = "";
        if (warPath.isEmpty())
          replayPath = String::fixPath("");
        else
          replayPath = String::buildFullName(warPath, "Replay");
        reg->writeString("replayPath", replayPath);
        updateKey("replayPath");

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
          reg->writeString ("replayPath", replayPath);
          updateKey("replayPath");

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
        String warPath = reg->readString("warPath");
        if (IsDlgButtonChecked(hWnd, IDC_OPENWITHTHIS))
          cmd = getAppPath(true) + " \"%1\"";
        else if (!warPath.isEmpty())
          cmd.printf("\"%s\" -loadfile \"%%1\"", String::buildFullName(warPath, "War3.exe"));
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
            if (warPath.isEmpty())
              warIcon = String::buildFullName(warPath, "replays.ico");
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
        reg->readBinary("chatFont", &lf, sizeof lf);
        CHOOSEFONT cf;
        memset(&cf, 0, sizeof cf);
        cf.lStructSize = sizeof cf;
        cf.hwndOwner = hWnd;
        cf.lpLogFont = &lf;
        cf.Flags = CF_EFFECTS | CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL | CF_SCREENFONTS;
        cf.rgbColors = reg->readInt("chatFg");
        if (ChooseFont(&cf))
        {
          reg->writeBinary("chatFont", &lf, sizeof lf);
          reg->writeInt("chatFg", cf.rgbColors);
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

        uint32 color = reg->readInt("chatBg");
        CHOOSECOLOR cc;
        memset(&cc, 0, sizeof cc);
        cc.lStructSize = sizeof cc;
        cc.hwndOwner = hWnd;
        cc.rgbResult = reg->readInt("chatBg");
        cc.Flags = CC_FULLOPEN | CC_RGBINIT;
        cc.lpCustColors = custColors;
        if (ChooseColor(&cc))
        {
          reg->writeInt("chatBg", cc.rgbResult);
          InvalidateRect(chatColors->getHandle(), NULL, TRUE);
        }
      }
      break;
    case IDC_CHATCOLORMODE:
      if (code == CBN_SELCHANGE)
      {
        reg->writeInt("chatColors", chatColorMode->getCurSel());
        InvalidateRect(chatColors->getHandle(), NULL, TRUE);
      }
      break;
    }
  }
  return 0;
}
