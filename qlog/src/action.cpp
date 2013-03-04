#include "action.h"
#include "base/file.h"
#include <locale>

String idToString(uint32 id)
{
  if ((id & 0xFFFF0000) == 0x000D0000)
    return String::format("0x%08X", id);
  else
  {
    String str = "";
    str += char(id >> 24);
    str += char(id >> 16);
    str += char(id >> 8);
    str += char(id);
    return str;
  }
}

struct ActionType
{
  uint8 type;
  String name;
  int numArgs;
  String argNames[16];
  int argLen[16];
  int argRepeat;
};

static inline int parse_int(char const* str, int& pos)
{
  int value = 0;
  while (str[pos] >= '0' && str[pos] <= '9')
    value = value * 10 + int(str[pos++] - '0');
  return value;
}
static inline int parse_hex(char const* str, int& pos)
{
  int value = 0;
  while ((str[pos] >= '0' && str[pos] <= '9') ||
         (str[pos] >= 'A' && str[pos] <= 'F'))
  {
    value *= 16;
    if (str[pos] >= '0' && str[pos] <= '9')
      value += int(str[pos++] - '0');
    else
      value += 10 + int(str[pos++] - 'A');
  }
  return value;
}
static ActionType* parse_type(uint8 id, char const* type, char const* name, int version)
{
  ActionType* action = new ActionType;
  action->type = id;
  action->name = name;
  action->numArgs = 0;
  action->argRepeat = -1;
  int pos = 0;
  while (type[pos])
  {
    if (type[pos] >= '0' && type[pos] <= '9')
      action->argLen[action->numArgs++] = parse_int(type, pos);
    else if (type[pos] == '$')
    {
      action->argLen[action->numArgs++] = -1;
      pos++;
    }
    else if (type[pos] == '!')
    {
      pos++;
      action->type = parse_hex(type, pos);
    }
    else if (type[pos] == '\'')
    {
      pos++;
      while (type[pos] != '\'')
      {
        if (type[pos] == '\\')
          pos++;
        action->argNames[action->numArgs - 1] += type[pos++];
      }
      pos++;
    }
    else if (type[pos] == '[')
    {
      pos += 2;
      int t = 0;
      if (type[pos] == '!') t = 1;
      else if (type[pos] == '>') t = 2;
      else if (type[pos] == '<') t = 3;
      if (type[++pos] != '=')
        t += 4;
      else
        pos++;
      int value = parse_int(type, pos);
      bool result = true;
      if (t == 0)
        result = (version == value);
      else if (t == 1)
        result = (version != value);
      else if (t == 2)
        result = (version >= value);
      else if (t == 3)
        result = (version <= value);
      else if (t == 6)
        result = (version > value);
      else if (t == 7)
        result = (version < value);
      if (result)
        pos++;
      else
      {
        while (type[pos] != ':')
          pos++;
        pos++;
      }
    }
    else if (type[pos] == '{')
    {
      pos++;
      action->argRepeat = parse_int(type, pos);
      pos++;
      action->argLen[action->numArgs] = parse_int(type, pos);
      break;
    }
    if (type[pos] == ':')
    {
      while (type[pos] != ']')
        pos++;
    }
    if (type[pos] == ']')
      pos++;
    if (type[pos] == ',')
      pos++;
  }
  return action;
}

