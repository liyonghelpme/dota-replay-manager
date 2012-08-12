#ifndef __FRAMEUI_DRAGDROP__
#define __FRAMEUI_DRAGDROP__

#include "base/types.h"
#include "frameui/framewnd.h"
#include <windows.h>

#define WM_DRAGDROP       (WM_USER+283)
#define WM_DRAGOVER       (WM_USER+284)
#define WM_DRAGLEAVE      (WM_USER+285)

class DropTarget : public IDropTarget
{
protected:
  int refCount;
  FORMATETC format;
  uint32 allowedEffects;
  bool allowDrop;
  WindowFrame* window;

  HRESULT __stdcall QueryInterface(REFIID iid, void** ppvObject);
  ULONG __stdcall AddRef();
  ULONG __stdcall Release();

  HRESULT __stdcall DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
  HRESULT __stdcall DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
  HRESULT __stdcall DragLeave();
  HRESULT __stdcall Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);

  uint32 DropEffect(uint32 grfKeyState, POINTL pt, uint32 dwAllowed);
public:
  DropTarget(WindowFrame* window, CLIPFORMAT format, uint32 allowedEffects);
  ~DropTarget();
};

uint32 DoDragDrop(CLIPFORMAT format, HGLOBAL data, uint32 allowedEffects);

#endif // __FRAMEUI_DRAGDROP__
