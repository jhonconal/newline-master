/************************************************************
Copyright (C), 2012-2014, HongHe Tech.
FileName       : global.cpp
Date           : 2012.03.12
Description    : 全局函数定义文件
Version        : 无
Function List  :
        1.
History:   1. 创建文件2013.03.12
***********************************************************/
#include <QTime>
#include <QMessageBox>
#include <QString>


#include "global.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* 日志文件指针 */
INT32       g_nLogLevel                 = 7;
FILE *g_pLogFile                        = NULL;

int g_switch_time = 5000;  //切换时间
int g_test_switch_time = 200;  //测试切换时间
int g_interface_switch_time = 10000;  //接口切换时间
int g_nDisplayStatusCmdInterval = 1000; //下发屏幕状态显示需要的时间间隔
int g_nAVTime = 10000;  //AV输入输出接口切换时间
int g_nVGATime = 10000; //VGA接口切换时间
int g_nHDMI1Time = 10000;   //HDMI1切换时间
int g_nHDMI2Time = 10000;   //HDMI2切换时间
int g_nHDMI3Time = 10000;   //HDMI3切换时间
int g_nComponentTime = 10000;   //分量切换时间

int g_nInsertStatus = 0;

int g_nTotalSNNum = 1;
int g_nCurrentSNIndex = 0;
QStringList g_strlistSN;
QString g_strSnFirst8Bit = "";  //保存待测红外框序列号前8位供序列号自动检查使用
QString g_strInfraredFirmVersion = "";  //红外框固件版本号
QString g_strSaveFilePath = ""; //测试结果excel保存路径
QString g_strNetworkTestUrl = "";   //网络测试网址

int g_nCurrentIndex = 0;  //当前轴索引  0代表X  1 代表Y
int g_nIsAppQuit = 0;  //程序是否退出0否1是

THREADINFO g_stThreadInfo[100];


COMPORTINFO g_stComportInfo;

/* 当前app运行目录 */
VOS_INT8    g_acCurAppRunDir[DEV_MAX_PATH_LEN]        = {0,};


/* 保存串口设备名称 */
QStringList g_strlistCOMDevName;
/* 当前串口设备名称 */
QString g_strComDevName;

/***********************************************************
Function     :   Pub_GetCurrentTimeClick
Description  :   获取当前时间戳ms
Input        :
Output       :   无
Return       :   无
Others       :   单位ms
Author       :
History      :   1. 创建函数2012.03.12
***********************************************************/
int Pub_GetCurrentTimeClick()
{
    QTime time = QTime::currentTime();
    int ms = time.hour(); ms *= 60;
    ms += time.minute(); ms *= 60;
    ms += time.second(); ms *= 1000;
    ms += time.msec();

    return ms;
}

/***********************************************************
Function     :   Pub_GetCurDir
Description  :   获取当前目录
Input        :
Output       :   无
Return       :   无
Others       :   对dll是无效的,获取路径为空
Author       :
History      :   1. 创建函数2012.03.12
***********************************************************/
void Pub_GetCurDir(VOS_INT8 *pCurDir)
{
    if (NULL == pCurDir)
    {
        return;
    }

    QString strRootDir;
     //解决中文路径名乱码并且无法比较的问题
    QTextCodec *temp_codec = QTextCodec::codecForName("System");

    QTextCodec::setCodecForLocale(temp_codec);
//    QTextCodec::setCodecForCStrings(temp_codec);
//    QTextCodec::setCodecForTr(temp_codec);

    strRootDir = QCoreApplication::applicationDirPath();
    vos_snprintf(pCurDir, sizeof(strRootDir.toStdString().c_str()),
                 "%s", strRootDir.toStdString().c_str());

    return;
}

