@echo off
setlocal enabledelayedexpansion

set srcfile=libsylixos.a

nm %srcfile% > %srcfile%_nm

findstr /C:" T "   < %srcfile%_nm    > func.txt
findstr /C:" W "   < %srcfile%_nm   >> func.txt
findstr /C:" D "   < %srcfile%_nm    > obj.txt
findstr /C:" B "   < %srcfile%_nm   >> obj.txt
findstr /C:" R "   < %srcfile%_nm   >> obj.txt
findstr /C:" S "   < %srcfile%_nm   >> obj.txt
findstr /C:" C "   < %srcfile%_nm   >> obj.txt
findstr /C:" V "   < %srcfile%_nm   >> obj.txt
findstr /C:" G "   < %srcfile%_nm   >> obj.txt

del %srcfile%_nm

set num=0

del symbol.c  1>NUL 2>&1
del symbol.h  1>NUL 2>&1
del symbol.ld 1>NUL 2>&1

echo -Wl,--no-undefined                                   >> symbol.ld
echo -Wl,--no-allow-shlib-undefined                       >> symbol.ld
echo -Wl,--error-unresolved-symbols                       >> symbol.ld
echo -Wl,--unresolved-symbols=report-all                  >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,bspDebugMsg           >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,udelay                >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,ndelay                >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,bspDelayUs            >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,bspDelayNs            >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,bspInfoCpu            >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,bspInfoCache          >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,bspInfoPacket         >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,bspInfoVersion        >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,bspInfoHwcap          >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,bspInfoRomBase        >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,bspInfoRomSize        >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,bspInfoRamBase        >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,bspInfoRamSize        >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,bspTickHighResolution >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,null                  >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,__lib_strerror        >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,_execl                >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,_execle               >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,_execlp               >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,_execv                >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,_execve               >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,_execvp               >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,_execvpe              >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,_spawnl               >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,_spawnle              >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,_spawnlp              >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,_spawnv               >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,_spawnve              >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,_spawnvp              >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,_spawnvpe             >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,_longjmp              >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,_setjmp               >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,__aeabi_read_tp       >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,msgget                >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,msgctl                >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,msgrcv                >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,msgsnd                >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,semget                >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,semctl                >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,semop                 >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,semtimedop            >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,shmget                >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,shmctl                >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,shmat                 >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,shmdt                 >> symbol.ld
echo -Wl,--ignore-unresolved-symbol,ftok                  >> symbol.ld

echo /********************************************************************************************************* >> symbol.c
echo ** 													>> symbol.c
echo **                                    �й������Դ��֯					>> symbol.c
echo **														>> symbol.c
echo **                                   Ƕ��ʽʵʱ����ϵͳ				>> symbol.c
echo **														>> symbol.c
echo **                                       SylixOS(TM)	>> symbol.c
echo **														>> symbol.c
echo **                               Copyright  All Rights Reserved		>> symbol.c
echo **														>> symbol.c
echo **--------------�ļ���Ϣ--------------------------------------------------------------------------------	>> symbol.c
echo **														>> symbol.c
echo ** ��   ��   ��: symbol.c								>> symbol.c
echo **														>> symbol.c
echo ** ��   ��   ��: makesymbol ����						>> symbol.c
echo **														>> symbol.c
echo ** �ļ���������: %date:~0,4% �� %date:~5,2% �� %date:~8,2% ��			>> symbol.c
echo **														>> symbol.c
echo ** ��        ��: ϵͳ sylixos ���ű�. (���ļ��� makesymbol �����Զ�����, �����޸�)	>> symbol.c
echo *********************************************************************************************************/	>> symbol.c
echo #ifdef __GNUC__                                        >> symbol.c
echo #if __GNUC__ ^<^= 4                                    >> symbol.c
echo #pragma GCC diagnostic warning "-w"                    >> symbol.c
echo #elif __GNUC__ ^>^= 7                                  >> symbol.c
echo #pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch" >> symbol.c
echo #endif                                                 >> symbol.c
echo #endif                                                 >> symbol.c
echo.														>> symbol.c
echo #include "symboltools.h"								>> symbol.c
echo.														>> symbol.c
echo #define SYMBOL_TABLE_BEGIN LW_STATIC_SYMBOL   _G_symLibSylixOS[] = {	>> symbol.c
echo.  														>> symbol.c
echo #define SYMBOL_TABLE_END };							>> symbol.c
echo.														>> symbol.c
echo #define SYMBOL_ITEM_FUNC(pcName)                       \>> symbol.c
echo     {   {(void *)0, (void *)0},                        \>> symbol.c
echo         #pcName, (char *)pcName,                       \>> symbol.c
echo         LW_SYMBOL_TEXT                                 \>> symbol.c
echo     },													>> symbol.c
echo.														>> symbol.c
echo #define SYMBOL_ITEM_OBJ(pcName)                       \>> symbol.c
echo     {   {(void *)0, (void *)0},                       \>> symbol.c
echo         #pcName, (char *)^&pcName,                     \>> symbol.c
echo         LW_SYMBOL_DATA                                \>> symbol.c
echo     },													>> symbol.c
echo.														>> symbol.c
echo /*********************************************************************************************************	>> symbol.c
echo   ȫ�ֶ�������											>> symbol.c
echo *********************************************************************************************************/	>> symbol.c
echo #ifdef SYLIXOS_EXPORT_KSYMBOL							>> symbol.c

