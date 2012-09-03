DotA Replay Manager 3.00

http://www.playdota.com/forums/showthread.php?p=110886
http://rivsoft.narod.ru/dotareplay.html

============== Changelog ==================================
**3.00**
 - Major remake!
 - Whole program re-written from scratch!
 - Beta Beta Beta!
 - Few functions still missing, such as search and hero chart

**2.10**
 - Added back/forward buttons for easier browsing. I haven't tested this feature extensively, expect bugs
 - Added "Replay options" button in game info tab, containing the options to modify replay (as it was before),
  delete replay and create backup
 - Fixed morphing heroes such as Dragon Knight sometimes not being detected correctly
**2.09**
 - Added ExtPresent tab for new script-based Presentation. Sample script is included, see readme for
  syntax description
 - Updated playdota forum icons (:cm:)
 - Now shows final items for older replays
 - Fixed a couple small bugs
**2.08**
 - Added hero icons to gold and experience graphs
 - Added zoom to gold and experience graphs
 - Added kill details to player info tab
 - Fixed APM chart
 - Added option to show empty slots in game info tab
**2.07**
 - Updated for 6.70
 - Fixed several bugs as reported on forums and by Blasz:
  Runes names in game chat and in timeline are now displayed properly
  Fixed hero levels sometimes being incorrect in game info tab
  Fixed several icons
  And others
**2.06**
 - Fixed a bug that made opening replays much slower
 - Improved support for AI replays - now shows heroes and items of AI players
 - Improved learned ability detection
 - Fixed multiple assists in game chat/timeline
**2.05b**
 - Fixed compatibility with new maps
 - Added final item build to game info tab, added tooltips for icons
 - Added "Find prev" button in chat log and action log
 - Fixed copying text in action log, enabled multiple selection
 - Chat no longer stops when { or } characters are encountered (by Blasz)