/***********************************************************
Function     :   ChangeMonthToNum
Description  :   转换日期格式为数字
Input        :
Output       :   无
Return       :   无
Others       :   "Sep 26 2012"转换为12 09 26
Author       :
History      :   1. 创建函数2012.09.26
***********************************************************/
void ChangeDateToNum(const VOS_INT8 *acToday, INT32 &nYear, int &nMonth, int &nDay)
{
    VOS_INT8    acTemp[10]      = {0,};
    INT32       nLen            = strlen(acToday);


    //------------------------------
    memset(acTemp, 0, sizeof(acTemp));
    memcpy(acTemp, (acToday + nLen  - 4), 4);  // 获取最后4位
    nYear = atoi(acTemp) - 2000;

    //------------------------------
    if     (memcmp(acToday, "Jan", 3) == 0 ) nMonth = 1;
    else if(memcmp(acToday, "Feb", 3) == 0 ) nMonth = 2;
    else if(memcmp(acToday, "Mar", 3) == 0 ) nMonth = 3;
    else if(memcmp(acToday, "Apr", 3) == 0 ) nMonth = 4;
    else if(memcmp(acToday, "May", 3) == 0 ) nMonth = 5;
    else if(memcmp(acToday, "Jun", 3) == 0 ) nMonth = 6;
    else if(memcmp(acToday, "Jul", 3) == 0 ) nMonth = 7;
    else if(memcmp(acToday, "Aug", 3) == 0 ) nMonth = 8;
    else if(memcmp(acToday, "Sep", 3) == 0 ) nMonth = 9;
    else if(memcmp(acToday, "Oct", 3) == 0 ) nMonth = 10;
    else if(memcmp(acToday, "Nov", 3) == 0 ) nMonth = 11;
    else if(memcmp(acToday, "Dec", 3) == 0 ) nMonth = 12;

    //------------------------------
    memset(acTemp, 0, sizeof(acTemp));
    memcpy(acTemp, (acToday + 3), (nLen - 7));
    Pub_Trim(acTemp);
    nDay = atoi(acTemp);
}

/***********************************************************
Function     :   Pub_Trim
Description  :   去掉字符串前后的空格和制表符
Input        :
Output       :   无
Return       :   无
Others       :   将缓冲区中，最前面和最后面的空格和制表符去掉
Author       :
History      :   1. 创建函数2012.09.26
***********************************************************/
void Pub_Trim(char *buffer)
{
    char *ptr1, *ptr2;
    int len;
    unsigned char c;
    
    len = strlen(buffer);
    if (len == 0 ) return;
    
    // 去掉最前面的空格
    for(ptr1 = buffer; *ptr1 != 0; ptr1++)
    {
        c = *ptr1;
        if ( c > ' ') break;
    }
    if( *ptr1 == '\0') // 空行
    {
        buffer[0] = '\0';
        return;
    }
    
    // 去掉最后面的空格
    for(ptr2 = buffer + len - 1; ptr2 != ptr1; ptr2--)
    {
        c = *ptr2;
        if ( c > ' ' ) break;
    }
    
    len = ptr2 - ptr1 + 1;
    memcpy(buffer, ptr1, len);
    *(buffer + len) = '\0';
}

/***********************************************************
Function     :   Pub_GetDllVersion
Description  :   获取dll的版本号
Input        :
Output       :   无
Return       :   无
Others       :   无
Author       :
History      :   1. 创建函数2012.09.26
***********************************************************/
void Pub_GetDllVersion(VOS_INT8 *acVersion)
{
    INT32       nYear           = 0;
    INT32       nMonth          = 0;
    INT32       nDay            = 0;
    VOS_INT8    acToday[30]     = {0,};
    
    //   Standard Predefined Macros
    // __DATE__   :编译时的日期
    // __FIlE__	  :编译时的文件名字 
    // __LINE__   :编译时所在的行 
    // __TIME__   :编译时的时间 

    memset(acToday, 0, sizeof(acToday));
    sprintf(acToday, "%s", __DATE__);   // 输出格式如下："Jun  8 2012"
    //转换英文月份为数字月份 "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
    ChangeDateToNum(acToday, nYear, nMonth, nDay);

    sprintf(acVersion, "0.1.%02d.%02d.%02d", nYear, nMonth, nDay);
}


/***********************************************************
Function     :   Pub_ConvertHexToStr
Description  :   转换16进制为字符串方便输出
Input        :   
Output       :   无
Return       :   NULL --- 失败 , 其它值--- 转换后的字符串
Others       :   无
Author       :
History      :   1. 创建函数2012.03.20
***********************************************************/
VOS_INT8 *Pub_ConvertHexToStr(const UINT8 *pBuf, const UINT32 nLen)
{
    static VOS_INT8     acBuf[20000]      = {0,};
    VOS_INT8            acTmpBuf[10]    = {0,};
    UINT32              ulIndex         = 0;
    UINT32              ulBufLen         = 0;
    
    if ((NULL == pBuf) || (0 >= nLen))
    {
        return NULL;
    }

    ulBufLen = sizeof(acBuf)/sizeof(acTmpBuf);
    if (ulBufLen >= nLen)
    {
        ulBufLen = nLen;
    }
    
    memset(acBuf, 0, sizeof(acBuf));
    memset(acTmpBuf, 0, sizeof(acTmpBuf));
    
    for(ulIndex=0; ulIndex<ulBufLen; ulIndex++)
    {
        vos_snprintf(acTmpBuf, sizeof(acTmpBuf), "%02X ", *(pBuf + ulIndex));
        strcat(acBuf, acTmpBuf);
    }
    
    return acBuf;
}


