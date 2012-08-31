#include "core/app.h"

#include "present.h"

#define OPT_SHIFT_MODE         56
#define OPT_SHIFT_ICONS        60

#define OPT_DATE               0x0000000000000001ULL
#define OPT_PATCH              0x0000000000000002ULL
#define OPT_MAP                0x0000000000000004ULL
#define OPT_NAME               0x0000000000000008ULL
#define OPT_MODE               0x0000000000000010ULL
#define OPT_HOST               0x0000000000000020ULL
#define OPT_SAVER              0x0000000000000040ULL
#define OPT_LENGTH             0x0000000000000080ULL
#define OPT_PLAYERS            0x0000000000000100ULL
#define OPT_SCORE              0x0000000000000200ULL
#define OPT_OBSERVERS          0x0000000000000400ULL
#define OPT_WINNER             0x0000000000000800ULL

#define OPT_PLAYER_LIST        0x0000000000001000ULL
#define OPT_PLAYER_COLORED     0x0000000000002000ULL
#define OPT_PLAYER_BYLANE      0x0000000000004000ULL
#define OPT_PLAYER_LEVEL       0x0000000000008000ULL
#define OPT_PLAYER_KD          0x0000000000010000ULL
#define OPT_PLAYER_CS          0x0000000000020000ULL
#define OPT_PLAYER_GOLD        0x0000000000040000ULL
#define OPT_PLAYER_LANE        0x0000000000080000ULL
#define OPT_PLAYER_APM         0x0000000000100000ULL
#define OPT_PLAYER_LEFT        0x0000000000200000ULL
#define OPT_PLAYER_ITEMS       0x0000000000400000ULL

#define OPT_DETAIL_SENTINEL    0x0000000000800000ULL
#define OPT_DETAIL_SCOURGE     0x0000000001000000ULL
#define OPT_DETAIL_SAVER       0x0000000002000000ULL
#define OPT_DETAIL_SKILLBUILD  0x0000000004000000ULL
#define OPT_DETAIL_SKILLTIME   0x0000000008000000ULL
#define OPT_DETAIL_ITEMBUILD   0x0000000010000000ULL
#define OPT_DETAIL_ITEMTIME    0x0000000020000000ULL
#define OPT_DETAIL_ITEMCOST    0x0000000040000000ULL

static const int AllOptions[] = {
  IDC_SCRIPT_DATE,
  IDC_SCRIPT_PATCH,
  IDC_SCRIPT_MAP,
  IDC_SCRIPT_NAME,
  IDC_SCRIPT_MODE,
  IDC_SCRIPT_HOST,
  IDC_SCRIPT_SAVER,
  IDC_SCRIPT_LENGTH,
  IDC_SCRIPT_PLAYERS,
  IDC_SCRIPT_SCORE,
  IDC_SCRIPT_OBSERVERS,
  IDC_SCRIPT_WINNER,

  IDC_SCRIPT_PLAYER_LIST,
  IDC_SCRIPT_PLAYER_COLORED,
  IDC_SCRIPT_PLAYER_BYLANE,
  IDC_SCRIPT_PLAYER_LEVEL,
  IDC_SCRIPT_PLAYER_KD,
  IDC_SCRIPT_PLAYER_CS,
  IDC_SCRIPT_PLAYER_GOLD,
  IDC_SCRIPT_PLAYER_LANE,
  IDC_SCRIPT_PLAYER_APM,
  IDC_SCRIPT_PLAYER_LEFT,
  IDC_SCRIPT_PLAYER_ITEMS,

  IDC_SCRIPT_DETAIL_SENTINEL,
  IDC_SCRIPT_DETAIL_SCOURGE,
  IDC_SCRIPT_DETAIL_SAVER,
  IDC_SCRIPT_DETAIL_SKILLBUILD,
  IDC_SCRIPT_DETAIL_SKILLTIME,
  IDC_SCRIPT_DETAIL_ITEMBUILD,
  IDC_SCRIPT_DETAIL_ITEMTIME,
  IDC_SCRIPT_DETAIL_ITEMCOST
};
static const int NumAllOptions = sizeof AllOptions / sizeof AllOptions[0];

