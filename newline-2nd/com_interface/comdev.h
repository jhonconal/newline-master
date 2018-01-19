#ifndef COMDEV_H
#define COMDEV_H


#include "pub/global.h"

#define COM_BUF_LEN                 (3000)
#define COM_BAUDRATE                (115200)

//class COMDEVSHARED_EXPORT COMDEV {
class COMDEV {
protected:
#ifdef VOS_WINDOWS
    HANDLE      m_hHandle;     //windows打开文件句柄
    HANDLE      m_hReadThread;
    INT32       m_nRealBufLen;
    UINT32      m_ulReadThdID;
#endif
    INT32       m_nBaudRate;
    INT32       m_nDataBit;
    VOS_INT8    m_cStopBit;
    VOS_INT8    m_cParity;
    UINT8       m_acComBuf[COM_BUF_LEN];
    INT32       m_nStartPos;   // 队头指针
    INT32       m_nEndPos;     // 队尾指针
    VOS_INT8    m_acPortName[DEV_MAX_PATH_LEN];
public:
    COMDEV();
    //INT32 OpenCom(char *pcPortName, INT32 nBaudRate, INT32 nDataBit, char cStopBit, char cParity);
    INT32 OpenCom(char *pcPortName, INT32 nBaudRate);
    static ULONG __stdcall ReadThread(LPVOID pParam);

    INT32 ReadCom(UINT8 *pBuf, const INT32 nLen);// <= 0 err,> 0 OK
    INT32 ReadComByHeader(UINT8 *pBuf, const INT32 nLen);// <= 0 err,> 0 OK
    INT32 ReadX5X7ComByHeader(UINT8 *pBuf, const INT32 nLen);// <= 0 err,> 0 OK
    INT32 WriteCom(UINT8 *pBuf, const INT32 nLen);// <= 0 err,> 0 OK
    INT32 IsOpenCom();  // 1:open, 0:closed
    INT32 CloseCom(void);//0 err ,1 ok
};

#endif // COMDEV_H
