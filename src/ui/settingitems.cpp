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
  hFont = FontSys::getFont(cfg.chatFont);
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
      SetBkColor(hDC, cfg.chatBg);
      ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    }
    return TRUE;
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

      SetBkColor(hDC, cfg.chatBg);
      SetTextColor(hDC, cfg.chatFg);
      SelectObject(hDC, hFont);

      String buf = format_time(curTime);
      DrawText(hDC, buf, buf.length(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      rc.left += 58;

      SIZE sz;
      buf = "[All] ";
      DrawText(hDC, buf, buf.length(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      GetTextExtentPoint32(hDC, buf, buf.length(), &sz);
      rc.left += sz.cx;

      if (cfg.chatColors == 0)
        SetTextColor(hDC, getDefaultColor(curSlot));
      else if (cfg.chatColors == 1)
        SetTextColor(hDC, getSlotColor(curSlot));
      else
        SetTextColor(hDC, getDarkColor(curSlot));
      buf = "d07.RiV: ";
      DrawText(hDC, buf, buf.length(), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      GetTextExtentPoint32(hDC, buf, buf.length(), &sz);
      rc.left += sz.cx;

      SetTextColor(hDC, cfg.chatFg);
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
  default:
    return M_UNHANDLED;
  }
  return 0;
}

void SettingsWindow::addAllItems()
{
  Frame* tab = tabs->addTab("General");

  Frame* tip = new StaticFrame("Warcraft III Folder:", tab);
  WindowFrame* item1 = addStringItem(0, &cfg.warPath);
  item1->setPoint(PT_TOPLEFT, 140, 7);
  item1->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item1, PT_BOTTOMLEFT, -4, -7);
  ButtonFrame* btn1 = new ButtonFrame("Reset", tab, IDC_RESETWARPATH);
  btn1->setPoint(PT_BOTTOMLEFT, item1, PT_BOTTOMRIGHT, 4, 0);
  btn1->setSize(60, 23);
  ButtonFrame* btn2 = new ButtonFrame("Browse...", tab, IDC_BROWSEWARPATH);
  btn2->setPoint(PT_BOTTOMLEFT, btn1, PT_BOTTOMRIGHT, 4, 0);
  btn2->setSize(60, 23);

  tip = new StaticFrame("Replay folder:", tab);
  WindowFrame* item2 = addStringItem(0, &cfg.replayPath);
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);
  item2->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item2, PT_BOTTOMLEFT, -4, -7);
  btn1 = new ButtonFrame("Reset", tab, IDC_RESETREPLAYPATH);
  btn1->setPoint(PT_BOTTOMLEFT, item2, PT_BOTTOMRIGHT, 4, 0);
  btn1->setSize(60, 23);
  btn2 = new ButtonFrame("Browse...", tab, IDC_BROWSEREPLAYPATH);
  btn2->setPoint(PT_BOTTOMLEFT, btn1, PT_BOTTOMRIGHT, 4, 0);
  btn2->setSize(60, 23);
  btn1 = new ButtonFrame("Apply", tab, IDC_APPLYREPLAYPATH);
  btn1->setPoint(PT_BOTTOMLEFT, btn2, PT_BOTTOMRIGHT, 4, 0);
  btn1->setSize(60, 23);

  item1 = addBoolItem(0, &cfg.hideEmpty);
  item1->setText("Hide empty folders");
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(228, 16);

  item2 = addBoolItem(0, &cfg.viewWindow);
  item2->setText("View replays in windowed mode");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);
  item2->setSize(228, 16);

  item1 = addBoolItem(0, &cfg.autoView);
  item1->setText("Automatically open new replays");
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(228, 16);

  item2 = addBoolItem(0, &cfg.autoCopy);
  item2->setText("Automatically copy new replays");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);
  item2->setSize(228, 16);

  tip = new StaticFrame("Copy to file:", tab);
  item1 = addStringItem(0, &cfg.copyFormat);
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(292, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item1, PT_BOTTOMLEFT, -4, -7);
  btn1 = new ButtonFrame("Help", tab, IDC_COPYFORMATHELP);
  btn1->setPoint(PT_BOTTOMLEFT, item1, PT_BOTTOMRIGHT, 4, 0);
  btn1->setSize(60, 23);

  //item2 = addBoolItem(0, &cfg.showDetails);
  //item2->setText("Show details in folder view");
  //item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 15);
  //item2->setSize(228, 16);

  ButtonFrame* group = new ButtonFrame("Select columns", tab, IDC_SEL_COL, BS_GROUPBOX);
  group->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, -50, 10);
  group->setSize(440, 70);

  item1 = addBoolItem(0, &cfg.selColumns, COL_SAVED);
  item1->setText("Date saved");
  item1->setSize(80, 16);
  item1->setPoint(PT_TOPLEFT, group, PT_TOPLEFT, 10, 16);

  item2 = addBoolItem(0, &cfg.selColumns, COL_SIZE);
  item2->setText("File size");
  item2->setSize(82, 16);
  item2->setPoint(PT_BOTTOMLEFT, item1, PT_BOTTOMRIGHT, 4, 0);

  tip = new StaticFrame("Warning: the following options require parsing every file"
    " and may slow down viewing.", tab);
  tip->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 2);

  item1 = addBoolItem(0, &cfg.selColumns, COL_NAME);
  item1->setText("Game name");
  item1->setSize(80, 16);
  item1->setPoint(PT_TOPLEFT, tip, PT_BOTTOMLEFT, 0, 2);

  item2 = addBoolItem(0, &cfg.selColumns, COL_RATIO);
  item2->setText("Lineup");
  item2->setSize(82, 16);
  item2->setPoint(PT_BOTTOMLEFT, item1, PT_BOTTOMRIGHT, 4, 0);

  item1 = addBoolItem(0, &cfg.selColumns, COL_LENGTH);
  item1->setText("Game length");
  item1->setSize(80, 16);
  item1->setPoint(PT_BOTTOMLEFT, item2, PT_BOTTOMRIGHT, 4, 0);

  item2 = addBoolItem(0, &cfg.selColumns, COL_MODE);
  item2->setText("Game mode");
  item2->setSize(82, 16);
  item2->setPoint(PT_BOTTOMLEFT, item1, PT_BOTTOMRIGHT, 4, 0);

  //item1 = addBoolItem(0, &cfg.saveCache);
  //item1->setText("Save game cache (speeds up parsing file info)");
  //item1->setSize(300, 16);
  //item1->setPoint(PT_TOPLEFT, tip, PT_BOTTOMLEFT, 0, 22);

  item1 = addBoolItem(0, &cfg.useTray);
  item1->setText("Minimize to system tray");
  item1->setSize(300, 16);
  item1->setPoint(PT_TOPLEFT, group, PT_BOTTOMLEFT, 50, 12);

  item2 = addBoolItem(0, &cfg.enableUrl);
  item2->setText("Enable URL in path bar (make sure to include http://)");
  item2->setSize(300, 16);
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);

  item1 = addBoolItem(0, &cfg.autoUpdate);
  item1->setText("Automatically check for updates");
  item1->setSize(300, 16);
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);

  item2 = addBoolItem(0, &cfg.autoLoadMap);
  item2->setText("Automatically load missing map data");
  item2->setSize(300, 16);
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);

  openWithThis = new ButtonFrame("Set this program as default for opening replays",
    tab, IDC_OPENWITHTHIS, BS_AUTOCHECKBOX);
  openWithThis->setSize(300, 16);
  openWithThis->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  String cmd;
  if (getRegString(HKEY_CURRENT_USER, "Software\\Classes\\Warcraft3.Replay\\shell\\open\\command", "", cmd) &&
      cmd == getAppPath(true) + " \"%1\"")
    openWithThis->setCheck(true);
  else
    openWithThis->setCheck(false);

  //////////////////////////////////////////////////////////////

  tab = tabs->addTab("Replay");

  tip = new StaticFrame("Your name(s) in replays:", tab);
  item1 = addStringItem(1, &cfg.ownNames);
  item1->setPoint(PT_TOPLEFT, 240, 7);
  item1->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item1, PT_BOTTOMLEFT, -4, -7);

  tip = new StaticFrame("Repeated action delay for skills (ms):", tab);
  item2 = addIntItem(1, &cfg.repDelay);
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);
  item2->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item2, PT_BOTTOMLEFT, -4, -7);

  tip = new StaticFrame("Repeated action delay for items (ms):", tab);
  item1 = addIntItem(1, &cfg.repDelayItems);
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item1, PT_BOTTOMLEFT, -4, -7);

  group = new ButtonFrame("Timeline mode", tab, IDC_TIMELINE, WS_CLIPSIBLINGS | BS_GROUPBOX);
  group->setPoint(PT_TOP, item1, PT_BOTTOM, 0, 10);
  group->setPoint(PT_LEFT, 50, 0);
  group->setSize(430, 152);

  item2 = addBoolItem(1, &cfg.drawWards);
  item2->setText("Draw wards");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 22);
  item2->setSize(228, 16);

  tip = new StaticFrame("Ward lifetime (seconds):", tab);
  item1 = addIntItem(1, &cfg.wardLife);
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item1, PT_BOTTOMLEFT, -4, -7);

  item2 = addBoolItem(1, &cfg.drawChat);
  item2->setText("Draw chat");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);
  item2->setSize(228, 16);

  tip = new StaticFrame("Chat fade time (seconds):", tab);
  item1 = addIntItem(1, &cfg.chatStaysOn);
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(228, 23);
  tip->setPoint(PT_BOTTOMRIGHT, item1, PT_BOTTOMLEFT, -4, -7);

  item2 = addBoolItem(1, &cfg.drawPings);
  item2->setText("Draw minimap pings");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 5);
  item2->setSize(228, 16);

  item1 = addBoolItem(1, &cfg.drawBuildings);
  item1->setText("Draw buildings");
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 5);
  item1->setSize(228, 16);

  item1 = addBoolItem(1, &cfg.showLevels);
  item1->setText("Show skill levels in build view");
  item1->setPoint(PT_TOPLEFT, group, PT_BOTTOMLEFT, 10, 10);
  item1->setSize(180, 16);
  WindowFrame* item3 = item1;

  //item2 = addBoolItem(1, &cfg.skillColors);
  //item2->setText("Color skills in build view");
  //item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 4);
  //item2->setSize(180, 16);
  item2 = item1;

  item1 = addBoolItem(1, &cfg.smoothGold);
  item1->setText("Smoother gold timeline");
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 4);
  item1->setSize(180, 16);

  item2 = addBoolItem(1, &cfg.relTime);
  item2->setText("Time relative to creep spawn");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 4);
  item2->setSize(180, 16);

  item1 = addBoolItem(1, &cfg.showEmptySlots);
  item1->setText("Show empty slots");
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 4);
  item1->setSize(180, 16);

  item2 = addBoolItem(1, &cfg.showAssemble);
  item2->setText("Show assembled items in itembuild");
  item2->setPoint(PT_BOTTOMLEFT, item3, PT_BOTTOMRIGHT, 0, 0);
  item2->setSize(180, 16);

  item3 = item1;

  //item1 = addBoolItem(1, &cfg.syncSelect);
  //item1->setText("Synchronize selection in build view");
  //item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 4);
  //item1->setSize(180, 16);
  item1 = item2;

  item2 = addBoolItem(1, &cfg.chatHeroes);
  item2->setText("Show hero names in chat");
  item2->setPoint(PT_TOPLEFT, item1, PT_BOTTOMLEFT, 0, 4);
  item2->setSize(180, 16);

  item1 = addBoolItem(1, &cfg.chatAssists);
  item1->setText("Show assists in chat");
  item1->setPoint(PT_TOPLEFT, item2, PT_BOTTOMLEFT, 0, 4);
  item1->setSize(180, 16);

  tip = new StaticFrame("Chat font:", tab);
  tip->setPoint(PT_TOPLEFT, item3, PT_BOTTOMLEFT, 0, 10);
  btn1 = new ButtonFrame("Font", tab, IDC_CHATFONT);
  btn1->setPoint(PT_BOTTOMLEFT, tip, PT_BOTTOMRIGHT, 5, 6);
  btn1->setSize(60, 23);

  Frame* tip2 = new StaticFrame("Player colors:", tab);
  tip2->setPoint(PT_BOTTOMLEFT, tip, PT_BOTTOMLEFT, 180, 0);
  chatColorMode = new ComboFrame(tab, IDC_CHATCOLORMODE);
  chatColorMode->setPoint(PT_BOTTOMLEFT, tip2, PT_BOTTOMRIGHT, 5, 5);
  chatColorMode->setPoint(PT_RIGHT, tab, PT_LEFT, 468, 0);
  chatColorMode->addString("Default");
  chatColorMode->addString("Adapted");
  chatColorMode->addString("Dark");
  chatColorMode->setCurSel(cfg.chatColors);

  chatColors = new ClickColor(tab, IDC_CHATCOLOR);
  chatColors->setPoint(PT_LEFT, 60, 0);
  chatColors->setPoint(PT_TOP, btn1, PT_BOTTOM, 0, 5);
  chatColors->setSize(408, 50);

  item1 = addBoolItem(1, &cfg.useLog);
  item1->setText("Log actions to log.txt");
  item1->setPoint(PT_TOPLEFT, chatColors, PT_BOTTOMLEFT, 0, 8);
  item1->setSize(180, 16);
}

