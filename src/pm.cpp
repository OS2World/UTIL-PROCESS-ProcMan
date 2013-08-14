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
//
// This program contains code derived from 
//    xWorkPlace, (C) 1997-2003 Ulrich M”ller.
//    cadh.pas, (C) 2003 Veit Kannegieser.
//
/***************************************************************************/
/* p m . e x e                                                             */
/*                                                                         */
/* This application is a process manager for eComStation.                  */
/*                                                                         */
/*                                                                         */
/***************************************************************************/
/***************************************************************************/
/*                                                                         */
/*   Modul:      PM.CPP, Process manager for eComStation                   */
/*                                                                         */
/*   Version:    0.244                                                     */
/*                                                                         */
/*   Tid:        010524, 040406, 37h                                       */
/*                                                                         */
/*  ---------------------------------------------------------------------  */
/*   010524: Stole most from BSEDIT.CPP. (MA,1h)                           */
/*  ---------------------------------------------------------------------  */
/*   010525: Wrote most code. (MA,10h)                                     */
/*  ---------------------------------------------------------------------  */
/*   010927: Added support for fastio$ kill. (MA,1h)                       */
/*  ---------------------------------------------------------------------  */
/*   020122: Added support for fullscreen using DosMon API. (MA,6h)        */
/*  ---------------------------------------------------------------------  */
/*   040314: Rewrote to use CADH.SYS, added switchlist (MA,6h)             */
/*  ---------------------------------------------------------------------  */
/*   040315: Bugfixes, added close method and WPS check (MA,3h)            */
/*  ---------------------------------------------------------------------  */
/*   040316: Bugfixes, added command line, new look for reboot/restart     */
/*           removed "Desktop" and reworked threading priorities (MA,6h)   */
/*  ---------------------------------------------------------------------  */
/*   040317: Removed DETACH support, added a way to open a FS cmdline,     */
/*           removed all logging without DEBUG (MA,2h)                     */
/*  ---------------------------------------------------------------------  */
/*   040406: Added kill WPS and reworked the menus a bit. (MA, 2h)         */
/*  ---------------------------------------------------------------------  */
/***************************************************************************/
#define INCL_WINSHELLDATA
#define INCL_WINWORKPLACE
#define INCL_WINSWITCHLIST
#define INCL_WINPROGRAMLIST
#define INCL_DOSDEVIOCTL
#define INCL_DOSDEVICES
#define INCL_DOSPROCESS 
#define INCL_DOSERRORS
#define INCL_DOS
#define INCL_VIO
#define INCL_KBD

#include <os2.h>
#include <bsedos.h>
#include <bsesub.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <process.h>
#include <conio.h>
#include <sys\stat.h>

#include <stlibvio.h>
#include <stlibstc.h>

#include "pm.h"

// Global (semaphore) variables
HEV      hSemPopup;
HEV      hSemExit;
HEV      hSemCadh;
HEV      hSemSleep;

BOOL     bIsFastIo;

/*-----------------------------------------------------------------------------
   main: Entrypoint into this program.
-----------------------------------------------------------------------------*/
int main(int argv, char *argc[])
    {
    BOOL        bUseCad, bUseKbd; 

    BOOL        bQuit = FALSE;
    ULONG       ulPostCt;

    TID         tidPopup, tidMonKbd, tidMonCad;

    APIRET	rc;


#ifdef DEBUG
    Log("--------------------------------------------------");
#endif
 
    // Set to false
    bUseCad = FALSE;
    bUseKbd = FALSE;

    // check for parameters
    if (argv == 2) 
       {
       if (strcmpi(argc[1], "kc") == 0)
          {
          bUseCad = TRUE;
          bUseKbd = TRUE;
          }

       if (strcmpi(argc[1], "k") == 0)
          {
          bUseKbd = TRUE;
          }

       if (strcmpi(argc[1], "c") == 0)
          {
          bUseCad = TRUE;
          }
       }
    else
       {
       bUseCad = TRUE;
       bUseKbd = TRUE;
       }

    // Get system info
    bIsFastIo   = DosQueryFastIo();
    
#ifdef DEBUG
    Log("Use cadh.sys  = %d", bUseCad);
    Log("Use Monitor   = %d", bUseKbd);
    Log("fastio$       = %d", bIsFastIo);
#endif


    // Print program info
    printf("%s - %s\n", IDS_APPTITLE, VERSION);
    printf("%s\n", IDS_INTRO_CRIGHT);
    printf("%s\n", IDS_INTRO_LINE);
    if (bUseCad == TRUE)
       printf("%s\n", IDS_INTRO_ACTIVATECAD);
    if (bUseKbd == TRUE)
       printf("%s\n", IDS_INTRO_ACTIVATEKBD);


    // Program cannot run if detached
    if (DosQueryDetached())
       {
       printf("\n%s\n%s\n", IDS_ERROR_DETACHED, IDS_MSG_FATALEXIT);
#ifdef DEBUG
       Log("%s %s", IDS_ERROR_DETACHED, IDS_MSG_FATALEXIT);
#endif
       return (1);
       }


    if (bUseCad == TRUE)
       {
       // Open/create CADH semaphore
       rc = DosOpenEventSem(IDS_CADH_SEM, &hSemCadh);
       if (rc != 0)
          {
          if ( rc = DosCreateEventSem(IDS_CADH_SEM, &hSemCadh, 0, 0) )
             {
             printf("\n%s %d\n%s\n", IDS_ERROR_NOCADHSEM, rc, IDS_MSG_FATALEXIT);
#ifdef DEBUG
             Log("%s %d %s", IDS_ERROR_NOCADHSEM, rc, IDS_MSG_FATALEXIT);
#endif
             return (1);
             }
          }
       }

    // Create semaphore to replace DosSleep()
    DosCreateEventSem(NULL, &hSemSleep, 0, 0);

    // Create semaphore to sync popup
    DosCreateEventSem(NULL, &hSemPopup, 0, 0);

    // Create semaphore to sync exit program
    DosCreateEventSem(NULL, &hSemExit, 0, 0);

    // Start threads
    tidPopup  = _beginthread(DisplayPopUp, NULL, 8192, NULL);

    if (bUseKbd == TRUE)
       tidMonKbd = _beginthread(KeyboardMon, NULL, 8192, NULL);

    if (bUseCad == TRUE)
       tidMonCad = _beginthread(CadMon, NULL, 8192, NULL);

    // Wait for program to exit
    DosWaitEventSem(hSemExit, SEM_INDEFINITE_WAIT);
 
    // Kill devicedriver thread
    if (bUseCad == TRUE)
       DosPostEventSem(hSemCadh);

    // Close semaphores
    if (bUseCad == TRUE)
       DosCloseEventSem(hSemCadh);


    DosCloseEventSem(hSemPopup);
    DosCloseEventSem(hSemExit);
    DosCloseEventSem(hSemSleep);

    return (0);
    }