W3GActionParser::W3GActionParser(int version)
{
  bufSize = 256;
  buffer = new uint8[bufSize];

  memset(types, 0, sizeof types);
  types[0x01] = parse_type(0x01, "", "Pause game", version);
  types[0x02] = parse_type(0x02, "", "Resume game", version);
  types[0x03] = parse_type(0x03, "1,'ispeed'", "Set game speed", version);
  types[0x04] = parse_type(0x04, "", "Increase game speed", version);
  types[0x05] = parse_type(0x05, "", "Decrease game speed", version);
  types[0x06] = parse_type(0x06, "$,'sfile'", "Save game", version);
  types[0x07] = parse_type(0x07, "4", "Save game finished", version);
  types[0x10] = parse_type(0x10, "[v<13?1:2],'iflags',4,'qid',[v>=7?8:0]", "Unit/building ability (no additional parameters)", version);
  types[0x11] = parse_type(0x11, "[v<13?1:2],'iflags',4,'qid',[v>=7?8:0],4,'fx',4,'fy'", "Unit/building ability (with target position)", version);
  types[0x12] = parse_type(0x12, "[v<13?1:2],'iflags',4,'qid',[v>=7?8:0],4,'fx',4,'fy',8,'hobject'", "Unit/building ability (with target position and target object ID)", version);
  types[0x13] = parse_type(0x13, "[v<13?1:2],'iflags',4,'qid',[v>=7?8:0],4,'fx',4,'fy',8,'htarget',8,'hitem'", "Give item to Unit / Drop item on ground", version);
  types[0x14] = parse_type(0x14, "[v<13?1:2],'iflags',4,'qid1',[v>=7?8:0],4,'fx1',4,'fy1',4,'qid2',9,4,'fx2',4,'fy2'", "Unit/building ability (with two target positions and two item ID's)", version);
  types[0x16] = parse_type(0x16, "1,'imode',2,'icount',{1:8}", "Change selection", version);
  types[0x17] = parse_type(0x17, "1,'igroup',2,'icount',{1:8}", "Assign group hotkey", version);
  types[0x18] = parse_type(0x18, "1,'igroup',1", "Select group hotkey", version);
  types[0x19] = parse_type(0x19, "[v<14?1,'isubgroup':4,'qid',8,'hobject']", "Select subgroup", version);
  types[0x1A] = parse_type(0x1A, "[v<=14?1,4,4,!1B:]", "Pre subselection", version);
  types[0x1B] = parse_type(0x1B, "[v<=14?1,8,'hobject',!1C:9]", "Unknown", version);
  types[0x1C] = parse_type(0x1C, "[v<=14?8,'hhero',!1D:1,8,'hobject']", "Select ground item", version);
  types[0x1D] = parse_type(0x1D, "[v<=14?1,'islot',4,'qunit',!1E:8,'hhero']", "Cancel hero revival", version);
  types[0x1E] = parse_type(0x1E, "1,'islot',4,'qunit'", "Remove unit from building queue", version);
  types[0x21] = parse_type(0x21, "8", "Unknown", version);
  types[0x20] = parse_type(0x20, "", "Cheat: TheDudeAbides (Fast cooldown)", version);
  types[0x22] = parse_type(0x22, "", "Cheat: SomebodySetUpUsTheBomb (Instant defeat)", version);
  types[0x23] = parse_type(0x23, "", "Cheat: WarpTen (Speeds construction)", version);
  types[0x24] = parse_type(0x24, "", "Cheat: IocainePowder (Fast Death/Decay)", version);
  types[0x25] = parse_type(0x25, "", "Cheat: PointBreak (Removes food limit)", version);
  types[0x26] = parse_type(0x26, "", "Cheat: WhosYourDaddy (God mode)", version);
  types[0x27] = parse_type(0x27, "1,4,'iamount'", "Cheat: KeyserSoze (Gold)", version);
  types[0x28] = parse_type(0x28, "1,4,'iamount'", "Cheat: LeafitToMe (Lumber)", version);
  types[0x29] = parse_type(0x29, "", "Cheat: ThereIsNoSpoon (Unlimited Mana)", version);
  types[0x2A] = parse_type(0x2A, "", "Cheat: StrengthAndHonor (No defeat)", version);
  types[0x2B] = parse_type(0x2B, "", "Cheat: itvexesme (Disable victory conditions)", version);
  types[0x2C] = parse_type(0x2C, "", "Cheat: WhoIsJohnGalt (Enable research)", version);
  types[0x2D] = parse_type(0x2D, "1,4,'iamount'", "Cheat: GreedIsGood (Gold and Lumber)", version);
  types[0x2E] = parse_type(0x2E, "4,'ftime'", "Cheat: DayLightSavings (Set a time of day)", version);
  types[0x2F] = parse_type(0x2F, "", "Cheat: ISeeDeadPeople (Remove fog of war)", version);
  types[0x30] = parse_type(0x30, "", "Cheat: Synergy (Disable tech tree requirements)", version);
  types[0x31] = parse_type(0x31, "", "Cheat: SharpAndShiny (Research upgrades)", version);
  types[0x32] = parse_type(0x32, "", "Cheat: AllYourBaseAreBelongToUs (Instant victory)", version);
  types[0x50] = parse_type(0x50, "1,'islot',4,'hflags'", "Change ally options", version);
  types[0x51] = parse_type(0x51, "1,'islot',4,'igold',4,'ilumber'", "Transfer resources", version);
  types[0x60] = parse_type(0x60, "8,$,'stext'", "Map trigger chat command", version);
  types[0x61] = parse_type(0x61, "", "ESC pressed", version);
  types[0x62] = parse_type(0x62, "12", "Scenario trigger", version);
  types[0x65] = parse_type(0x65, "!66", "", version);
  types[0x66] = parse_type(0x66, "[v<=6?!67:]", "Enter choose hero skill submenu", version);
  types[0x67] = parse_type(0x67, "[v<=6?4,'fx',4,'fy',4,!68:]", "Enter choose building submenu", version);
  types[0x68] = parse_type(0x68, "[v<=6?16,!69:4,'fx',4,'fy',4]", "Minimap signal (ping)", version);
  types[0x69] = parse_type(0x69, "[v<=6?!6A:],16", "Continue game (B)", version);
  types[0x6A] = parse_type(0x6A, "16", "Continue game (A)", version);
  types[0x6B] = parse_type(0x6B, "$,'sfile',$,'sgroup',$,'skey',4,'ivalue'", "SyncStoredInteger", version);
  types[0x70] = parse_type(0x70, "$,'sfile',$,'sgroup',$,'skey'", "SyncStored?", version);
  types[0x75] = parse_type(0x75, "1", "Unknown", version);
}
W3GActionParser::~W3GActionParser()
{
  for (int i = 0; i < 256; i++)
    delete types[i];
  delete[] buffer;
}

