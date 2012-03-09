#include "timeline.h"
#include <string.h>
#include <math.h>

W3GActionList::W3GActionList()
{
  memset(acounter, 0, sizeof acounter);
  quickLoad = true;
  spawn_time = 0x80000000;
  lane_count = 0;
  lane_x = 0;
  lane_y = 0;
  home_x = 0;
  home_y = 0;
}

static float team_x[] = {-6864, 6400};
static float team_y[] = {-6784, 6096};

void W3GActionList::start(bool quick, int team)
{
  float x = team_x[team];
  float y = team_y[team];
  actions.clear();
  quickLoad = quick;
  home_x = x;
  home_y = y;
  Movement& m = actions.push();
  m.state = STATE_ALIVE;
  m.time = 0;
  m.x = x;
  m.y = y;
}
void W3GActionList::addEvent(uint32 time, int type, int arg)
{
  if (!quickLoad)
    action_times.push(time);
  acounter[type]++;
  if (arg >= 0 && arg < 12)
  {
    if (type == ACTION_ASSIGNHOTKEY)
      hkassign[arg]++;
    else if (type == ACTION_SELECTHOTKEY)
      hkuse[arg]++;
  }
}
static void fix_vector(uint32 delta, float x1, float y1, float& x2, float& y2)
{
  if (delta == 0)
  {
    x2 = x1;
    y2 = y1;
  }
  else
  {
    float len = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    if (len > float(delta) * 0.5f)
    {
      len = float(delta) * 0.5f / len;
      x2 = x1 + (x2 - x1) * len;
      y2 = y1 + (y2 - y1) * len;
    }
  }
}
static bool valid_coord(float coord)
{
  return (*(int*)(&coord)) != 0xFFFFFFFF;
}

void W3GActionList::addPointEvent(uint32 time, uint32 id, float x, float y)
{
  if (!quickLoad)
    action_times.push(time);
  if (id == ID_RIGHTCLICK)
  {
    if (!quickLoad && valid_coord(x) && valid_coord(y))
    {
      int prev = actions.length() - 1;
      if (prev >= 0 && actions[prev].time < time)
      {
        if (prev > 0)
          fix_vector(time - actions[prev].time, actions[prev - 1].x, actions[prev - 1].y,
            actions[prev].x, actions[prev].y);
        Movement& m = actions.push();
        m.state = STATE_ALIVE;
        m.time = time;
        m.x = x;
        m.y = y;
      }
      if (time > spawn_time && time < spawn_time + 180000)
      {
        lane_count++;
        lane_x += x;
        lane_y += y;
      }
    }
    acounter[ACTION_RIGHTCLICK]++;
  }
  else if (id <= ID_HOLD)
    acounter[ACTION_BASIC]++;
  else if (id >= ID_GIVE && id <= ID_USEITEM6)
    acounter[ACTION_ITEM]++;
  else
    acounter[ACTION_ABILITY]++;
}
void W3GActionList::setTeam(uint32 time, int team)
{
  float x = team_x[team];
  float y = team_y[team];
  home_x = x;
  home_y = y;
  if (!quickLoad)
  {
    int prev = actions.length() - 1;
    if (prev > 0)
    {
      if (actions[prev - 1].state == STATE_DEAD)
      {
        actions[prev - 1].x = x;
        actions[prev - 1].y = y;
        actions[prev].x = x;
        actions[prev].y = y;
        return;
      }
      else
        fix_vector(time - actions[prev].time, actions[prev - 1].x, actions[prev - 1].y,
          actions[prev].x, actions[prev].y);
    }
    Movement& m = actions.push();
    m.state = STATE_ALIVE;
    m.time = time;
    m.x = x;
    m.y = y;
    actions.push(m);
  }
}
void W3GActionList::addDeath(uint32 time, uint32 length)
{
  if (!quickLoad)
  {
    int prev = actions.length() - 1;
    if (prev > 0)
      fix_vector(time - actions[prev].time, actions[prev - 1].x, actions[prev - 1].y,
        actions[prev].x, actions[prev].y);
    Movement& m1 = actions.push();
    m1.state = STATE_DEAD;
    m1.time = time;
    m1.x = home_x;
    m1.y = home_y;
    Movement& m2 = actions.push();
    m2.state = STATE_ALIVE;
    m2.time = time + length;
    m2.x = home_x;
    m2.y = home_y;
  }
}

int W3GActionList::getPosition(uint32 time, float& x, float& y) const
{
  int left = 0;
  int right = actions.length() - 1;
  while (left < right)
  {
    int mid = (left + right + 1) / 2;
    if (actions[mid].time > time)
      right = mid - 1;
    else
      left = mid;
  }
  if (actions[left].state == STATE_DEAD)
    return STATE_DEAD;
  x = actions[left].x;
  y = actions[left].y;
  if (left > 0)
    fix_vector(time - actions[left].time, actions[left - 1].x, actions[left - 1].y, x, y);
  return STATE_ALIVE;
}

int W3GActionList::getLane() const
{
  float x = lane_x;
  float y = lane_y;
  if (lane_count)
  {
    x /= lane_count;
    y /= lane_count;
    if ((x > 4000 && y < -5000) || (x > -1500 && y < -6000) || (x > 5500 && y < 1000))
      return LANE_BOTTOM;
    if ((x < -4500 && y > 3500) || (x < -5500 && y > -3000) || (x < 1000 && y > 4500))
      return LANE_TOP;
    if ((x + y > -7000 && x + y < 4000 && x - y > -1000 && x - y < 2000) ||
        (x + y > -2500 && x + y < 1000 && x - y > -1500 && x - y < 2300))
      return LANE_MIDDLE;
    return LANE_ROAMING;
  }
  else
    return LANE_AFK;
}
