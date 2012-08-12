#include "core/app.h"

#include "ui/mainwnd.h"
#include "viewitem.h"

ViewItem* ViewItem::push(ViewItem* item)
{
  if (this)
  {
    ViewItem* cur = next;
    while (cur)
    {
      ViewItem* temp = cur->next;
      delete cur;
      cur = temp;
    }
    if (isPermanent())
    {
      next = item;
      item->prev = this;
    }
    else
    {
      if (prev)
        prev->next = item;
      item->prev = prev;
      delete this;
    }
  }
  return item;
}
void ViewItem::free()
{
  if (this)
  {
    ViewItem* cur = next;
    while (cur)
    {
      ViewItem* temp = cur->next;
      delete cur;
      cur = temp;
    }
    cur = prev;
    while (cur)
    {
      ViewItem* temp = cur->prev;
      delete cur;
      cur = temp;
    }
    delete this;
  }
}
ViewItem* ViewItem::back()
{
  ViewItem* temp = prev;
  if (!isPermanent())
  {
    temp->next = NULL;
    delete this;
  }
  return temp;
}

void SettingsViewItem::apply(MainWnd* wnd)
{
  wnd->setView(MAINWND_SETTINGS);
}

void FolderViewItem::apply(MainWnd* wnd)
{
  wnd->setView(MAINWND_FOLDER);
}

#include "ui/replaywnd.h"

void ReplayViewItem::apply(MainWnd* wnd)
{
  wnd->setAddress(path);
  ReplayWindow* r = (ReplayWindow*) wnd->setView(MAINWND_REPLAY);
  r->openReplay(path);
  r->setViewItem(this);
  r->setTab(tab);
  if (player)
    r->setPlayer(player);
}