**2.05**
 - Fixed chat log search to be case insensitive
 - Added search for Action Log
 - Added rightclick menu for Action Log that allows copying lines
 - Fixed "Copy name" option from rightclick menu in game info tab (it used to copy broken text)
 - Added assists to "Copy stats" option from rightclick menu in game info tab
 - Added "Copy matchup" button on game info tab, copies list of players and heroes in one line
 - Fixed some crashes
 - Fixed gold timeline (integer overflow ftl)
 - Finally added drag-and-drop support for folder view (tree view still doesn't support it)
 - Fixed draft view to work for -cm (note that dota is bugged atm and only lists sentinel picks/bans
  in -cd)
 - Added streaks and kill combos to game chat and timeline view (use Chat Filters button on top
  right to hide them)
 - Added support for -switch mode, colors in chat now correctly display the player's current color and
  it doesn't say "has been killed by his teammate" incorrectly anymore
 - Using blink dagger doesn't drop wards all over the timeline picture as much; the solution is still
  temporary and needs more replay data to work correctly
 - Added assists to game chat (disabled by an option in Settings), note that dota is bugged atm and doesn't
  store this information correctly
 - Removed the 8192 size limit on cache; gamecache now uses game date/time as a key instead of filename,
  this should remove duplicate replays, there is a very low chance that two different replays have the same
  date/time. As a side effect gamecache file may grow very large, delete it and cache replays again if it
  causes trouble
 - Fixed hero chart to show correct heroes and games
 - Added game mode filter to hero chart (suggested by tk1)
 - Added buildings to Timeline view, they should correctly disappear when they are destroyed. Timeline view
  is now resizable, map image has been updated to the latest version. Dead heroes now disappear correctly
 - Fixed odd time marks in graph views (e.g -0:58 to -1:00)
 - Added PlayDota smiley tags for items (assuming urn will be :urn: when it is added)
**2.04**
 - Happy New Year
 - Fixed some bugs
 - Fixed hero chart a little, it now shows the heroes in the correct order as they appear in taverns
 - Updated to 6.65 (the only difference between this and automaticaly loading data is that icons
  will update properly)
 - Added support for new replay data: roshan, aegis, runes, correct gamemode, hero levels (affects
  xp timeline)
 - Now recognizes game start (creep spawn) and adjusts lane detection accordingly, added option to
  show all times relative to creep spawn like in actual game
 - Fixed scepter recipe and items purchased in the side shops
 - Added Draft tab, showing hero pool for -cd and bans/picks for -cd/-cm (sorry, haven't tested this
  at all since I don't have any 6.65 -cd replays, so if it doesn't I'll hotfix it later)
 - Added Action log tab, which shows a very detailed low level log of the game (I'll add search function
  for it soon). It shows hero/item/ability/etc icons and names wherever applicable, player colors
  etc, and works with any map, not only dota (the map must be in your wc3 folder under the name
  specified in the replay). Loading map and re-parsing the replay takes about 10 seconds.
 - Haven't fixed crashes yet, I'll work for it soon
**2.03**
 - Fixed various crashes
 - Fixed Sven's icon in hero chart (reported by Jager)
 - Fixed other heroes' skills appearing in build view
**2.02b**
 - Added colored names/hero names in timeline tab as well
 - Fixed techies icon and a few others
**2.02**
 - From now on, instead of resources.mpq the program will come with patch.mpq and when you run
  it the first time it will merge the two files - don't rename anything yourself. This way map
  data you loaded yourself will not be erased after every patch
 - Fixed some more bugs in reading player stats
 - Added an option to show hero names in chat log
 - Colored player names in chat events (e.g. hero kills)
 - Added chat search function (by text or by player)
 - Added chat filters (e.g. only show hero kills)
 - Updated the list of forum icons for Presentation tab (like :puck:)
 - Added a set of forum icons for playdota.com
**2.01b**
 - Fixed to correctly read recipes again
**2.01**
 - Fixed major bug that prevented players and stats from loading correctly
 - Fixed to work with 6.60+
 - Fixed a bug that prevented upgradeable skills from registering correctly
  (e.g. all ultimates affected by scepter, Ogre Magi abilities etc.)
 - Changed hero chart a little to accomodate 9 taverns
**2.00**
 - Major rewrite, the program now reads all DotA data from the map
 - Hero kills, tower/rax kills etc. are now shown in game chat
 - Added "Use D-A forum icons" in Presentation tab, which forces the program to use dota-allstars
  forum icons for heroes (e.g. :cmai:)
**1.05**
 **NOTE** Since much of the program was adapted to unicode and thus gamecache format has
  changed. If you see a lot of games with bad game names in the folder view (it should only
  happen to local games) you should consider caching replays again (button in settings).
  Also since a LOT major changes have been introduced expect new bugs >.<
 - The window is now resizeable
 - Added sentinel and scourge total gold in gold timeline mode (suggested by HiT0Mi)
 - Reworked a lot of stuff to hopefully work properly with unicode. Game Chat and Timeline
  modes should display chinese and other languages properly, and player names using
  non-english letters should work better now.
 - Added progress bar when folder is being populated (since it takes a while in some cases).
 - Fixed game mode determination to match the one in DotA, and adjusted lane determination
  to work better with modes that make creeps spawn at 2:00 (MM, VR and AP)
 - Fixed replay decompression algorithm to work with non-standard replays made at replays.net,
  garena and the like
 - Optimized replay parsing algorithm for caching data
 - Added minimap image to Game Info mode (click to enlarge)
 - Added replay workshop (remove chat messages and pings from the replay)
 - Countless minor bugfixes
 - Finally added map parser. For now it only parses abilities - that is it should now correctly
  determine which skills the heroes have
**1.04b**
 - I hope it now REALLY works for 6.52(c)
 - Fixed version number in the program
**1.04**
 - Updated for 6.52 (haven't tested thoroughly yet)
 - Removed the error dialogs that popped up when invalid replay file was opened (to simplify
  listing through the replays)
 - Some bug fixes
**1.03b**
 - Added "Copy player name" item to drop down menu in game info tab
**1.03**
 - Fixed both impales and god's strength to work with new versions
 - Game length is now correct, it indicates when the tree/throne went down (if the game was
  finished)
**1.02b**
 - Fixed several bugs with hero chart
 - Added some support for ladder replays (game info, game chat and action tabs work)
 - Changed date format in rename template from DD-MM-YYYY to YYYY-MM-DD for correct replay order
  when sorted alphabetically (suggested by greater.morphling)
**1.02**
 - Adapted to 6.51
 - Made separate repeated action delays for skills and items
 - Fixed a bug with columns in search mode
 - Tree view gets automatically updated every time a new replay is detected
 - Added hero chart, which shows replays for different heroes (another item in the tree view)
**1.01b**
 - Added your own name box to settings to use when replay saver was not properly identified (if
  the game was not finished)
 - The screenshot parser counts copies of the same game once now
 - Added "Show player stats" button to game info tab to display information similar to screenshot
  parser mode
 - Fixed some composite items in item build (Orchid and Guinsoo)
 - Eliminated OpenGL usage in gold/exp timeline and actions (Timeline mode still uses it)
**1.01**
 - Fixed a bug in gold/exp timeline for games less than 5v5 (if, say, purple was missing then
  yellow will take his place and there will be some problems)
 - Fixed loading images for 16 bit color desktop (problem reported by RockBuster)
 - Added experimental screenshot parsing feature (join a game, press Print Screen, select Parse
  screenshot item in the tree view, you will see the list of players that joined the game. Works
  pretty well in 1280x1024 but has problems with other resolutions)
 - Added "By date" option for the tree view
 - When you batch copy/backup a replay cached information will be copied as well, this removes a
  large delay that can appear if you open the folder with back up'ed replays after playing a lot
  of games
**1.00**
 - Adapted to 6.50: added invoker, fixed new skills for Drow ranger and Necro'lic, changed
  recipes/item costs
 - Fixed time in Gold/Exp timelines
 - Fixed a bug in presentation mode which caused the program to crash when certain options
  were set (by K[a]ne)
 - Added "Reset" button in settings
 - It is recommended that you increase Repeated action delay in settings to something like 3
  or even 5 seconds. The new skill algorithm should fix most errors this change may cause,
  and I've seen replays where someone managed to click the skill 3 times with a 4 second
  interval
 - Added an option to remove basic commands (e.g. rightclick, move/attack/stop, pre
  subselection) from log files
 - Added an option to make Gold timeline smoother (because when player buys many items on
  the same fountain trip the timeline looks awkward)
 - Added game mode option in Present tab
**0.98b**
 - Fixed old N'aix, Replicate
 - Fixed one option in the settings which appeared in both tabs by mistake
 - Improved skill learning algorithm (repeated actions are now handled better)
 - Added experience timeline tab
**0.98**
 - Made brown and purple a little lighter for better presentation (suggested by RaMbOMaN)
 - Added player statistics - lists games for given player. Type "Player:<name>" in path bar
  or right click player in game info and select Find games (suggested by SnowWar)
 - Player build and actions tab now share selection (selecting a player in one will select
  the same player in another)
 - Added button for caching all replays in the Replays folder (you need to click it if you
  want to use player statistics
 - Added browse button for replay path
**0.97b**
 - Fixed bottom and mid lanes which were swapped by mistake in one of the last versions
**0.97**
 - Fixed a couple bugs that caused the program to crash when players did not select a hero
 - Fixed -sp mode which was not working correctly
 - Added keyboard shortcuts for file list view (Ctrl-C, Ctrl-V, Enter, F2, Delete)
 - Fixed dagon and necrobook, now base items are properly grayed
**0.96b**
 - Improved winner determination: if none of the methods worked the program checks player's
  positions and if too many are inside enemy base then they are considered winners
 - You can now copy player stats from Game info tab (Ctrl-C) (suggested by FoR)
**0.96**
 - Improved gold timeline visualisation (added lines)
 - Added replay search function (next to Explore button in folder view mode)
 - Enabled loading replays directly from internet (as long as the filename starts with http:)
**0.95**
 - Added longest AFK time in actions tab
 - Added gold timeline tab, displaying build costs over time for all players on a single graph
 - Added game mode identification (even if saver wasn't sentinel), added game commands to game chat
 - Enabled detail view in file list (option in settings)
 - Lane detection algorithm fixed to work better with -rd (from 2:00-5:00 to 3:30-6:30)
**0.94b**
 - Slippers of agility fixed, got wrong item id last time =(
 - Removed the log file which I forgot to remove last time (if you need it it's in the options now)
 - Fixed setting this program as default replay program, now it works if you didn't install WC3 on
   the computer (it adds all necessary keys and sets the icon), also if WC3 wasn't properly installed
   and replays weren't opened with it turning this option on and off will properly bind replays to WC3
 - It will now detect 6.49c properly since IceFrog never used 'c' suffix (AFAIK)
**0.94**
 - Added combined items in item build, recipe scrolls now have a scroll icon (option available in
   the settings menu)
 - Colored skills in skill build view (option available in the settings menu)
 - Synchronized selection in skill and item build lists (option available in the settings menu)
 - Added tabs for settings (they don't fit on one page anymore)
**0.93b**
 - Fixed a lot of skills on heroes with engineering upgrade-based skills: Ogre Magi, Krobelus,
   Syllabear and all aghanim users
 - Added skill levels to build view (option added to settings)
 - Added player colors in combo boxes (player selection), also added unselectable Sentinel and
   Scourge entries
 - Added support for Open With function, added option to set this program as default for opening
   replays
 - You can drag replay files from Explorer into this program now
**0.93**
 - Uncapped replay speed, now it can be from -99 to 99 (suggested by esby)
 - Added minimap pings to timeline mode
 - Tweaked presentations a bit
 - Added update checking
 - Added player colors in lists
**0.92b**
 - Fixed fissure in versions < 6.49
 - Fixed endgame info for < 6.49 and -sp mode in >= 6.49
**0.92**
 - Fixed "Watch replay" button. Now instead of "double clicking" the replay it properly finds WC3
   install path and passes the replay. Also added an option to launch WC3 in windowed mode.
 - Added a lot of options to Settings menu
 - Added wards for Timeline tab.
 - Rewrote file operations (delete and paste) to use system functions - to show progress bar and
   for familiar interface.
 - Added Presentation tab with tons of options.
 - Fixed some abilities (that either were not detected or belonged to a wrong hero).
===========================================================

DotA Replay Manager is a tool for browsing Warcraft III replays with additional features for parsing
DotA replays. I've been developing it for almost 5 years now and it is currently one of the most
powerful parsers around.

Features:
* Explorer-like fully functional replay browser that allows you to organise your replays
* List replays in tree view by folder or by date
* Auto-copying new replays (from LastReplay.w3g) and batch copying existing replays
* NOT YET IMPLEMENTED: Replay search with a lot of options like game name, length, map version,
 player names and heroes
* Viewing replays directly from internet (if a URL is supplied)
* NOT YET IMPLEMENTED: Displays a list of replays with statistics for player or hero

Replay viewing features:
* DotA data is fully loaded from the map, so when a new version of DotA is released the program
 will automatically read the new data
* Displays extended game information and list of players with score, lane, item build
* Colored chat log with lots of game messages; search and filtering is supported (NO ITS NOT.. YET)
* Timeline view - displays *estimated* hero movement over time - an animated version of the replay
 with many features.
* Hero builds - skill and item orders
* Action charts - including different action types, group hotkeys used, and APM over time graph.
* Gold and experience timeline graphs
* Presentation - a complex scripting language that allows you to format a replay for a forum
 thread. Simple GUI script generator included
* Shows hero pool for -rd/-cd and bans/picks for -cd/-cm 
* Full action log with graphical information for in-depth replay analyzis

============== User interface =============================
Honestly, this should be intuitive.

=============== Script ====================================
Starting with version 2.09, DotA Replay Manager supports scripted replay presentation.
In 3.00 this language was slightly enhanced, and a powerful editor with syntax highlighting
and smart suggestions was added.

The syntax is fairly simple:
* Comments are started by $$, and continue until the end of the line.
* All text not contained in curly braces is copied directly to output (including newlines).
* All replay data is accessed through a hierarchy of variables, addressed as {item.subitem.subitem} etc.
* All variables are of string type.
* Some variables contain a list of values, such as a list of all players - those can be iterated through
 with {for} block: {for p in game.players}{p.name}{endfor} would print the names of all players (with
 no spaces in between).
* Conditions are added with {if} blocks: {if player.name == "RiV"}Hi riv!{else}Hi {player.name}{endif}
 A condition is either a variable (which is true unless it doesn't exist, is empty, or equals "false"),
 or a case-insensitive comparison, or a logical expression consisting of other conditions.

Two sample scripts are included.
I'm not listing the full variable list because it can easily be viewed in the editor by pressing
Ctrl+Space.

=============== Settings ==================================
Settings are opened by selecting the first item in the tree view.

- Warcraft folder: locate the Warcraft III installation path so that the program can find map files and
 game data.
- Replay folder: locate the folder to build the replay tree from. Avoid specifying a large directory
 such as "C:", because the tree is rebuilt every time a file in that directory changes
- Hide empty folders: hide folders in tree view that do not contain any replays

- View replays in windowed mode: when "View replay" button is clicked Warcraft III will launch in a
 windowed mode.

- Automatically view new replays: when the program detects that LastReplay.w3g file in the replay folder
 has been updated it will open it.
- Automatically copy new replays: when LastReplay.w3g is updated the program will create a copy of it
 (new file name is specified by the next option).
- Copy to file: specifies the new file name for automatically copier replays. Press Help for a list of
 available tags. It is recommended that you use the <n> tag because it ensures that no files are
 overwritten.

- Select columns: choose additional columns to display in folder view. The second line contains
 columns that require parsing every replay, information in them might not appear immediately
  To cache all games click "Cache all replays" button on the bottom.

- Minimize to system tray: check this box to hide the program window and display a small icon in the
 Notification area (lower right corner of the screen) when it is minimized. Useful when the program is
 running in background mode, saving replays.
- Enable URL in path bar (make sure you type http://): if the requested replay file name starts with
 http: then the program will attempt to look for in in the internet.
- Check for updates automatically (every day): check this box to allow the program to connect to
 internet and check for updates (when you run this program).
- Automatically load missing map data: the program will automatically parse the map file specified
 in the replay. Otherwise, a dialog box will be displayed, allowing you to choose an appropriate
 action.
- Set this program as default for opening replays: check this box to use this program for opening
 replay files, uncheck it to use Warcraft III

- Your name(s) (if replay saver was not found): list the names you use when playing (separated by spaces).
 If replay parser could not be identified for some reason, this information will be used instead.
 (NOT YET IMPLEMENTED)

- Repeated action delay for skills (ms): specifies the mimimal amount of time that should pass between two
 learned skills - because the replay can sometimes contain several learn skill actions when a player clicks
 the button too quickly.
- Repeated action delay for items (ms): specifies the mimimal amount of time that should pass between two
 bought items - because the replay can sometimes contain several buy item actions when a player clicks
 the button too quickly.

- Draw wards: check this box to see wards in Timeline mode.
- Ward lifetime (seconds): set the amount of time a single ward lasts. Since the program cannot determine
 whether the ward is sentry or observer they have a common lifetime.
- Draw chat: check this box to see chat in Timeline mode.
- Chat fade time (seconds): set the amount of time a chat message lasts on a screen (it will dissappear
 faster if game speed is increased).
- Draw minimap pings: check this box to see minimap pings in Timeline mode.
- Draw buildings: check this box to see buildings in Timeline mode that aren't destroyed.

- Show skill levels in build view: check this box to append skill levels to skill names in build list.
- Show assembled items in itembuild: check this box to view completed items in itembuild list (colored
 green), items used as ingredients will be colored gray.
- Smoother gold timeline: check this box to remove sharp jumps in gold timeline (which appear when player
 buys several items at the same time).

- Log actions: check this box to create a log.txt file containing most player's actions with ItemIDs,
 coordinates, ObjectIDs. This might significantly slow down parsing.

- Cache all replays: goes through every replay in the Replays folder and retrieves some information from
 them for searching and viewing details in folders.

d07.RiV@gmail.com
