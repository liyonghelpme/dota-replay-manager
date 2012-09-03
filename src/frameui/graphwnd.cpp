#include "core/app.h"

#include "frameui/fontsys.h"
#include "graphwnd.h"

static void FixRect(RECT& rc, int x0, int y0, int x1, int y1, bool vinv = false)
{
  if (x0 < x1)
  {
    rc.left = x0;
    rc.right = x1;
  }
  else
  {
    rc.left = x1;
    rc.right = x0;
  }
  if (vinv ? y1 < y0 : y0 < y1)
  {
    rc.top = y0;
    rc.bottom = y1;
  }
  else
  {
    rc.bottom = y1;
    rc.top = y0;
  }
}

GraphWindow::GraphWindow(Frame* parent, uint32 color, uint32 lineColor)
  : WindowFrame(parent)
{
  painter = NULL;
  bkColor = color;
  lnColor = lineColor;
  ruleColor = ((((color & 0xFF) * 4 + (lineColor & 0xFF)) / 5) & 0xFF) |
              ((((((color >> 8) & 0xFF) * 4 + ((lineColor >> 8) & 0xFF)) / 5) & 0xFF) << 8) |
              ((((((color >> 16) & 0xFF) * 4 + ((lineColor >> 16) & 0xFF)) / 5) & 0xFF) << 16);
  hMinorColor = ((((color & 0xFF) * 9 + (lineColor & 0xFF)) / 10) & 0xFF) |
              ((((((color >> 8) & 0xFF) * 9 + ((lineColor >> 8) & 0xFF)) / 10) & 0xFF) << 8) |
              ((((((color >> 16) & 0xFF) * 9 + ((lineColor >> 16) & 0xFF)) / 10) & 0xFF) << 16);
  vMinorColor = hMinorColor;

  rcSourceMax.left = max_int32;
  rcSourceMax.top = min_int32;
  rcSourceMax.right = min_int32;
  rcSourceMax.bottom = max_int32;
  rcSource = rcSourceMax;

  marginX = 50;
  marginY = -20;

  tipRect.left = tipRect.top = tipRect.right = tipRect.bottom = 0;
  tipGraph = -1;
  tipText = "";

  clickMode = -1;

  cursors[0] = LoadCursor(getApp()->getInstanceHandle(), MAKEINTRESOURCE(IDC_HAND_MOVE));
  cursors[1] = LoadCursor(getApp()->getInstanceHandle(), MAKEINTRESOURCE(IDC_ZOOM_IN));
  cursors[2] = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));

  if (WNDCLASSEX* wcx = createclass("GraphWindow"))
  {
    wcx->style |= CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    RegisterClassEx(wcx);
  }
  create("", WS_CHILD, WS_EX_CLIENTEDGE);

  if (cfg.useOGL)
    painter = new OGLPainter(hWnd, bkColor, FontSys::getSysFont());
  else
    painter = new GDIPainter(FontSys::getSysFont());
}
GraphWindow::~GraphWindow()
{
  delete painter;
}

