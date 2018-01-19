/************************************************************
Copyright (C), 2012-2014, HongHe Tech.
FileName       : global.h
Date           : 2012.03.12
Description    : 获取展台属性
Version        : 无
Function List  :
        1.
History:   1. 创建文件2013.03.12
***********************************************************/
#ifndef COM_GLOBAL_H
#define COM_GLOBAL_H

#include "os_header.h"

#ifdef __cplusplus
extern "C"
{
#endif

//----------------------------------------------------
/* macro defines here begin */
/* snprintf in linux, sprintf_s in windows */
#ifdef VOS_WINDOWS
#define vos_snprintf    sprintf_s
#define vos_closesocket closesocket
#else
#define vos_snprintf    snprintf
#define vos_closesocket       close
#endif


/* define return value */
#define RET_INVALID     -11
#define RET_SUCCESS     0
#define RET_FAILURE     1
#define RET_RSPCMDERR   -1
#define RET_RSPDATERR   -2
#define RET_FILENOTEXISTS -4
#define READ_CFG_ERR_RET -100 //读取配置项错误所返回的值


/* 文件路径最大长度 */
#define DEV_MAX_PATH_LEN        256
/* 命令最大长度 */
#define CMD_PKT_MAX_LEN         256
/*
#ifdef VOS_MAC
#define COM_DSP_DEV_MAX_NUM             10
#define COM_DSP_DEV_BASE_NAME           ("/dev/tty.usbmodem")
//#define COM_DSP_DEV_BASE_NAME1          ("/dev/tty.SLAB_USBtoUART")
//#define COM_DSP_DEV_BASE_NAME2          ("/dev/tty.usbserial")
#elif VOS_LINUX
#define COM_DSP_DEV_MAX_NUM             3
#define COM_DSP_DEV_BASE_NAME           ("/dev/ttyACM")
#elif VOS_WINDOWS
#define COM_DSP_DEV_MAX_NUM             50
#define COM_DEV_BASE_NAME               ("COM")
#define DSP_DEV_NAME_1                  ("XR21B1411")
#define DSP_VID_PID_1                   ("#vid_04e2&pid_1411")
#define DSP_DEV_NAME_2                  ("USB Serial Port")
#define DSP_VID_PID_2                   ("#vid_0403&pid_6001")
#endif
*/
#ifdef VOS_WINDOWS
#define COM_DSP_DEV_MAX_NUM             50
#define COM_DEV_BASE_NAME               ("COM")
#define DSP_DEV_NAME_1                  ("XR21B1411")
#define DSP_VID_PID_1                   ("#vid_04e2&pid_1411")
#define DSP_DEV_NAME_2                  ("USB Serial Port")
#define DSP_VID_PID_2                   ("#vid_0403&pid_6001")
#endif

#define COM_DATA_PKG_LEN (64)


/* macro defines here end */
//----------------------------------------------------

//----------------------------------------------------
/* struct defines here begin */
typedef int             INT32;
typedef short           INT16;
typedef char            VOS_INT8;
typedef unsigned int    UINT32;
typedef unsigned short  UINT16;
typedef unsigned char   UINT8;
typedef long long       INT64;

#ifdef VOS_WINDOWS
typedef unsigned int    vos_pthread_t;
typedef unsigned int    THREADRETURN;
#else
typedef pthread_t       vos_pthread_t;
typedef void *          THREADRETURN;
#endif

#ifdef VOS_WINDOWS
typedef unsigned int THREADRETURN;
#else
typedef void * THREADRETURN;
#endif

typedef struct tagThreadInfo
{
    vos_pthread_t  threadID;   // 线程ID
    int            stopFlag;   // 0/1，是否需要退出线程
} THREADINFO;
extern THREADINFO g_stThreadInfo[100];

typedef THREADRETURN (*THREAD_FUNC)(void *);



/* 日志级别，默认级别 */
typedef enum dbglevel_t
{
    EN_EMERG     = 0,
    EN_ALERT     = 1,
    EN_CRIT      = 2,
    EN_ERR       = 3,
    EN_WARNING   = 4,
    EN_NOTICE    = 5,
    EN_INFO      = 6,
    EN_DEBUG     = 7,
    EN_BUT       = 8
}LOG_LEVEL_T;




/* struct defines here end */
//----------------------------------------------------

//----------------------------------------------------
/* extern defines here begin */
extern INT32 g_nLogLevel;
extern FILE *g_pLogFile;
extern INT32 g_nCurrentIndex;

extern int g_nInsertStatus;

typedef struct tagBurnInfo
{
    QString strTouchVersion;
    QString strTouchSN;
    QString strPanelSN;
    QString strModelNumber;
}BURNINFO;

typedef struct tagCOMPort
{
    INT32 nComNo;   //串口号
    INT32 nBaudRate;    //波特率
}COMPORTINFO;
extern COMPORTINFO g_stComportInfo;
extern VOS_INT8    g_acCurAppRunDir[DEV_MAX_PATH_LEN];


extern int g_nTotalSNNum;
extern int g_nCurrentSNIndex;
extern QStringList g_strlistSN;

extern QStringList g_strlistCOMDevName;
extern QString g_strComDevName;
extern QString g_strSnFirst8Bit;  //保存待测红外框序列号前8位供序列号自动检查使用
extern QString g_strInfraredFirmVersion;  //红外框固件版本号
extern QString g_strSaveFilePath; //测试结果excel保存路径
extern QString g_strNetworkTestUrl;   //网络测试网址

extern int g_switch_time;  //切换时间
extern int g_test_switch_time;  //测试切换时间
extern int g_interface_switch_time;  //接口切换时间
extern int g_nAVTime;  //AV输入输出接口切换时间
extern int g_nVGATime; //VGA接口切换时间
extern int g_nHDMI1Time;   //HDMI1切换时间
extern int g_nHDMI2Time;   //HDMI2切换时间
extern int g_nHDMI3Time;   //HDMI3切换时间
extern int g_nComponentTime;   //分量切换时间

extern int g_nDisplayStatusCmdInterval; //下发屏幕状态显示需要的时间间隔
extern int g_nIsAppQuit;  //程序是否退出0否1是

/* extern defines here end */
//----------------------------------------------------


//----------------------------------------------------
/* public function defines here begin */
void Pub_GetCurDir(VOS_INT8 *pCurDir);
void Pub_Trim(char *buffer);
void Pub_GetDllVersion(VOS_INT8 *acVersion);
void Pub_MSleep(int micSeconds);
int Pub_GetCurrentTimeClick();
VOS_INT8 *Pub_ConvertHexToStr(const UINT8 *pBuf, const UINT32 nLen);
void LogToDebugFile(const VOS_INT8 *pcFunc,
                    LOG_LEVEL_T endebugLevel, const VOS_INT8 *fmt, ...);
/* 日志函数 */
#define HHT_LOG(fmt, ...)             LogToDebugFile(__FUNCTION__, fmt, __VA_ARGS__)

void Pub_GetCurAppDir(QString &strPath);
INT32 Pub_GetValueBySectionName(const VOS_INT8 *pszCfgName, QString &strOutputValue, const VOS_INT8 *pszCfgPath);
INT32 Pub_CheckComData(UINT8 *acRcvBuf);
VOS_INT8 *PUB_ConvertHexToStr(const UINT8 *pBuf, const UINT32 nLen);
int Pub_GetTimeInterval(const int nPreTime);

int VOS_CreateThread(vos_pthread_t *pThreadId, THREAD_FUNC pFuncAddr, void *pstTubeInfo);
void MakeByte2UCharList(QByteArray data, QList<unsigned char> &dataUCharList);
void Pub_GetCurVersion(char *version, const int len);
static void GetYMD(const char *src, int &y, int &m, int &d);

//检查程序是否运行
int check_app_run(void);
extern void ReadConfig();


/* public function defines here end */
//----------------------------------------------------

#ifdef __cplusplus
}
#endif

#ifdef VOS_WINDOWS
    #if _MSC_VER >= 1600
    #pragma execution_character_set("utf-8")    //告诉windows编译器源码的编码格式解决代码中直接赋值的中文运行时显示乱码问题,放在这个头文件里是为了确保所有界面类都可以包含到
    #endif
#endif //VOS_WINDOWS

#endif // GLOBAL_H
