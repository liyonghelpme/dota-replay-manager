#ifndef __REPLAY_MESSAGE__
#define __REPLAY_MESSAGE__

#define CHAT_ALL              0x00
#define CHAT_ALLIES           0x01
#define CHAT_OBSERVERS        0x02
#define CHAT_PRIVATE          0x03
#define CHAT_COMMAND          0x7C
#define CHAT_ASCOMMAND        0x80
#define CHAT_PING             0x7E
#define CHAT_NOTIFY           0x7F
#define CHAT_NONE             0x7D

#define CHAT_NOTIFY_LEAVER    1
#define CHAT_NOTIFY_PAUSE     2
#define CHAT_NOTIFY_CONTROL   3
#define CHAT_NOTIFY_KILL      4
#define CHAT_NOTIFY_TOWER     5
#define CHAT_NOTIFY_BARRACKS  6
#define CHAT_NOTIFY_COURIER   7
#define CHAT_NOTIFY_TREE      8
#define CHAT_NOTIFY_ROSHAN    9
#define CHAT_NOTIFY_AEGIS     10
#define CHAT_NOTIFY_GAMEMODE  11
#define CHAT_NOTIFY_RUNE      12
#define CHAT_NOTIFY_PICKS     13
#define CHAT_NOTIFY_FASTKILL  14
#define CHAT_NOTIFY_SPREE     15

struct W3GMessage
{
  uint8 id;
  uint32 mode;
  uint32 notifyType;
  float x;
  float y;
  uint32 time;
  String text;
  int line;

  W3GMessage()
  {
    id = 0;
    mode = 0;
    notifyType = 0;
    x = 0;
    y = 0;
    time = 0;
    line = -1;
  }
};

#endif // __REPLAY_MESSAGE__