void GraphWindow::clear()
{
  for (int i = 0; i < graphs.length(); i++)
  {
    graphs[i].points.clear();
    graphs[i].enabled = false;
  }
}
void GraphWindow::addGraph(int id, uint32 color, bool dash)
{
  while (id >= graphs.length())
  {
    Graph& g = graphs.push();
    g.color = 0;
    g.dash = false;
    g.enabled = false;
  }
  Graph& g = graphs[id];
  g.color = color;
  g.dash = dash;
  g.enabled = true;
  g.points.clear();
}
void GraphWindow::enableGraph(int id, bool enable)
{
  graphs[id].enabled = enable;

  rebuildRect();

  invalidate();
}
void GraphWindow::addGraphPoint(int id, int x, int y, bool dash)
{
  Point& p = graphs[id].points.push();
  p.x = x;
  p.y = y;
  p.dash = dash;

  if (graphs[id].enabled)
  {
    bool match = (rcSource.left == rcSourceMax.left &&
                  rcSource.top == rcSourceMax.top &&
                  rcSource.right == rcSourceMax.right &&
                  rcSource.bottom == rcSourceMax.bottom);
    extendRect(x, y);
    if (match)
      rcSource = rcSourceMax;
  }
}
void GraphWindow::smoothGraph(int id, int factor)
{
  Graph& g = graphs[id];
  int count = 1;
  for (int i = 1; i < g.points.length(); i++)
  {
    while (count > 1 && abs(g.points[i].y - g.points[count - 1].y) >
        (g.points[i].x - g.points[count - 1].x) * factor)
      count--;
    if (count < i)
      g.points[count] = g.points[i];
    count++;
  }
  g.points.resize(count);
}
void GraphWindow::combineGraphs(int id, int id2)
{
  if (id2 >= graphs.length())
    return;
  Graph* g[2] = {&graphs[id], &graphs[id2]};
  if (g[1]->points.length() == 0)
    return;
  if (g[0]->points.length() == 0)
  {
    for (int i = 0; i < g[1]->points.length(); i++)
      g[0]->points.push(g[1]->points[i]);
    return;
  }
  Array<Point> ng;
  int cur[2] = {0, 0};
  while (cur[0] < g[0]->points.length() || cur[1] < g[1]->points.length())
  {
    if (cur[0] < g[0]->points.length() && (cur[1] >= g[1]->points.length() ||
      g[1]->points[cur[1]].x > g[0]->points[cur[0]].x))
    {
      Point& p = ng.push();
      p.x = g[0]->points[cur[0]].x;
      p.y = g[0]->points[cur[0]].y;
      p.dash = g[0]->points[cur[0]].dash;
      if (cur[1] == 0)
        p.y += g[1]->points[0].y;
      else if (cur[1] >= g[1]->points.length())
        p.y += g[1]->points[g[1]->points.length() - 1].y;
      else
      {
        Point& pa = g[1]->points[cur[1] - 1];
        Point& pb = g[1]->points[cur[1]];
        p.y += pa.y + int(sint64(pb.y - pa.y) * (p.x - pa.x) / (pb.x - pa.x));
        p.dash &= pb.dash;
      }
      cur[0]++;
    }
    else
    {
      Point& p = ng.push();
      p = g[1]->points[cur[1]];
      if (cur[0] == 0)
        p.y += g[0]->points[0].y;
      else if (cur[0] >= g[0]->points.length())
        p.y += g[0]->points[g[0]->points.length() - 1].y;
      else
      {
        Point& pa = g[0]->points[cur[0] - 1];
        Point& pb = g[0]->points[cur[0]];
        p.y += pa.y + int(sint64(pb.y - pa.y) * (p.x - pa.x) / (pb.x - pa.x));
        p.dash &= pb.dash;
      }
      cur[1]++;
    }
  }
  g[0]->points = ng;
  rebuildRect();
}
void GraphWindow::extendRect(int x, int y)
{
  if (x < rcSourceMax.left)
    rcSourceMax.left = x;
  if (x > rcSourceMax.right)
    rcSourceMax.right = x;
  if (marginY > 0)
  {
    if (y - (rcSourceMax.top - y) / 20 < rcSourceMax.bottom)
      rcSourceMax.bottom = y - (rcSourceMax.top - y) / 20;
    if (y > rcSourceMax.top)
      rcSourceMax.top = y;
  }
  else
  {
    if (y < rcSourceMax.bottom)
      rcSourceMax.bottom = y;
    if (y + (y - rcSourceMax.bottom) / 20 > rcSourceMax.top)
      rcSourceMax.top = y + (y - rcSourceMax.bottom) / 20;
  }
}
void GraphWindow::rebuildRect()
{
  bool match = (rcSource.left == rcSourceMax.left &&
                rcSource.top == rcSourceMax.top &&
                rcSource.right == rcSourceMax.right &&
                rcSource.bottom == rcSourceMax.bottom);
  rcSourceMax.left = max_int32;
  rcSourceMax.top = min_int32;
  rcSourceMax.right = min_int32;
  rcSourceMax.bottom = max_int32;
  for (int i = 0; i < graphs.length(); i++)
  {
    if (graphs[i].enabled)
    {
      for (int j = 0; j < graphs[i].points.length(); j++)
        extendRect(graphs[i].points[j].x, graphs[i].points[j].y);
    }
  }
  if (match)
    rcSource = rcSourceMax;
  else
  {
    if (rcSource.left < rcSourceMax.left) rcSource.left = rcSourceMax.left;
    if (rcSource.right > rcSourceMax.right) rcSource.right = rcSourceMax.right;
    if (rcSource.bottom < rcSourceMax.bottom) rcSource.bottom = rcSourceMax.bottom;
    if (rcSource.top > rcSourceMax.top) rcSource.top = rcSourceMax.top;
    if (rcSource.left >= rcSource.right || rcSource.bottom >= rcSource.top)
      rcSource = rcSourceMax;
  }
}

