#include "core/app.h"

#include "base/version.h"
#include "ui/updatedlg.h"
#include "aboutdlg.h"

static const char projectURL[] = "http://www.rivsoft.net/projects/dotareplay/";
HICON AboutDialog::hIcon = NULL;

INT_PTR CALLBACK AboutDialog::DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    {
      if (hIcon == NULL)
        hIcon = (HICON) LoadImage(getInstance(), MAKEINTRESOURCE(IDI_MAIN),
          IMAGE_ICON, 48, 48, 0);
      HWND hIconWnd = GetDlgItem(hDlg, IDC_APPICON);
      SendMessage(hIconWnd, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
      SetWindowPos(hIconWnd, NULL, 0, 0, 48, 48, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
      SetDlgItemText(hDlg, IDC_APPINFO, String::format("DotA Replay Version %s",
        formatVersion(UpdateDialog::thisVersion)));
    }
    return TRUE;
  case WM_COMMAND:
    switch (LOWORD(wParam))
    {
    case IDC_OPENWEB:
      OpenURL(projectURL);
      return TRUE;
    case IDOK:
    case IDCANCEL:
      EndDialog(hDlg, LOWORD(wParam));
      return TRUE;
    }
    break;
  }
  return FALSE;
}
int AboutDialog::run()
{
  return DialogBox(getInstance(), MAKEINTRESOURCE(IDD_ABOUTBOX),
    getApp()->getMainWindow(), DlgProc);
}
