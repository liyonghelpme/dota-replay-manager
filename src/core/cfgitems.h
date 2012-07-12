cfg_int(splitterPos, 200);
cfg_int(wndX, CW_USEDEFAULT);
cfg_int(wndY, 0);
cfg_int(wndWidth, CW_USEDEFAULT);
cfg_int(wndHeight, 0);
cfg_int(wndShow, SW_SHOWNORMAL);

//////////////////////////////////////////////

#ifdef cfg_init
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
cfg_string(warPath, _warPath)
cfg_string(replayPath, _replayPath)
cfg_int(maxFiles, 2000);
cfg_int(viewWindow, 0);
cfg_int(autoView, 1);
cfg_int(autoCopy, 0);
cfg_string(copyFormat, "Autocopy\\replay<n>");
cfg_int(showDetails, 0);
cfg_int(selColumns, 0);
cfg_int(saveCache, 1);
cfg_int(useTray, 1);
cfg_int(enableUrl, 1);
cfg_int(autoUpdate, 1);

cfg_string(ownNames, "");
cfg_int(repDelay, 3000);
cfg_int(repDelayItems, 1000);
cfg_int(drawWards, 1);
cfg_int(wardLife, 360);
cfg_int(drawChat, 1);
cfg_int(chatStaysOn, 16);
cfg_int(drawPings, 1);
cfg_int(drawBuildings, 1);
cfg_int(showLevels, 1);
cfg_int(skillColors, 1);
cfg_int(smoothGold, 1);
cfg_int(relTime, 1);
cfg_int(showEmptySlots, 1);
cfg_int(showAssemble, 1);
cfg_int(syncSelect, 1);
cfg_int(chatHeroes, 1);
cfg_int(chatAssists, 1);
cfg_binary(chatFont, &_chatFont, sizeof _chatFont);
cfg_int(chatColors, 1);
cfg_int(chatBg, 0x000000);
cfg_int(chatFg, 0xFFFFFF);
cfg_int(useLog, 0);
