project : D:\SOURCE\c\ecs\pm\pm.exe .SYMBOLIC
!define BLANK ""
D:\SOURCE\c\ecs\pm\pm.obj : D:\SOURCE\c\ecs\pm\pm.cpp .AUTODEPEND
 @D:
 cd D:\SOURCE\c\ecs\pm
 *wpp386 pm.cpp -i=C:\WATCOM\h;C:\TOOLKIT\h;C:\BIN\h; -w4 -e25 -zp4 -zq -od -d2 -bm -5r -bt=os2 -mf -dDEBUG

D:\SOURCE\c\ecs\pm\pm.exe : D:\SOURCE\c\ecs\pm\pm.obj .AUTODEPEND
 @D:
 cd D:\SOURCE\c\ecs\pm
 @%write pmd.lk1 FIL pm.obj
 @%append pmd.lk1 
!ifneq BLANK ""
 *wlib -q -n -b pm.imp 
 @%append pmd.lk1 LIBR pm.imp
!endif
 *wlink name pm d all SYS os2v2 libr stcond.lib,ststd.lib,stwind.lib,mysql.lib op maxe=25 op caseex op q @pmd.lk1
 del pmd.lk1
 mc pm.ver