void SettingsWindow::updateExtra()
{
  chatColorMode->setCurSel(cfg.chatColors);

  getApp()->reloadWarData();
  notify(WM_UPDATEPATH, 0, 0);
  InvalidateRect(chatColors->getHandle(), NULL, TRUE);
}
uint32 SettingsWindow::handleExtra(uint32 message, uint32 wParam, uint32 lParam)
{
  if (message == WM_COMMAND && lParam)
  {
    int id = LOWORD(wParam);
    int code = HIWORD(wParam);
    switch (id)
    {
    case IDC_RESETWARPATH:
      if (code == BN_CLICKED)
      {
        String warPath = "";
        getRegString(HKEY_CURRENT_USER, "Software\\Blizzard Entertainment\\Warcraft III",
          "InstallPath", warPath);
        cfg.warPath = warPath;
        updateKey(&cfg.warPath);
        getApp()->reloadWarData();
      }
      break;
    case IDC_BROWSEWARPATH:
      if (code == BN_CLICKED)
      {
        String warPath;
        if (browseForFolder("Select Warcraft III folder", warPath))
        {
          cfg.warPath = warPath;
          updateKey(&cfg.warPath);
          getApp()->reloadWarData();
        }
      }
      break;
    case IDC_RESETREPLAYPATH:
      if (code == BN_CLICKED)
      {
        if (String(cfg.warPath).isEmpty())
          cfg.replayPath = String::fixPath("");
        else
          cfg.replayPath = String::fixPath(String::buildFullName(cfg.warPath, "Replay"));
        updateKey(&cfg.replayPath);

        notify(WM_UPDATEPATH, 0, 0);
      }
      break;
    case IDC_BROWSEREPLAYPATH:
      if (code == BN_CLICKED)
      {
        String replayPath;
        if (browseForFolder("Select replay folder", replayPath))
        {
          cfg.replayPath = replayPath;
          updateKey(&cfg.replayPath);

          notify(WM_UPDATEPATH, 0, 0);
        }
      }
      break;
    case IDC_APPLYREPLAYPATH:
      if (code == BN_CLICKED)
        notify(WM_UPDATEPATH, 0, 0);
      break;
    case IDC_COPYFORMATHELP:
      if (code == BN_CLICKED)
      {
        MessageBox(getApp()->getMainWindow(),
            "Specify the name of the destination file.\n"
            "Extension .w3g will be appended automatically\n\n"
            "You can use the following tags (tags are text in angle brackets):\n"
            "<n> - insert an integer number so that the filename becomes unique.\n"
            "<version> - insert Warcraft patch version used in the game (e.g. \"1.21\")\n"
            "<map> - DotA version (e.g. \"6.49b\")\n"
            "<name> - game name (e.g. \"dota -ap no noobs\")\n"
            "<sentinel> - number of sentinel players\n"
            "<scourge> - number of scourge players\n"
            "<sentinel kills> - sentinel score (may be wrong)\n"
            "<scourge kills> - scourge score (may be wrong)\n"
            "<time fmt> - date of the replay file, fmt can contain the following:\n"
            "  %y - Year without century, %Y - Year with century\n"
            "  %b - Short month name, %B - Full month name, %m - Month as number\n"
            "  %d - Day of month, %j - Day of year\n"
            "  %a - Short day of week, %A - Full day of week, %w - Weekday as number\n"
            "  %H - Hour in 24-hour format, %I - Hour in 12-hour format, %p - A.M./P.M.\n"
            "  %M - Minute, %S - Second\n"
            "<winner> - game winner (\"Unknown\", \"Sentinel\" or \"Scourge\")\n"
            "In the following tags ID can be either player index (1-10), \"host\" or \"saver\":\n"
            "<player ID> - player name\n"
            "<hero ID> - player's hero\n"
            "<win ID> - whether the player won or lost (\"Won\" or \"Lost\")\n"
            "<kills ID> - player's hero kills\n"
            "<deaths ID> - player's deaths\n"
            "<creeps ID> - player's creep kills\n"
            "<denies ID> - player's creep denies\n\n"
            "For batch copying replays the following tags are available:\n"
            "<file> - original file title (e.g. \"LastReplay\")\n"
            "<path> - original file path (e.g. \"Dota\\IHL\\\")",
          "Help", MB_OK);
      }
      break;
    case IDC_OPENWITHTHIS:
      if (code == BN_CLICKED)
      {
        String cmd = "";
        if (openWithThis->checked())
          cmd = getAppPath(true) + " \"%1\"";
        else if (!String(cfg.warPath).isEmpty())
          cmd.printf("\"%s\" -loadfile \"%%1\"", String::buildFullName(cfg.warPath, "War3.exe"));
        HKEY hKey;
        HRESULT result;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\Warcraft3.Replay\\shell\\open\\command",
          0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
        {
          RegSetValueEx(hKey, "", 0, REG_SZ, (LPBYTE) cmd.c_str(), cmd.length() + 1);
          RegCloseKey(hKey);
        }
        else if ((result = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\.w3g", 0, NULL, REG_OPTION_NON_VOLATILE,
          KEY_SET_VALUE, NULL, &hKey, NULL)) == ERROR_SUCCESS)
        {
          RegSetValueEx(hKey, "", 0, REG_SZ, (LPBYTE) "Warcraft3.Replay", strlen("Warcraft3.Replay") + 1);
          RegCloseKey(hKey);
          if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\Warcraft3.Replay\\shell\\open\\command",
            0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS)
          {
            RegSetValueEx(hKey, "", 0, REG_SZ, (LPBYTE) cmd.c_str(), cmd.length() + 1);
            RegCloseKey(hKey);
          }
          if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\Warcraft3.Replay\\DefaultIcon",
            0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS)
          {
            String warIcon;
            if (String(cfg.warPath).isEmpty())
              warIcon = String::buildFullName(cfg.warPath, "replays.ico");
            else
              warIcon = getAppPath(true) + ",0";
            RegSetValueEx(hKey, "", 0, REG_SZ, (LPBYTE) warIcon.c_str(), warIcon.length() + 1);
            RegCloseKey(hKey);
          }
        }
        else
        {
          LPTSTR pBuffer;
          FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL, result, 0, (LPTSTR) &pBuffer, 4, NULL);
          MessageBox(getApp()->getMainWindow(), pBuffer, "Error", MB_ICONERROR | MB_OK);
          LocalFree((HLOCAL) pBuffer);
        }
      }
      break;
    case IDC_CHATFONT:
      if (code == BN_CLICKED)
      {
        CHOOSEFONT cf;
        memset(&cf, 0, sizeof cf);
        cf.lStructSize = sizeof cf;
        cf.hwndOwner = getApp()->getMainWindow();
        cf.lpLogFont = &cfg.chatFont;
        cf.Flags = CF_EFFECTS | CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL | CF_SCREENFONTS;
        cf.rgbColors = cfg.chatFg;
        if (ChooseFont(&cf))
        {
          cfg.chatFg = cf.rgbColors;
          chatColors->setFont(FontSys::getFont(cfg.chatFont));
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
        cc.hwndOwner = getApp()->getMainWindow();
        cc.rgbResult = cfg.chatBg;
        cc.Flags = CC_FULLOPEN | CC_RGBINIT;
        cc.lpCustColors = custColors;
        if (ChooseColor(&cc))
        {
          cfg.chatBg = cc.rgbResult;
          InvalidateRect(chatColors->getHandle(), NULL, TRUE);
        }
      }
      break;
    case IDC_CHATCOLORMODE:
      if (code == CBN_SELCHANGE)
      {
        cfg.chatColors = chatColorMode->getCurSel();
        InvalidateRect(chatColors->getHandle(), NULL, TRUE);
      }
      break;
    default:
      return M_UNHANDLED;
    }
    return 0;
  }
  return M_UNHANDLED;
}