/*-----------------------------------------------------------------------------
   KeyboardMon: Look for keyboard press.
-----------------------------------------------------------------------------*/
void KeyboardMon(void *)
    {
    BOOL        bQuit = FALSE;
    ULONG       ulPostCt;


    PTIB        ptib = NULL;
    PPIB        ppib = NULL;
    int         prioType, prioLevel;

    KBDINFO     kdbInfo;

    USHORT      usCount;
    KEYPACKET   keybuf;
    HMONITOR    hmonKbd = (HMONITOR)0;
    MONIN       monInBuf = {0};
    MONOUT      monOutBuf = {0};



    // Get main thread priority
    DosGetInfoBlocks(&ptib, &ppib);

    prioType = ((ptib->tib_ptib2->tib2_ulpri) >> 8) & 0x00FF;
    prioLevel = (ptib->tib_ptib2->tib2_ulpri) & 0x001F;

    // Setup DosMonRead
    monInBuf.cb  = sizeof(MONIN); 
    monOutBuf.cb = sizeof(MONOUT);

    // Setup KbdGetStatus
    kdbInfo.cb = sizeof(KBDINFO);

    // While not requested to die
    while (bQuit == FALSE)
          {
          // Do not suck to much cpu
          DosWaitEventSem(hSemSleep, 1000);
              
          // Dont check unless popup is down 
          DosQueryEventSem(hSemPopup, &ulPostCt);
          if (ulPostCt == 0) 
             {
             // Check to see if we are in PM or not
             if (GetScreenGroup() > 1) 
                {
                // Set this thread as full speed (while monitor open)
                DosSetPrty(PRTYS_THREAD, PRTYC_TIMECRITICAL, +31, 0);

                // Open monitor chain (should always work)
                DosMonOpen("KBD$", &hmonKbd);

                // This is fullscreen so DosMonReg should work
                if (DosMonReg(hmonKbd, (PBYTE)&monInBuf, (PBYTE)&monOutBuf, MONITOR_DEFAULT, GetScreenGroup()) == NO_ERROR) 
                   {
                   // Try to read from monitor
                   usCount = sizeof(keybuf);
                   if (DosMonRead((PBYTE)&monInBuf, DCWW_WAIT, (PBYTE)&keybuf, &usCount) == NO_ERROR) 
                      {
                      // OK, send it further
                      DosMonWrite((PBYTE)&monOutBuf, (PBYTE)&keybuf, usCount);

                      // Only check when popup is down 
                      DosQueryEventSem(hSemPopup, &ulPostCt);
                      if (ulPostCt == 0) 
                         {
                         // If its our key, set semaphore
                         if ( (keybuf.cp.fsState & 256) && (keybuf.cp.fsState & 1024) )
                            {
#ifdef DEBUG
                            Log("Monitor fullscreen popup request");
#endif
                            DosPostEventSem(hSemPopup);
                            }
                         }              
                      }
                   }
                // Close monitor chain
                DosMonClose(hmonKbd);

                // Monitor now closed, lower the priority
                DosSetPriority (PRTYS_THREAD, prioType, prioLevel, 0);
                } 
             else 
                {
                // Only check when popup is down 
                DosQueryEventSem(hSemPopup, &ulPostCt);
                if (ulPostCt == 0) 
                   {
                   // This is PM, Use KbdGetStatus
                   KbdGetStatus(&kdbInfo, 0);

                   // If its our key, set semaphore
                   if ( (kdbInfo.fsState & 256) && (kdbInfo.fsState & 1024) )
                      {
#ifdef DEBUG
                      Log("Monitor in PM popup request");
#endif
                      DosPostEventSem(hSemPopup);
                      }
                   }              
                }
             }

          // Check if exit program is requested
          DosQueryEventSem(hSemExit, &ulPostCt);
          if (ulPostCt != 0) 
             bQuit = TRUE;
          }
    }