uint32 GraphWindow::onMessage(uint32 message, uint32 wParam, uint32 lParam)
{
  switch (message)
  {
  case WM_NCHITTEST:
    {
      POINT pt;
      pt.x = GET_X_LPARAM(lParam);
      pt.y = GET_Y_LPARAM(lParam);
      ScreenToClient(hWnd, &pt);
      RECT rc;
      GetClientRect(hWnd, &rc);
      if (marginX > 0)
        rc.left += marginX;
      else
        rc.right += marginX;
      if (marginY > 0)
        rc.top += marginY;
      else
        rc.bottom += marginY;
      if (pt.x >= rc.left && pt.x < rc.right &&
          pt.y >= rc.top && pt.y < rc.bottom)
        return HTCLIENT;
      else
        return HTBORDER;
    }
    break;
  case WM_SETCURSOR:
    if (LOWORD(lParam) == HTCLIENT)
    {
      if (rcSource.left == rcSourceMax.left &&
          rcSource.top == rcSourceMax.top &&
          rcSource.right == rcSourceMax.right &&
          rcSource.bottom == rcSourceMax.bottom)
        SetCursor(cursors[1]);
      else
        SetCursor(cursors[0]);
    }
    else
      SetCursor(cursors[2]);
    return TRUE;
  case WM_ERASEBKGND:
    if (painter)
      painter->erase((HDC) wParam, hWnd, bkColor);
    return TRUE;
  case WM_SIZE:
    GetClientRect(hWnd, &rcWindow);
    if (marginX > 0)
      rcWindow.left += marginX;
    else
      rcWindow.right += marginX;
    if (marginY > 0)
      rcWindow.top += marginY;
    else
      rcWindow.bottom += marginY;
    if (painter)
      painter->onSize(width(), height());
    return 0;
  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hDC = BeginPaint(hWnd, &ps);
      GetClientRect(hWnd, &rcWindow);
      if (marginX > 0)
        rcWindow.left += marginX;
      else
        rcWindow.right += marginX;
      if (marginY > 0)
        rcWindow.top += marginY;
      else
        rcWindow.bottom += marginY;
      if (painter)
      {
        painter->begin(hDC);

        if (rcSource.right > rcSource.left && rcSource.top > rcSource.bottom)
        {
          drawHRules(rcSource.bottom, rcSource.top);
          drawVRules(rcSource.left, rcSource.right);

          for (int i = 0; i < graphs.length(); i++)
          {
            Graph& g = graphs[i];
            if (g.enabled && g.points.length() > 1)
            {
              int px = g.points[0].x;
              int py = g.points[0].y;
              int j = 1;
              while (g.points[j].x <= rcSource.left && j < g.points.length())
              {
                px = g.points[j].x;
                py = g.points[j].y;
                j++;
              }
              while (px < rcSource.right && j < g.points.length())
              {
                int cx = g.points[j].x;
                int cy = g.points[j].y;

                bool valid = true;
                int x[2] = {mapx(px), mapx(cx)};
                int y[2] = {mapy(py), mapy(cy)};
                for (int p = 0; p < 2; p++)
                {
                  if ((y[0] <= rcWindow.top && y[1] <= rcWindow.top) ||
                      (y[0] >= rcWindow.bottom && y[1] >= rcWindow.bottom) ||
                      x[0] >= rcWindow.right || x[1] <= rcWindow.left)
                  {
                    valid = false;
                    break;
                  }
                  if (x[p] < rcWindow.left && x[1 - p] > rcWindow.left)
                  {
                    y[p] += (y[1 - p] - y[p]) * (rcWindow.left - x[p]) / (x[1 - p] - x[p]);
                    x[p] = rcWindow.left;
                  }
                  if (x[p] > rcWindow.right && x[1 - p] < rcWindow.right)
                  {
                    y[p] += (y[1 - p] - y[p]) * (rcWindow.right - x[p]) / (x[1 - p] - x[p]);
                    x[p] = rcWindow.right;
                  }
                  if (y[p] < rcWindow.top && y[1 - p] > rcWindow.top)
                  {
                    x[p] += (x[1 - p] - x[p]) * (rcWindow.top - y[p]) / (y[1 - p] - y[p]);
                    y[p] = rcWindow.top;
                  }
                  if (y[p] > rcWindow.bottom && y[1 - p] < rcWindow.bottom)
                  {
                    x[p] += (x[1 - p] - x[p]) * (rcWindow.bottom - y[p]) / (y[1 - p] - y[p]);
                    y[p] = rcWindow.bottom;
                  }
                }
                if ((y[0] > rcWindow.top || y[1] > rcWindow.top) &&
                    (y[0] < rcWindow.bottom || y[1] < rcWindow.bottom) &&
                    x[0] < rcWindow.right && x[1] > rcWindow.left)
                {
                  painter->pen(g.color, g.dash | g.points[j].dash);
                  painter->line(x[0], y[0], x[1], y[1]);
                }

                px = cx;
                py = cy;
                j++;
              }
            }
          }
        }

        painter->pen(lnColor, false);
        if (marginX > 0)
          painter->line(rcWindow.left, rcWindow.top, rcWindow.left, rcWindow.bottom);
        else
          painter->line(rcWindow.right, rcWindow.top, rcWindow.right, rcWindow.bottom);
        if (marginY > 0)
          painter->line(rcWindow.left, rcWindow.top, rcWindow.right, rcWindow.top);
        else
          painter->line(rcWindow.left, rcWindow.bottom, rcWindow.right, rcWindow.bottom);

        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hWnd, &pt);
        if (tipGraph >= 0)
        {
          painter->color(bkColor | 0xFF000000);
          painter->fillrect(tipRect.left, tipRect.top, tipRect.right, tipRect.bottom);
          painter->pen(graphs[tipGraph].color, false);
          painter->rect(tipRect.left, tipRect.top, tipRect.right, tipRect.bottom);
          painter->text((tipRect.left + tipRect.right) / 2, (tipRect.top + tipRect.bottom) / 2,
            tipText, ALIGN_X_CENTER | ALIGN_Y_CENTER);
        }

        painter->end();
      }
      EndPaint(hWnd, &ps);
    }
    return 0;