static const int PlayerOptions[] = {
  IDC_SCRIPT_PLAYER_BYLANE,
  IDC_SCRIPT_PLAYER_LEVEL,
  IDC_SCRIPT_PLAYER_KD,
  IDC_SCRIPT_PLAYER_CS,
  IDC_SCRIPT_PLAYER_GOLD,
  IDC_SCRIPT_PLAYER_LANE,
  IDC_SCRIPT_PLAYER_APM,
  IDC_SCRIPT_PLAYER_LEFT,
  IDC_SCRIPT_PLAYER_ITEMS
};
static const int NumPlayerOptions = sizeof PlayerOptions / sizeof PlayerOptions[0];

static void UpdateGeneratorEnables(HWND hDlg)
{
  int mode = SendMessage(GetDlgItem(hDlg, IDC_SCRIPTMODE), CB_GETCURSEL, 0, 0);
  EnableWindow(GetDlgItem(hDlg, IDC_SCRIPTICONS), mode == 1);

  BOOL players = (IsDlgButtonChecked(hDlg, IDC_SCRIPT_PLAYER_LIST) == BST_CHECKED);
  EnableWindow(GetDlgItem(hDlg, IDC_SCRIPT_PLAYER_COLORED), (mode != 0) && players);
  for (int i = 0; i < NumPlayerOptions; i++)
    EnableWindow(GetDlgItem(hDlg, PlayerOptions[i]), players);

  BOOL details = (IsDlgButtonChecked(hDlg, IDC_SCRIPT_DETAIL_SENTINEL) == BST_CHECKED)
              || (IsDlgButtonChecked(hDlg, IDC_SCRIPT_DETAIL_SCOURGE) == BST_CHECKED)
              || (IsDlgButtonChecked(hDlg, IDC_SCRIPT_DETAIL_SAVER) == BST_CHECKED);
  EnableWindow(GetDlgItem(hDlg, IDC_SCRIPT_DETAIL_SKILLBUILD), details);
  EnableWindow(GetDlgItem(hDlg, IDC_SCRIPT_DETAIL_SKILLTIME), details &&
    (IsDlgButtonChecked(hDlg, IDC_SCRIPT_DETAIL_SKILLBUILD) == BST_CHECKED));
  EnableWindow(GetDlgItem(hDlg, IDC_SCRIPT_DETAIL_ITEMBUILD), details);
  EnableWindow(GetDlgItem(hDlg, IDC_SCRIPT_DETAIL_ITEMTIME), details &&
    (IsDlgButtonChecked(hDlg, IDC_SCRIPT_DETAIL_ITEMBUILD) == BST_CHECKED));
  EnableWindow(GetDlgItem(hDlg, IDC_SCRIPT_DETAIL_ITEMCOST), details &&
    (IsDlgButtonChecked(hDlg, IDC_SCRIPT_DETAIL_ITEMBUILD) == BST_CHECKED));
}

struct ScriptGenerator
{
  String& result;
  int mode;
  int icons;
  bool hasDetails;

  ScriptGenerator(String& r, int m, int i)
    : result(r)
    , mode(m)
    , icons(i)
  {
    result = "";
    hasDetails = false;
    if (mode != 1)
      icons = 0;
  }

  void newline(bool compact = true)
  {
    if (mode == 2)
      result += (compact ? "<br>" : "<br>\n");
    else
      result += (compact ? "{endl}" : "\n");
  }

  void addDetail(String name, String value)
  {
    if (!hasDetails && mode == 2)
      result += "<table>\n";
    hasDetails = true;
    if (mode == 0)
      result += String::format("%s: %s\n", name, value);
    else if (mode == 1)
      result += String::format("[b]%s:[/b] %s\n", name, value);
    else if (mode == 2)
      result += String::format("<tr><td align='right'><b>%s:</b></td></td>%s</td></tr>\n", name, value);
  }
  void endDetails()
  {
    if (hasDetails)
    {
      if (mode == 2)
        result += "</table>\n";
      else
        result += "\n";
    }
  }