/*-----------------------------------------------------------------------------
   CadMon: Look for C-A-D from device driver.
-----------------------------------------------------------------------------*/
void CadMon(void *)
    {
    BOOL        bQuit = FALSE;
    ULONG       ulPostCt;

    APIRET	rc;
    HFILE	hfd;
    ULONG 	action,plen;



    // Try to open CADH
    if ( (rc = DosOpen(IDS_CADH_FILE, (PHFILE)&hfd, (PULONG)&action, (ULONG)0, FILE_SYSTEM, FILE_OPEN, OPEN_SHARE_DENYNONE | OPEN_FLAGS_NOINHERIT | OPEN_ACCESS_READONLY, (ULONG)0)) ) 
       {
       printf("\n%s %d\n%s\n", IDS_ERROR_NOCADH, rc, IDS_MSG_FATALEXIT);
#ifdef DEBUG
       Log("%s %d %s", IDS_ERROR_NOCADH, rc, IDS_MSG_FATALEXIT);
#endif
       // Ensure program dies !
       DosPostEventSem(hSemExit);
       return;
       }

    // Tell CADH to look for CAD
    plen = sizeof(hSemCadh);
    if ( (rc = DosDevIOCtl(hfd, (ULONG)0x80, (ULONG)0x00, (PULONG*)&hSemCadh, sizeof(hSemCadh), &plen, NULL, 0, NULL)) != 0) 
       {
       DosClose(hfd);
       printf("\n%s %d\n%s\n", IDS_ERROR_NOCADHON, rc, IDS_MSG_FATALEXIT);
#ifdef DEBUG
       Log("%s %d %s", IDS_ERROR_NOCADHON, rc, IDS_MSG_FATALEXIT);
#endif
       // Ensure program dies !
       DosPostEventSem(hSemExit);
       return;
       }


    // While not requested to die
    while (bQuit == FALSE)
          {
          // Only check when popup is down 
          DosQueryEventSem(hSemPopup, &ulPostCt);
          if (ulPostCt == 0) 
             {
             // Wait for CADH
             DosWaitEventSem(hSemCadh, SEM_INDEFINITE_WAIT);
             DosResetEventSem(hSemCadh, &ulPostCt);

#ifdef DEBUG
             Log("CAD popup request");
#endif

             // Show popup 
             DosPostEventSem(hSemPopup);
             }
          else
             {
             // If popup is up save CPU in main thread
             DosWaitEventSem(hSemSleep, 1000);
             }

          // Check if exit program is requested
          DosQueryEventSem(hSemExit, &ulPostCt);
          if (ulPostCt != 0) 
             bQuit = TRUE;
          }


    // Tell CADH to stop looking for CAD
    plen = sizeof(hSemCadh);
    if ( (rc = DosDevIOCtl(hfd, (ULONG)0x80, (ULONG)0x01, (PULONG*)&hSemCadh, sizeof(hSemCadh), &plen, NULL, 0, NULL)) != 0) 
       {
       DosClose(hfd);
       printf("\n%s %d\n%s\n", IDS_ERROR_NOCADHOFF, rc, IDS_MSG_FATALEXIT);
#ifdef DEBUG
       Log("%s %d %s", IDS_ERROR_NOCADHOFF, rc, IDS_MSG_FATALEXIT);
#endif
       // Ensure program dies !
       DosPostEventSem(hSemExit);
       return;
       }

    DosClose(hfd);
    }