//////////////////////////////////////
  case WM_LBUTTONDOWN:
    if (rcSource.right > rcSource.left && rcSource.top > rcSource.bottom)
    {
      SetCapture(hWnd);
      clickX = GET_X_LPARAM(lParam);
      clickY = GET_Y_LPARAM(lParam);
      if (rcSource.left == rcSourceMax.left &&
          rcSource.top == rcSourceMax.top &&
          rcSource.right == rcSourceMax.right &&
          rcSource.bottom == rcSourceMax.bottom)
      {
        if (clickX < rcWindow.left) clickX = rcWindow.left;
        if (clickX > rcWindow.right) clickX = rcWindow.right;
        if (clickY > rcWindow.bottom) clickY = rcWindow.bottom;
        if (clickY < rcWindow.top) clickY = rcWindow.top;
        dragX = clickX;
        dragY = clickY;
        clickMode = 1;
      }
      else
      {
        dragX = 0;
        dragY = 0;
        clickMode = 0;
      }
    }
    return 0;
  case WM_MOUSEMOVE:
    if (rcSource.right > rcSource.left && rcSource.top > rcSource.bottom)
    {
      bool wasTracking = (tipGraph >= 0);

      tipGraph = -1;
      tipText = "";

      int x = GET_X_LPARAM(lParam);
      int y = GET_Y_LPARAM(lParam);

      if (clickMode == 0)
      {
        int dx = unmaph(x - clickX);
        int dy = unmapv(y - clickY);
        rcSource.left += (dragX - dx);
        rcSource.right += (dragX - dx);
        rcSource.top += (dragY - dy);
        rcSource.bottom += (dragY - dy);
        int deltaX = maph(dx - dragX);
        int deltaY = mapv(dy - dragY);
        dragX = dx;
        dragY = dy;
        if (rcSource.left < rcSourceMax.left)
        {
          rcSource.right = rcSourceMax.left + rcSource.right - rcSource.left;
          rcSource.left = rcSourceMax.left;
        }
        else if (rcSource.right > rcSourceMax.right)
        {
          rcSource.left = rcSourceMax.right - rcSource.right + rcSource.left;
          rcSource.right = rcSourceMax.right;
        }
        if (rcSource.bottom < rcSourceMax.bottom)
        {
          rcSource.top = rcSourceMax.bottom + rcSource.top - rcSource.bottom;
          rcSource.bottom = rcSourceMax.bottom;
        }
        else if (rcSource.top > rcSourceMax.top)
        {
          rcSource.bottom = rcSourceMax.top - rcSource.top + rcSource.bottom;
          rcSource.top = rcSourceMax.top;
        }

        invalidate();
      }
      else if (clickMode == 1)
      {
        if (x < rcWindow.left) x = rcWindow.left;
        if (x > rcWindow.right) x = rcWindow.right;
        if (y > rcWindow.bottom) y = rcWindow.bottom;
        if (y < rcWindow.top) y = rcWindow.top;
        HDC hDC = GetDC(hWnd);

        RECT rc;
        FixRect(rc, clickX, clickY, dragX, dragY);
        DrawFocusRect(hDC, &rc);
        FixRect(rc, clickX, clickY, x, y);
        DrawFocusRect(hDC, &rc);

        dragX = x;
        dragY = y;

        ReleaseDC(hWnd, hDC);
      }
      else if (x > rcWindow.left && x < rcWindow.right &&
        y > rcWindow.top && y < rcWindow.bottom)
      {
        int bestDistance = 8;
        int tipY = 0;
        for (int i = 0; i < graphs.length(); i++)
        {
          if (graphs[i].points.length() > 0 && x > mapx(graphs[i].points[0].x) - bestDistance &&
            x < mapx(graphs[i].points[graphs[i].points.length() - 1].x) + bestDistance)
          {
            int gy = getGraphY(i, unmapx(x));
            int dist = abs(mapy(gy) - y);
            if (dist < bestDistance)
            {
              tipGraph = i;
              tipY = gy;
              bestDistance = dist;
            }
          }
        }
        if (tipGraph >= 0)
          tipText = formatTip(unmapx(x), tipY);
      }

      RECT oldTip = tipRect;
      if (!tipText.isEmpty())
      {
        ::Point pt = FontSys::getTextSize(FontSys::getSysFont(), tipText);
        if (x - 20 - pt.x < 0)
        {
          tipRect.left = x + 10;
          tipRect.right = x + 20 + pt.x;
        }
        else
        {
          tipRect.left = x - 20 - pt.x;
          tipRect.right = x - 10;
        }
        if (y - 20 - pt.y < 0)
        {
          tipRect.top = y + 10;
          tipRect.bottom = y + 20 + pt.y;
        }
        else
        {
          tipRect.top = y - 20 - pt.y;
          tipRect.bottom = y - 10;
        }
      }
      if (wasTracking)
        InvalidateRect(hWnd, &oldTip, TRUE);
      if (tipGraph >= 0)
        InvalidateRect(hWnd, &tipRect, FALSE);

      if (wasTracking != (tipGraph >= 0))
      {
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof tme;
        tme.dwFlags = (tipGraph >= 0 ? TME_LEAVE : TME_CANCEL);
        tme.dwHoverTime = HOVER_DEFAULT;
        tme.hwndTrack = hWnd;
        TrackMouseEvent(&tme);
      }
    }
    return 0;
  case WM_MOUSELEAVE:
    tipGraph = -1;
    InvalidateRect(hWnd, &tipRect, TRUE);
    return 0;
  case WM_LBUTTONUP:
    if (clickMode == 1 && rcSource.right > rcSource.left && rcSource.top > rcSource.bottom)
    {
      HDC hDC = GetDC(hWnd);
      RECT rc;
      FixRect(rc, clickX, clickY, dragX, dragY);
      DrawFocusRect(hDC, &rc);
      ReleaseDC(hWnd, hDC);

      int x = GET_X_LPARAM(lParam);
      int y = GET_Y_LPARAM(lParam);
      if (x < rcWindow.left) x = rcWindow.left;
      if (x > rcWindow.right) x = rcWindow.right;
      if (y > rcWindow.bottom) y = rcWindow.bottom;
      if (y < rcWindow.top) y = rcWindow.top;
      FixRect(rcSource, unmapx(clickX), unmapy(clickY), unmapx(x), unmapy(y), true);
      if (abs(clickX - x) < 5 || abs(clickY - y) < 5 ||
        rcSource.right == rcSource.left || rcSource.bottom == rcSource.top)
      {
        int wx = unmapx(clickX);
        int wy = unmapy(clickY);
        rcSource.left = wx - (rcSourceMax.right - rcSourceMax.left) / 8;
        rcSource.right = wx + (rcSourceMax.right - rcSourceMax.left) / 8;
        rcSource.top = wy + (rcSourceMax.top - rcSourceMax.bottom) / 8;
        rcSource.bottom = wy - (rcSourceMax.top - rcSourceMax.bottom) / 8;
        if (rcSource.left < rcSourceMax.left)
        {
          rcSource.right = rcSourceMax.left + rcSource.right - rcSource.left;
          rcSource.left = rcSourceMax.left;
        }
        else if (rcSource.right > rcSourceMax.right)
        {
          rcSource.left = rcSourceMax.right - rcSource.right + rcSource.left;
          rcSource.right = rcSourceMax.right;
        }
        if (rcSource.bottom < rcSourceMax.bottom)
        {
          rcSource.top = rcSourceMax.bottom + rcSource.top - rcSource.bottom;
          rcSource.bottom = rcSourceMax.bottom;
        }
        else if (rcSource.top > rcSourceMax.top)
        {
          rcSource.bottom = rcSourceMax.top - rcSource.top + rcSource.bottom;
          rcSource.top = rcSourceMax.top;
        }
      }
      invalidate();
    }
    if (clickMode >= 0)
    {
      ReleaseCapture();
      clickMode = -1;
    }
    return 0;
  case WM_RBUTTONUP:
    if (clickMode < 0)
    {
      rcSource = rcSourceMax;
      invalidate();
    }
    return 0;
  }
  return M_UNHANDLED;
}
void GraphWindow::drawHRule(int y, char const* text)
{
  if (y < rcSource.bottom || y >= rcSource.top)
    return;
  y = mapy(y);
  if (text)
  {
    painter->pen(lnColor, false);
    if (marginX < 0)
      painter->text(rcWindow.right + 2, y, text, ALIGN_X_LEFT | ALIGN_Y_CENTER);
    else
      painter->text(rcWindow.left - 2, y, text, ALIGN_X_RIGHT | ALIGN_Y_CENTER);
    painter->pen(ruleColor, false);
  }
  else
    painter->pen(hMinorColor, false);
  painter->line(rcWindow.left, y, rcWindow.right, y);
}
void GraphWindow::drawVRule(int x, char const* text)
{
  if (x < rcSource.left || x >= rcSource.right)
    return;
  x = mapx(x);
  if (text)
  {
    painter->pen(lnColor, false);
    if (marginY < 0)
      painter->text(x, rcWindow.bottom + 2, text, ALIGN_X_CENTER | ALIGN_Y_TOP);
    else
      painter->text(x, rcWindow.top - 2, text, ALIGN_X_CENTER | ALIGN_Y_BOTTOM);
    painter->pen(ruleColor, false);
  }
  else
    painter->pen(vMinorColor, false);
  painter->line(x, rcWindow.top, x, rcWindow.bottom);
}
int GraphWindow::mapx(int x)
{
  return int(sint64(x - rcSource.left) * (rcWindow.right - rcWindow.left) /
    (rcSource.right - rcSource.left) + rcWindow.left);
}
int GraphWindow::mapy(int y)
{
  return int(sint64(y - rcSource.top) * (rcWindow.bottom - rcWindow.top) /
    (rcSource.bottom - rcSource.top) + rcWindow.top);
}
int GraphWindow::maph(int w)
{
  return int(sint64(w) * (rcWindow.right - rcWindow.left) / (rcSource.right - rcSource.left));
}
int GraphWindow::mapv(int h)
{
  return int(sint64(h) * (rcWindow.bottom - rcWindow.top) / (rcSource.bottom - rcSource.top));
}
int GraphWindow::unmapx(int x)
{
  return int(sint64(x - rcWindow.left) * (rcSource.right - rcSource.left) /
    (rcWindow.right - rcWindow.left) + rcSource.left);
}
int GraphWindow::unmapy(int y)
{
  return int(sint64(y - rcWindow.top) * (rcSource.bottom - rcSource.top) /
    (rcWindow.bottom - rcWindow.top) + rcSource.top);
}
int GraphWindow::unmaph(int w)
{
  return int(sint64(w) * (rcSource.right - rcSource.left) / (rcWindow.right - rcWindow.left));
}
int GraphWindow::unmapv(int h)
{
  return int(sint64(h) * (rcSource.bottom - rcSource.top) / (rcWindow.bottom - rcWindow.top));
}