for /f "tokens=3 delims= " %%i in (func.txt) do @(
    if not "%%i"=="__sylixos_version" (
        echo extern int  %%i^(^); >> symbol.c
        echo -Wl,--ignore-unresolved-symbol,%%i >> symbol.ld
        set /a num+=1
    )
)

for /f "tokens=3 delims= " %%i in (obj.txt) do @(
    if not "%%i"=="__sylixos_version" (
        echo extern int  %%i; >> symbol.c
        echo -Wl,--ignore-unresolved-symbol,%%i >> symbol.ld
        set /a num+=1
    )
)

echo.
echo /*********************************************************************************************************	>> symbol.c
echo   ϵͳ��̬���ű�										>> symbol.c
echo *********************************************************************************************************/	>> symbol.c
echo SYMBOL_TABLE_BEGIN										>> symbol.c

for /f "tokens=3 delims= " %%i in (func.txt) do @(
    if not "%%i"=="__sylixos_version" (
        echo     SYMBOL_ITEM_FUNC^(%%i^) >> symbol.c
    )
)

for /f "tokens=3 delims= " %%i in (obj.txt) do @(
    if not "%%i"=="__sylixos_version" (
        echo     SYMBOL_ITEM_OBJ^(%%i^) >> symbol.c
    )
)
echo SYMBOL_TABLE_END										>> symbol.c
echo #endif													>> symbol.c
echo /*********************************************************************************************************	>> symbol.c
echo   END													>> symbol.c
echo *********************************************************************************************************/	>> symbol.c


echo /*********************************************************************************************************	>> symbol.h
echo **														>> symbol.h
echo **                                    �й������Դ��֯	>> symbol.h
echo **														>> symbol.h
echo **                                   Ƕ��ʽʵʱ����ϵͳ			>> symbol.h
echo **														>> symbol.h
echo **                                       SylixOS(TM)	>> symbol.h
echo **														>> symbol.h
echo **                               Copyright  All Rights Reserved	>> symbol.h
echo **														>> symbol.h
echo **--------------�ļ���Ϣ--------------------------------------------------------------------------------	>> symbol.h
echo **														>> symbol.h
echo ** ��   ��   ��: symbol.h								>> symbol.h
echo **														>> symbol.h
echo ** ��   ��   ��: makesymbol ����						>> symbol.h
echo **														>> symbol.h
echo ** �ļ���������: %date:~0,4% �� %date:~5,2% �� %date:~8,2% ��		>> symbol.h
echo **														>> symbol.h
echo ** ��        ��: ϵͳ sylixos ���ű�. (���ļ��� makesymbol �����Զ�����, �����޸�)	>> symbol.h
echo *********************************************************************************************************/	>> symbol.h
echo.														>> symbol.h
echo #ifndef __SYMBOL_H										>> symbol.h
echo #define __SYMBOL_H										>> symbol.h
echo.														>> symbol.h
echo #include "SylixOS.h"									>> symbol.h
echo #include "symboltools.h"								>> symbol.h
echo.														>> symbol.h
echo #ifdef SYLIXOS_EXPORT_KSYMBOL							>> symbol.h
echo #define SYM_TABLE_SIZE %num%							>> symbol.h
echo extern  LW_STATIC_SYMBOL  _G_symLibSylixOS[SYM_TABLE_SIZE];					>> symbol.h
echo.														>> symbol.h	
echo static LW_INLINE  INT symbolAddAll (VOID)				>> symbol.h
echo {														>> symbol.h
echo     return  (symbolAddStatic((LW_SYMBOL *)_G_symLibSylixOS, SYM_TABLE_SIZE));	>> symbol.h
echo }														>> symbol.h
echo #else													>> symbol.h
echo static LW_INLINE  INT symbolAddAll (VOID)				>> symbol.h
echo {														>> symbol.h
echo     return  (ERROR_NONE);								>> symbol.h
echo }														>> symbol.h
echo #endif													>> symbol.h
echo.														>> symbol.h
echo #endif                                                                  /*  __SYMBOL_H                  */	>> symbol.h
echo /*********************************************************************************************************	>> symbol.h
echo   END													>> symbol.h
echo *********************************************************************************************************/	>> symbol.h

del func.txt
del obj.txt
@echo on