/*-----------------------------------------------------------------------------
   DisplayPopUp: Show the app.
-----------------------------------------------------------------------------*/
void DisplayPopUp(void *)
    {
    char                szString[255];

    STVIOLISTBOX        lb;
    BOOL                bQuit = FALSE, bPopupQuit = FALSE,  bAppExit = FALSE;

    int                 iRet = 0, iCounter, iNumRecords;

    PSTVIO              pstv;
    PSTVIOWINDOW        pmainWnd;
    int                 aiRet[2];
    int		        iPos;

    PPROCINFO           pi;
    CHAR                *szDisplayList;
    PCHAR               *pszSelectList;   
    APIRET              rc;

    PTIB                ptib = NULL;
    PPIB                ppib = NULL;
    int                 prioType, prioLevel;

    PID                 pidWPS;

    USHORT              usOptions;

    BOOL                bListMethod;
    HSWITCH             hswSwitchTo;

    BOOL                bOpenCmdPrompt;


    // ### I doubt this should be hardcoded ###
    bListMethod = FALSE;

    hswSwitchTo = 0;
    bOpenCmdPrompt = FALSE;


    // Get this threads default performance
    DosGetInfoBlocks(&ptib, &ppib);

    prioType = ((ptib->tib_ptib2->tib2_ulpri) >> 8) & 0x00FF;
    prioLevel = (ptib->tib_ptib2->tib2_ulpri) & 0x001F;


#ifdef DEBUG
    Log("Popup PriorityType  before = %d", prioType);
    Log("Popup PriorityLevel before = %d", prioLevel);
#endif


    // Do this until user wants to quit
    while (bAppExit == FALSE)
          {
          // Wait for popup request
          DosWaitEventSem(hSemPopup, SEM_INDEFINITE_WAIT);

          // Ensure this thread runs full speed
          DosSetPriority (PRTYS_THREAD, PRTYC_TIMECRITICAL, +31, 0);

          pidWPS = GetWpsPid();
#ifdef DEBUG
          DosGetInfoBlocks(&ptib, &ppib);

          prioType = ((ptib->tib_ptib2->tib2_ulpri) >> 8) & 0x00FF;
          prioLevel = (ptib->tib_ptib2->tib2_ulpri) & 0x001F;

          Log("Popup PriorityType  after  = %d", prioType);
          Log("Popup PriorityLevel after  = %d", prioLevel);

          Log("Popup Show");
#endif

        
          // Do a viopopup, assume it always works.
          usOptions = 0;
          if (VioPopUp(&usOptions, 0) == NO_ERROR)
             {
             // Draw the window
             pstv = VioWinInit(-1, -1);

             VioSetGlobal(pstv, TRUE, 255, TRUE, TRUE, 30,
                          COLOR_WHITE, COLOR_BLUE,
                          COLOR_WHITE, COLOR_BLUE,
                          COLOR_WHITE, COLOR_BLUE,
                          COLOR_BLUE, COLOR_WHITE);

             sprintf(szString, IDS_APPTITLE);
             VioDesktop(pstv, szString, 177, TRUE);

             iPos = 78 - strlen(VERSION);
             VioStatusPrint(pstv, iPos, 0, FALSE, VERSION);
             VioStatusPrint(pstv, 2, 1, FALSE, IDS_APPKEYS);
             pmainWnd = VioOpenStandardWindow(pstv, 3, 3, 21, 75, IDS_WNDTITLE);

             // Prepare for the listbox.
             // --------------------------------------------
             memset(&lb, 0, sizeof(STVIOLISTBOX));

             lb.row1 = 4;
             lb.col1 = 3;
             lb.rowTo = 13;
             lb.colTo = 69;
             lb.startat = 0;
             lb.mark = FALSE;

             bPopupQuit = FALSE;
             while (bPopupQuit == FALSE)
                   {
                   VioColorPrint(pmainWnd, 2, 3, COLOR_WHITE, COLOR_YELLOW, "                                                                     ");

                   if (bListMethod)
                      {
                      VioColorPrint(pmainWnd, 2, 3, COLOR_WHITE, COLOR_YELLOW, IDS_TASKINFO_HEAD);
                      VioPrint(pmainWnd, 18, 3, IDS_WNDKEYST1);
                      VioPrint(pmainWnd, 19, 3, IDS_WNDKEYST2);
                      }
                   else
                      {
                      VioColorPrint(pmainWnd, 2, 3, COLOR_WHITE, COLOR_YELLOW, IDS_SWITCHINFO_HEAD);
                      VioPrint(pmainWnd, 18, 3, IDS_WNDKEYSS1);
                      VioPrint(pmainWnd, 19, 3, IDS_WNDKEYSS2);
                      }
                        
                   VioSetCursor(pmainWnd, 0, 0, FALSE);


                   if (bListMethod)
                      {
                      // Show running processes with amap info
                      //----------------------------------------
                      pi = (PPROCINFO) GetRunningProcesses(&iNumRecords);
                      szDisplayList = (char *)calloc(iNumRecords, sizeof(CHAR) * 80);
                      pszSelectList = (PCHAR *)calloc(iNumRecords, sizeof(PCHAR));

                      char          szProgType[16];
                      char          *pszBuffer;

                   
                      for (iCounter = 0; iCounter < iNumRecords; iCounter++) 
                          {
                          switch (pi[iCounter].iType)
                                 {
                                 case 0:
                                 case 1:
                                      sprintf(szProgType, IDS_PROGTYPE_SYSTEM);
                                      break;

                                 case 2:
                                      sprintf(szProgType, IDS_PROGTYPE_TEXT);
                                      break;

                                 case 3:
                                      sprintf(szProgType, IDS_PROGTYPE_PM);
                                      break;

                                 case 4:
                                      sprintf(szProgType, IDS_PROGTYPE_DETACH);
                                      break;
                                 }
                       
                          pszBuffer = szDisplayList + (iCounter * 80);
                          sprintf(pszBuffer, IDS_TASKINFO_DEF, 
                                  pi[iCounter].pid, szProgType, pi[iCounter].iThreads, pi[iCounter].szPidPath);

                          pszSelectList[iCounter] = pszBuffer;
                          }
                      }
                   else 
                      {
                      // Show switch list (minimal extra info)
                      //----------------------------------------
                      pi = (PPROCINFO) GetSwitchList(pidWPS, &iNumRecords);
                      szDisplayList = (char *)calloc(iNumRecords, sizeof(CHAR) * 80);
                      pszSelectList = (PCHAR *)calloc(iNumRecords, sizeof(PCHAR));

                      char          szProgType[16];
                      char          *pszBuffer;

                  
                      for (iCounter = 0; iCounter < iNumRecords; iCounter++) 
                          {
                          switch (pi[iCounter].iType)
                                 {
                                 case PROG_DEFAULT:
                                      sprintf(szProgType, IDS_PROGTYPE_DEFAULT);
                                      break;

                                 case PROG_FULLSCREEN:
                                 case PROG_WINDOWABLEVIO:
                                      sprintf(szProgType, IDS_PROGTYPE_TEXT);
                                      break;

                                 case PROG_PM:
                                      sprintf(szProgType, IDS_PROGTYPE_PM);
                                      break;

                                 case PROG_VDM:
                                 case PROG_WINDOWEDVDM:
                                      sprintf(szProgType, IDS_PROGTYPE_DOSWIN);
                                      break;

                                 case PROG_WPS:
                                      sprintf(szProgType, IDS_PROGTYPE_WPS);
                                      break;
                                 }
                       
                          pszBuffer = szDisplayList + (iCounter * 80);
                          sprintf(pszBuffer, IDS_SWITCHINFO_DEF, 
                                  pi[iCounter].szPidPath, szProgType);

                          pszSelectList[iCounter] = pszBuffer;
                          }
                      }


                   // Show the listbox
                   lb.llen = iNumRecords;
                   VioListBox(pmainWnd, &lb, 0, pszSelectList, aiRet, 0);

                   bQuit = FALSE;
                   while (bQuit == FALSE)
                         {
                         iRet = VioListBox(pmainWnd, &lb, 1, pszSelectList, aiRet, 0);
                         switch (iRet)
                                {
                                case ID_KEY_CLOSE_V:
                                case ID_KEY_CLOSE_G:
                                     if (!bListMethod)
                                        {
                                        WinPostMsg(pi[lb.curpos].hwnd, WM_CLOSE, 0, 0);
                                        DosWaitEventSem(hSemSleep, 1000);
                                        bQuit = TRUE;
                                        }
                                     break;


                                case ID_KEY_KILLWPS_V:
                                case ID_KEY_KILLWPS_G:
                                     if (VioMessageBox(pstv, 2, IDS_KILLWPSPROCESS, IDS_KILLWPSPROCESSQUERY, "Workplace Shell", "") == TRUE)
                                        {                               
                                        // Kill the WPS process 
                                        PID pidWPS = GetWpsPid();
                                        rc = DosKillProcess(DKP_PROCESS, pidWPS);
#ifdef DEBUG
                                        Log("DosKillProcess rc = %d for Workplace Shell (pid = %d)", rc, pidWPS);
#endif
                                        // No error, update                          
                                        if (!rc)
                                           {
                                           DosWaitEventSem(hSemSleep, 1000);
                                           bQuit = TRUE;
                                           }
                                        }
                                     break;


                                case ID_KEY_KILL_V:
                                case ID_KEY_KILL_G:
                                     if (VioMessageBox(pstv, 2, IDS_KILLPROCESS, IDS_KILLPROCESSQUERY, pi[lb.curpos].szPidPath, "") == TRUE)
                                        {                               
                                        // If pid is WPS (in WPS process) we cannot
                                        // do anything else than WM_CLOSE
                                        if (pi[lb.curpos].bIsWPS)
                                           {
                                           VioMessageBox(pstv, 0, IDS_KILLWPSAPP0, IDS_KILLWPSAPP1, IDS_KILLWPSAPP2, IDS_KILLWPSAPP3);
                                           WinPostMsg(pi[lb.curpos].hwnd, WM_CLOSE, 0, 0);
                                           DosWaitEventSem(hSemSleep, 1000);
                                           bQuit = TRUE;
                                           }
                                        else
                                           {
                                           rc = DosKillProcess(DKP_PROCESS, pi[lb.curpos].pid);
#ifdef DEBUG
                                           Log("DosKillProcess rc = %d for %s (pid = %d)", rc, pi[lb.curpos].szPidPath, pi[lb.curpos].pid);
#endif
                                           if (rc)
                                              {
                                              rc = DosKillFastIo(pi[lb.curpos].pid);
                                              DisplayError(pstv, "DosKillFastIo", rc);
                                              }
                                           DosWaitEventSem(hSemSleep, 1000);
                                           bQuit = TRUE;
                                           }
                                        }
                                     break;

                                // case ID_KEY_PRIORITY_V:
                                // case ID_KEY_PRIORITY_G:
                                //      if (bListMethod)
                                //         {
                                //         rc = DosSetPriority(PRTYS_PROCESS , PRTYC_IDLETIME, PRTYD_MINIMUM, pi[lb.curpos].pid);
                                //         DisplayError(pstv, "DosSetPriority", rc);
                                //         break;
                                //         }

                                case KEY_ENTER:
                                     if (!bListMethod)
                                        {
#ifdef DEBUG
                                        Log("Switch to requested: %s", pi[lb.curpos].szPidPath);
#endif
                                        hswSwitchTo = pi[lb.curpos].hSwitch;
                                        bQuit = TRUE;
                                        bPopupQuit = TRUE;
                                        }
                                     break;

                                case KEY_ESC:
                                     bQuit = TRUE;
                                     bPopupQuit = TRUE;
                                     break;

                                case KEY_F1:
                                     if (bListMethod)
                                        {
                                        VioMessageBox(pstv, 0, "*FEATURE*", "", "Not implemented yet !", "");
                                        }
                                     else
                                        {
                                        VioMessageBox(pstv, 0, "*FEATURE*", "", "Not implemented yet !", "");
                                        }
                                     break;

                                case KEY_F2:
                                     bListMethod = !bListMethod;
                                     bQuit = TRUE;
                                     break;
                                
                                case KEY_F3:
                                     if (bListMethod)
                                        {
                                        VioMessageBox(pstv, 0, "*FEATURE*", "", "Not implemented yet !", "");
                                        }
                                     else
                                        {
                                        VioMessageBox(pstv, 0, "*FEATURE*", "", "Not implemented yet !", "");
                                        }
                                     break;

                                case KEY_F4:
                                     if (VioMessageBox(pstv, 2, IDS_WPS, IDS_WPSYMWR, "", IDS_WPSAYS) == TRUE)
                                        ResetWPS();
                                     break;
                                
                                case KEY_F5:
                                     bOpenCmdPrompt = TRUE;
                                     bQuit = TRUE;
                                     bPopupQuit = TRUE;
                                     break;
                                
                                case KEY_F7:
                                     if (bListMethod)
                                        {
                                        if (VioMessageBox(pstv, 2, IDS_SHUTDOWN, IDS_SHUTDOWNYMWR, "", IDS_SHUTDOWNAYS) == TRUE)
                                           DisplayShutdown(pstv, FALSE);
                                        }
                                     break;
                                
                                case KEY_F8:
                                     if (VioMessageBox(pstv, 2, IDS_REBOOT, IDS_REBOOTYMWR, "", IDS_REBOOTAYS) == TRUE)
                                        DisplayShutdown(pstv, TRUE);
                                     break;

                                case KEY_F9:
                                     bQuit = TRUE;
                                     break;

                                case KEY_F10:
                                     bQuit = TRUE;
                                     bPopupQuit = TRUE;
                                     bAppExit = TRUE;
                                     break;
                                }
                         }
                   VioListBox(pmainWnd, &lb, 2, NULL, NULL, 0);

                   free(pi);

                   free(szDisplayList);
                   free(pszSelectList);
                   }

             VioCloseWindow(pmainWnd);
             VioWinExit(pstv);

             // End the popup
             VioEndPopUp(0);
             }

          // We are "popped down", reset semaphore to 
          // allow further popup's.
          unsigned long         ulPostCount;


          DosResetEventSem(hSemPopup, &ulPostCount);

          // Reset thread prio
          DosSetPriority (PRTYS_THREAD, prioType, prioLevel, 0);

          if (bOpenCmdPrompt == TRUE)
             {
             bOpenCmdPrompt = FALSE;
             hswSwitchTo = OpenCmdPrompt();
             }

          if (hswSwitchTo != NULL)
             {
             WinSwitchToProgram(hswSwitchTo);
             hswSwitchTo = NULL;
             }
#ifdef DEBUG
          Log("Popup Hide");
#endif
          }

    // User wants program to die 
    DosPostEventSem(hSemExit);
    }


