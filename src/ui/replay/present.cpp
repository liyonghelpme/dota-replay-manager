#include "core/app.h"
#include "graphics/imagelib.h"
#include "graphics/glib.h"
#include "dota/colors.h"
#include "base/utils.h"
#include "dota/consts.h"
#include "frameui/fontsys.h"

#include "present.h"

void ReplayPresentTab::onSetReplay()
{
}

ReplayPresentTab::ReplayPresentTab(Frame* parent)
  : ReplayTab(parent)
{
  editor = new ScriptEditor(this);
  editor->setPoint(PT_TOPLEFT, 10, 10);
  editor->setPoint(PT_BOTTOMRIGHT, -10, -10);

  File* file = File::open("K:\\Progs\\DotAReplay\\debug\\ppreset1.txt", File::READ);
  String text = file->gets(true);
  editor->setText(text);
  delete file;
}

uint32 ReplayPresentTab::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  return M_UNHANDLED;
}
