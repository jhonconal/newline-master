#ifndef OPSOPERATE_H
#define OPSOPERATE_H
#include "Pub/global.h"
#include "comdev.h"

#define COM_HEADER (0x7F)
#define COM_ENDER (0xCF)
#define COM_TRANS_FILELIST_START (0xAB)  //�����ļ��б�ʼ
#define COM_TRANS_FILENAME_START (0xAC)    //�����ļ�������ʼ�����ļ�
#define COM_TRANS_FILE (0xAD)    //�����ļ�����
#define COM_TRANS_FILENAME_STOP (0xAE)    //�����ļ�����ֹͣ�����ļ�
#define COM_TRANS_FILELIST_STOP (0xAF)  //�����ļ��б����

#define COM_VALID_DATA_LEN (50) //ÿ����Ч���ݳ���
//#define MAX_FILE_NUM (100)  //������ļ�������Ϊ100

THREADRETURN OpsCom_ThreadFn(void *pParam);

void OPS_SendFilesToAndroid(int nFileNum, QList<QList<unsigned char>>listAllFilesName, QList<QList<unsigned char>> listAllFilesData);
int OPS_DeleteSingleAppFromAndroid(QString appName);
void ReadConfig();
void OPS_QuitThread();
int   Ops_WriteFiles(COMDEV *pstComDev);
int   Ops_WriteStatus();
#endif // OPSOPERATE_H

