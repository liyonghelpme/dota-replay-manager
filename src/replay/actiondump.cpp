#include "actiondump.h"

#include "replay/orders.h"
#include "base/utils.h"
#include <stdio.h>

void FileActionLogger::addAction(uint32 time, W3GPlayer* player, String text)
{
  static char buf[256];
  sprintf(buf, "%6s ", format_time(time));
  log->write(buf, strlen(buf));
  sprintf(buf, "%-16s ", player ? player->org_name : "");
  log->write(buf, strlen(buf));
  log->write(text.c_str(), text.length());
  log->write("\r\n", 2);
}

ActionDumper::ActionDumper(W3GReplay* replay, MapData* data)
{
  w3g = replay;
  version = vGetMinor(replay->getVersion());
  parser = new W3GActionParser(version);
  mapData = data;
  ownData = false;
}
ActionDumper::~ActionDumper()
{
  delete parser;
  if (ownData)
    delete mapData;
}
void ActionDumper::dump(ActionLogger* log, int detail)
{
  if (detail == DUMP_FULLNAMES && mapData == NULL)
  {
    mapData = new MapData(w3g->getGameInfo()->map);
    if (!mapData->isLoaded())
    {
      delete mapData;
      mapData = NULL;
      detail = DUMP_FULL;
    }
    else
      ownData = true;
  }

  time = 0;
  pause = false;
  player = NULL;
  this->detail = detail;
  logger = log;
  parseBlocks(w3g->getFile());
}

