#ifndef OPSOPERATE_EXTERN_H
#define OPSOPERATE_EXTERN_H
extern void OPS_SendFilesToAndroid(int nFileNum, QList<QList<unsigned char>>listAllFilesName, QList<QList<unsigned char>> listAllFilesData);
extern int    OPS_DeleteSingleAppFromAndroid(QString appName);
extern int   Ops_WriteStatus();   //-1 :正在写入,0:成功,其他失败
extern void OPS_QuitThread();

#endif // OPSOPERATE_EXTERN_H

