#include "core/app.h"
#include "simpledlg.h"
#include "frameui/window.h"

struct TextPromptDlgParam
{
  String title;
  String prompt;
  String* text;
};
static INT_PTR CALLBACK TextPromptDlgFunc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    {
      TextPromptDlgParam* param = (TextPromptDlgParam*) lParam;
      SetWindowText(hDlg, param->title.c_str());
      SetDlgItemText(hDlg, IDC_PROMPT, param->prompt.c_str());
      SetDlgItemText(hDlg, IDC_INPUT, param->text->c_str());
      SetWindowLong(hDlg, DWL_USER, (LONG) param->text);
      SetFocus(GetDlgItem(hDlg, IDC_INPUT));
    }
    return FALSE;
  case WM_COMMAND:
    if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
    {
      if (LOWORD(wParam == IDOK))
      {
        String* text = (String*) GetWindowLong(hDlg, DWL_USER);
        HWND hInput = GetDlgItem(hDlg, IDC_INPUT);
        if (text && hInput)
          *text = Window::getWindowText(hInput);
      }
      EndDialog(hDlg, LOWORD(wParam));
      return TRUE;
    }
    break;
  }
  return FALSE;
}

int getQueryString(HWND hParent, String title, String prompt, String& text)
{
  TextPromptDlgParam param = {title, prompt, &text};
  return DialogBoxParam(getApp()->getInstanceHandle(), MAKEINTRESOURCE(IDD_TEXTPROMPT),
    hParent, TextPromptDlgFunc, (LPARAM) &param);
}