/***********************************************************
Function     :   LogToDebugFile
Description  :   用于各模块输出日志
Input        :    DEBUG_LEVEL_T endebugLevel ----日志级别
                    const char *fmt   ----需要输出到日志的字符串
Output       :   无
Return       :   无
Others       :   调用处字符串不需要换行，此函数处理
Author       :
History      :   1. 创建函数2012.03.12
***********************************************************/
void LogToDebugFile(const char *pcFunc, LOG_LEVEL_T endebugLevel, const char *fmt, ...)
{
    struct tm *newtime;
    time_t ltime;
    va_list v_list;
    char acLogPath[400]  = {0,};
    //FILE *fp = NULL;
    QDir dir;

    if (NULL == pcFunc)
    {
        return;
    }

    if (0 == g_nLogLevel)
    {// 不输出调试信息
        return;
    }
    
    if (endebugLevel > g_nLogLevel)
    {
        return;   // 当前的调试输出级别不够
    }
    
    memset(acLogPath, 0, sizeof(acLogPath));
    if (0 == strlen(g_acCurAppRunDir))
    {
        QString strAppPath;
        Pub_GetCurAppDir(strAppPath);
        vos_snprintf(g_acCurAppRunDir, sizeof(g_acCurAppRunDir), "%s/log",
                    strAppPath.toStdString().c_str());
    }
    vos_snprintf(acLogPath, sizeof(acLogPath), "%s/hht_log.txt", g_acCurAppRunDir);

    if (NULL == g_pLogFile)
    {
        g_pLogFile = fopen(acLogPath, "w");
    }
    if(NULL == g_pLogFile)
    {
        return;
    }

    /* Get the time in seconds */
    time (&ltime);

    /* convert it to the structure tm */
    newtime = localtime(&ltime);

    /* flush message */
    va_start(v_list, fmt);
    fprintf(g_pLogFile, "[%02d-%02d-%02d %02d:%02d:%02d %7d][%s()]",
        (newtime->tm_year + 1900), (newtime->tm_mon + 1), newtime->tm_mday,
        newtime->tm_hour, newtime->tm_min, newtime->tm_sec,
        Pub_GetCurrentTimeClick(), pcFunc);
    vfprintf(g_pLogFile, fmt, v_list);
    /* 此处强制换行输出，保证每条日志包含时间等信息 */
    fprintf(g_pLogFile, "\n");
    fflush(g_pLogFile);
    va_end(v_list);

    return;
}

/***********************************************************
Function     :   Pub_MSleep
Description  :   公共休眠函数
Input        :   
                 
Output       :   无
Return       :   无
Others       :   单位毫秒ms
Author       :
History      :   1. 创建函数2012.03.12
***********************************************************/
void Pub_MSleep(int micSeconds)
{
#ifdef VOS_WINDOWS
    Sleep(micSeconds);  // Windows下，Sleep 的单位是毫秒
#elif VOS_LINUX    
    usleep(micSeconds * 1000);  // <unistd.h>，usleep的单位是微秒
#elif VOS_MAC
    usleep(micSeconds * 1000);
#endif

    return;
}



/***********************************************************
Function     :   Pub_GetCurAppDir
Description  :   获取app路径
Input        :   无
Output       :   无
Return       :   返回配置项的值
Others       :  
Author       :
History      :   1. 创建函数2012.09.03
***********************************************************/
void Pub_GetCurAppDir(QString &strAppPath)
{
    //解决中文路径名乱码并且无法比较的问题
    QTextCodec *temp_codec = QTextCodec::codecForName("System");

    QTextCodec::setCodecForLocale(temp_codec);
//    QTextCodec::setCodecForCStrings(temp_codec);
//    QTextCodec::setCodecForTr(temp_codec);
    
    strAppPath = QDir::currentPath();
    
    return;
}