/*-----------------------------------------------------------------------------
   GetRunningProcesses: 
-----------------------------------------------------------------------------*/
PVOID GetRunningProcesses(int *iNumRecords)
     {
     char               szPidPath[_MAX_PATH];
     char               *pBuf;
     QSPTRREC           *pRecHead;
     QSPREC             *pApp;
     int                iAntal = 0;
     PPROCINFO          pi;


     pBuf = (char*) malloc(0x8000);

     // Find out amount of processes
     DosQuerySysState(QS_PROCESS, 0, 0, 0, (char *) pBuf, 0x8000);

     pRecHead = (QSPTRREC *)pBuf;
     pApp = pRecHead->pProcRec;
     while (pApp->RecType == 1) 
           {
           iAntal++;
           pApp=(QSPREC *)((pApp->pThrdRec) + pApp->cTCB);
           }

     // Alloc struct for processes
     pi = (PPROCINFO) calloc(iAntal, sizeof(PROCINFO));

     // Get processes
     DosQuerySysState(QS_PROCESS, 0, 0, 0, (char *) pBuf, 0x8000);

     pRecHead = (QSPTRREC *)pBuf;
     pApp = pRecHead->pProcRec;

     *iNumRecords = iAntal;

     iAntal = 0;
     while (pApp->RecType == 1)
           {
           memset(szPidPath, 0, _MAX_PATH);
           DosQueryModuleName(pApp->hMte, 512, szPidPath);
           if (strlen(szPidPath) == 0)
              strcpy(szPidPath, IDS_SYSTEMTASK);

           pi[iAntal].pid = pApp->pid;
           pi[iAntal].pidParent = pApp->ppid;
           pi[iAntal].iType = pApp->type;
           pi[iAntal].iStatus = pApp->stat;
           pi[iAntal].iThreads = pApp->cTCB;
           strcpy(pi[iAntal].szPidPath, szPidPath);
           iAntal++;
           pApp=(QSPREC *)((pApp->pThrdRec) + pApp->cTCB);
           }

     free(pBuf);

     SortByPID(pi, *iNumRecords);

     return((PPROCINFO) pi);
     }