void GraphWindow::drawHRules(int start, int end)
{
  EnumStruct es;
  enumCount(es);
  while (mapv(-es.val) < 15)
    nextCount(es);

  for (int y = ((start + es.val - 1) / es.val) * es.val; y < end; y += es.val)
    drawHRule(y, String(y));
}
void GraphWindow::drawVRules(int start, int end)
{
  EnumStruct es;
  enumCount(es);
  while (maph(es.val) < 30)
    nextCount(es);

  for (int x = ((start + es.val - 1) / es.val) * es.val; x < end; x += es.val)
    drawVRule(x, String(x));
}

int GraphWindow::getGraphY(int id, int x)
{
  Graph& g = graphs[id];
  if (g.points.length() == 0)
    return 0;
  int left = 0;
  int right = g.points.length() - 1;
  if (x <= g.points[left].x)
    return g.points[left].y;
  if (x >= g.points[right].x)
    return g.points[right].y;
  while (left < right - 1)
  {
    int mid = (left + right) / 2;
    if (x >= g.points[mid].x)
      left = mid;
    else
      right = mid;
  }
  return int(g.points[left].y + sint64(g.points[left + 1].y - g.points[left].y) *
    (x - g.points[left].x) / (g.points[left + 1].x - g.points[left].x));
}
