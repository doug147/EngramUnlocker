# MyEngramUnlocker
 Automatic engram unlocker based on levelup or chat command built on ARK Server API

## Features
- [x] Pulls in all engrams, including ones that are part of mods
- [x] Identifies if engrams are tekgrams, including mod engrams
- [x] Read from config file
- [x] Save to config file
- [x] Admin only mode
- [x] Automatically learn engrams on level up
- [x] Learn engrams based on chat commands
- [x] Toggle ability to learn Tek automatically
- [x] Stores all engrams and requirements in local vector
- [x] Toggle ability to notify player HUD
- [x] Toggle ability to ignore level requirements on execution of chat command
- [x] Chat commands to modify settings on the fly
- [ ] Dump all engrams to text file
- [ ] Allow only users with role to use chat command
- [ ] Remove engrams from auto unlocking by specifying them in configuration file
- [ ] Allow engrams from auto unlocking by specifying them in configuration file
- [ ] Chat commands to add/remove engrams to configuration file on the fly

Settings:
  - UseChatCommand
  - IgnoreLevelRequirements
  - AutoLearnEngramsOnLevelUp
  - AutoLearnTek
  - NotifyPlayerHUD

Admin only commands:
  - `/toggle chatcmd`             Toggle use of chat command to unlock engrams
  - `/toggle lvlup`               Toggle whether or not engrams autoassign on level-up
  - `/toggle tek`                 Toggle whether or not tek is automatically learned
  - `/toggle admin`               Toggle admin only mode (only admins can execute /unlock commands)
  - `/toggle ignorelvl`           Toggle whether or not level requirements are ignored when unlocking engrams
  - `/dump`                       Dumps all engrams to engrams.txt
  - `/save`                       Save the config file!
  
Chat commands:
  - `/unlockengrams`              Unlocks all engrams
  - `/engrams`                    Unlocks all engrams
  - `/unlock`                     Unlocks all engrams