/*-----------------------------------------------------------------------------
   SortByPID: 
-----------------------------------------------------------------------------*/
void SortByPID(PPROCINFO pi, int iAntal)
     {
     PROCINFO   po;
     int        iOuter, iInner;


     for (iOuter = 0; iOuter < iAntal; iOuter++) 
         {
         for (iInner = 0; iInner < iAntal; iInner++) 
             {
             if (pi[iOuter].pid < pi[iInner].pid)
                {
                memcpy(&po, &pi[iOuter], sizeof(PROCINFO));
                memcpy(&pi[iOuter], &pi[iInner], sizeof(PROCINFO));
                memcpy(&pi[iInner], &po, sizeof(PROCINFO));
                }
             }
         }
     }


/*-----------------------------------------------------------------------------
   SortByName: 
-----------------------------------------------------------------------------*/
void SortByName(PPROCINFO pi, int iAntal)
     {
     PROCINFO   po;
     int        iOuter, iInner;


     for (iOuter = 0; iOuter < iAntal; iOuter++) 
         {
         for (iInner = 0; iInner < iAntal; iInner++) 
             {
             if (strcmp(pi[iOuter].szPidPath, pi[iInner].szPidPath) < 0)
                {
                memcpy(&po, &pi[iOuter], sizeof(PROCINFO));
                memcpy(&pi[iOuter], &pi[iInner], sizeof(PROCINFO));
                memcpy(&pi[iInner], &po, sizeof(PROCINFO));
                }
             }
         }
     }


/*-----------------------------------------------------------------------------
   GetSwitchList: 
-----------------------------------------------------------------------------*/
PVOID GetSwitchList(PID pidWPS, int *iNumRec)
     {
     PVOID              pBuffer;
     PSWBLOCK           pSB;
     PPROCINFO          pi;
     int                iCounter;
     int                iCount;
     int                iNumRecords;


     // Get switchlist
     iNumRecords = WinQuerySwitchList(NULLHANDLE, NULL, 0);
     pBuffer = malloc((iNumRecords * sizeof(SWENTRY)) + sizeof(HSWITCH));
     WinQuerySwitchList(NULLHANDLE, (SWBLOCK*)pBuffer, (iNumRecords * sizeof(SWENTRY)) + sizeof(HSWITCH));

     // Uses a pi array since easier to save pid etc.
     pi = (PPROCINFO) calloc(iNumRecords, sizeof(PROCINFO));

     // Move switch list info to pi
     pSB = (PSWBLOCK)(pBuffer);
     iCount = 0;
     for (iCounter = 0; iCounter < iNumRecords; iCounter++ ) 
         {
         // Dont add pid == 0 (switch to faked item)
         if (pSB->aswentry[iCounter].swctl.idProcess != 0)
            {
            // Dont add desktop (dont want to be able to kill it)
            if ( strcmpi(pSB->aswentry[iCounter].swctl.szSwtitle, IDS_DESKTOP) != 0 )
               {
               pi[iCount].pid = pSB->aswentry[iCounter].swctl.idProcess;
               pi[iCount].iType = pSB->aswentry[iCounter].swctl.bProgType;
               strcpy(pi[iCount].szPidPath, pSB->aswentry[iCounter].swctl.szSwtitle);
               pi[iCount].hSwitch = pSB->aswentry[iCounter].hswitch;

               if (pSB->aswentry[iCounter].swctl.idProcess == pidWPS)
                  {
                  pi[iCount].bIsWPS = TRUE;
                  pi[iCount].iType = PROG_WPS;
                  }
               else
                  {
                  pi[iCount].bIsWPS = FALSE;
                  }

               pi[iCount].hwnd = pSB->aswentry[iCounter].swctl.hwnd;
   
               iCount++;
               }
            }
         }
     free(pBuffer);

     *iNumRec = iCount;
     SortByName(pi, iCount);

     return((PPROCINFO) pi);
     }