void ActionDumper::parseBlocks(File* file)
{
  while (!file->eof())
  {
    int block_id = file->getc();
    switch (block_id)
    {
    // TimeSlot block
    case 0x1E:
    case 0x1F:
      {
        int length = file->read_int16();
        int time_inc = file->read_int16();
        if (!pause)
          time += time_inc;
        if (length > 2)
          parseActions(file, length - 2);
      }
      break;
    // Player chat message (version >= 1.07)
    case 0x20:
      if (version > 2)
      {
        uint8 id = file->getc();
        int length = file->read_int16();
        int end = file->tell() + length;
        uint8 flags = file->getc();
        if (detail != DUMP_SIMPLE && flags == 0x20)
        {
          uint32 mode = file->read_int32();
          String text;
          text.resize(length - 6);
          file->read(text.getBuffer(), length - 6);
          text.setLength(length - 6);

          player = w3g->getPlayerById(id);
          if (mode == CHAT_ALL)
            logger->addAction(time, player, "[All] " + text);
          else if (mode == CHAT_ALLIES)
            logger->addAction(time, player, "[Allies] " + text);
          else if (mode == CHAT_OBSERVERS)
            logger->addAction(time, player, "[Observers] " + text);
          else if (mode >= CHAT_PRIVATE)
            logger->addAction(time, player, "[Private] " + text);
        }
        file->seek(end, SEEK_SET);
      }
      break;
    // unknown (Random number/seed for next frame)
    case 0x22:
      file->seek(file->getc(), SEEK_CUR);
      break;
    // unknown (startblocks)
    case 0x1A:
    case 0x1B:
    case 0x1C:
      file->seek(4, SEEK_CUR);
      break;
    // unknown (very rare, appears in front of a 'LeaveGame' action)
    case 0x23:
      file->seek(10, SEEK_CUR);
      break;
    // Forced game end countdown (map is revealed)
    case 0x2F:
      file->seek(8, SEEK_CUR);
      break;
    // LeaveGame
    case 0x17:
      {
        int reason = file->read_int32();
        uint8 id = file->getc();
        int result = file->read_int32();
        int unknown = file->read_int32();

        if (detail != DUMP_SIMPLE)
          logger->addAction(time, w3g->getPlayerById(id), "Left the game");
      }
      break;
    case 0:
      file->seek(0, SEEK_END);
      break;
    }
  }
}
void ActionDumper::parseActions(File* file, int length)
{
  int end = file->tell() + length;
  while (file->tell() < end)
  {
    uint8 id = file->getc();
    player = w3g->getPlayerById(id);
    int block = file->read_int16();
    int next = file->tell() + block;

    while (file->tell() < next)
    {
      uint8 type = parser->parse(file);
      if (type == 0x01)
        pause = true;
      else if (type == 0x02)
        pause = false;

      if (detail == DUMP_SIMPLE)
        logger->addAction(time, player, parser->print());
      else
      {
        switch (type)
        {
        // Pause game
        case 0x01:
        // Resume game
        case 0x02:
          add("%n");
          break;
        // Set game speed in single player game (options menu)
        case 0x03:
          add("Set game speed to %i");
          break;
        // Increase game speed in single player game (Num+)
        case 0x04:
        // Decrease game speed in single player game (Num-)
        case 0x05:
          add("%n");
          break;
        // Save game
        case 0x06:
          add("Saving game to %s");
          break;
        // Save game finished
        case 0x07:
          add("%n");
          break;
        // Unit/building ability (no additional parameters)
        case 0x10:
          add("Immediate order: %a1 (flags: %x0)");
          break;
        // Unit/building ability (with target position)
        case 0x11:
          if (parser->arg_int(3) == -1 && parser->arg_int(4) == -1)
            add("Point order: %a1 (flags: %x0)");
          else
            add("Point order: %a1 (X: %f3, Y: %f4, flags: %x0)");
          break;
        // Unit/building ability (with target position and target object ID)
        case 0x12:
          if (parser->arg_int64(5) == -1)
          {
            if (parser->arg_int(3) == -1 && parser->arg_int(4) == -1)
              add("Target order: %a1 (flags: %x0)");
            else
              add("Target order: %a1 (X: %f3, Y: %f4, flags: %x0)");
          }
          else if (parser->arg_int(3) == -1 && parser->arg_int(4) == -1)
            add("Target order: %a1 (Target: %Y5, flags: %x0)");
          else
            add("Target order: %a1 (X: %f3, Y: %f4, Target: %Y5, flags: %x0)");
          break;
        // Give item to Unit / Drop item on ground
        case 0x13:
          if (parser->arg_int64(5) == -1)
          {
            if (parser->arg_int(3) == -1 && parser->arg_int(4) == -1)
              add("Target+object order: %a1 (Object: %Y6, flags: %x0)");
            else
              add("Target+object order: %a1 (X: %f3, Y: %f4, Object: %Y6, flags: %x0)");
          }
          else if (parser->arg_int(3) == -1 && parser->arg_int(4) == -1)
            add("Target+object order: %a1 (Target: %Y5, Object: %Y6, flags: %x0)");
          else
            add("Target+object order: %a1 (X: %f3, Y: %f4, Target: %Y5, Object: %Y6, flags: %x0)");
          break;
        // Unit/building ability (with two target positions and two item IDs)
        case 0x14:
          if (parser->arg_int(3) == -1 && parser->arg_int(4) == -1)
          {
            if (parser->arg_int(7) == -1 && parser->arg_int(8) == -1)
              add("Two points order: %a1, %a5, flags: %x0");
            else
              add("Two points order: %a1, %a5 (X: %f7, Y: %f8), flags: %x0");
          }
          else if (parser->arg_int(7) == -1 && parser->arg_int(8) == -1)
            add("Two points order: %a1 (X: %f3, Y: %f4), %a5, flags: %x0");
          else
            add("Two points order: %a1 (X: %f3, Y: %f4), %a5 (X: %f7, Y: %f8), flags: %x0");
          break;
        // Change Selection (Unit, Building, Area)
        case 0x16:
          if (parser->arg_int(1) == 0)
          {
            if (parser->arg_int(0) == 0x01)
              add("Add to selection");
            else
              add("Remove from selection");
          }
          else if (parser->arg_int(0) == 0x01)
            add("Add to selection: %Y2" + String(", %Y") * (parser->arg_int(1) - 1));
          else
            add("Remove to selection: %Y2" + String(", %Y") * (parser->arg_int(1) - 1));
          break;
        // Assign Group Hotkey
        case 0x17:
          if (parser->arg_int(1) == 0)
            add("Assign group %c");
          else
            add("Assign group %c: %Y2" + String(", %Y") * (parser->arg_int(1) - 1));
          break;
        // Select Group Hotkey
        case 0x18:
          add("Select group %c");
          break;
        // Select Subgroup
        case 0x19:
          if (version >= 14)
            add("Select subgroup: %a, %Y");
          else
            add("Select subgroup: %i");
          break;
        // Pre Subselection
        case 0x1A:
          add("%n");
          break;
        // Select Ground Item
        case 0x1C:
          add("Select ground item %Y1");
          break;
        // Cancel hero revival
        case 0x1D:
          add("Cancel hero revival %Y");
          break;
        // Remove unit from building queue
        case 0x1E:
          if (parser->arg_int(0) == 0)
            add("Cancel training/upgrading %a1");
          else
            add("Remove unit/upgrade #%i from queue (%a)");
          break;
        // Single Player Cheats
        case 0x20:
        case 0x22:
        case 0x23:
        case 0x24:
        case 0x25:
        case 0x26:
          add("%n");
          break;
        case 0x27:
          add("Cheat: KeyserSoze (%i1 Gold)");
          break;
        case 0x28:
          add("Cheat: LeafitToMe (%i1 Lumber)");
          break;
        case 0x29:
        case 0x2A:
        case 0x2B:
        case 0x2C:
          add("%n");
          break;
        case 0x2D:
          add("Cheat: GreedIsGood (%i1 Gold and Lumber)");
          break;
        case 0x2E:
          add("Cheat: DayLightSavings (Set a time of day to %f)");
          break;
        case 0x2F:
        case 0x30:
        case 0x31:
        case 0x32:
          add("%n");
          break;
        // Change ally options
        case 0x50:
          {
            uint32 flags = parser->arg_int(1);
            String opts = "";
            if (flags & 0x1F)
            {
              if (!opts.isEmpty()) opts += ", ";
              opts += "allied";
            }
            if (flags & 0x20)
            {
              if (!opts.isEmpty()) opts += ", ";
              opts += "vision";
            }
            if (flags & 0x40)
            {
              if (!opts.isEmpty()) opts += ", ";
              opts += "control";
            }
            if (flags & 0x400)
            {
              if (!opts.isEmpty()) opts += ", ";
              opts += "victory";
            }
            if (opts.isEmpty())
              opts = "enemy";
            add("Change ally options towards %p: " + opts);
          }
          break;
        // Transfer resources
        case 0x51:
          add("Send %i1 gold and %i2 lumber to %p0");
          break;
        // Map trigger chat command (?)
        case 0x60:
          add("[Chat command] %s1");
          break;
        // ESC pressed
        case 0x61:
          add("%n");
          break;
        // Minimap signal (ping)
        case 0x68:
          add("Map ping (X: %f, Y: %f)");
          break;
        // SyncStoredInteger actions (endgame action)
        case 0x6B:
          add("SyncStoredInteger (%s, %s, %s, %u)", true);
          break;
        // SyncStored? action
        case 0x70:
          add("SyncStored? (%s, %s, %s)", true);
          break;
        }
      }
    }

    file->seek(next, SEEK_SET);
  }
  file->seek(end, SEEK_SET);
}
void ActionDumper::add(char const* fmt, bool noPlayer)
{
  int last_arg = -1;
  String text = String::format("0x%02X: ", parser->type());
  while (*fmt)
  {
    if (*fmt == '%')
    {
      fmt++;
      char c = *fmt++;
      int arg = 0;
      if (c != 'n')
      {
        if (*fmt >= '0' && *fmt <= '9')
        {
          while (*fmt >= '0' && *fmt <= '9')
            arg = arg * 10 + int(*fmt++ - '0');
        }
        else
          arg = last_arg + 1;
        last_arg = arg;
      }

      if (c == 'n')
        text += parser->name();
      else if (c == 'a')
      {
        uint32 id = parser->arg_int(arg);
        UnitData* unit = NULL;
        if (mapData)
          unit = mapData->getData()->getUnitById(id);
        if (unit || isGoodId(id))
        {
          if (unit && detail == DUMP_FULLNAMES)
            text += String::format("[%s %s]", id2String(id),
              String(unit->getStringData("Name", 0)).toAnsi());
          else
            text += String::format("[%s]", id2String(id));
        }
        else
        {
          char const* oid = orderId2String(id);
          if (oid)
            text += oid;
          else
            text += String::format("0x%08X", id);
        }
      }
      else if (c == 'u')
      {
        uint32 id = parser->arg_int(arg);
        UnitData* unit = NULL;
        if (mapData)
          unit = mapData->getData()->getUnitById(id);
        if (unit || isGoodId(id))
        {
          if (unit && detail == DUMP_FULLNAMES)
            text += String::format("[%s %s]", id2String(id),
              String(unit->getStringData("Name", 0)).toAnsi());
          else
            text += String::format("[%s]", id2String(id));
        }
        else
          text += String((int) id);
      }
      else if (c == 'i')
        text += String((int) parser->arg_int(arg));
      else if (c == 'f')
        text += String::format("%.2f", parser->arg_float(arg));
      else if (c == 's')
        text += parser->arg_string(arg);
      else if (c == 'p')
      {
        W3GPlayer* p = w3g->getPlayerById(parser->arg_int(arg));
        if (p)
          text += p->name;
        else
          text += "(unknown)";
      }
      else if (c == 'c')
      {
        int key = parser->arg_int(arg);
        if (c == 9)
          text += '0';
        else
          text += char(key + '1');
      }
      else if (c == 'x')
        text += String::format("0x%04X", parser->arg_int(arg));
      else if (c == 'X')
        text += String::format("0x%08X", parser->arg_int(arg));
      else if (c == 'Y')
      {
        uint64 i64 = parser->arg_int64(arg);
        text += String::format("0x%08X%08X", int(i64 >> 32), int(i64));
      }
    }
    else
      text += *fmt++;
  }

  logger->addAction(time, noPlayer ? NULL : player, text);
}
