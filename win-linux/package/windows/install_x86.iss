﻿
#define os_arch 'win_32'
#define PATH_PREFIX 'win_32\build'
#define VC_REDIST_VER 'vcredist_x86.exe'

#include "common.iss"


[Setup]
OutputBaseFileName    =DesktopEditors_x86
MinVersion            =6.1
;ArchitecturesAllowed    =x86


[Files]
#ifndef SCRIPT_CUSTOM_FILES
Source: data\libs\qt\win32\*;                                         DestDir: {app}\; Flags: ignoreversion recursesubdirs;
#endif