/*-----------------------------------------------------------------------------
   DisplayShutdown: 
-----------------------------------------------------------------------------*/
void DisplayShutdown(PSTVIO pstv, BOOL bFlag)
     {
     PSTVIOWINDOW     pwnd;

     HAB                hab      = NULLHANDLE;
     HMQ                hmq      = NULLHANDLE;
     HFILE              hIOCTL;
     unsigned long      ulAction;
     int                iRet;



#ifdef DEBUG
     Log("DisplayShutDown  = %d", bFlag);
#endif

     pwnd = VioOpenStandardWindow(pstv, 7, 12, 11, 55, IDS_SD_TITLE);

     VioPrint(pwnd, 3, 4, IDS_SD_WAIT);

     hab = WinInitialize(0);
     hmq = WinCreateMsgQueue(hab, 0);
 
     // Prevent our program from hanging the shutdown.  If this call is
     // omitted, the system will wait for us to do a WinDestroyMsgQueue.
     WinCancelShutdown(hmq, TRUE);

     // Open DOS$ (must be done before DosShutDown
     iRet = DosOpen(IDS_DRIVER_DOS, &hIOCTL, &ulAction, 0L, 0, 1, 0x0011, 0L);

     /* Shutdown the system! */
     DosShutdown(0);

     if (bFlag) 
        { 
        VioPrint(pwnd, 3, 4, IDS_SD_CLEARLINE);
        VioPrint(pwnd, 4, 4, IDS_SD_RESTART1);
        VioPrint(pwnd, 6, 4, IDS_SD_RESTART2);
        /* Restart the machine */
        DosDevIOCtl(hIOCTL, 0xd5, 0xab, NULL, 0, NULL, NULL, 0, NULL);     
        }
     else
        {
        VioPrint(pwnd, 3, 4, IDS_SD_CLEARLINE);
        VioPrint(pwnd, 4, 4, IDS_SD_SHUTDOWN1);
        VioPrint(pwnd, 6, 4, IDS_SD_SHUTDOWN2);
        }

     // Wait forever
     while (0 == 0)
           VioGetKey(pwnd, TRUE, FALSE);

     VioCloseWindow(pwnd);
     }


/*-----------------------------------------------------------------------------
   DisplayError: 
-----------------------------------------------------------------------------*/
void DisplayError(PSTVIO pstv, char *szErrorFunc, int rc)
     {
     char       szString[256];

     
     if (rc != NO_ERROR)
        {
        switch (rc) 
               {
               case 3:
                    sprintf(szString, "Driver not loaded: %d", rc);
                    break;

               case 13:
                    sprintf(szString, "ERROR_INVALID_DATA: %d", rc);
                    break;

               case 217:
                    sprintf(szString, "ERROR_ZOMBIE_PROCESS: %d", rc);
                    break;

               case 303:
                    sprintf(szString, "ERROR_INVALID_PROCID: %d", rc);
                    break;

               case 304:
                    sprintf(szString, "ERROR_INVALID_PDELTA: %d", rc);
                    break;

               case 305:
                    sprintf(szString, "ERROR_NOT_DESCENDANT: %d", rc);
                    break;

               case 307:
                    sprintf(szString, "ERROR_INVALID_PCLASS: %d", rc);
                    break;

               case 308:
                    sprintf(szString, "ERROR_INVALID_SCOPE: %d", rc);
                    break;

               case 309:
                    sprintf(szString, "ERROR_INVALID_THREADID: %d", rc);
                    break;

               default:
                    sprintf(szString, "other error : %d", rc);
                    break;
                    }
        VioMessageBox(pstv, 0, "*FAILURE*", szErrorFunc, szString, "");
        }
     }


//-----------------------------------------------------------------------------
//  OpenCmdPrompt: Opens 'emergency' command prompt
//-----------------------------------------------------------------------------
HSWITCH OpenCmdPrompt(void)
     {
     STARTDATA          sd = {0};
     PID                pid;
     ULONG              ulSessID;
     APIRET             rc;

     PVOID              pBuffer;
     PSWBLOCK           pSB;
     int                iCounter;
     int                iNumRecords;
     HSWITCH            hswSwitchTo;


     sd.Length  = sizeof(STARTDATA);
     sd.Related = SSF_RELATED_INDEPENDENT;
     sd.FgBg    = SSF_FGBG_BACK;
     sd.TraceOpt = SSF_TRACEOPT_NONE;
     sd.PgmTitle = IDS_CMDLINETITLE;
     sd.PgmName = "CMD.EXE";
     sd.PgmInputs = "/K";
     sd.TermQ = 0;
     sd.Environment = 0;
     sd.InheritOpt = SSF_INHERTOPT_SHELL;
     sd.SessionType = SSF_TYPE_FULLSCREEN;

     rc = DosStartSession(&sd, &ulSessID, &pid);  /* Start the session */

#ifdef DEBUG
     if (rc)
        Log("DosStartSession rc=%d ", rc);
#endif

     // Get switchlist
     iNumRecords = WinQuerySwitchList(NULLHANDLE, NULL, 0);
     pBuffer = malloc((iNumRecords * sizeof(SWENTRY)) + sizeof(HSWITCH));
     WinQuerySwitchList(NULLHANDLE, (SWBLOCK*)pBuffer, (iNumRecords * sizeof(SWENTRY)) + sizeof(HSWITCH));

     // Find recently started window
     hswSwitchTo = NULL;

     pSB = (PSWBLOCK)(pBuffer);
     for (iCounter = 0; iCounter < iNumRecords; iCounter++ ) 
         {
         if ( strncmp(pSB->aswentry[iCounter].swctl.szSwtitle, IDS_CMDLINETITLE, IDI_CMDLINETITLELEN) == 0 ) 
            hswSwitchTo = pSB->aswentry[iCounter].hswitch;
         }
     free(pBuffer);

#ifdef DEBUG
     if (hswSwitchTo == NULL)
        Log("Switchhandle for %s not found", IDS_CMDLINETITLE);
#endif

     return(hswSwitchTo);
     }


//-----------------------------------------------------------------------------
//  GetTID1Prio: Get priority for thread 1 in a process
//-----------------------------------------------------------------------------
int GetTID1Prio(PID pid)
     {
     char               *pBuf;
     QSPTRREC           *pRecHead;
     QSPREC             *pApp;
     int                iPrio;


     pBuf = (char*) malloc(32768);
     DosQuerySysState(QS_SUPPORTED, 0, pid, 0, (char *) pBuf, 32768);

     pRecHead = (QSPTRREC *)pBuf;

     pApp = pRecHead->pProcRec;

     if (pApp->pThrdRec->tid == 1)
        iPrio = pApp->pThrdRec->priority;
     else
        iPrio = 0;

     free(pBuf);

     return (iPrio);
     }