/***********************************************************
Function     :   Pub_GetValueBySectionName
Description  :   获取配置文件制定项信息
Input        :   pszCfgName 配置项名称, COMMON/debuglevel
                     pszCfgPath 配置文件文件全路径包含文件名
Output       :   无
Return       :   返回配置项的值
Others       :  
Author       :
History      :   1. 创建函数2012.09.03
***********************************************************/
INT32 Pub_GetValueBySectionName(const VOS_INT8 *pszCfgName, QString &strOutputValue, const VOS_INT8 *pszCfgPath)
{
    INT32           nRet            = 0;
    QString         strKey          = "";
    QString         strValue        = "";
    INT32           nSize           = -1;
    INT32           nPos            = -1;

    if ((NULL == pszCfgName) || (NULL == pszCfgPath))
    {
        HHT_LOG(EN_INFO, "invalid input parameters");
        return RET_INVALID;
    }
    
    if((QFile::exists(pszCfgPath)) == false)
    {
        HHT_LOG(EN_INFO, "file[%s] is not exist", pszCfgPath);
        return RET_INVALID;
    } 
    
    QSettings settings(pszCfgPath, QSettings::IniFormat);
    settings.setIniCodec("UTF-8");
    
    strValue = pszCfgName;
    
    strValue = settings.value(strValue, -1).toString();
    nRet = strValue.toInt();
    if (-1 == nRet)
    {
        return nRet;
    }
    nSize = strValue.size();
    nPos = strValue.indexOf("/");
    if (-1 != nPos)
    {
        strValue.chop(nSize - nPos);
    }
    strOutputValue = strValue;
    
    return RET_SUCCESS;
}

/***********************************************************
Function     :   PUB_ConvertHexToStr
Description :   转换16进制为字符串方便输出
Input         :   
Output       :   无
Return       :   NULL --- 失败 , 其它值--- 转换后的字符串
Others       :   无
Author       :
History      :   1. 创建函数2011.05.09
***********************************************************/
VOS_INT8 *PUB_ConvertHexToStr(const UINT8 *pBuf, const UINT32 nLen)
{
    static VOS_INT8     acBuf[650]      = {0,};
    VOS_INT8            acTmpBuf[10]    = {0,};
    UINT32              ulIndex         = 0;
    UINT32              ulBufLen        = 0;
    
    if ((NULL == pBuf) || (0 >= nLen))
    {
        return NULL;
    }

    ulBufLen = sizeof(acBuf)/sizeof(acTmpBuf);
    if (ulBufLen >= nLen)
    {
        ulBufLen = nLen;
    }
    
    memset(acBuf, 0, sizeof(acBuf));
    memset(acTmpBuf, 0, sizeof(acTmpBuf));
    
    for(ulIndex=0; ulIndex<ulBufLen; ulIndex++)
    {
        vos_snprintf(acTmpBuf, sizeof(acTmpBuf), "%02X ", *(pBuf + ulIndex));
        strcat(acBuf, acTmpBuf);
    }

    return acBuf;
}


INT32 check_app_run(void)
{
    HANDLE hMutex;
    //创建句柄
    hMutex = ::CreateMutex(NULL, FALSE, (LPCWSTR)"PanelInspector");
    if (NULL == hMutex)
    {
        return 1;
    }
    if (ERROR_ALREADY_EXISTS == ::GetLastError())
    {
        QMessageBox::warning(NULL, "警告", "程序已运行");
        CloseHandle(hMutex);
        return 1;
    }
    return 0;
}

/***********************************************************
Function     :   Pub_GetTimeInterval
Description  :   获取时间间隔
Input        :   之前时间
Output       :   无
Return       :   时间间隔
Others       :   无
Author       :
History      :   1. 创建函数2011.11.29
***********************************************************/
int Pub_GetTimeInterval(const int nPreTime)
{
    int   nInterval   = 0;
    int   nCurTime    = 0;

    if (0 > nPreTime)
    {
        return 0;
    }

    nCurTime = Pub_GetCurrentTimeClick();
    nInterval = abs(nCurTime - nPreTime);

    return nInterval;
}

