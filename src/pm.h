/***************************************************************************/
/* F r e e C o d e                                                         */
/*                                                                         */
/* THIS CODE IS FREEWARE AND AS SUCH YOU ARE PERMITTED TO DO WHAT YOU WISH */
/* WITH IT. THESE PROGRAMS ARE PROVIDED AS IS WITHOUT ANY WARRANTY,        */
/* EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO FITNESS FOR ANY      */
/* PARTICULAR PURPOSE.                                                     */
/*                                                                         */
/* However, we would appriciate if you shared any enhancements to us       */
/* Please send them to www.jma.se/sw and we will include them in           */
/* future distributions.                                                   */
/*                                                                         */
/***************************************************************************/
/***************************************************************************/
/* p m . e x e                                                             */
/*                                                                         */
/* This application is a process manager for eComStation.                  */
/*                                                                         */
/*                                                                         */
/***************************************************************************/
#include "api16.h"

// constants
//-----------------------
#define VERSION                 "Beta 4 (0.244) - April 6, 2004"
// #define VERSION                 "Internal version (0.238) - Mars 17, 2004"


#define IDS_DRIVER_DOS          "\\DEV\\DOS$"
#define IDS_CADH_FILE           "CADH$$$$"
#define IDS_CADH_SEM            "\\SEM32\\CADH"
#define IDS_FASTIO              "/dev/fastio$"

#define PROG_WPS                99

#define IDS_DESKTOP             "Desktop"

#define IDS_INTRO_CRIGHT        "FreeCode http://www.jma.se/sw"
#define IDS_INTRO_LINE          "------------------------------"
#define IDS_INTRO_ACTIVATECAD   "Press Ctrl+Alt+Del to activate"
#define IDS_INTRO_ACTIVATEKBD   "Press and hold Ctrl+Ctrl to activate"

#define IDS_MSG_FATALEXIT       "Cannot continue !"

#define IDS_ERROR_DETACHED      "Cannot run as detached"

#define IDS_ERROR_NOCADH        "CADH.SYS not loaded, code = "
#define IDS_ERROR_NOCADHSEM     "CADH semaphore failed, code = "
#define IDS_ERROR_NOCADHON      "Could not enable CADH, code = "
#define IDS_ERROR_NOCADHOFF     "Could not disable CADH, code = "

#define IDS_APPTITLE            "eComStation ProcMan"
#define IDS_APPKEYS             "Esc=Hide  F1=Help  F9=Refresh  F10=Unload"

#define IDS_WNDTITLE            "Running programs:"
#define IDS_WNDKEYSS1           "Enter=Switch_to  C=Close  K=Kill                              "
#define IDS_WNDKEYSS2           "F2=Tasks  F3=Show  F4=Restart_WPS  F5=Command_line  F8=Reboot "
#define IDS_WNDKEYST1           "K=Kill  W=Kill_WPS                                            "
#define IDS_WNDKEYST2           "F2=Programs  F3=Show  F4=Restart_WPS  F7=Shutdown  F8=Reboot  "

#define IDS_TASKINFO_HEAD       "Pid:  Type:   Thrs:  Title:"
#define IDS_SWITCHINFO_HEAD     "Title:                                                  Type:"

#define IDS_PROGTYPE_DEFAULT    "DEF"
#define IDS_PROGTYPE_SYSTEM     "SYS"
#define IDS_PROGTYPE_TEXT       "VIO"
#define IDS_PROGTYPE_PM         "PM "
#define IDS_PROGTYPE_WPS        "WPS"
#define IDS_PROGTYPE_DETACH     "DET"
#define IDS_PROGTYPE_DOSWIN     "DOS"

#define IDS_TASKINFO_DEF        "%4.4d  %s  (%3.3d)  %s"
#define IDS_SWITCHINFO_DEF      "%-55.55s  %s"

#define IDS_KILLPROCESS         "Kill Process"
#define IDS_KILLPROCESSQUERY    "Do you really want to kill"

#define IDS_KILLWPSPROCESS         "Kill WPS"
#define IDS_KILLWPSPROCESSQUERY    "Do you really want to kill"

#define IDS_KILLWPSAPP0         "Kill WPS application"
#define IDS_KILLWPSAPP1         "You are trying to kill a WPS app."
#define IDS_KILLWPSAPP2         "If it does not die you might try"
#define IDS_KILLWPSAPP3         "killing it by restarting WPS."

#define IDS_WPS                 "Restart WPS"
#define IDS_WPSAYS              "Are you sure ?"
#define IDS_WPSYMWR             "Workplace shell will restart !!"

#define IDS_SHUTDOWN            "Shutdown System"
#define IDS_SHUTDOWNAYS         "Are you sure ?"
#define IDS_SHUTDOWNYMWR        "Your machine will shutdown !!"

#define IDS_REBOOT              "Restart System"
#define IDS_REBOOTAYS           "Are you sure ?"
#define IDS_REBOOTYMWR          "Your machine will restart !!"

#define IDS_SD_TITLE            "Shut down the computer"
#define IDS_SD_WAIT             " Please wait while the system shuts down...  "
#define IDS_SD_CLEARLINE        "                                             "             
#define IDS_SD_RESTART1         "           Shutdown is now complete          "
#define IDS_SD_RESTART2         "        The computer will now restart        "
#define IDS_SD_SHUTDOWN1        "           Shutdown is now complete          "
#define IDS_SD_SHUTDOWN2        "      You may safely power of the computer   "

#define IDS_CMDLINETITLE        "ProcMan command line"
#define IDI_CMDLINETITLELEN     20

#define IDS_SYSTEMTASK          "*SYSINIT*"


#define ID_KEY_KILL_V           'K'
#define ID_KEY_KILL_G           'k'

#define ID_KEY_KILLWPS_V        'W'
#define ID_KEY_KILLWPS_G        'w'

#define ID_KEY_CLOSE_V          'C'
#define ID_KEY_CLOSE_G          'c'

// #define ID_KEY_PRIORITY_V       'P'
// #define ID_KEY_PRIORITY_G       'p'


// struct declarations
//-----------------------
typedef struct _PROCINFO
       {
       PID              pid;
       PID              pidParent;
       ULONG            iType;
       ULONG            iStatus;
       ULONG            iThreads;
       char             szPidPath[_MAX_PATH];
       BOOL             bIsWPS;
       HSWITCH          hSwitch;
       HWND             hwnd;
       } PROCINFO;
typedef PROCINFO *PPROCINFO;


// functions
//-----------------------
void KeyboardMon(void *);
void CadMon(void *);
void DisplayPopUp(void *);
PVOID GetRunningProcesses(int *iNumRecords);
PVOID GetSwitchList(PID pidWPS, int *iNumRecords);
void DisplayError(PSTVIO pstv, char *szErrorFunc, int rc);
void DisplayShutdown(PSTVIO pstv, BOOL bFlag);
HSWITCH OpenCmdPrompt(void);
int GetScreenGroup(void);
int GetTID1Prio(PID pid);
BOOL ResetWPS(void);
PID GetWpsPid(void);
void SortByPID(PPROCINFO pi, int iAntal);
void SortByName(PPROCINFO pi, int iAntal);
APIRET DosKillFastIo(PID pid);
BOOL DosQueryFastIo(void);
BOOL DosQueryDetached(void);
void Log(char *szFormat, ...);


