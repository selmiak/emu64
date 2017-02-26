//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: emu64_commands.h                      //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte Änderung am 26.02.2017                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#ifndef EMU64_COMMANDS_H
#define EMU64_COMMANDS_H

// Defineren aller Kommandozeilen Parameter
enum CMD_COMMAND {CMD_HELP,CMD_VERSION,CMD_MULTIPLE_INSTANCE,CMD_NOSPLASH,CMD_SOFTRESET,CMD_HARDRESET,\
                 CMD_MOUNT_DISK,CMD_MOUNT_CRT,CMD_UMOUNT_CRT,CMD_POKE_64,CMD_LIMITCYCLES};
static const CMD_STRUCT command_list[]{
    {CMD_MOUNT_DISK,"m","mount-disk","Mountet ein Diskimage. --mount-disk <Laufwerksnummer (8-11)> <Dateiname>",2},
    {CMD_MOUNT_CRT,"","mount-crt","Steckt ein Modulimage in den Expansionport ein. --mount-crt <Dateiname>",1},
    {CMD_UMOUNT_CRT,"","umount-crt","Entfernt ein Modulimage aus dem Expansionsport. --umount-crt",0},
    {CMD_SOFTRESET,"r","soft-reset","Führt einen Soft Reset durch.",0},
    {CMD_HARDRESET,"R","hard-reset","Führt einen Hard Reset durch.",0},
    {CMD_POKE_64,"","poke64","Schreibt ein Byte an eine C64 Adresse. --poke64 <Adresse (0-65535)> <Wert (0-255)>",2},
    {CMD_LIMITCYCLES,"","limitcycles","Beendet den Emulator nach n Zyklen mit Exitcode 1. --limitcycles <nZyklen>",1},
    {CMD_NOSPLASH,"","nosplash","Der SplashScreen wird beim Start nicht angezeigt.",0},
    {CMD_MULTIPLE_INSTANCE,"","multiple-instance","Ermöglicht das starten mehrerer Emu64 Instanzen.",0},
    {CMD_HELP,"?","help","Dieser Text.",0},
    {CMD_VERSION,"","version","Zeigt die aktuelle Versionsnummer an.",0}
};

#define command_list_count sizeof(command_list) / sizeof(command_list[0])

#endif // EMU64_COMMANDS_H
