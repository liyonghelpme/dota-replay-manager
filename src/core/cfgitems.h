regbasic(int, splitterPos, 200);
regbasic(int, wndX, CW_USEDEFAULT);
regbasic(int, wndY, 0);
regbasic(int, wndWidth, CW_USEDEFAULT);
regbasic(int, wndHeight, 0);
regbasic(int, wndShow, SW_SHOWNORMAL);

regbasic(int, byDate, 0);

regbasic(uint64, lastVersionCheck, 0);
regbasic(uint64, lastVersionNotify, 0);

//////////////////////////////////////////////

#ifdef cfginit
String _warPath = "";
getRegString(HKEY_CURRENT_USER, "Software\\Blizzard Entertainment\\Warcraft III",
  "InstallPath", _warPath);
if (_warPath.isEmpty())
  getRegString(HKEY_LOCAL_MACHINE, "Software\\Blizzard Entertainment\\Warcraft III",
    "InstallPath", _warPath);
String _replayPath;
if (_warPath.isEmpty())
  _replayPath = String::fixPath("");
else
  _replayPath = String::buildFullName(_warPath, "Replay");
LOGFONT _chatFont;
memset (&_chatFont, 0, sizeof _chatFont);
HDC hDC = GetDC(NULL);
strcpy(_chatFont.lfFaceName, "Georgia");
_chatFont.lfHeight = -MulDiv(10, GetDeviceCaps(hDC, LOGPIXELSY), 72);
ReleaseDC(NULL, hDC);
#endif

#ifdef cfgconst
enum {COL_SAVED = 0x01,
      COL_SIZE = 0x02,
      COL_NAME = 0x04,
      COL_RATIO = 0x10,
      COL_LENGTH = 0x20,
      COL_MODE = 0x40
};
#endif

regstring(warPath, _warPath);
regstring(replayPath, _replayPath);
regbasic(int, hideEmpty, 1);
//regbasic(int, maxFiles, 2000);
regbasic(int, viewWindow, 0);
regbasic(int, autoView, 1);
regbasic(int, autoCopy, 0);
regstring(copyFormat, "Autocopy\\replay<n>");
//regbasic(int, showDetails, 0);
regbasic(int, selColumns, (COL_SAVED | COL_NAME | COL_RATIO));
regarray(int, colOrder, 7);
regarray(int, colWidth, 7);
regarray(int, colSort, 7);
//regbasic(int, saveCache, 1);
regbasic(int, useTray, 1);
regbasic(int, enableUrl, 1);
regbasic(int, autoUpdate, 1);
regarray(int, giColWidth, 16);
regarray(int, giLColWidth, 16);

#ifdef cfginit
for (int i = 0; i < 7; i++)
{
  colOrder[i] = i;
  colWidth[i] = LVSCW_AUTOSIZE_USEHEADER;
  colSort[i] = i;
}
for (int i = 0; i < 16; i++)
  giColWidth[i] = LVSCW_AUTOSIZE_USEHEADER;
for (int i = 0; i < 4; i++)
  giLColWidth[i] = LVSCW_AUTOSIZE_USEHEADER;
#endif

regstring(ownNames, "");
regbasic(int, repDelay, 3000);
regbasic(int, repDelayItems, 1000);
regbasic(int, drawWards, 1);
regbasic(int, wardLife, 360);
regbasic(int, drawChat, 1);
regbasic(int, chatStaysOn, 16);
regbasic(int, drawPings, 1);
regbasic(int, drawBuildings, 1);
regbasic(int, showLevels, 1);
regbasic(int, skillColors, 1);
regbasic(int, smoothGold, 1);
regbasic(int, relTime, 1);
regbasic(int, showEmptySlots, 1);
regbasic(int, showAssemble, 1);
regbasic(int, syncSelect, 1);
regbasic(int, chatHeroes, 1);
regbasic(int, chatAssists, 1);
regbasic(LOGFONT, chatFont, _chatFont);
regbasic(int, chatColors, 1);
regbasic(int, chatBg, 0x000000);
regbasic(int, chatFg, 0xFFFFFF);
regbasic(int, useLog, 0);
regbasic(int, useOGL, 0);

regstrarray(fmtPresets);
regstring(fmtScript, "");
regbasic(uint64, fmtGenerator, 0);

regbasic(uint32, chatFilters, 0xFFFFFFFF);

regbasic(int, autoLoadMap, 0);