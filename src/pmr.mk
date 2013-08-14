project : D:\SOURCE\c\ecs\pm\pm.exe .SYMBOLIC
!define BLANK ""
D:\SOURCE\c\ecs\pm\pm.obj : D:\SOURCE\c\ecs\pm\pm.cpp .AUTODEPEND
 @D:
 cd D:\SOURCE\c\ecs\pm
 *wpp386 pm.cpp -i=C:\WATCOM\h;C:\TOOLKIT\h;C:\BIN\h; -w4 -e25 -zp4 -zq -otexan -bm -5r -bt=os2 -mf -dRELEASE

D:\SOURCE\c\ecs\pm\pm.exe : D:\SOURCE\c\ecs\pm\pm.obj .AUTODEPEND
 @D:
 cd D:\SOURCE\c\ecs\pm
 @%write pmr.lk1 FIL pm.obj
 @%append pmr.lk1 
!ifneq BLANK ""
 *wlib -q -n -b pm.imp 
 @%append pmr.lk1 LIBR pm.imp
!endif
 *wlink name pm SYS os2v2 libr stconr.lib,ststr.lib,stwinr.lib,mysql.lib op maxe=25 op caseex op q @pmr.lk1
 del pmr.lk1
 mc pm.ver
