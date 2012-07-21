#include "core/app.h"
#include "frameui/fontsys.h"

#include "folderwnd.h"

FolderWindow::FolderWindow(Frame* parent)
  : WindowFrame(parent)
{
  create("", WS_CHILD, 0);
}