  void addPlayers()
  {
    uint64 options = cfg.fmtGenerator;
    if (options & OPT_PLAYER_BYLANE)
      options &= ~OPT_PLAYER_LANE;
    if (mode == 0)
      options &= ~OPT_PLAYER_COLORED;
    int cols = 2;
    for (uint64 flag = OPT_PLAYER_LEVEL; flag <= OPT_PLAYER_ITEMS; flag <<= 1)
      if (options & flag)
        cols++;
    if (mode == 2)
    {
      result += "<table>\n";
      result += "<tr><th>Name</th><th>Hero</th>";
      if (options & OPT_PLAYER_LANE)
        result += "<th>Lane</th>";
      if (options & OPT_PLAYER_LEVEL)
        result += "<th>Level</th>";
      if (options & OPT_PLAYER_KD)
        result += "<th>KD</th>";
      if (options & OPT_PLAYER_CS)
        result += "<th>CS</th>";
      if (options & OPT_PLAYER_GOLD)
        result += "<th>Gold</th>";
      if (options & OPT_PLAYER_APM)
        result += "<th>APM</th>";
      if (options & OPT_PLAYER_LEFT)
        result += "<th>Left</th>";
      if (options & OPT_PLAYER_ITEMS)
        result += "<th>Items</th>";
      result += "</tr>\n";
    }
    result += "{for team in teams}";
    if (mode == 0)
      result += "{team.name}:\n";
    else if (mode == 1)
      result += "[color={team.color}][size=3]{team.name}:[/size][/color]\n";
    else if (mode == 2)
      result += String::format("<tr><td colspan='%d'><font color='{team.color}' "
        "size='+1'>{team.name}:</font></td></tr>\n", cols);
    if (options & OPT_PLAYER_BYLANE)
    {
      result += "{for lane in team.lanes}";
      if (mode == 0)
        result += " {lane.name}\n";
      else if (mode == 1)
        result += "{lane.name}\n";
      else if (mode == 2)
        result += String::format("<tr><td colspan='%d'>{lane.name}:</td></tr>\n", cols);
      result += "{for player in lane.players}";
    }
    else
      result += "{for player in team.players}";
    if (mode != 2)
    {
      if (mode == 0)
        result += "  {align left 40}";
      if (icons == 1)
        result += "{player.hero.pdicon} ";
      if (options & OPT_PLAYER_COLORED)
        result += "[color={player.color}]";
      result += "{player.name}";
      if (options & OPT_PLAYER_COLORED)
        result += "[/color]";
      if (icons != 1)
        result += " - {player.hero}";
      if (options & OPT_PLAYER_LEVEL)
        result += ", level {player.level}";
      if (options & OPT_PLAYER_LANE)
        result += " ({player.lane})";
      if (mode == 0)
        result += "{endalign}";

      if (options & OPT_PLAYER_KD)
      {
        if (mode == 0)
          result += " K/D/A: {align left 10}{player.stats.herokills}/"
            "{player.stats.deaths}/{player.stats.assists}{endalign}";
        else
          result += ", K/D/A: {player.stats.herokills}/"
            "{player.stats.deaths}/{player.stats.assists}";
      }
      if (options & OPT_PLAYER_CS)
      {
        if (mode == 0)
          result += " C/D/N: {align left 12}{player.stats.creepkills}/"
            "{player.stats.creepdenies}/{player.stats.neutralkills}{endalign}";
        else
          result += ", C/D/N: {player.stats.creepkills}/"
            "{player.stats.creepdenies}/{player.stats.neutralkills}";
      }
      if (options & OPT_PLAYER_GOLD)
      {
        if (mode == 0)
          result += " {align right 6} gold{endalign}";
        else
          result += ", {player.gold} gold";
      }
      if (options & OPT_PLAYER_APM)
      {
        if (mode == 0)
          result += " {align right 3} apm{endalign}";
        else
          result += ", {player.apm} apm";
      }
      if (options & OPT_PLAYER_LEFT)
      {
        if (mode == 0)
          result += " left: {align left 6}{player.left}{endalign}";
        else
          result += ", left: {player.left}";
      }
      if (options & OPT_PLAYER_ITEMS)
      {
        if (mode != 0)
          result += ", ";
        result += "items: [{for item in player.inventory}{if not item.first}, {endif}";
        if (icons == 1)
          result += "{item.pdicon}";
        else
          result += "{item.name}";
        result += "{endfor}]";
      }

      result += "\n";
    }
    else
    {
      result += "<tr><td>";
      if (options & OPT_PLAYER_COLORED)
        result += "<font color='{player.color}'>";
      result += "{player.name}";
      if (options & OPT_PLAYER_COLORED)
        result += "</font>";
      result += "</td><td>{player.hero}</td>";
      if (options & OPT_PLAYER_LANE)
        result += "<td>{player.lane}</td>";
      if (options & OPT_PLAYER_LEVEL)
        result += "<td>{player.level}</td>";
      if (options & OPT_PLAYER_KD)
        result += "<td>{player.herokills}/{player.stats.deaths}/{player.stats.assists}</td>";
      if (options & OPT_PLAYER_CS)
        result += "<td>{player.creepkills}/{player.stats.creepdenies}/{player.stats.neutralkills}</td>";
      if (options & OPT_PLAYER_GOLD)
        result += "<td>{player.gold}</td>";
      if (options & OPT_PLAYER_APM)
        result += "<td>{player.apm}</td>";
      if (options & OPT_PLAYER_LEFT)
        result += "<td>{player.left}</td>";
      if (options & OPT_PLAYER_ITEMS)
        result += "<td>{for item in player.inventory}{if not item.first}, {endif}{item.pdicon}{endfor}</td>";
      result += "</tr>\n";
    }
    if (options & OPT_PLAYER_BYLANE)
      result += "{endfor}{endfor}";
    else
      result += "{endfor}";
    result += "{endfor}";
    if (mode == 2)
      result += "</table>";
    result += "\n";
  }
  void addPlayerDetails(String player)
  {
    uint64 options = cfg.fmtGenerator;
    if (mode == 1)
      result += "[size=3]";
    else if (mode == 2)
      result += "<font size='+1'>";
    result += "Detailed report for ";
    if (icons == 1)
      result += String::format("{%s.hero.pdicon} ", player);
    if (mode == 1)
      result += String::format("[color={%s.color}]", player);
    else if (mode == 2)
      result += String::format("<font color='{%s.color}'>", player);
    result += String::format("{%s.name}", player);
    if (icons != 1)
      result += String::format(" - {%s.hero}", player);
    if (mode == 1)
      result += "[/color]:[/color]";
    else if (mode == 2)
      result += "</font>:</font>";
    else
      result += ":";
    newline(false);

    if (mode == 2 && (options & OPT_DETAIL_SKILLBUILD) &&
        (options & OPT_DETAIL_ITEMBUILD))
      result += "<table><tr valign='top'><td width='300'>\n";

    if (options & OPT_DETAIL_SKILLBUILD)
    {
      if (mode == 0)
        result += "Skill build:\n";
      else if (mode == 1)
        result += "[b]Skill build:[/b][list=1]\n";
      else if (mode == 2)
        result += "<b>Skill build:</b>\n<table>\n";
      result += String::format("{for skill in %s.skills}", player);
      if (mode == 0)
      {
        result += "{align right 4}{skill.index}{endalign}. ";
        if (options & OPT_DETAIL_SKILLTIME)
          result += "{align left 25}{skill.name}{endalign}{align right 7}{skill.time}{endalign}\n";
        else
          result += "{skill.name}\n";
      }
      else if (mode == 1)
      {
        result += "[*]{skill.name}";
        if (options & OPT_DETAIL_SKILLTIME)
          result += " ({skill.time})";
        result += "\n";
      }
      else if (mode == 2)
      {
        result += "<tr><td align='right'>{skill.index}.</td><td>{skill.name}</td>";
        if (options & OPT_DETAIL_SKILLTIME)
          result += "<td align='right'>{skill.time}</td>";
        result += "</tr>\n";
      }
      result += "{endfor}";
      if (mode == 1)
        result += "[/list]\n";
      else if (mode == 2)
        result += "</table>\n";
    }

    if (mode == 2 && (options & OPT_DETAIL_SKILLBUILD) &&
        (options & OPT_DETAIL_ITEMBUILD))
      result += "</td><td width='300'>\n";

    if (options & OPT_DETAIL_ITEMBUILD)
    {
      if (mode == 0)
        result += "Item build:\n";
      else if (mode == 1)
        result += "[b]Item build:[/b][list]\n";
      else if (mode == 2)
        result += "<b>Item build:</b>\n<table>\n";
      result += String::format("{for item in %s.items}", player);
      if (mode == 0)
      {
        result += "  ";
        if (options & (OPT_DETAIL_ITEMTIME | OPT_DETAIL_ITEMCOST))
          result += "{align left 30}{item.name}{endalign}";
        else
          result += "{item.name}";
        if (options & OPT_DETAIL_ITEMTIME)
          result += "{align right 7}{item.time}{endalign}";
        if (options & OPT_DETAIL_ITEMCOST)
          result += " {align right 10}{item.cost} gold{endalign}";
        result += "\n";
      }
      else if (mode == 1)
      {
        result += "[*]";
        if (icons == 1)
          result += "{item.pdicon} ";
        result += "{item.name}";
        if (options & OPT_DETAIL_ITEMTIME)
          result += " ({item.time})";
        if (options & OPT_DETAIL_ITEMCOST)
          result += " {item.cost} gold";
        result += "\n";
      }
      else if (mode == 2)
      {
        result += "<tr><td>{item.name}</td>";
        if (options & OPT_DETAIL_ITEMTIME)
          result += "<td align='right'>{item.time}</td>";
        if (options & OPT_DETAIL_ITEMCOST)
          result += "<td align='right'>{item.cost}</td>";
        result += "</tr>\n";
      }
      result += "{endfor}";
      if (mode == 1)
        result += "[/list]\n";
      else if (mode == 2)
        result += "</table>\n";
    }

    if (mode == 2 && (options & OPT_DETAIL_SKILLBUILD) &&
        (options & OPT_DETAIL_ITEMBUILD))
      result += "</td></tr></table>\n";
  }
};

