#ifndef __SYSTEM_TRAY_H__
#define __SYSTEM_TRAY_H__

void CreateTrayIcon (HINSTANCE hInstance, HWND hWnd);
void SetTrayIcon (HICON id);
void SetTrayText (char* fmt, ...);
void DeleteTrayIcon ();
HICON GetTrayIcon ();

void TrayNotify (char const* title, char const* text);

#define WM_TRAYNOTIFY         WM_USER+57
#define IDI_TRAY              1758
#define ID_TRAY_OPEN          1266
#define ID_TRAY_EXIT          1268

#endif // __SYSTEM_TRAY_H__
