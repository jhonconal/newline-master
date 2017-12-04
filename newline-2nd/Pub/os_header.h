/************************************************************
Copyright (C), 2012-2014, HongHe Tech.
FileName       : os_header.h
Date           : 2012.03.12
Description    : 各平台引用的头文件声明
Version        : 无
Function List  :
        1.
History:   1. 创建文件2013.03.12
***********************************************************/

#ifndef OS_HEADER_H
#define OS_HEADER_H

//define common header files across platform
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <ctype.h>
#include <fcntl.h>

#include <QString>      //QString
#include <QStringList>
#include <QDir>
#include <QPair>
#include <QTime>
#include <QTimer>
#include <QTextCodec>
#include <QCoreApplication>
#include <QSettings>
#include <QDebug>
#ifndef VOS_WINDOWS
#define VOS_WINDOWS
#endif

#ifdef VOS_WINDOWS
#include <windows.h>
#include <winbase.h>
#include <devguid.h>      //GUID_DEVCLASS_PORTS
#include <SetupAPI.h>     //HDEVINFO
#pragma comment(lib,"setupapi.lib")

// adding "pragma warning" to avoid C4995 waning: "gets(vsprintf):#pragma deprecated"
#pragma warning(push)
#pragma warning(disable:4995)
#include <Dshow.h>        //ICaptureGraphBuilder2,IBaseFilter
#pragma warning(pop)
#pragma warning(disable:4995)
// add end


#include <process.h>        //_beginthreadex
//#include <atlstr.h>     //cstring
//#include <comdef.h>
//#include <QLibrary>
//#include <QList>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <termios.h>
#include <sys/ioctl.h>          // ioctl TIOCEXCL

#endif



#endif // OS_HEADER_H