INT_PTR CALLBACK ReplayPresentTab::ScriptGenDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    {
      SetWindowLong(hDlg, DWL_USER, lParam);
      SendMessage(GetDlgItem(hDlg, IDC_SCRIPTMODE), CB_ADDSTRING, 0, (LPARAM) "Plain text");
      SendMessage(GetDlgItem(hDlg, IDC_SCRIPTMODE), CB_ADDSTRING, 0, (LPARAM) "BB-code");
      SendMessage(GetDlgItem(hDlg, IDC_SCRIPTMODE), CB_ADDSTRING, 0, (LPARAM) "HTML");
      SendMessage(GetDlgItem(hDlg, IDC_SCRIPTICONS), CB_ADDSTRING, 0, (LPARAM) "None");
      SendMessage(GetDlgItem(hDlg, IDC_SCRIPTICONS), CB_ADDSTRING, 0, (LPARAM) "PlayDota");

      SendMessage(GetDlgItem(hDlg, IDC_SCRIPTMODE), CB_SETCURSEL,
        (cfg.fmtGenerator >> OPT_SHIFT_MODE) & 0x0F, 0);
      SendMessage(GetDlgItem(hDlg, IDC_SCRIPTICONS), CB_SETCURSEL,
        (cfg.fmtGenerator >> OPT_SHIFT_ICONS) & 0x0F, 0);
      for (int i = 0; i < NumAllOptions; i++)
        CheckDlgButton(hDlg, AllOptions[i], (cfg.fmtGenerator & (1 << i)) ? BST_CHECKED : BST_UNCHECKED);
      UpdateGeneratorEnables(hDlg);
    }
    return TRUE;
  case WM_COMMAND:
    switch (LOWORD(wParam))
    {
    case IDC_SCRIPTMODE:
      if (HIWORD(wParam) == CBN_SELCHANGE)
      {
        UpdateGeneratorEnables(hDlg);
        return TRUE;
      }
      break;
    case IDC_SCRIPT_PLAYER_LIST:
    case IDC_SCRIPT_DETAIL_SENTINEL:
    case IDC_SCRIPT_DETAIL_SCOURGE:
    case IDC_SCRIPT_DETAIL_SAVER:
    case IDC_SCRIPT_DETAIL_SKILLBUILD:
    case IDC_SCRIPT_DETAIL_ITEMBUILD:
      UpdateGeneratorEnables(hDlg);
      return TRUE;
    case IDOK:
    case IDCANCEL:
      cfg.fmtGenerator = 0;
      cfg.fmtGenerator |= uint64(SendMessage(GetDlgItem(hDlg, IDC_SCRIPTMODE), CB_GETCURSEL,
        0, 0)) << OPT_SHIFT_MODE;
      cfg.fmtGenerator |= uint64(SendMessage(GetDlgItem(hDlg, IDC_SCRIPTICONS), CB_GETCURSEL,
        0, 0)) << OPT_SHIFT_ICONS;
      for (int i = 0; i < NumAllOptions; i++)
        if (IsDlgButtonChecked(hDlg, AllOptions[i]) == BST_CHECKED)
          cfg.fmtGenerator |= (1 << i);
      if (LOWORD(wParam) == IDOK && GetWindowLong(hDlg, DWL_USER))
      {
        ScriptGenerator gen(*(String*) GetWindowLong(hDlg, DWL_USER),
          int((cfg.fmtGenerator >> OPT_SHIFT_MODE) & 0x0F),
          int((cfg.fmtGenerator >> OPT_SHIFT_ICONS) & 0x0F));
        if (cfg.fmtGenerator & OPT_DATE)
          gen.addDetail("Date", "{replay.time}");
        if (cfg.fmtGenerator & OPT_PATCH)
          gen.addDetail("Patch version", "{replay.wc3version}");
        if (cfg.fmtGenerator & OPT_MAP)
          gen.addDetail("Map", "{game.map}");
        if (cfg.fmtGenerator & OPT_NAME)
          gen.addDetail("Game name", "{game.name}");
        if (cfg.fmtGenerator & OPT_MODE)
          gen.addDetail("Game mode", "{game.mode}");
        if (cfg.fmtGenerator & OPT_HOST)
          gen.addDetail("Host name", "{game.host}");
        if (cfg.fmtGenerator & OPT_SAVER)
          gen.addDetail("Replay saver", "{replay.saver}");
        if (cfg.fmtGenerator & OPT_LENGTH)
          gen.addDetail("Game length", "{game.length}");
        if (cfg.fmtGenerator & OPT_PLAYERS)
          gen.addDetail("Players", "{game.players.ratio}");
        if (cfg.fmtGenerator & OPT_SCORE)
          gen.addDetail("Game score", "{game.kills}");
        if (cfg.fmtGenerator & OPT_OBSERVERS)
          gen.addDetail("Observers", "{game.observers}");
        if (cfg.fmtGenerator & OPT_WINNER)
        {
          if (gen.mode == 0)
            gen.addDetail("Winner", "{game.winner}");
          else if (gen.mode == 1)
          {
            gen.result += "[spoiler=\"Winner\"][color={game.winner.color}]{game.winner}[/color][/spoiler]\n";
            gen.hasDetails = true;
          }
          else if (gen.mode == 2)
            gen.addDetail("Winner", "<font color='{game.winner.color}'>{game.winner}</font>");
        }
        if ((cfg.fmtGenerator & OPT_PLAYER_LIST) ||
            ((cfg.fmtGenerator & (OPT_DETAIL_SENTINEL | OPT_DETAIL_SCOURGE | OPT_DETAIL_SAVER)) &&
             (cfg.fmtGenerator & (OPT_DETAIL_SKILLBUILD | OPT_DETAIL_ITEMBUILD))))
          gen.endDetails();

        if (cfg.fmtGenerator & OPT_PLAYER_LIST)
        {
          gen.addPlayers();
          if ((cfg.fmtGenerator & (OPT_DETAIL_SENTINEL | OPT_DETAIL_SCOURGE | OPT_DETAIL_SAVER)) &&
              (cfg.fmtGenerator & (OPT_DETAIL_SKILLBUILD | OPT_DETAIL_ITEMBUILD)) && gen.mode != 2)
            gen.result += "\n";
        }

        if ((cfg.fmtGenerator & (OPT_DETAIL_SENTINEL | OPT_DETAIL_SCOURGE | OPT_DETAIL_SAVER)) &&
            (cfg.fmtGenerator & (OPT_DETAIL_SKILLBUILD | OPT_DETAIL_ITEMBUILD)))
        {
          if ((cfg.fmtGenerator & OPT_DETAIL_SENTINEL) &&
              (cfg.fmtGenerator & OPT_DETAIL_SCOURGE))
          {
            gen.result += "{for player in game.players}{if not player.first}";
            gen.newline();
            gen.result += "{endif}";
            gen.addPlayerDetails("player");
            gen.result += "{endfor}\n";
          }
          else if (!(cfg.fmtGenerator & OPT_DETAIL_SENTINEL) &&
                   !(cfg.fmtGenerator & OPT_DETAIL_SCOURGE))
          {
            gen.result += "{if replay.saver.team}";
            gen.addPlayerDetails("replay.saver");
            gen.result += "{endif}\n";
          }
          else if (cfg.fmtGenerator & OPT_DETAIL_SAVER)
          {
            if (cfg.fmtGenerator & OPT_DETAIL_SENTINEL)
            {
              gen.result += "{for player in sentinel.players}{if not player.first}";
              gen.newline();
              gen.result += "{endif}";
              gen.addPlayerDetails("player");
              gen.result += "{endfor}\n";

              gen.result += "{if replay.saver.team == scourge}";
              gen.newline();
              gen.addPlayerDetails("replay.saver");
              gen.result += "{endif}\n";
            }
            else
            {
              gen.result += "{if replay.saver.team == sentinel}";
              gen.addPlayerDetails("replay.saver");
              gen.newline();
              gen.result += "{endif}\n";

              gen.result += "{for player in scourge.players}{if not player.first}";
              gen.newline();
              gen.result += "{endif}";
              gen.addPlayerDetails("player");
              gen.result += "{endfor}\n";
            }
          }
          else if (cfg.fmtGenerator & OPT_DETAIL_SENTINEL)
          {
            gen.result += "{for player in sentinel.players}{if not player.first}";
            gen.newline();
            gen.result += "{endif}";
            gen.addPlayerDetails("player");
            gen.result += "{endfor}\n";
          }
          else
          {
            gen.result += "{for player in scourge.players}{if not player.first}";
            gen.newline();
            gen.result += "{endif}";
            gen.addPlayerDetails("player");
            gen.result += "{endfor}\n";
          }
        }
      }
      EndDialog(hDlg, LOWORD(wParam));
      return TRUE;
    }
    break;
  }
  return FALSE;
}