//-----------------------------------------------------------------------------
// GetScreenGroup: Get current foreground screen group
//-----------------------------------------------------------------------------
int GetScreenGroup(void)
    {
    SEL         selGSeg,selLSeg;
    PGINFOSEG   GSeg;


    DosGetInfoSeg(&selGSeg,&selLSeg);
    GSeg=(PGINFOSEG)((selGSeg &~7)<<13);

    return(GSeg->sgCurrent);
    }



//-----------------------------------------------------------------------------
//   GetWpsPid: Get PID for WPS process
//-----------------------------------------------------------------------------
PID GetWpsPid(void)
     {
     PVOID              pBuffer;
     PSWBLOCK           pSB;
     int                iCounter;
     int                iNumRecords;
     PID                pidWPS;


     // Assume not found
     pidWPS = NULL;

     // Get switchlist
     iNumRecords = WinQuerySwitchList(NULLHANDLE, NULL, 0);
     pBuffer = malloc((iNumRecords * sizeof(SWENTRY)) + sizeof(HSWITCH));
     WinQuerySwitchList(NULLHANDLE, (SWBLOCK*)pBuffer, (iNumRecords * sizeof(SWENTRY)) + sizeof(HSWITCH));

     // Move switch list info to pi
     pSB = (PSWBLOCK)(pBuffer);
     for (iCounter = 0; iCounter < iNumRecords; iCounter++ ) 
         {
         if ( strncmp(pSB->aswentry[iCounter].swctl.szSwtitle, "Workplace Shell", 15) == 0 ) 
            pidWPS = pSB->aswentry[iCounter].swctl.idProcess;
         }
     free(pBuffer);

     return(pidWPS);
     }


//-----------------------------------------------------------------------------
// DosQueryDetached: Checks if current application runs detached.
//-----------------------------------------------------------------------------
BOOL DosQueryDetached(void)
     {
     PPIB        ppib = NULL;


     // Get process information
     DosGetInfoBlocks(NULL, &ppib);

     if (ppib->pib_ultype == 4)
        return (TRUE);
     else
        return (FALSE);
     }


//-----------------------------------------------------------------------------
//  DosQueryFastIo: Checks if fastio$ (by Holger Veit) is loaded.
//-----------------------------------------------------------------------------
BOOL DosQueryFastIo(void)
     {
     HFILE	hfd;
     ULONG 	action,plen;


     if ( DosOpen((PSZ)IDS_FASTIO, (PHFILE)&hfd, (PULONG)&action, (ULONG)0, FILE_SYSTEM, FILE_OPEN, OPEN_SHARE_DENYNONE|OPEN_FLAGS_NOINHERIT|OPEN_ACCESS_READONLY, (ULONG)0) ) 
        {
        return (FALSE);
	}
     else
        {
        DosClose(hfd);
        return (TRUE);
        }
     }


//-----------------------------------------------------------------------------
//  DosKillFastIo: Kill process using fastio$ (by Holger Veit).
//-----------------------------------------------------------------------------
APIRET DosKillFastIo(PID pid)
       {
       APIRET	rc;
       HFILE	hfd;
       ULONG 	action,plen;



#ifdef DEBUG
     Log("DosKillFastIo = %d", pid);
#endif

       if ( (rc = DosOpen((PSZ)IDS_FASTIO, (PHFILE)&hfd, (PULONG)&action, (ULONG)0, FILE_SYSTEM, FILE_OPEN, OPEN_SHARE_DENYNONE|OPEN_FLAGS_NOINHERIT|OPEN_ACCESS_READONLY, (ULONG)0)) ) 
          {
          return rc;
	  }

       if ( (rc = DosDevIOCtl(hfd, (ULONG)0x76, (ULONG)0x65, (PULONG*)&pid,sizeof(USHORT),&plen, NULL, 0, NULL)) != 0) 
          {
          DosClose(hfd);
          return rc;
          }
       DosClose(hfd);
       return NO_ERROR;
       }


//-----------------------------------------------------------------------------
//   ResetWPS: (Hopefully) restarts WPS.
//-----------------------------------------------------------------------------
BOOL ResetWPS(void)
     { 
     BOOL       bRc;
     HAB        habShutdown = WinInitialize(0);
     HMQ        hmq = WinCreateMsgQueue(habShutdown, 0);


#ifdef DEBUG
     Log("ResetWPS requested");
#endif

     // find out current profile names
     PRFPROFILE Profiles;
     Profiles.cchUserName = Profiles.cchSysName = 0;
     
     // Assume failure
     bRc = FALSE;

     // first query their file name lengths
     if (PrfQueryProfile(habShutdown, &Profiles))
        {
        // allocate memory for filenames
        Profiles.pszUserName  = (char *)malloc(Profiles.cchUserName);
        Profiles.pszSysName  = (char *)malloc(Profiles.cchSysName);

        if (Profiles.pszSysName)
           {
           // get filenames
           if (PrfQueryProfile(habShutdown, &Profiles))
              {
              // "change" INIs to these filenames:
              // THIS WILL RESET THE WPS
              if (PrfReset(habShutdown, &Profiles))
                 bRc = TRUE;

              free(Profiles.pszSysName);
              free(Profiles.pszUserName);
              }
           }
        }
     WinDestroyMsgQueue(hmq);
     WinTerminate(habShutdown);

     return(bRc);
     }


//-----------------------------------------------------------------------------
//  Log: Skickar information till log-filen.
//-----------------------------------------------------------------------------
void Log(char *szFormat, ...)
     {
     va_list            argptr;

     FILE               *hLogFile;
     time_t             ltime;
     struct tm          *curtime;

     char               szData[1024];



     va_start(argptr, szFormat);
     vsprintf(szData, szFormat, argptr);
     va_end(argptr);

     time(&ltime);
     curtime = localtime(&ltime);

     hLogFile = fopen(".\\procman.txt", "a");

     fprintf(hLogFile, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d\t%s\n",
             1900 + curtime->tm_year, curtime->tm_mon + 1, curtime->tm_mday,
             curtime->tm_hour, curtime->tm_min + 1, curtime->tm_sec,
             szData);
     fclose(hLogFile);
     }


