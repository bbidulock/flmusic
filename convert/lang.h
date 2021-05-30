#ifndef _LANG_H_
#define _LANG_H_
//static const char _s[]={""};
#ifdef WITH_ENGLISH
static const char SELECT_SOUNDFILE_s[]={"Select a soundfile"};
static const char PAUSE_PLAYER_s[]={"Pause"};
static const char RESTART_PLAYER_s[]={"Continue"};
static const char PLAYER_TITLE_s[]={"Audio CD/File Player"};
static const char VOLUME_s[]={"Volume"};
static const char BALANCE_s[]={"Balance"};
static const char TRACK_s[]={"Track"};
static const char OF_s[]={"of"};
#ifndef USE_PROGRESS_BAR
static const char ELAPSED_s[]={"Elapsed:"};
#endif
static const char FILE_START_s[]={"File Start"};
static const char FILE_STOP_s[]={"File Stop"};
static const char CD_START_s[]={"CD Start"};
static const char CD_STOP_s[]={"CD Stop"};
static const char EXIT_s[]={"Exit"};
static const char MUTE_s[]={"Mute"};
static const char UNMUTE_s[]={"Unmute"};
static const char OPEN_TRAY_s[]={"Open CD Tray"};
static const char CLOSE_TRAY_s[]={"Close CD Tray"};
static const char GOTO_TRACK_s[]={"Goto Track:"};
static const char CHANGE_TRACK_s[]={"Change Track"};
static const char CHANGE_FRAME_s[]={"Change Frame"};
#endif
#ifdef WITH_GERMAN
static const char SELECT_SOUNDFILE_s[]={"Audio-Datei wählen"};
static const char PAUSE_PLAYER_s[]={"Unterbrechen"};
static const char RESTART_PLAYER_s[]={"Fortsetzen"};
static const char PLAYER_TITLE_s[]={"Audio CD/Datei hören"};
static const char VOLUME_s[]={"Lautstärke"};
static const char BALANCE_s[]={"Richtung"};
static const char TRACK_s[]={"Spur"};
static const char OF_s[]={"von"};
#ifndef USE_PROGRESS_BAR
static const char ELAPSED_s[]={"Gespielt:"};/* Verstrichen: */
#endif
static const char FILE_START_s[]={"Datei hören"};
static const char FILE_STOP_s[]={"Datei schließen"};
static const char CD_START_s[]={"CD hören"};
static const char CD_STOP_s[]={"CD beenden"};
static const char EXIT_s[]={"Beenden"};
static const char MUTE_s[]={"Verstummen"};
static const char UNMUTE_s[]={"Hören"};
static const char OPEN_TRAY_s[]={"CD Lade öffnen"};
static const char CLOSE_TRAY_s[]={"CD Lade schliessen"};
static const char GOTO_TRACK_s[]={"Spur wählen:"};
static const char CHANGE_TRACK_s[]={"Spurwechsel"};
static const char CHANGE_FRAME_s[]={"Rahmenwechsel"};
#endif

#endif /* _LANG_H_ */
