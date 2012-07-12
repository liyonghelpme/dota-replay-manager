#include "core/app.h"
#include "frameui/fontsys.h"

#include "folderwnd.h"

FolderWindow::FolderWindow(Window* parent)
{
  create(0, 0, 10, 10, "", WS_CHILD | WS_VISIBLE, 0, parent->getHandle());
}
