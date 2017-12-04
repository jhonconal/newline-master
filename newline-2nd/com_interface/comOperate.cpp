#include <QtGui>
#include <QTime>
#include <windows.h>
#include "time.h"
#include "comOperate.h"
#include <QDebug>
#include <QMessageBox>
//#include <QSqlDatabase>
//#include <QSqlError>
//#include <QSqlQuery>
#include <QDir>
#include <QFile>

COMDEV g_stComDev;


/***********************************************************
Function     :   COM_GetComPortNameFromIndex
Description  :   通过串口号获取串口完整名称
Input        :   nPortIndex 串口编号, pcComportName 串口名称
Output       :   无
Return       :   0 success,others failure
Others       :   无
Author       :
History      :   1. 创建函数2015.01.14
***********************************************************/

INT32 COM_GetComPortNameFromIndex(const INT32 nPortIndex, VOS_INT8 * pcComportName)
{
    VOS_INT8 acDevName[DEV_MAX_PATH_LEN] = {0,};

    if ((0 > nPortIndex) || (NULL == pcComportName))
    {
        return RET_INVALID;
    }

    memset(acDevName, 0, sizeof(acDevName));
    if (0 < nPortIndex)
    {
        vos_snprintf(acDevName, sizeof(acDevName), "\\\\.\\%s%d", COM_DEV_BASE_NAME, nPortIndex);
    }
    else
    {
        vos_snprintf(acDevName, sizeof(acDevName), "");
    }

    memcpy(pcComportName, acDevName, sizeof(acDevName));
    
    return RET_SUCCESS;
}


/***********************************************************
Function     :   Com_Write
Description  :   通过串口设置功能
Input        :   pstComDev 串口设备
Output       :   无
Return       :   0 success,others failure
Others       :   无
Author       :
History      :   1. 创建函数2017.02.10
***********************************************************/

INT32 Com_Write(COMDEV * pstComDev, UINT8* pcSndBuf, INT32 nLen)
{
    INT32 i = 0;
    INT32 nRet = 0;
    INT32 nDataLen = 0;
    UINT8 acRcvBuf[COM_DATA_PKG_LEN] = {0,};
    UINT8 ucCheckSum = 0;
    
    if ((NULL == pstComDev) || (NULL == pcSndBuf))
    {
        return RET_INVALID;
    }

    //write
    memset(acRcvBuf, 0, sizeof(acRcvBuf));
    nRet = COM_SndCmdPkg(pstComDev, pcSndBuf, acRcvBuf, nLen, COM_CMD_DELAY_TIME);
    if (RET_SUCCESS != nRet)
    {
        HHT_LOG(EN_ERR, "write (%s) failed", Pub_ConvertHexToStr(pcSndBuf, nLen));
        return RET_FAILURE;
    }
    HHT_LOG(EN_INFO, " write (%s) ok", Pub_ConvertHexToStr(pcSndBuf, nLen));

//    Pub_MSleep(COM_CMD_DELAY_TIME);
//    HHT_LOG(EN_INFO, "  read again to check");
    ucCheckSum = 0;
    HHT_LOG(EN_INFO, "  read again to check contant(%s)",Pub_ConvertHexToStr(acRcvBuf, nLen+1));//---------->回读成功
    nDataLen = acRcvBuf[1];
    if ((acRcvBuf[0]==pcSndBuf[0])&&(0==memcmp(&pcSndBuf[2],&acRcvBuf[2],7)) && (0xA1 == acRcvBuf[9]) && (0x00 == acRcvBuf[10])
        && (0 == memcmp(&pcSndBuf[10], &acRcvBuf[11], 2)) && (0xCF == acRcvBuf[nDataLen+2]))
    {
        ucCheckSum = 0;
        for (i=1; i<nDataLen+1; i++)
        {
            ucCheckSum += acRcvBuf[i];
        }
         HHT_LOG(EN_INFO, " CHeckSum(%d)=?acRcvBuf(%d)",ucCheckSum,acRcvBuf[nDataLen+1]);//---------->
        if (ucCheckSum == acRcvBuf[nDataLen+1])  //success回读成功
        {
           return RET_SUCCESS;
        }
    }
    return RET_FAILURE;
}


/***********************************************************
Function     :   COM_SndCmdPkg
Description  :   串口发送命令接口
Input        :   pstComDev 串口设备, pcBuf 要发送的命令, pRcvBuf 命令返回, nDelayMs 延迟时间
Output       :   无
Return       :   0 success,others failure
Others       :   无
Author       :
History      :   1. 创建函数2015.01.14
***********************************************************/

INT32 COM_SndCmdPkg(COMDEV * pstComDev, const UINT8 * pcBuf, UINT8 * pRcvBuf, INT32 nLen, const INT32 nDelayMs)
{
    INT32 nRet = -1;
    INT32 nPreTime = 0;
    INT32 nTimeOut = 0;
    bool bIsReading = false;
    
    if ((NULL == pstComDev) || (NULL == pcBuf) || (NULL == pRcvBuf))
    {
        return RET_INVALID;
    }

    nTimeOut = 2000;   //超时时间设置为2s
    nRet = pstComDev->WriteCom((unsigned char*)pcBuf, nLen);
    if (0 >= nRet)
    {
        return RET_FAILURE;
    }
    HHT_LOG(EN_INFO, "write (%s) ok", Pub_ConvertHexToStr(pcBuf, nRet));//写成功
    
//    Sleep(nDelayMs);
    nPreTime = Pub_GetCurrentTimeClick();
    while ((nTimeOut >= Pub_GetTimeInterval(nPreTime)) && (! bIsReading))
    {
        Pub_MSleep(2);
        bIsReading = true;
        nRet = pstComDev->ReadComByHeader(pRcvBuf,COM_RSP_CMD_LEN);
        if (0 >= nRet)
        {
//            HHT_LOG(EN_ERR, "read com failed");
            bIsReading = false;
            continue;
        }
        HHT_LOG(EN_INFO, "read (%s)", Pub_ConvertHexToStr(pRcvBuf, nRet));
        return RET_SUCCESS;
    }

    return RET_FAILURE;
}
