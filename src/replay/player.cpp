#include "replay/consts.h"
#include "replay/replay.h"

#include "player.h"

W3GSlot::W3GSlot(File* f, W3GHeader const& hdr)
{
  memset(this, 0, sizeof(W3GSlot));
  slot_status = f->getc();
  computer = f->getc();
  team = f->getc();
  color = f->getc();
  race = convert_race(f->getc());
  org_color = color;
  if (hdr.major_v >= 3)
    ai_strength = f->getc();
  if (hdr.major_v >= 7)
    handicap = f->getc();
}
void W3GPlayer::init()
{
  initiator = false;

  race = 0;

  time = 0;
  left = false;
  leave_reason = 0;
  leave_result = 0;

  hero = NULL;
  sel = NULL;
  heroId = 0;
  sdied = 0;
  skilled = 0;
  level = 0;
  lane = 0;
  item_cost = 0;

  said_ff = false;
  memset(&share, 0, sizeof share);
  memset(&slot, 0, sizeof slot);
  memset(&stats, 0, sizeof stats);
  memset(&pkilled, 0, sizeof pkilled);
  memset(&pdied, 0, sizeof pdied);
  memset(&level_time, 0, sizeof level_time);
}
W3GPlayer::W3GPlayer(uint8 id, String n)
{
  player_id = id;
  org_name = name = n;

  init();
}
W3GPlayer::W3GPlayer(File* f)
{
  player_id = f->getc();
  name = f->readString();
  if (name.isEmpty())
    name.printf("Player %d", player_id);
  org_name = name;

  init();

  uint8 key = f->getc();
  if (key == 1)
    f->seek(1, SEEK_CUR);
  else if (key == 8)
    f->seek(8, SEEK_CUR);
}

String W3GPlayer::format() const
{
  return String::format("%s [%d-%d]", name, stats[STAT_KILLS], stats[STAT_DEATHS]);
}
String W3GPlayer::format_full() const
{
  return String::format("@%d|%d@ [%d-%d]", player_id, slot.color, stats[STAT_KILLS], stats[STAT_DEATHS]);
}
