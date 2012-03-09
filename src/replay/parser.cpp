#include <stdarg.h>
#include <stdio.h>

#include "core/app.h"
#include "base/utils.h"

#include "replay.h"
#include "replay/action.h"

class ParseState
{
  File* file;
public:
  bool pause;
  bool continue_game;
  uint32 time;

  W3GPlayer* last_kill_target;
  int last_kill_message;
  W3GPlayer* last_kill_player;
  bool last_kill_first;
  uint32 last_kill_time;

  uint32 last_kill[16];
  int quick_kill[16];
  int spree[16];
  int runestore[16];
  W3GPlayer* playerid[16];

  bool switch_accept[16];

  W3GActionParser* parser;

  ParseState(bool quick);
  ~ParseState();

  void log(W3GPlayer* player);
};
ParseState::ParseState(bool quick)
{
  memset(this, NULL, sizeof ParseState);

  Registry* reg = getApp()->getRegistry();
  if (!quick && reg->readInt("useLog"))
    file = File::open("log.txt", File::REWRITE);

}
ParseState::~ParseState()
{
  delete parser;
  delete file;
}
void ParseState::log(W3GPlayer* player)
{
  if (file == NULL)
    return;
  String str = "";
  str.printf("%6s ", format_time(time));
  if (player)
    str.printf("Player: %-16s ", player->org_name);
  file->write(str.getBuffer(), str.length());
  str = parser->print();
  file->write(str.getBuffer(), str.length());
  file->putc('\r');
  file->putc('\n');
}

