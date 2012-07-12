#ifndef __REPLAY_TIMELINE__
#define __REPLAY_TIMELINE__

#include "base/types.h"
#include "base/array.h"
#include "replay/consts.h"

#define STATE_ALIVE     0
#define STATE_DEAD      1

class W3GActionList
{
  bool quickLoad;
  uint32 spawn_time;
  Array<uint32> action_times;
  struct Movement
  {
    uint32 state;
    uint32 time;
    float x;
    float y;
  };
  Array<Movement> actions;
  int acounter[NUM_ACTIONS];
  int hkassign[16];
  int hkuse[16];
  int lane_count;
  float lane_x;
  float lane_y;
  float home_x;
  float home_y;
public:
  W3GActionList();
  ~W3GActionList()
  {
  }
  void start(bool quickLoad, int team);
  void setTeam(uint32 time, int team);
  void addEvent(uint32 time, int type, int arg = 0);
  void addPointEvent(uint32 time, uint32 id, float x, float y);
  void addDeath(uint32 time, uint32 length);

  void setSpawnTime(uint32 time)
  {
    spawn_time = time;
  }

  int count() const
  {
    return action_times.length();
  }
  uint32 operator [](int i) const
  {
    return action_times[i];
  }

  int getPosition(uint32 time, float& x, float& y, float* dx = NULL, float* dy = NULL) const;
  int getActionCounter(int type) const
  {
    return acounter[type];
  }
  int getHotkeyAssign(int key) const
  {
    return hkassign[key];
  }
  int getHotkeyUse(int key) const
  {
    return hkuse[key];
  }

  int getLane() const;
};

struct W3GPlayer;
struct W3GWard
{
  float x;
  float y;
  W3GPlayer* player;
  uint32 time;
  void set(float _x, float _y, W3GPlayer* _player, uint32 _time)
  {
    x = _x;
    y = _y;
    player = _player;
    time = _time;
  }
};

#endif // __REPLAY_TIMELINE__