uint8 W3GActionParser::parse(File* file)
{
  parsed = types[file->getc()];
  if (parsed)
  {
    int length = 0;
    int offset[16];
    int start = file->tell();
    for (int i = 0; i < parsed->numArgs; i++)
    {
      offset[i] = length;
      if (parsed->argLen[i] >= 0)
        length += parsed->argLen[i];
      else
      {
        file->seek(start + length, SEEK_SET);
        while (file->getc())
          length++;
        length++;
      }
    }
    offset[parsed->numArgs] = length;
    int repeat = 0;
    if (parsed->argRepeat >= 0)
    {
      file->seek(start + offset[parsed->argRepeat], SEEK_SET);
      file->read(&repeat, parsed->argLen[parsed->argRepeat]);
    }
    if (repeat)
      length += parsed->argLen[parsed->numArgs] * repeat;
    if (length > bufSize)
    {
      delete[] buffer;
      bufSize = length;
      buffer = new uint8[bufSize];
    }
    file->seek(start, SEEK_SET);
    file->read(buffer, length);
    for (int i = 0; i <= parsed->numArgs; i++)
      args[i] = buffer + offset[i];
    return parsed->type;
  }
  else
    return 0;
}
uint8 W3GActionParser::type() const
{
  return parsed->type;
}
String W3GActionParser::name() const
{
  return parsed->name;
}
uint32 W3GActionParser::arg_int(int i, uint32 def) const
{
  uint32 result = def;
  if (parsed->argLen[i])
    memcpy(&result, args[i], parsed->argLen[i]);
  return result;
}
uint64 W3GActionParser::arg_int64(int i, uint64 def) const
{
  uint64 result = def;
  if (i >= parsed->numArgs)
    memcpy(&result, args[parsed->numArgs] + (i - parsed->numArgs) * 8, parsed->argLen[parsed->numArgs]);
  else if (parsed->argLen[i])
    memcpy(&result, args[i], parsed->argLen[i]);
  return result;
}
float W3GActionParser::arg_float(int i) const
{
  float result = 0;
  memcpy(&result, args[i], parsed->argLen[i]);
  return result;
}
char const* W3GActionParser::arg_string(int i) const
{
  return (char*) args[i];
}

String W3GActionParser::print() const
{
  if (parsed == NULL)
    return "0x??: Unknown";
  String result = "";
  result.printf("0x%02X: %s", parsed->type, parsed->name);
  bool first = true;
  for (int i = 0; i < parsed->numArgs; i++)
  {
    if (!parsed->argNames[i].isEmpty())
    {
      if (first)
      {
        result += " [";
        first = false;
      }
      else
        result += ", ";
      String name = parsed->argNames[i];
      if (name[0] == 'i')
        result.printf("%s=%d", name.c_str() + 1, arg_int(i));
      else if (name[0] == 'q')
        result.printf("%s='%s'", name.c_str() + 1, idToString(arg_int(i)));
      else if (name[0] == 'h')
      {
        result.printf("%s=0x", name.c_str() + 1);
        for (uint8* c = args[i + 1] - 1; c >= args[i]; c--)
          result.printf("%02X", int(*c) & 0xFF);
      }
      else if (name[0] == 's')
        result.printf("%s=\"%s\"", name.c_str() + 1, args[i]);
      else if (name[0] == 'f')
      {
        if (arg_int(i) == -1)
          result.printf("%s=?", name.c_str() + 1);
        else
          result.printf("%s=%f", name.c_str() + 1, arg_float(i));
      }
      else
      {
        result.printf("%s=0x", name);
        for (uint8* c = args[i + 1] - 1; c >= args[i]; c--)
          result.printf("%02X", int(*c) & 0xFF);
      }
    }
  }
  if (!first)
    result += "]";
  return result;
}