bool W3GReplay::parseBlocks()
{
  int prev = 0;

  int leaves = 0;
  int leave_unknown = 0;
  ParseState state(quickLoad);
  state.parser = new W3GActionParser(hdr.major_v);
  for (int i = 0; i < numPlayers; i++)
    state.playerid[plist[i]->slot.org_color] = plist[i];

  while (!replay->eof())
  {
    int block_id = replay->getc();
    switch (block_id)
    {
    // TimeSlot block
    case 0x1E:
    case 0x1F:
      {
        int length = replay->read_int16();
        int time_inc = replay->read_int16();
        if (!state.pause)
          state.time += time_inc;
        if (length > 2 && !parseActions(length - 2, &state))
          return false;
      }
      break;
    // Player chat message (version >= 1.07)
    case 0x20:
      if (hdr.major_v > 2)
      {
        uint8 id = replay->getc();
        int length = replay->read_int16();
        int end = replay->tell() + length;
        if (!quickLoad)
        {
          uint8 flags = replay->getc();
          if (flags == 0x20)
          {
            W3GMessage& msg = messages.push();
            msg.id = id;
            msg.mode = replay->read_int32();
            msg.text.resize(length - 6);
            replay->read(msg.text.getBuffer(), length - 6);
            msg.text.setLength(length - 6);
            msg.time = state.time;

            if (msg.text.find("ff", 0, FIND_CASE_INSENSITIVE))
              players[id]->said_ff = true;
          }
        }
        replay->seek(end, SEEK_SET);
      }
      break;
    // unknown (Random number/seed for next frame)
    case 0x22:
      replay->seek(replay->getc(), SEEK_CUR);
      break;
    // unknown (startblocks)
    case 0x1A:
    case 0x1B:
    case 0x1C:
      replay->seek(4, SEEK_CUR);
      break;
    // unknown (very rare, appears in front of a 'LeaveGame' action)
    case 0x23:
      replay->seek(10, SEEK_CUR);
      break;
    // Forced game end countdown (map is revealed)
    case 0x2F:
      replay->seek(8, SEEK_CUR);
      break;
    // LeaveGame
    case 0x17:
      {
        leaves++;
        int reason = replay->read_int32();
        uint8 id = replay->getc();
        int result = replay->read_int32();
        int unknown = replay->read_int32();

        players[id]->time = state.time;
        players[id]->left = true;
        players[id]->leave_reason = reason;
        players[id]->leave_result = result;

        if (leave_unknown)
          leave_unknown = unknown - leave_unknown;
        if (leaves == numPlayers)
          game->saver = players[id];
        else if (!quickLoad)
          addMessage(CHAT_NOTIFY_LEAVER, id, state.time, "%s has left the game.",
            players[id]->format_full());
        if (reason == 0x01)
        {
          switch (result)
          {
          case 0x08: game->ladder_lost[players[id]->slot.team] = true; break;
          case 0x09: game->ladder_winner = players[id]->slot.team; break;
          }
        }
        else if (reason == 0x0C && game->saver)
        {
          switch (result)
          {
          case 0x07:
            if (leave_unknown > 0 && state.continue_game)
              game->ladder_winner = game->saver->slot.team;
            else
              game->ladder_lost[game->saver->slot.team] = true;
            break;
          case 0x08: game->ladder_lost[game->saver->slot.team] = true; break;
          case 0x09: game->ladder_winner = game->saver->slot.team; break;
          case 0x0B:
            if (leave_unknown > 0)
              game->ladder_winner = game->saver->slot.team;
            break;
          }
        }
        else if (reason == 0x0C)
        {
          switch (result)
          {
          case 0x07: game->ladder_lost[15] = true; break;
          case 0x08: game->ladder_winner = players[id]->slot.team; break;
          case 0x09: game->ladder_winner = 15;
          }
        }
        leave_unknown = unknown;
      }
      break;
    case 0:
      replay->seek(0, SEEK_END);
      break;
    }
    prev = block_id;
  }
  game->end_time = state.time;
  return true;
}
bool W3GReplay::parseActions(int length, void* arg)
{
  ParseState& state = * (ParseState*) arg;
  W3GActionParser* parser = state.parser;
  int end = replay->tell() + length;
  while (replay->tell() < end)
  {
    uint8 id = replay->getc();
    W3GPlayer* player = players[id];
    int length = replay->read_int16();
    int next = replay->tell() + length;

    bool was_deselect = false;
    bool was_subgroup = false;
    uint8 prev = 0;

    while (replay->tell() < next)
    {
      uint8 type = parser->parse(replay);
      state.log(player);
      switch (type)
      {
      // Pause game
      case 0x01:
        state.pause = true;
        if (!quickLoad)
          addMessage(CHAT_NOTIFY_PAUSE, id, state.time, "%s has paused the game.", player->name);
        break;
      // Resume game
      case 0x02:
        state.pause = false;
        if (!quickLoad)
          addMessage(CHAT_NOTIFY_PAUSE, id, state.time, "%s has resumed the game.", player->name);
        break;
      // Save game
      case 0x06:
        if (!quickLoad)
          addMessage(CHAT_NOTIFY_PAUSE, id, state.time, "%s has saved the game to %s.",
            player->name, parser->arg_string(0));
        break;
      // Unit/building ability (no additional parameters)
      case 0x10:
        {
          int flags = parser->arg_int(0);
          uint32 itemid = parser->arg_int(1);

          Dota::Object object;
          dota->getObjectById(itemid, &object);

          player->actions.addEvent(state.time, ACTION_OTHER);
          if (player->sel)
            player->sel->actions[player->index]++;

          if (object.type == OBJECT_ABILITY)
          {
            if (player->hero && player->hero->hero->hasAbility(object.ability))
              player->sel = player->hero;
            if (player->sel == NULL || !player->sel->hero->hasAbility(object.ability))
            {
              for (int i = 0; i < heroes.length(); i++)
              {
                if (heroes[i].hero == object.ability->hero)
                {
                  player->sel = &heroes[i];
                  break;
                }
              }
            }
            if (player->sel && player->sel->hero->hasAbility(object.ability))
              player->sel->pushAbility(object.ability, state.time);
          }
          else if (object.type == OBJECT_ITEM)
          {
            if (object.item->cost != 0)
              player->inv.addItem(object.item, state.time);
          }
        }
        break;
      // Unit/building ability (with target position)
      case 0x11:
        {
          int flags = parser->arg_int(0);
          uint32 itemid = parser->arg_int(1);
          __int64 unk = parser->arg_int64(2, -1);
          uint32 checkx = parser->arg_int(3);
          uint32 checky = parser->arg_int(4);
          float targx = parser->arg_float(3);
          float targy = parser->arg_float(4);

          if (player->inv.wards && unk != -1 && checkx != -1 && checky != -1 &&
            itemid >= ID_USEITEM1 && itemid <= ID_USEITEM6)
          {
            player->inv.wards--;
            if (!quickLoad)
              wards.push().set(targx, targy, player, state.time);
          }

          player->actions.addPointEvent(state.time, itemid, targx, targy);
          if (player->sel)
            player->sel->actions[player->index]++;
        }
        break;
      // Unit/building ability (with target position and target object ID)
      case 0x12:
      // Give item to Unit / Drop item on ground
      case 0x13:
      // Unit/building ability (with two target positions and two item IDs)
      case 0x14:
        player->actions.addPointEvent(state.time, parser->arg_int(1),
          parser->arg_float(3), parser->arg_float(4));
        if (player->sel)
          player->sel->actions[player->index]++;
        break;
      // Change Selection (Unit, Building, Area)
      case 0x16:
        {
          uint8 mode = parser->arg_int(0);
          int count = parser->arg_int(1);

          if (mode == 0x02 || !was_deselect)
            player->actions.addEvent(state.time, ACTION_SELECT);
          was_deselect = (mode == 0x02);
        }
        break;
      // Assign Group Hotkey
      case 0x17:
        player->actions.addEvent(state.time, ACTION_ASSIGNHOTKEY, parser->arg_int(0));
        break;
      // Select Group Hotkey
      case 0x18:
        player->actions.addEvent(state.time, ACTION_SELECTHOTKEY, parser->arg_int(0));
        break;
      // Select Subgroup
      case 0x19:
        if (hdr.major_v >= 14)
        {
          uint32 itemid = parser->arg_int(0);
          uint64 object = parser->arg_int64(1);
          if (player->race == 0)
            player->race = convert_race (itemid);
          Dota::Hero* hero = dota->getHeroById(itemid);
          if (hero)
          {
            W3GHero* h = getHero(object, player->slot.team, hero);
            if (h)
            {
              player->sel = h;
              h->hero = hero;
              if (player->hero == NULL && player->heroId == itemid)
                player->hero = h;
            }
          }

          if (was_subgroup)
            player->actions.addEvent(state.time, ACTION_SUBGROUP);
        }
        else
        {
          uint8 subgroup = parser->arg_int(0);

          if (subgroup != 0 && subgroup != 0xFF && !was_subgroup)
            player->actions.addEvent(state.time, ACTION_SUBGROUP);
          was_subgroup = (subgroup == 0xFF);
        }
        break;
      // Pre Subselection
      case 0x1A:
        was_subgroup = (prev == 0x19 || prev == 0);
        break;
      // Select Ground Item
      case 0x1C:
      // Cancel hero revival
      case 0x1D:
      // Remove unit from building queue
      case 0x1E:
        player->actions.addEvent(state.time, ACTION_OTHER);
        break;
      // Single Player Cheats
      case 0x20:
      case 0x22:
      case 0x23:
      case 0x24:
      case 0x25:
      case 0x26:
      case 0x27:
      case 0x28:
      case 0x29:
      case 0x2A:
      case 0x2B:
      case 0x2C:
      case 0x2D:
      case 0x2E:
      case 0x2F:
      case 0x30:
      case 0x31:
      case 0x32:
        if (!quickLoad)
          addMessage(0, id, state.time, "%s uses a cheat %s", player->name, parser->name().cut(7));
        break;
      // Change ally options
      case 0x50:
        {
          uint8 slot = parser->arg_int(0);
          uint32 flags = parser->arg_int(1);
          bool share = ((flags & 0x40) != 0);
          if (!quickLoad)
          {
            W3GPlayer* other = getPlayerInSlot(slot);
            if (share && !player->share[slot])
              addMessage(CHAT_NOTIFY_CONTROL, id, state.time, "%s shares control with %s.",
                player->format_full(), other->format_full());
            else
              addMessage(CHAT_NOTIFY_CONTROL, id, state.time, "%s disables control sharing with %s.",
                player->format_full(), other->format_full());
          }
          player->share[slot] = share;
        }
        break;
      // Transfer resources
      case 0x51:
        if (!quickLoad)
        {
          uint8 slot = parser->arg_int(0);
          uint32 gold = parser->arg_int(1);
          uint32 lumber = parser->arg_int(2);
          W3GPlayer* other = getPlayerInSlot (slot);
          if (other)
            addMessage(0, id, state.time, "%s sends %s %d gold and %d lumber.",
              player->format_full(), other->format_full(), gold, lumber);
        }
        break;
      // Map trigger chat command (?)
      case 0x60:
        {
          char const* text = parser->arg_string(1);
          if (!quickLoad)
          {
            int cur = messages.length() - 1;
            for (int count = 0; cur >= 0 && count < 10; cur--, count++)
              if (messages[cur].id == id)
                break;
            if (cur < 0 || messages[cur].id != id || messages[cur].text != text)
            {
              W3GMessage& msg = messages.push();
              msg.id = id;
              msg.mode = CHAT_COMMAND;
              msg.text = text;
              msg.time = state.time;

              if (msg.text.find("ff", 0, FIND_CASE_INSENSITIVE))
                player->said_ff = true;
            }
          }
          if (state.time < 15000)
            parseMode(player, text);

          // handle -switch
          if ((game->gmode & MODE_SO) && !game->has_observers)
          {
            if (dotaInfo->switch_who != 0 && state.time - dotaInfo->switch_start < 60000)
            {
              if (!strcmp(text, "-ok") || !strcmp(text, "-switch accept"))
              {
                state.switch_accept[player->index] = true;
                int bad = 0;
                int good = 0;
                int total = 0;
                for (int i = 0; i < numPlayers; i++)
                {
                  if (!plist[i]->left && !plist[i]->slot.computer)
                  {
                    total++;
                    if (state.switch_accept[i])
                      good++;
                    else
                      bad++;
                  }
                }
                if (bad < 2 && good >= total / 2 && (good > 1 || total != 2))
                {
                  addMessage(0, 0, state.time,
                    "The switch process between %s and %s has been completed successfully",
                    dotaInfo->switch_who->format_full(), dotaInfo->switch_with->format_full());
                  uint8 temp = dotaInfo->switch_who->slot.color;
                  dotaInfo->switch_who->slot.color = dotaInfo->switch_with->slot.color;
                  dotaInfo->switch_with->slot.color = temp;
                  temp = dotaInfo->switch_who->slot.team;
                  dotaInfo->switch_who->slot.team = dotaInfo->switch_with->slot.team;
                  dotaInfo->switch_with->slot.team = temp;

                  dotaInfo->switch_who->actions.setTeam(state.time, dotaInfo->switch_who->slot.team);
                  dotaInfo->switch_with->actions.setTeam(state.time, dotaInfo->switch_with->slot.team);
                  dotaInfo->switch_who = NULL;
                }
              }
              else if (!strcmp(text, "-no"))
                dotaInfo->switch_who = NULL;
            }
            else
            {
              if (!strncmp(text, "-switch ", 8) && text[8] >= '1' && text[8] <= '5')
              {
                memset(state.switch_accept, 0, sizeof state.switch_accept);
                state.switch_accept[player->index] = true;
                int slot = int(text[8] - '0');
                if (player->slot.color < 6)
                  slot += 6;
                if (dotaInfo->switch_with = getPlayerInSlot(slot))
                {
                  dotaInfo->switch_who = player;
                  dotaInfo->switch_start = state.time;
                }
              }
            }
          }
        }
        break;
      // ESC pressed
      case 0x61:
        player->actions.addEvent(state.time, ACTION_ESC);
        break;
      // Enter select hero skill submenu
      case 0x66:
      // Enter select building submenu
      case 0x67:
        player->actions.addEvent(state.time, ACTION_OTHER);
        break;
      // Minimap signal (ping)
      case 0x68:
        if (!quickLoad)
        {
          W3GMessage& msg = messages.push();
          msg.id = id;
          msg.mode = CHAT_PING;
          msg.time = state.time;
          msg.x = parser->arg_float(0);
          msg.y = parser->arg_float(1);
        }
        break;
      // Continue Game (BlockB)
      case 0x69:
      // Continue Game (BlockA)
      case 0x6A:
        state.continue_game = true;
        break;
      // SyncStoredInteger actions (endgame action)
      case 0x6B:
        parseEndgame(parser->arg_string(1), parser->arg_string(2),
                     parser->arg_int(3), &state);
        break;
      }
      prev = type;
    }
    replay->seek(next, SEEK_SET);
  }
  replay->seek(end, SEEK_SET);
  return true;
}
void W3GReplay::parseMode(W3GPlayer* player, char const* text)
{
  if (player)
    for (int i = 0; i < numPlayers; i++)
      if (plist[i]->slot.color < player->slot.color)
        return;
  if (!strcmp(text, "-wtf"))
  {
    game->gmode |= MODE_WTF;
    return;
  }
  if (game->gmode & (~MODE_WTF))
    return;
  for (int i = 0; text[i] != '-'; i++)
    if (text[i] == 0)
      return;
  String mode = "";
  for (int i = 0; text[i]; i++)
  {
    if (text[i] >= 'a' && text[i] <= 'z')
      mode += text[i];
    else if (text[i] >= 'A' && text[i] <= 'Z')
      mode += char(text[i] + 'a' - 'A');
  }
  game->gmode |= ::parseMode(mode, &game->game_mode);
  if (dotaInfo && dotaInfo->start_time == 0)
  {
    uint32 start = getModeTime(game->gmode);
    for (int i = 0; i < numPlayers; i++)
      plist[i]->actions.setSpawnTime(start);
  }
}
void W3GReplay::parseEndgame(String slot, String stat, uint32 value, void* arg)
{
  ParseState& state = * (ParseState*) arg;
  if (dotaInfo == NULL)
    return;
  dotaInfo->endgame = true;

  if (slot == "Global")
  {
    if (stat == "Winner")
    {
      int id = getBuildingId(2 - value, BUILDING_THRONE, 0, 0);
      if (id >= 0)
        dotaInfo->bdTime[id] = state.time;
      game->winner = value;
      dotaInfo->end_time = state.time;
    }
  }
  else if (slot == "Data")
  {
    static char __side[5][16] = {"Sentinel", "Scourge", "The Sentinel", "The Scourge", "Neutral Creeps"};
    static char __sideid[5][16] = {"s", "u", "n"};
    static char __lane[3][16] = {"top", "middle", "bottom"};
    static char __rax[3][16] = {"melee", "ranged"};
    static char __tact[2][16] = {"destroyed", "denied"};
    if (stat == "GameStart")
    {
      dotaInfo->start_time = state.time;
      for (int i = 0; i < numPlayers; i++)
        plist[i]->actions.setSpawnTime(state.time);
    }
    else if (stat.substring(0, 4) == "Pool")
    {
      Dota::Hero* hero = dota->getHero(value);
      if (hero == NULL)
        hero = dota->getHeroById(value);
      if (hero)
        dotaInfo->draft.pool[dotaInfo->draft.numPool++] = hero;
    }
    else if (stat.substring(0, 3) == "Ban")
    {
      int team = stat.substring(3).toInt();
      team = (team > 5 ? 1 : 0);
      if (dotaInfo->draft.numBans[0] == 0 && dotaInfo->draft.numBans[1] == 0 &&
          dotaInfo->draft.numPicks[0] == 0 && dotaInfo->draft.numPicks[1] == 0)
        dotaInfo->draft.firstPick = team;
      Dota::Hero* hero = dota->getHero(value);
      if (hero == NULL)
        hero = dota->getHeroById(value);
      if (hero)
      {
        dotaInfo->draft.bans[team][dotaInfo->draft.numBans[team]++] = hero;
        if (!quickLoad)
        {
          W3GPlayer* captain = getCaptain(team);
          if (captain)
            addMessage(CHAT_NOTIFY_PICKS, 0, state.time, "%s has banned %s",
              captain->format_full(), hero->shortName);
        }
      }
    }
    else if (stat.substring(0, 4) == "Pick")
    {
      int team = stat.substring(4).toInt();
      team = (team > 5 ? 1 : 0);
      if (dotaInfo->draft.numBans[0] == 0 && dotaInfo->draft.numBans[1] == 0 &&
          dotaInfo->draft.numPicks[0] == 0 && dotaInfo->draft.numPicks[1] == 0)
        dotaInfo->draft.firstPick = team;
      Dota::Hero* hero = dota->getHero(value);
      if (hero == NULL)
        hero = dota->getHeroById(value);
      if (hero)
      {
        dotaInfo->draft.picks[team][dotaInfo->draft.numPicks[team]++] = hero;
        if (!quickLoad)
        {
          W3GPlayer* captain = getCaptain(team);
          if (captain)
            addMessage(CHAT_NOTIFY_PICKS, 0, state.time, "%s has picked %s",
              captain->format_full(), hero->shortName);
        }
      }
    }
    else if (stat.substring(0, 6) == "Assist")
    {
      W3GPlayer* player = state.playerid[stat.substring(6).toInt()];
      W3GPlayer* target = state.playerid[value];
      if (player && target && player->slot.team != target->slot.team)
      {
        if (!quickLoad && state.last_kill_target == target &&
          state.time - state.last_kill_time < 250 && state.last_kill_player != player &&
          getApp()->getRegistry()->readInt("chatAssists"))
        {
          W3GMessage& msg = messages[state.last_kill_message];
          if (state.last_kill_first)
            msg.text += " Assist: ";
          else
            msg.text += "/";
          state.last_kill_first = false;
          msg.text += player->format_full();
        }
        player->stats[STAT_ASSISTS]++;
      }
    }
    else if (stat.substring(0, 4) == "Hero")
    {
      W3GPlayer* player = state.playerid[value];
      W3GPlayer* target = state.playerid[stat.substring(4).toInt()];
      bool kill_counts = false;
      state.last_kill_player = player;
      state.last_kill_target = target;
      state.last_kill_time = state.time;
      state.last_kill_message = messages.length();
      state.last_kill_first = true;

      if (player && target && player->slot.team != target->slot.team)
      {
        if (state.time - state.last_kill[player->index] < 18000)
          state.quick_kill[player->index]++;
        else
          state.quick_kill[player->index] = 1;
        state.last_kill[player->index] = state.time;
        state.spree[player->index]++;
        player->stats[STAT_KILLS]++;
        kill_counts = true;
      }
      if (target)
      {
        if (player != target)
          state.spree[target->index] = 0;
        target->actions.addDeath(state.time, 4000 * target->level);
        target->stats[STAT_DEATHS]++;
      }
      if (!quickLoad)
      {
        if (player && target)
        {
          if (player == target)
            addMessage(CHAT_NOTIFY_KILL, player->player_id, state.time,
              "%s has killed himself.", player->format_full());
          else if (player->slot.team == target->slot.team)
            addMessage(CHAT_NOTIFY_KILL, player->player_id, state.time,
              "%s was killed by his teammate %s.", target->format_full(), player->format_full());
          else
          {
            addMessage(CHAT_NOTIFY_KILL, player->player_id, state.time,
              "%s was been killed by %s.", target->format_full(), player->format_full());
            target->pdied[player->index]++;
            player->pkilled[target->index]++;
          }
        }
        else if (target)
        {
          addMessage(CHAT_NOTIFY_KILL, target->player_id, state.time,
            "%s was killed by @%s@.", target->format_full(), __sideid[value / 6]);
          if (target->slot.team == 1 - (value / 6))
            target->sdied++;
        }
      }
      if (kill_counts)
      {
        int spree = state.spree[player->index];
        int quick = state.quick_kill[player->index];
        if (!quickLoad)
        {
          if (spree >= 3)
            addMessage(CHAT_NOTIFY_SPREE, player->player_id, state.time,
              "%s @m%d@", player->format_full(), spree >= 10 ? 0 : spree);
          if (quick >= 2)
            addMessage(CHAT_NOTIFY_FASTKILL, player->player_id, state.time,
              "%s @k%d@", player->format_full(), quick > 5 ? 5 : quick);
        }
      }
    }
    else if (stat.substring(0, 5) == "Tower")
    {
      W3GPlayer* player = state.playerid[value];
      int side = int(stat[5] - '0');
      int lane = int(stat[7] - '0');
      int level = int(stat[6] - '0');
      int id = getBuildingId(side, BUILDING_TOWER, lane, level);
      if (id >= 0)
      {
        if (level == 4 && dotaInfo->bdTime[id])
          id++;
        dotaInfo->bdTime[id] = state.time;
      }
      if (!quickLoad)
      {
        int act = ((value <= 5) == (side == 0) ? 1 : 0);
        if (player && act == 0)
          player->skilled++;
        if (player)
          addMessage(CHAT_NOTIFY_TOWER, player->player_id, state.time,
            "%s %s level %d tower was %s by %s.", __side[side], __lane[lane],
            level, __tact[act], player->format_full());
        else
          addMessage(CHAT_NOTIFY_TOWER, player->player_id, state.time,
            "%s %s level %d tower was %s by @%s@.", __side[side], __lane[lane],
            level, __tact[act], __sideid[value / 6]);
      }
    }
    else if (stat.substring(0, 3) == "Rax")
    {
      W3GPlayer* player = state.playerid[value];
      int side = int(stat[3] - '0');
      int lane = int(stat[4] - '0');
      int type = int(stat[5] - '0');
      int id = getBuildingId(side, type ? BUILDING_RANGED : BUILDING_MELEE, lane, 0);
      if (id >= 0)
        dotaInfo->bdTime[id] = state.time;
      if (!quickLoad)
      {
        int act = ((value <= 5) == (side == 0) ? 1 : 0);
        if (player && act == 0)
          player->skilled++;
        if (player)
          addMessage(CHAT_NOTIFY_BARRACKS, player->player_id, state.time,
            "%s %s %s barracks were %s by %s.", __side[side], __lane[lane],
            __rax[type], __tact[act], player->format_full());
        else
          addMessage(CHAT_NOTIFY_BARRACKS, player->player_id, state.time,
            "%s %s %s barracks were %s by %s.", __side[side], __lane[lane],
            __rax[type], __tact[act], __sideid[value / 6]);
      }
    }
    else if (stat == "Mode")
    {
      W3GPlayer* player = state.playerid[value];
      if (player)
        game->gmode = (game->gmode & MODE_WTF) | ::parseMode(stat.substr(4), &game->game_mode);
      else
        game->gmode = ::parseMode(stat.substr(4), &game->game_mode);
      if (dotaInfo->start_time == 0)
      {
        uint32 start = getModeTime(game->gmode);
        for (int i = 0; i < numPlayers; i++)
          plist[i]->actions.setSpawnTime(start);
      }
      if (!quickLoad)
      {
        if (player)
          addMessage(CHAT_NOTIFY_GAMEMODE, 0, state.time,
            "%s has selected %s mode", player->format_full(), game->game_mode);
        else
          addMessage(CHAT_NOTIFY_GAMEMODE, 0, state.time,
            "Game mode automatically set to %s", player->format_full(), game->game_mode);
      }
    }
    else if (stat.substring(0, 5) == "Level")
    {
      W3GPlayer* player = state.playerid[value];
      int level = stat.substring(5).toInt();
      if (player)
      {
        while (player->level < level)
          player->level_time[++player->level] = state.time;
      }
    }
    else if (stat.substring(0, 2) == "CK")
    {
      W3GPlayer* player = state.playerid[value];
      Array<String> sub;
      if (player && stat.match("CK(\\d+)D(\\d+)N(\\d+)", &sub))
      {
        player->stats[STAT_CREEPS] = sub[1].toInt();
        player->stats[STAT_DENIES] = sub[2].toInt();
        player->stats[STAT_NEUTRALS] = sub[3].toInt();
      }
    }
    else if (!quickLoad)
    {
      if (stat.substring(0, 7) == "Courier")
      {
        W3GPlayer* player = state.playerid[value];
        W3GPlayer* target = state.playerid[stat.substring(7).toInt()];
        if (target)
        {
          if (player)
            addMessage(CHAT_NOTIFY_COURIER, player->player_id, state.time,
              "%s's courier was killed by %s.", target->format_full(), player->format_full());
          else
            addMessage(CHAT_NOTIFY_COURIER, target->player_id, state.time,
              "%s's courier was killed by @%s@.", target->format_full(), __sideid[value / 6]);
        }
      }
      else if (stat == "Tree")
        addMessage(CHAT_NOTIFY_TREE, 0, state.time, "The World Tree is at %d%%!", value);
      else if (stat == "Throne")
        addMessage(CHAT_NOTIFY_TREE, 0, state.time, "The Frozen Throne is at %d%%!", value);
      else if (stat == "Roshan")
        addMessage(CHAT_NOTIFY_ROSHAN, 0, state.time, "Roshan has been slain by @%s@!", __sideid[value]);
      else if (stat == "AegisOn")
      {
        W3GPlayer* player = state.playerid[value];
        if (player)
          addMessage(CHAT_NOTIFY_AEGIS, 0, state.time,
            "%s has acquired Aegis of the Immortal", player->format_full());
      }
      else if (stat == "AegisOff")
      {
        W3GPlayer* player = state.playerid[value];
        if (player)
          addMessage(CHAT_NOTIFY_AEGIS, 0, state.time,
            "%s has lost the Aegis of the Immortal", player->format_full());
      }
      else if (stat.substring(0, 9) == "RuneStore")
      {
        W3GPlayer* player = state.playerid[value];
        int type = stat.substring(9).toInt();
        if (player)
        {
          state.runestore[player->index] = type;
          addMessage(CHAT_NOTIFY_RUNE, 0, state.time,
            "%s has bottled the @r%d@ rune", player->format_full(), type);
        }
      }
      else if (stat.substring(0, 7) == "RuneUse")
      {
        W3GPlayer* player = state.playerid[value];
        int type = stat.substring(7).toInt();
        if (player)
        {
          if (state.runestore[player->index] == type)
          {
            state.runestore[player->index] = 0;
            addMessage(CHAT_NOTIFY_RUNE, 0, state.time,
              "%s has used the stored @r%d@ rune", player->format_full(), type);
          }
          else
            addMessage(CHAT_NOTIFY_RUNE, 0, state.time,
              "%s has acquired a @r%d@ rune", player->format_full(), type);
        }
      }
    }
  }
  else
  {
    W3GPlayer* player = state.playerid[slot.toInt()];
    if (player == 0)
      return;
    if (stat == "id")
    {
      if (value > 5)
        value++;
      player->slot.color = value;
      player->slot.team = (value > 5 ? 1 : 0);
      player->actions.setTeam(state.time, player->slot.team);
    }
    else
    {
      int col = stat.toInt();
      if (col >= 1 && col <= 7)
        player->stats[col - 1] = value;
      else if (col == 9)
      {
        player->heroId = value;
        if (player->hero == NULL)
        {
          Dota::Hero* hero = dota->getHeroById(value);
          if (hero)
            player->hero = newHero(player->slot.team, hero);
        }
      }
      else if (stat[0] == '8')
      {
        dotaInfo->item_data = true;
        int index = int(stat[2] - '0');
        if (index >= 0 && index <= 5)
          player->inv.final[index] = dota->getItemById(value);
      }
    }
  }
}