/***********************************************************
Function     :   VOS_CreateThread
Description :   创建线程函数
Input         :   unsigned int *pThreadId--- 线程id
                    unsigned int *pFuncAddr--- 线程处理函数
                    void *pstConnInfo--- 处理函数的参数
Output       :   无
Return       :   无
Others       :   _beginthreadex会自动调用_endthreadex
最好在线程函数中不要调用_endthreadex()函数,
最安全的方式是让线程函数自然的return,
如果在线程函数中调用了_endthreadex函数的话,
线程就会在调用_endthreadex函数调用的地方立即退出,
不会继续向下执行,导致在线程局部存储空间
中的局部对象或者变量的析构函数不会被调用,
有可能导致资源的泄漏!
Author       :
History      :   1. 创建函数2011.08.08
***********************************************************/
INT32 VOS_CreateThread(vos_pthread_t *pThreadId, THREAD_FUNC pFuncAddr, void *pstTubeInfo)
{
    int err = 0;

#ifdef VOS_WINDOWS
    HANDLE  hThread;
    //if ((NULL == pThreadId) || (NULL == pFuncAddr) || (NULL == pstConnInfo))
    if ((NULL == pThreadId) || (NULL == pFuncAddr))
    {
        return -1;
    }

    hThread = (HANDLE)_beginthreadex(NULL,  //security
                                    0,      //stack size
                                    (unsigned int (__stdcall *)(void *))pFuncAddr,  //entry point
                                    (void *)pstTubeInfo,    //arg list
                                    0,      //0--running, CREATE_SUSPENDED--need resume
                                    pThreadId);
    if (NULL == hThread)
    {
        return -1;
    }

#else     //  Linux, Mac
    err = pthread_create(pThreadId, NULL, pFuncAddr, pstTubeInfo);
#endif

    if(*pThreadId == 0)
    {
        return -1;
    }

    for(int i=0; i<100; i++)
    {
        if( g_stThreadInfo[i].threadID == 0)
        {
            g_stThreadInfo[i].threadID = *pThreadId;
            g_stThreadInfo[i].stopFlag = 0;
            break;
        }
    }

    return err;
}

/***********************************************************
Function     :   MakeByte2UCharList
Description  :   将QByteArray 数据转换为16进制QList<unsigned char>
Input        :   
Output       :   无
Return       :   无
Others       :   无
Author       :
History      :   1. 创建函数2014.09.04
***********************************************************/

void MakeByte2UCharList(QByteArray data, QList<unsigned char> &dataUCharList)
{
    for(int i=0;i<data.size();i++)
    {
        unsigned char a = (data[i]&0xFF) ;
        dataUCharList.append(a);
    }
}

/***********************************************************
Function     :   Pub_GetCurVersion
Description  :   获取当前时间戳版本号
Input        :   version:版本信息的数组 len:数组长度，要大于10
Output       :   无
Return       :   无
Others       :   单位ms
Author       :
History      :   1. 创建函数2012.03.12
***********************************************************/
void Pub_GetCurVersion(char *version, const int len)
{
    int y, m, d;

    if (10 >= len)
    {//check length
        return;
    }
    
    //   Standard Predefined Macros
    // __DATE__   :编译时的日期
    // __FIlE__      :编译时的文件名字 
    // __LINE__   :编译时所在的行 
    // __TIME__   :编译时的时间 
    char today[30];
    sprintf(today, "%s", __DATE__);   // 输出格式如下："Jun  8 2012"
    // "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
    GetYMD(today, y, m, d);
    
#ifdef HHTMAINTAIN
    sprintf(version, "3.%02d.%02d.%02d-M", y, m, d);
#else
    sprintf(version, "3.%02d.%02d.%02d", y, m, d);
#endif

    return;
}

/***********************************************************
Function     :   GetYMD
Description  :   从时间戳中解析年月日
Input        :   src格式-"Jun  8 2012"  "May 23 2011"
Output       :   无
Return       :   无
Others       :   无
Author       :
History      :   1. 创建函数2012.03.12
***********************************************************/
static void GetYMD(const char *src, int &y, int &m, int &d)
{
    int len = strlen(src);
    char temp[10];
    
    //------------------------------
    memset(temp, 0, sizeof(temp));
    memcpy(temp, src + len  - 4, 4);  // 获取最后4位
    y = atoi(temp) - 2000;
    
    //------------------------------
    if     (memcmp(src, "Jan", 3) == 0 ) m = 1;
    else if(memcmp(src, "Feb", 3) == 0 ) m = 2;
    else if(memcmp(src, "Mar", 3) == 0 ) m = 3;
    else if(memcmp(src, "Apr", 3) == 0 ) m = 4;
    else if(memcmp(src, "May", 3) == 0 ) m = 5;
    else if(memcmp(src, "Jun", 3) == 0 ) m = 6;
    else if(memcmp(src, "Jul", 3) == 0 ) m = 7;
    else if(memcmp(src, "Aug", 3) == 0 ) m = 8;
    else if(memcmp(src, "Sep", 3) == 0 ) m = 9;
    else if(memcmp(src, "Oct", 3) == 0 ) m = 10;
    else if(memcmp(src, "Nov", 3) == 0 ) m = 11;
    else if(memcmp(src, "Dec", 3) == 0 ) m = 12;

    //------------------------------
    memset(temp, 0, sizeof(temp));
    memcpy(temp, src + 3, len - 7);
    Pub_Trim(temp);
    d = atoi(temp);
}




#ifdef __cplusplus
}
#endif
