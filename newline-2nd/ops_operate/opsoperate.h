#ifndef OPSOPERATE_H
#define OPSOPERATE_H
#include "Pub/global.h"
#include "comdev.h"

#define COM_HEADER (0x7F)
#define COM_ENDER (0xCF)
#define COM_TRANS_FILELIST_START (0xAB)  //传送文件列表开始
#define COM_TRANS_FILENAME_START (0xAC)    //传送文件名并开始传输文件
#define COM_TRANS_FILE (0xAD)    //传送文件内容
#define COM_TRANS_FILENAME_STOP (0xAE)    //传送文件名且停止传输文件
#define COM_TRANS_FILELIST_STOP (0xAF)  //传送文件列表结束

#define COM_VALID_DATA_LEN (50) //每包有效数据长度
//#define MAX_FILE_NUM (100)  //最大传输文件数定义为100

THREADRETURN OpsCom_ThreadFn(void *pParam);

void OPS_SendFilesToAndroid(int nFileNum, QList<QList<unsigned char>>listAllFilesName, QList<QList<unsigned char>> listAllFilesData);
int OPS_DeleteSingleAppFromAndroid(QString appName);
void ReadConfig();
void OPS_QuitThread();
int   Ops_WriteFiles(COMDEV *pstComDev);
int   Ops_WriteStatus();
#endif // OPSOPERATE_H

