#include "core/app.h"

#include "ui/mainwnd.h"
#include "viewitem.h"
#include "ui/searchwnd.h"
#include "ui/searchres.h"
#include "ui/folderwnd.h"
#include "ui/replaywnd.h"

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
    if (isPermanent() && !item->equal(this))
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
bool SettingsViewItem::equal(ViewItem* item)
{
  SettingsViewItem* other = dynamic_cast<SettingsViewItem*>(item);
  return (other != NULL);
}


void SearchViewItem::apply(MainWnd* wnd)
{
  SearchWindow* search = (SearchWindow*) wnd->setView(MAINWND_SEARCH);
  search->setPath(((FolderWindow*) wnd->getView(MAINWND_FOLDER))->getPath());
}
bool SearchViewItem::equal(ViewItem* item)
{
  SearchViewItem* other = dynamic_cast<SearchViewItem*>(item);
  return (other != NULL);
}

void SearchResViewItem::apply(MainWnd* wnd)
{
  wnd->setAddress("#searchres");
  SearchResults* search = (SearchResults*) wnd->setView(MAINWND_SEARCHRES);
  search->rebuild();
}
bool SearchResViewItem::equal(ViewItem* item)
{
  SearchResViewItem* other = dynamic_cast<SearchResViewItem*>(item);
  return (other != NULL);
}


void FolderViewItem::apply(MainWnd* wnd)
{
  wnd->setAddress(path);
  FolderWindow* f = (FolderWindow*) wnd->setView(MAINWND_FOLDER);
  f->setPath(path);
}
bool FolderViewItem::equal(ViewItem* item)
{
  FolderViewItem* other = dynamic_cast<FolderViewItem*>(item);
  return (other && path.icompare(other->path) == 0);
}


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
bool ReplayViewItem::equal(ViewItem* item)
{
  ReplayViewItem* other = dynamic_cast<ReplayViewItem*>(item);
  return (other && path.icompare(other->path) == 0);
}
