                   *****-------------------------------*****
                               P r o c M a n
                   *****-------------------------------*****
                       a process manager for eComStation

                           Beta 4 - April 6, 2004
                                   J M A

                  ** BETA VERSION - USE AT YOUR OWN RISK !! **


What is ProcMan ?
-------------------------------------------------------------------------------
ProcMan is a process manager for eComStation. 

ProcMan is tested on eComStation but should work on Warp 4 and Warp Server 
for eBusiness.

Its beta quality but it does work. It kills processes quite good but there 
are still missing features and its not that well tested.



Installation
-------------------------------------------------------------------------------
This program should be started in PM and will not work if started as detached. 
The easiest way is to place the application exefile somewhere (need not be in 
the path) and create a object/shortcut in your startup folder. 
You should run it minimized.


== xf86sup.sys ==
To enhance the kill'ing feature of ProcMan you may optionally use Holger Veit's
xf86sup.sys driver. It contains a "hard kill" feature that may help killing
applications that does not die without it.

- Download the xf86sup.zip package from hobbes
  http://hobbes.nmsu.edu/pub/os2/system/x11/XFree86/xf86sup.zip
  or a newer version from
  ftp://ftp.xfreeos2.dyndns.org/pub/misc/xfreeos2/4.3.0/Lib.zip

- Extract the file xf86sup.sys 
- Copy the file xf86sup.sys to a directory of your choice.
- Install it into your config.sys:
  DEVICE=X:\DIR\cadh.sys
  Where X:\DIR is the directory you placed xf86sup.sys in.
- Reboot


== cadh.sys ==
You can also (optionally) use Veit Kannegieser's CADH.SYS driver that allows 
you to use Ctrl-Alt-Del to activate ProcMan. Using this will enable you to 
activate ProcMan even if PM has crashed.

- Download the cadh.zip package from hobbes
  http://hobbes.nmsu.edu/pub/os2/util/process/cadh.zip
- Extract the file cadh.sys 
- Copy the file cadh.sys into your \OS2\BOOT directory.
- Install it into your config.sys:
  BASEDEV=cadh.sys
- Reboot

NOTE: Do *NOT* run the CAD_POP.EXE deamon program. 
      Doing so will stop you from activating ProcMan.


Usage
-------------------------------------------------------------------------------
You should always start ProcMan with a parameter:
pm k       Uses Ctrl-Ctrl as activation key
pm c       Uses Ctrl-Alt-Del as activation key, requires cadh.sys !
pm kc      Uses both methods (also requires cadh.sys !)

When the program is properly installed and running press the activation key.

A fullscreen window will pop up showing running programs in your system.

This view is similar to the "window list" in PM (Ctrl-Esc) but you will see
hidden and "non jumpable" programs. And you have a few more 

Keys help:
Arrow keys	Move up and down in the list of programs
Enter           Switches to selected program
C               Closes the selected program (clean exit)
K		Kills the selected program (unclean exit)
Esc		Exit the popup (ProcMan continues in background)
F1		Shows help screen
F2              Switch to task display
F3              Show information about seleted program
F4              Restart Workplace Shell
F5              Opens fullscreen commandline prompt
F8              Reboot
F9	        Refresh the list of running programs
F10		Exits and unloads ProcMan


If you press F2 the view will change to showing all the running processes in 
your system. This view shows more technical information on running processes
and you should be aware that its much easier to destroy things in this view.

Keys help:
Arrow keys	Move up and down in the list of processes
K		Kill a process a normal way (DosKillProcess)
W		Kill WPS
Esc		Exit the popup (ProcMan continues in background)
F1		Shows help screen
F2              Switch back to program display
F3              Show information about seleted process
F4              Restart Workplace Shell
F7              Shutdown
F8              Reboot
F9	        Refresh the list of running programs
F10		Exits and unloads ProcMan


**** Warning ****
Killing processes can kill your entire system !!
ProcMan can in many cases kill a process that OS/2 is unable to kill. 
If some application hangs WPS its likely it can be killed. However, when a 
program is "hard" killed its unsaved data is lost and it may leave system 
resources in use that cannot be restored without a reboot.
Use ProcMan as a last chance method. 
Unless you are really sure - restart your system as soon as possible after
killing a process with ProcMan.



Uninstallation
-------------------------------------------------------------------------------
Remove the files from this archive.
Remove any object/shortcut in your startup folder.
Optionally remove cadh.sys and xf86sup.sys from disk and config.sys



Contact
-------------------------------------------------------------------------------
You can contact the developer (me :) at:
mail(AT)jma(DOT)se


