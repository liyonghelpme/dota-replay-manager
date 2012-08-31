#ifndef __REPLAY_CONSTS_H__
#define __REPLAY_CONSTS_H__

#include "dota/consts.h"

#define RACE_HUMAN            1
#define RACE_ORC              2
#define RACE_NIGHTELF         3
#define RACE_UNDEAD           4
#define RACE_RANDOM           5

int convert_race(int race);
String getRaceIcon(int race);

#define SPEED_SLOW            0
#define SPEED_NORMAL          1
#define SPEED_FAST            2

#define VISIBILITY_HIDE       0
#define VISIBILITY_EXPLORED   1
#define VISIBILITY_VISIBLE    2
#define VISIBILITY_DEFAULT    3

#define OBSERVERS_NONE        0
#define OBSERVERS_DEFEAT      2
#define OBSERVERS_FULL        3
#define OBSERVERS_REFEREES    4

#define GAME_LADDER           0x01
#define GAME_CUSTOM           0x09
#define GAME_SINGLE           0x0D
#define GAME_LADDERTEAM       0x20

#define AI_EASY               0
#define AI_NORMAL             1
#define AI_INSANE             2

#define SELECT_TEAMRACE       0x00
#define SELECT_RACE           0x01
#define SELECT_NONE           0x03
#define SELECT_TEAM           0x04
#define SELECT_LADDER         0xCC

#define ACTION_RIGHTCLICK     0
#define ACTION_SELECT         1
#define ACTION_SELECTHOTKEY   2
#define ACTION_ASSIGNHOTKEY   3
#define ACTION_ABILITY        4
#define ACTION_BASIC          5
#define ACTION_SUBGROUP       6
#define ACTION_ITEM           7
#define ACTION_ESC            8
#define ACTION_OTHER          9
#define NUM_ACTIONS           10

String action_name(int id);

#define END_TIME              0x7FFFFFFF

#define FLAGS_QUEUE           0x0001
#define FLAGS_TOALL           0x0002
#define FLAGS_AOE             0x0004
#define FLAGS_GROUP           0x0008
#define FLAGS_NOFORMATION     0x0010
#define FLAGS_SUBGROUP        0x0040
#define FLAGS_AUTOCAST        0x0100

#define ID_RIGHTCLICK         0x000D0003
#define ID_STOP               0x000D0004
#define ID_CANCEL             0x000D0008
#define ID_RALLYPOINT         0x000D000C
#define ID_ATTACK             0x000D000F
#define ID_ATTACKGROUND       0x000D0010
#define ID_MOVE               0x000D0012
#define ID_PATROL             0x000D0016
#define ID_HOLD               0x000D0019
#define ID_GIVE               0x000D0021
#define ID_SWAPITEM1          0x000D0022
#define ID_SWAPITEM2          0x000D0023
#define ID_SWAPITEM3          0x000D0024
#define ID_SWAPITEM4          0x000D0025
#define ID_SWAPITEM5          0x000D0026
#define ID_SWAPITEM6          0x000D0027
#define ID_USEITEM1           0x000D0028
#define ID_USEITEM2           0x000D0029
#define ID_USEITEM3           0x000D002A
#define ID_USEITEM4           0x000D002B
#define ID_USEITEM5           0x000D002C
#define ID_USEITEM6           0x000D002D

#define STAT_KILLS            0
#define STAT_DEATHS           1
#define STAT_CREEPS           2
#define STAT_DENIES           3
#define STAT_ASSISTS          4
#define STAT_GOLD             5
#define STAT_NEUTRALS         6

#define WINNER_UNKNOWN        0
#define WINNER_SENTINEL       1
#define WINNER_SCOURGE        2
#define WINNER_GSENTINEL      -1
#define WINNER_GSCOURGE       -2
#define WINNER_PSENTINEL      -3
#define WINNER_PSCOURGE       -4

#define EVENT_HEROKILL      1
#define EVENT_TOWERKILL     2
#define EVENT_RAXKILL       3
#define EVENT_TREEHEALTH    4
#define EVENT_COURIERKILL   5
#define EVENT_PLAYERLEAVE   6
#define EVENT_ROSHANKILL    7
#define EVENT_AEGISON       8
#define EVENT_AEGISOFF      9
#define EVENT_GAMEMODE      10
#define EVENT_RUNESTORE     11
#define EVENT_RUNEUSE       12
#define EVENT_POOL          13
#define EVENT_HEROBAN       14
#define EVENT_HEROPICK      15
#define EVENT_GAMESTART     16
#define EVENT_LEVELUP       17
#define EVENT_HEROASSIST    18

#endif // __REPLAY_CONSTS_H__
