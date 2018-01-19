#include "opsoperate.h"
#include "comOperate.h"
#include "ss_pub.h"
#include <QMutex>
#define  SUPORT_X5X7_DEVICE  1  //支持 X5/X7

int g_nWriteStatus = -1;
int g_nWriteFileFlag = 0;
int g_nWriteX5X7FileFlag = 0;
int g_nDeleteSingleAppFlag=0;//接收已经删除一个APP指令
int g_nWriteDeleteSingleAppCommandsFlag=0;//写请求删除一个APP指令
int g_nClearAllAppCommandsFlag=0;//写请求清除APP列表指令
int g_nJustClearAppInAndroidFlags =0;//写请求清除Android本地APP
int g_nCameraCommandsFlag=0;//切换摄像头指令标识
int g_nPubUSBCommandsFlag=-1;//切换PubUSB指令标识
int g_nX9FirmwareCheckFlag=1;//X9固件确认Flags
int g_nX5X7HandShakeFlag =1;//X5 X7 通讯握手
extern bool   g_nX9FirmwareCheckStatus;//是否是X9固件状态
int g_nFileNum = 0;
int g_nTotalPkgNum = 0;
int g_nCurrentPkgNum = 0;
extern QString   g_tryDeleteAppName;//尝试从列表删除的APP-->mainDialog
QList<QList<unsigned char>> g_listFilesName;
QList<QList<unsigned char>> g_listAllFilesData;

/***********************************************************
Function     :   OpsCom_ThreadFn
Description  :   OPS串口控制主线程
Input        :   pParm 线程参数
Output       :   无
Return       :   无
Others       :   无
Author       :
History      :   1. 创建函数2017.05.23
***********************************************************/

THREADRETURN OpsCom_ThreadFn(void *pParam)
{
    INT32 nRet = -1;
    INT32 i = 0;
    INT32 nDataLen = 0;    
    INT32 nRetrytimes = 0;
    INT32 nAppNameLen = 0;  //要打开的应用名称长度    
    const INT32 nWriteMaxCount = 3;
    VOS_INT8 acDevName[DEV_MAX_PATH_LEN] = {0,};    
    UINT8 ucChecksum = 0;
    UINT8 acCmdSign[6] = {0x99, 0xA2, 0xB3, 0xC4, 0x02, 0xFF};    //命令标识
    //上摄像头指令: 7F YY 99 A2 B3 C4 02 FF 80 A0 CHK CF   7F YY 99 A2 B3 C4 02 FF 80 A1 ZZ CHK CF
    //上摄像头指令: 7F YY 99 A2 B3 C4 02 FF 81 A0 CHK CF   7F YY 99 A2 B3 C4 02 FF 81 A1 ZZ CHK CF
    UINT8 acCameraUpSign[2]={0x80,0xA0}; //上摄像头请求
    UINT8 acCameraDownSign[2]={0x81,0xA0};//下摄像头请求
    UINT8 acRecvCameraSign[2]={0,};//摄像头接收

    //7F YY 99 A2 B3 C4 02 FF 92 A0 CHK CF   7F YY 99 A2 B3 C4 02 FF 92 A1  ZZ CHK CF //PubUSB接收
     UINT8 acPubUSBSign[2]={0x92,0xA0}; //切换PubUSB请求

    //ZOOM IN指令: 7F YY 99 A2 B3 C4 02 FF 60 A0 CHK CF   7F YY 99 A2 B3 C4 02 FF 60 A1 ZZ CHK CF
    //ZOOM OUT指令: 7F YY 99 A2 B3 C4 02 FF 61 A0 CHK CF   7F YY 99 A2 B3 C4 02 FF 61 A1 ZZ CHK CF
    UINT8 acZoomInSign[2]={0x60,0xA0};//Zoom in请求
    UINT8 acZoomOutSign[2]={0x61,0xA0};//Zoom out请求
     //PgUp指令: 7F YY 99 A2 B3 C4 02 FF 70 A0 CHK CF   7F YY 99 A2 B3 C4 02 FF 70 A1 ZZ CHK CF
     //PgDn指令: 7F YY 99 A2 B3 C4 02 FF 71 A0 CHK CF   7F YY 99 A2 B3 C4 02 FF 71 A1 ZZ CHK CF
     UINT8 acPgUpSign[2]={0x70,0xA0};//PgUp请求
     UINT8 acPgDnSign[2]={0x71,0xA0};//PgDn请求

    UINT8 acClearListSign[2]={0x90,0xA1}; //清除列表
   // UINT8 acDeleteSignleSign[2]={0x91,0xA1};//删除一个APP
    UINT8 acRcvBuf[COM_DATA_PKG_LEN] = {0,};
    UINT8 acAppName[COM_DATA_PKG_LEN] = {0,};
    UINT8 acSndBuf[COM_DATA_PKG_LEN] = {0,};
    Q_UNUSED(pParam);

#if     !SUPORT_X5X7_DEVICE
    memset(acDevName, 0, sizeof(acDevName));
    nRet = COM_GetComPortNameFromIndex(g_stComportInfo.nComNo, acDevName);
    if (RET_SUCCESS != nRet)
    {
        HHT_LOG(EN_ERR, "Get comport(%d) name failed", g_stComportInfo.nComNo);
        return (THREADRETURN)1;
    }
    if (0 != g_stComDev.IsOpenCom())
    {
        g_stComDev.CloseCom();
        Pub_MSleep(COM_CMD_DELAY_TIME);
    }

    for (;;)
    {
        if(1==g_nIsAppQuit)
        {
            HHT_LOG(EN_INFO," Exit Ops com Thread");
            break;
        }
        nRet = g_stComDev.OpenCom(acDevName, g_stComportInfo.nBaudRate);
        if (RET_SUCCESS != nRet)
        {
            HHT_LOG(EN_ERR, "Open com (%s) failed", acDevName);
            PUB_SS.PostOpenComFailed();
            Pub_MSleep(1);
            continue;
        }
        else
        {
            PUB_SS.PostOpenComSuccess();
            break;
        }
    }
    HHT_LOG(EN_ERR, "Open com (%s) success", acDevName);
#endif
//    PUB_SS.PostCameraStatusCheck();//测试摄像头弹窗

    for (;;)
    {
        if (1 == g_nIsAppQuit)
        {
            HHT_LOG(EN_INFO, "  Exit Ops com Thread");
            break;
        }
#if     !SUPORT_X5X7_DEVICE
        //X9 firmware check commands
        if(g_nX9FirmwareCheckFlag ==1)
        {
            UINT8 mSndBuf[13]= {0x7F,0x09,0x99,0xA2,0xB3,0xC4,0x02,0xFF,0x93,0xA0,0xEF,0xCF};
            while((10>nRetrytimes)&&(g_nX9FirmwareCheckFlag==1))
            {
                int Ret= g_stComDev.WriteCom(mSndBuf,13);
                if(Ret ==0)
                {
                    nRetrytimes++;
                    Pub_MSleep(100);
                    HHT_LOG(EN_INFO, "Write request check x9 firmware status commands failed.");
                }
                else
                {
                    HHT_LOG(EN_INFO, "Write request check x9 firmware status commands success.");
                    nRetrytimes ++;
                    Pub_MSleep(100);
                }
                if(nRetrytimes>=10||g_nX9FirmwareCheckFlag!=1)
                {
                    nRetrytimes=0;
                    break;
                }
            }
        }
#endif
         //清除APP列表
         //7F YY 99 A2 B3 C4 02 FF 90 A0 CHK CF / 7F YY 99 A2 B3 C4 02 FF 90 A1 ZZ CHK CF 清除列表
        if(g_nClearAllAppCommandsFlag == 1)
        {
            qDebug()<<Q_FUNC_INFO<<"Write clear app list commands .";
            HHT_LOG(EN_INFO, "Write clear app list commands .");
             UINT8 mSndBuf[13]= {0x7F,0x09,0x99,0xA2,0xB3,0xC4,0x02,0xFF,0x90,0xA0,0xEC,0xCF};
             while (3 > nRetrytimes)
             {
                 nRet = g_stComDev.WriteCom(mSndBuf,13);
                 if (0 == nRet)
                 {
                     nRetrytimes++;
                     Pub_MSleep(100);
                     HHT_LOG(EN_ERR, "Write Clear App List Commands to Com failed (%d)", nRetrytimes);
                     continue;
                 }
                 else
                 {
                     HHT_LOG(EN_ERR, "Write Clear App List Commands to Com success .");
                     break;
                 }
             }
             g_nClearAllAppCommandsFlag =0;//复位写请求删除标识
        }
        if(g_nJustClearAppInAndroidFlags==1)
        {
            qDebug()<<Q_FUNC_INFO<<"Just clear app list in Android commands .";
            HHT_LOG(EN_INFO, "Just clear app list in Android commands .");
             UINT8 mSndBuf[13]= {0x7F,0x09,0x99,0xA2,0xB3,0xC4,0x02,0xFF,0x94,0xA0,0xF0,0xCF};
             while (3 > nRetrytimes)
             {
                 nRet = g_stComDev.WriteCom(mSndBuf,13);
                 if (0 == nRet)
                 {
                     nRetrytimes++;
                     Pub_MSleep(100);
                     HHT_LOG(EN_ERR, "Write Just clear app list in Android commands  to Com failed (%d)", nRetrytimes);
                     continue;
                 }
                 else
                 {
                     HHT_LOG(EN_ERR, "Write Just clear app list in Android commands  Com success .");
                     break;
                 }
             }
            g_nJustClearAppInAndroidFlags =0;
        }
        //-------------------------------------------------------------------------------------------------------
        //7F YY 99 A2 B3 C4 02 FF 91 A0  WW WW XX XX XX ……CHK CF   7F YY 99 A2 B3 C4 02 FF 91 A1 ZZ   WW WW XX XX XX ……CHK CF 删除一个APP
        if(g_nWriteDeleteSingleAppCommandsFlag ==1)
        {
            qDebug()<<Q_FUNC_INFO<<"Write delete An App commands .";
            HHT_LOG(EN_INFO, "Write delete An App commands .");
             //添加读取删除一个APP
            int result = OPS_DeleteSingleAppFromAndroid(g_tryDeleteAppName);//
            if(result ==0)
            {
                g_nDeleteSingleAppFlag =1;
            }
            else
            {
                g_nDeleteSingleAppFlag =0;
            }
            g_nWriteDeleteSingleAppCommandsFlag=0;
            g_tryDeleteAppName ="";
        }
        //-------------------------------------------------------------------------------------------------------
        //7F YY 99 A2 B3 C4 02 FF 92 A0 CHK CF   7F YY 99 A2 B3 C4 02 FF 92 A1  ZZ CHK CF //PubUSB接收
        //切换PubUSB请求
        if(g_nPubUSBCommandsFlag ==1)
        {
            HHT_LOG(EN_INFO, "Write allow switch commands .");
            UINT8 mSndBuf[13]= {0x7F,0x0A,0x99,0xA2,0xB3,0xC4,0x02,0xFF,0x92,0xA1,0x01,0xF1,0xCF};
            while (3 > nRetrytimes)
            {
                nRet = g_stComDev.WriteCom(mSndBuf,13);
                if (0 == nRet)
                {
                    nRetrytimes++;
                    Pub_MSleep(100);
                    HHT_LOG(EN_ERR, "Write allow switch PubUSB  Commands to Com failed (%d)", nRetrytimes);
                    continue;
                }
                else
                {
                    HHT_LOG(EN_ERR, "Write allow switch PubUSB  Commands to Com success .");
                    break;
                }
            }
            g_nPubUSBCommandsFlag =-1;//复位写请求switch标识
        }
        else if (g_nPubUSBCommandsFlag ==0)
        {
            HHT_LOG(EN_INFO, "Write do not allow switch commands .");
            UINT8 mSndBuf[13]= {0x7F,0x0A,0x99,0xA2,0xB3,0xC4,0x02,0xFF,0x92,0xA1,0x00,0xF0,0xCF};
            while (3 > nRetrytimes)
            {
                nRet = g_stComDev.WriteCom(mSndBuf,13);
                if (0 == nRet)
                {
                    nRetrytimes++;
                    Pub_MSleep(100);
                    HHT_LOG(EN_ERR, "Write not allow switch PubUSB  Commands to Com failed (%d)", nRetrytimes);
                    continue;
                }
                else
                {
                    HHT_LOG(EN_ERR, "Write not allow switch PubUSB  Commands  to Com success .");
                    break;
                }
            }
            g_nPubUSBCommandsFlag =-1;//复位写请求switch标识
        }
         //-------------------------------------------------------------------------------------------------------
        // 添加向串口写入摄像头状态指令
        if(g_nCameraCommandsFlag==1)
        {//可用状态
            int nRetrytimes =0;
            //write positive commands
            UINT8 mSndBuf[13]={0,};
            if(0==memcmp(&acRecvCameraSign[0],acCameraUpSign,2))
            {//UP
                UINT8 comands[13] = {0x7F,0x0A,0x99,0xA2,0xB3,0xC4,0x02,0xFF,0x80,0xA1,0x01,0xDF,0xCF};
                memcpy(mSndBuf,comands,13);
            }
            else if (0==memcmp(&acRecvCameraSign[0],acCameraDownSign,2))
            {//DOWN
                UINT8 comands[13] = {0x7F,0x0A,0x99,0xA2,0xB3,0xC4,0x02,0xFF,0x81,0xA1,0x01,0xE0,0xCF};
                memcpy(mSndBuf,comands,13);
            }
            HHT_LOG(EN_INFO, "write=========1========>%s",Pub_ConvertHexToStr(mSndBuf,13));
            while (3 > nRetrytimes)
            {
                nRet = g_stComDev.WriteCom(mSndBuf,13);
                if (0 == nRet)
                {
                    nRetrytimes++;
                    Pub_MSleep(100);
                    HHT_LOG(EN_ERR, "Write Camera Positive Commands to Com failed (%d)", nRetrytimes);
                    continue;
                }
                else
                {
                    memset(acRecvCameraSign,0,sizeof(acRecvCameraSign));
                    break;
                }
            }
            g_nCameraCommandsFlag = 0;
        }
        else if(g_nCameraCommandsFlag==-1)
        {//不可用状态
            int nRetrytimes =0;
            //write negative commands
            UINT8 mSndBuf[13]={0,};
            if(0==memcmp(&acRecvCameraSign[0],acCameraUpSign,2))
            {//UP
                UINT8 comands[13]= {0x7F,0x0A,0x99,0xA2,0xB3,0xC4,0x02,0xFF,0x80,0xA1,0x00,0xDE,0xCF};
                memcpy(mSndBuf,comands,13);
            }
            else if (0==memcmp(&acRecvCameraSign[0],acCameraDownSign,2))
            {//DOWN
                UINT8 comands[13]= {0x7F,0x0A,0x99,0xA2,0xB3,0xC4,0x02,0xFF,0x81,0xA1,0x00,0xDF,0xCF};
                memcpy(mSndBuf,comands,13);
            }
            HHT_LOG(EN_INFO, "write=========-1========>%s",Pub_ConvertHexToStr(mSndBuf,13));
            while (3 > nRetrytimes)
            {
                nRet = g_stComDev.WriteCom(mSndBuf,13);
                if (0 == nRet)
                {
                    nRetrytimes++;
                    Pub_MSleep(100);
                    HHT_LOG(EN_ERR, "Write Camera Negative Commands to Com failed (%d)", nRetrytimes);
                    continue;
                }
                else
                {
                    memset(acRecvCameraSign,0,sizeof(acRecvCameraSign));
                    break;
                }
            }
            g_nCameraCommandsFlag = 0;
        }
        // --------------------------------------------------------------------------------------------------------
        if (1 == g_nWriteFileFlag)  //write commands
        {
            nRet = Ops_WriteFiles(&g_stComDev);
            if (RET_SUCCESS != nRet)
            {
                HHT_LOG(EN_ERR, "Ops write files to Android failed");
                PUB_SS.PostSendFileToAndroidFailed();//
            }
            else
            {
                HHT_LOG(EN_INFO, "[X9 X8 X6]: Ops write files to Android success");
            }
            g_nWriteFileFlag = 0;
        }
#if     !SUPORT_X5X7_DEVICE
        if(1==g_nWriteX5X7FileFlag)
        {
            //X5 X7 Frimware status
            nRet = Ops_WriteX5X7Files(&g_stComDev);
            if(RET_SUCCESS!=nRet)
            {
                HHT_LOG(EN_ERR, "[X5X7]: Ops write files to Android failed");
                PUB_SS.PostSendFileToAndroidFailed();//
            }
            else
            {
                HHT_LOG(EN_INFO, "[X5X7]: Ops write files to Android success");
            }
            g_nWriteX5X7FileFlag==0;
        }
#endif
        memset(acRcvBuf, 0, sizeof(acRcvBuf));
        nRet = g_stComDev.ReadComByHeader(acRcvBuf,COM_DATA_PKG_LEN);//read commands
        if (0 >= nRet)
        {
            Pub_MSleep(10);
            continue;
        }
        HHT_LOG(EN_INFO, " ==> get data(%s)", Pub_ConvertHexToStr(acRcvBuf, COM_DATA_PKG_LEN));

        //----------------------------------------------------------------------------------------------------------
        //清除APP列表
        if((COM_HEADER ==acRcvBuf[0])&&(0==memcmp(&acRcvBuf[2],acCmdSign,6))
            &&(0==memcmp(&acRcvBuf[8],acClearListSign,2))&&(0x00 == acRcvBuf[10]))
        {
            PUB_SS.PostClearAllApp();//success flag
            qDebug()<<Q_FUNC_INFO<<"Android Clear App List  success. ";
            HHT_LOG(EN_INFO, "Android Clear App List  success. ");
        }
        //接收Android请求打开一个Windows APP指令
        if ((COM_HEADER == acRcvBuf[0]) && (0 == memcmp(&acRcvBuf[2], acCmdSign, 6))
                &&(0xA1 == acRcvBuf[8]) && (0xA0 == acRcvBuf[9]))   //安卓下打开windows系统应用指令
        {
            nDataLen = acRcvBuf[1];
            nAppNameLen = nDataLen -6-2-1;    //应用名称长度=数据长度-命令标识长度-命令码长度-校验和长度
            memcpy(acAppName, &acRcvBuf[10], nAppNameLen);

            QByteArray baAppName;
            baAppName.clear();
            for (i=0; i<nAppNameLen; i++)
            {
                baAppName.append(acAppName[i]);
            }
            QString strAppName = QString::fromLocal8Bit(baAppName);
            HHT_LOG(EN_INFO, "  app name (%s)", strAppName.toLocal8Bit().data());
            HHT_LOG(EN_INFO, "  app byte array name(%s)", baAppName.data());
            /*notice gui to start app*/
            PUB_SS.PostOpenApp(strAppName);//qstring

            nRetrytimes = 0;
            while (nWriteMaxCount > nRetrytimes)
            {
                memset(acSndBuf, 0, sizeof(acSndBuf));
                acSndBuf[0] = COM_HEADER;
                acSndBuf[1] = acRcvBuf[1]+1;    //数据长度
                memcpy(&acSndBuf[2], acCmdSign, 6);
                acSndBuf[8] = 0xA1;
                acSndBuf[9] = 0xA1;
                acSndBuf[10] = 0x00;    //状态ZZ
                memcpy(&acSndBuf[11], &acRcvBuf[10], nAppNameLen);
                ucChecksum = 0;
                for (i=1; i<11+nAppNameLen; i++)
                {
                    ucChecksum += acSndBuf[i];
                }
                acSndBuf[11+nAppNameLen] = ucChecksum;
                acSndBuf[11+nAppNameLen+1] = COM_ENDER;
                nRet = g_stComDev.WriteCom(acSndBuf, 11+nAppNameLen+2);
                if (0 >= nRet)
                {
                    nRetrytimes++;
                    Pub_MSleep(1);
                    continue;
                }
                else
                {
                    HHT_LOG(EN_INFO, "write (%s) ok", Pub_ConvertHexToStr(acSndBuf, nRet));
                    break;
                }
            }
        }
#if     !SUPORT_X5X7_DEVICE
        //接受来自Android的X9 firmware响应
        if((COM_HEADER ==acRcvBuf[0])&&(0 == memcmp(&acRcvBuf[2], acCmdSign, 6))
            &&(0x93 == acRcvBuf[8]) && (0xA1 == acRcvBuf[9])&&(0x00 == acRcvBuf[10]))
        {
                HHT_LOG(EN_INFO, "Received X9 Firmware check echo from Android.");
                PUB_SS.PostX9FirmwareCheck();//发送X9固件确认信号
                g_nX9FirmwareCheckFlag =-1;
        }
#endif
        //----------------------------------------------------------------------------------------------------------
        // 添加读取底层调用摄像头指令信息
        //upper 7F YY 99 A2 B3 C4 02 FF 80 A0 CHK CF   7F YY 99 A2 B3 C4 02 FF 80 A1 ZZ CHK CF
        //lower 7F YY 99 A2 B3 C4 02 FF 81 A0 CHK CF   7F YY 99 A2 B3 C4 02 FF 81 A1 ZZ CHK CF
        if(((COM_HEADER ==acRcvBuf[0])&&(0==memcmp(&acRcvBuf[2],acCmdSign,6))
            &&(COM_ENDER==acRcvBuf[11]))&&((0==memcmp(&acRcvBuf[8],acCameraUpSign,2))
            ||(0==memcmp(&acRcvBuf[8],acCameraDownSign,2))))
        {
            if(0==memcmp(&acRcvBuf[8],acCameraUpSign,2))
            {
                memcpy(&acRecvCameraSign[0],acCameraUpSign,2);
            }
            else if (0==memcmp(&acRcvBuf[8],acCameraDownSign,2))
            {
                memcpy(&acRecvCameraSign[0],acCameraDownSign,2);
            }
            HHT_LOG(EN_INFO, "  Receive Cemera Commands Data(%s)", Pub_ConvertHexToStr(acRcvBuf, COM_DATA_PKG_LEN));
            qDebug()<<"Receive calling camera commands success.";
            HHT_LOG(EN_INFO, "Receive calling camera commands success.");
            PUB_SS.PostCameraStatusCheck();
        }
         //----------------------------------------------------------------------------------------------------------
        //切换PubUSB请求
        if((COM_HEADER ==acRcvBuf[0])&&(0==memcmp(&acRcvBuf[2],acCmdSign,6))
            &&(0==memcmp(&acRcvBuf[8],acPubUSBSign,2)))
        {
                HHT_LOG(EN_INFO, "  Receive Switch PubUSB Commands Data(%s)", Pub_ConvertHexToStr(acRcvBuf, COM_DATA_PKG_LEN));
                HHT_LOG(EN_INFO, "Receive calling switch request commands success.");
                PUB_SS.PostPubUsbStatusCheck();
        }
        //----------------------------------------------------------------------------------------------------------
        //添加读取删除一个APP
        /*
        if(((COM_HEADER ==acRcvBuf[0])&&(0==memcmp(&acRcvBuf[2],acCmdSign,6))
            &&(COM_ENDER==acRcvBuf[11]))&&((0==memcmp(&acRcvBuf[8],acDeleteSignleSign,2))))
        {
                g_nDeleteSingleAppFlag =1;
                HHT_LOG(EN_INFO, "  Receive Delete An App Returns Commands Data(%s)", Pub_ConvertHexToStr(acRcvBuf, COM_DATA_PKG_LEN));
        }
        else
        {
             qDebug()<<"Receive delete An App commands failed.";
             HHT_LOG(EN_INFO, "Receive delete An App commands failed.");
        }
        */
        //  -----------------------------------------------------------------------------------------------------------

        //添加增加遥控器功能指令
        if ((COM_HEADER == acRcvBuf[0]) && (0 == memcmp(&acRcvBuf[2], acCmdSign, 6))
                &&(0==memcmp(&acRcvBuf[8],acPgUpSign,2)))
        {
                HHT_LOG(EN_INFO, "Receive PgUp commands success.");
                keybd_event(VK_PRIOR,0,0,0);//发送系统pgUp虚拟键盘值
                Sleep(100);
                keybd_event(VK_PRIOR,0,KEYEVENTF_KEYUP,0);
        }
        else if ((COM_HEADER == acRcvBuf[0]) && (0 == memcmp(&acRcvBuf[2], acCmdSign, 6))
                &&(0==memcmp(&acRcvBuf[8],acPgDnSign,2)))
        {
                HHT_LOG(EN_INFO, "ReceivePgDn commands success.");
                keybd_event(VK_NEXT,0,0,0);//发送系统pgDn虚拟键盘值
                Sleep(100);
                keybd_event(VK_NEXT,0,KEYEVENTF_KEYUP,0);
        }
        if ((COM_HEADER == acRcvBuf[0]) && (0 == memcmp(&acRcvBuf[2], acCmdSign, 6))
                &&(0==memcmp(&acRcvBuf[8],acZoomInSign,2)))
        {
                HHT_LOG(EN_INFO, "Receive ZoomIn commands success.");
                keybd_event(VK_CONTROL,0,0,0);
                keybd_event(VK_OEM_MINUS,0,0,1<<29);//发送系统Ctrl -虚拟键盘值
                Sleep(100);
                keybd_event(VK_OEM_MINUS,0,KEYEVENTF_KEYUP,0);
                keybd_event(VK_CONTROL,0,KEYEVENTF_KEYUP,0);
        }
        else if ((COM_HEADER == acRcvBuf[0]) && (0 == memcmp(&acRcvBuf[2], acCmdSign, 6))
                 &&(0==memcmp(&acRcvBuf[8],acZoomOutSign,2)))
        {
                HHT_LOG(EN_INFO, "Receive ZoomOut commands success.");
                keybd_event(VK_CONTROL,0,0,0);
                keybd_event(VK_OEM_PLUS,0,0,1<<29);//发送系统Ctrl +虚拟键盘值
                Sleep(100);
                keybd_event(VK_OEM_PLUS,0,KEYEVENTF_KEYUP,0);
                keybd_event(VK_CONTROL,0,KEYEVENTF_KEYUP,0);
        }
        //------------------------------------------------------------------------------------------------------------

    }
    return RET_SUCCESS;
}

/***********************************************************
Function     :   OPS_QuitThread
Description  :   退出OPS串口控制主线程
Input        :   无
Output       :   无
Return       :   无
Others       :   无
Author       :
History      :   1. 创建函数2017.05.24
***********************************************************/

void OPS_QuitThread()
{
    g_nIsAppQuit = 1;
}

/***********************************************************
Function     :   OPS_SendFilesToAndroid
Description  :   OPS发送文件到Android接口供GUI调用
Input        :  nFileNum 文件数, listAllFilesName要写入的文件名列表, listAllFilesData 要写入的文件内容列表
Output       :   无
Return       :   无
Others       :   无
Author       :
History      :   1. 创建函数2017.05.23
***********************************************************/

void OPS_SendFilesToAndroid(int nFileNum, QList<QList<unsigned char>>listAllFilesName, QList<QList<unsigned char>> listAllFilesData)
{
    g_nFileNum = nFileNum;
    g_listFilesName = listAllFilesName;
    g_listAllFilesData = listAllFilesData;

    g_nWriteFileFlag = 1;
}

/***********************************************************
Function     :   Ops_WriteFiles
Description  :   OPS发送文件到Android具体实现接口
Input        :   pstComDev 串口设备
Output       :   无
Return       :   0 success, others failure
Others       :   无
Author       :
History      :   1. 创建函数2017.05.23
***********************************************************/

int Ops_WriteFiles(COMDEV *pstComDev)
{
    INT32 i = 0;
    INT32 j = 0;
    INT32 k = 0;
    INT32 nRet = -1;
    INT32 nDataLen = 0;
    INT32 nRetrytimes = 0;
    INT32 nTotalPkgNum = 0;
    INT32 nLastPkgDataNum = 0;  //最有一个数据包的长度可能为满包(50)也可能不是
    const INT32 nWriteMaxCount = 3;
    UINT8 ucChecksum = 0;
    UINT8 acSndBuf[COM_DATA_PKG_LEN] = {0,};
    //UINT8 acRcvBuf[COM_DATA_PKG_LEN] = {0,};
    UINT8 acCmdSign[6] = {0x99, 0xA2, 0xB3, 0xC4, 0x02, 0xFF};    //命令标识

    if (NULL == pstComDev)
    {
        HHT_LOG(EN_INFO, "======Com dev not found===== ");
        return RET_INVALID;
    }
    /*OPS向Android传送文件列表开始*/
    acSndBuf[0] = COM_HEADER;
    acSndBuf[1] = 0x0B; //YY数据长度
    memcpy(&acSndBuf[2], acCmdSign, 6);
    acSndBuf[8] = COM_TRANS_FILELIST_START;
    acSndBuf[9] = 0xA0;
    acSndBuf[10] = g_nFileNum % 0x100;
    acSndBuf[11] = g_nFileNum / 0x100;
    for (i=1; i<12; i++)
    {
        ucChecksum += acSndBuf[i];
    }
    acSndBuf[12] = ucChecksum;
    acSndBuf[13] = COM_ENDER;
    while (nWriteMaxCount > nRetrytimes)
    {
        nRet = Com_Write(pstComDev, acSndBuf, 14);
        //        HHT_LOG(EN_INFO, "  COM_RECV CHECK STATUS(%d)",nRet);
        //        HHT_LOG(EN_INFO, "  START SEND COMMAND (%s)", Pub_ConvertHexToStr(acSndBuf, 14));
        if (RET_SUCCESS != nRet)
        {
            nRetrytimes++;
            Pub_MSleep(1);
            continue;
        }
        else
        {
            break;
        }
    }

    if (RET_SUCCESS != nRet)
    {
        g_nWriteStatus = 1;
        HHT_LOG(EN_INFO, "======Write status(%d)===== ",g_nWriteStatus);
        return g_nWriteStatus;
    }

    g_nTotalPkgNum = 0;
    for (i=0; i<g_nFileNum; i++)
    {
        QList <unsigned char> listFileData = g_listAllFilesData[i];
        if (listFileData.size() % COM_VALID_DATA_LEN)
        {
            nTotalPkgNum = listFileData.size() / COM_VALID_DATA_LEN+1;
        }
        else
        {
            nTotalPkgNum = listFileData.size() / COM_VALID_DATA_LEN;
        }
        g_nTotalPkgNum += nTotalPkgNum;
    }
    PUB_SS.PostTotalPkgNum(g_nTotalPkgNum);
    HHT_LOG(EN_INFO, "PUB_SS->>PostTotalPkgNum: (%d)",g_nTotalPkgNum);
    g_nCurrentPkgNum = 0;

    for (i=0; i<g_nFileNum; i++)
    {
        HHT_LOG(EN_INFO, "++++++++++++Strat sending file names");
        /*OPS向Android传输文件名*/
        /*OPS向Android传送文件名且开始传输文件*/
        memset(acSndBuf, 0, sizeof(acSndBuf));
        acSndBuf[0] = COM_HEADER;
        HHT_LOG(EN_INFO,"1.///////////////////////////////////////////file name length:[%d]////////////////////////////////////",g_listFilesName[i].size());
#if 1
        if(g_listFilesName[i].size()>COM_VALID_DATA_LEN)
        {
                g_nWriteStatus = -100;
                PUB_SS.PostFileNameTooLong();
                Pub_MSleep(100);
                return g_nWriteStatus;
        }
        nDataLen = 6 + 2 + 2 + g_listFilesName[i].size() + 1; //YY数据长度(命令标识+命令码+总包数+文件名+校验位)
        acSndBuf[1] = nDataLen;
        memcpy(&acSndBuf[2], acCmdSign, 6);
        acSndBuf[8] = COM_TRANS_FILENAME_START;
        acSndBuf[9] = 0xA0;
        QList <unsigned char> listFileData = g_listAllFilesData[i];
        if (listFileData.size() % COM_VALID_DATA_LEN)
        {
            nTotalPkgNum = listFileData.size() / COM_VALID_DATA_LEN+1;
            nLastPkgDataNum = listFileData.size() % COM_VALID_DATA_LEN;
        }
        else
        {
            nTotalPkgNum = listFileData.size() / COM_VALID_DATA_LEN;
            nLastPkgDataNum = COM_VALID_DATA_LEN;
        }
        acSndBuf[10] = nTotalPkgNum % 0x100;
        acSndBuf[11] = nTotalPkgNum / 0x100;

        for (j=0; j<g_listFilesName[i].size(); j++)
        {
            acSndBuf[12+j] = g_listFilesName[i][j];
        }

        ucChecksum = 0;
        for (j=1; j<nDataLen+1; j++)
        {
            ucChecksum += acSndBuf[j];
        }
        acSndBuf[nDataLen+1] = ucChecksum;
        acSndBuf[nDataLen+2] = COM_ENDER;
        nRetrytimes = 0;
        while (nWriteMaxCount > nRetrytimes)
        {
            nRet = Com_Write(pstComDev, acSndBuf, nDataLen+3);
            // HHT_LOG(EN_INFO, "  START WRITE FILE COMMAND (%s)", Pub_ConvertHexToStr(acSndBuf, nDataLen+3));
            if (RET_SUCCESS != nRet)
            {
                nRetrytimes++;
                Pub_MSleep(1);
                continue;
            }
            else
            {
                break;
            }
        }

        if (RET_SUCCESS != nRet)
        {
            g_nWriteStatus = 2;
            HHT_LOG(EN_INFO, "======Write status(%d)===== ",g_nWriteStatus);
            return g_nWriteStatus;
        }
#else
        QList <unsigned char> listFileData = g_listAllFilesData[i];
        if (listFileData.size() % COM_VALID_DATA_LEN)
        {
            nTotalPkgNum = listFileData.size() / COM_VALID_DATA_LEN+1;
            nLastPkgDataNum = listFileData.size() % COM_VALID_DATA_LEN;
        }
        else
        {
            nTotalPkgNum = listFileData.size() / COM_VALID_DATA_LEN;
            nLastPkgDataNum = COM_VALID_DATA_LEN;
        }
        int nTotalFileNameNum =0 , nLastFileNameNum=0;
        QList<unsigned char>listFileName  =g_listFilesName[i];
        if(listFileName.size()%COM_VALID_DATA_LEN)
        {
              nTotalFileNameNum  = listFileName.size()/COM_VALID_DATA_LEN +1;
              nLastFileNameNum   = listFileName.size()%COM_VALID_DATA_LEN;
        }
        else
        {
             nTotalFileNameNum  = listFileName.size()/COM_VALID_DATA_LEN;
             nLastFileNameNum   = COM_VALID_DATA_LEN;
        }
        //TODO
#endif

        HHT_LOG(EN_INFO, "++++++++++++Strat sending file content");

        /*OPS向Android传输文件内容*/
        for (j=0; j<nTotalPkgNum; j++)
        {
            memset(acSndBuf, 0, sizeof(acSndBuf));
            acSndBuf[0] = COM_HEADER;
            memcpy(&acSndBuf[2], acCmdSign, 6);
            acSndBuf[8] = COM_TRANS_FILE;
            acSndBuf[9] = 0xA0;
            acSndBuf[10] = j % 0x100;
            acSndBuf[11] = j / 0x100;
            if (j  != nTotalPkgNum-1)
            {
                nDataLen = 6 + 2 + 2 + COM_VALID_DATA_LEN + 1; //YY数据长度(命令标识+命令码+包索引+该包有效数据+校验位)
                for (k=0; k<COM_VALID_DATA_LEN; k++)
                {
                    acSndBuf[12+k] = listFileData[k+j*COM_VALID_DATA_LEN];
                }
            }
            else
            {
                nDataLen = 6 + 2 + 2 + nLastPkgDataNum + 1; //YY数据长度(命令标识+命令码+包索引+该包有效数据+校验位)
                for (k=0; k<nLastPkgDataNum; k++)
                {
                    acSndBuf[12+k] = listFileData[k+j*COM_VALID_DATA_LEN];
                }
            }
            acSndBuf[1] = nDataLen;
            ucChecksum = 0;
            for (k=1; k<nDataLen+1; k++)
            {
                ucChecksum += acSndBuf[k];
            }
            acSndBuf[nDataLen+1] = ucChecksum;
            acSndBuf[nDataLen+2] = COM_ENDER;

            nRetrytimes = 0;
            while (nWriteMaxCount > nRetrytimes)
            {
                nRet = Com_Write(pstComDev, acSndBuf, nDataLen+3);
                //                HHT_LOG(EN_INFO, " WRITE FILE COMMAND (%d)(%s)",j, Pub_ConvertHexToStr(acSndBuf,nDataLen+3));
                if (RET_SUCCESS != nRet)
                {
                    nRetrytimes++;
                    Pub_MSleep(1);
                    continue;
                }
                else
                {
                    break;
                }
            }

            if (RET_SUCCESS != nRet)
            {
                g_nWriteStatus = 3;
                HHT_LOG(EN_INFO, "======Write status(%d)===== ",g_nWriteStatus);
                return g_nWriteStatus;
            }
            else
            {
                g_nCurrentPkgNum++;
                PUB_SS.PostCurrentPkgNum(g_nCurrentPkgNum);
            }
        }

        HHT_LOG(EN_INFO, " ++++++++++++Strat sending tanslate file stop commands");
        /*OPS向Android传输文件名且停止传输文件*/
        memset(acSndBuf, 0, sizeof(acSndBuf));
        acSndBuf[0] = COM_HEADER;
        HHT_LOG(EN_INFO,"2.///////////////////////////////////////////file name length:[%d]////////////////////////////////////",g_listFilesName[i].size());

#if 1
        if(g_listFilesName[i].size()>COM_VALID_DATA_LEN)
        {
                g_nWriteStatus = -200;
                PUB_SS.PostFileNameTooLong();
                Pub_MSleep(100);
                return g_nWriteStatus;
        }
        nDataLen = 6 + 2 + 2 + g_listFilesName[i].size() + 1; //YY数据长度(命令标识+命令码+总包数+文件名+校验位)
        acSndBuf[1] = nDataLen;
        memcpy(&acSndBuf[2], acCmdSign, 6);
        acSndBuf[8] = COM_TRANS_FILENAME_STOP;
        acSndBuf[9] = 0xA0;
        if (listFileData.size() % COM_VALID_DATA_LEN)
        {
            nTotalPkgNum = listFileData.size() / COM_VALID_DATA_LEN+1;
        }
        else
        {
            nTotalPkgNum = listFileData.size() / COM_VALID_DATA_LEN;
        }
        acSndBuf[10] = nTotalPkgNum % 0x100;
        acSndBuf[11] = nTotalPkgNum / 0x100;
        for (j=0; j<g_listFilesName[i].size(); j++)
        {
            acSndBuf[12+j] = g_listFilesName[i][j];
        }
        ucChecksum = 0;
        for (j=1; j<nDataLen+1; j++)
        {
            ucChecksum += acSndBuf[j];
        }
        acSndBuf[nDataLen+1] = ucChecksum;
        acSndBuf[nDataLen+2] = COM_ENDER;
        nRetrytimes = 0;
        while (nWriteMaxCount > nRetrytimes)
        {
            nRet = Com_Write(pstComDev, acSndBuf, nDataLen+3);
            //            HHT_LOG(EN_INFO, "  STOP SEND FILE COMMAND (%s)", Pub_ConvertHexToStr(acSndBuf, nDataLen+3));
            if (RET_SUCCESS != nRet)
            {
                nRetrytimes++;
                Pub_MSleep(1);
                continue;
            }
            else
            {
                break;
            }
        }

        if (RET_SUCCESS != nRet)
        {
            g_nWriteStatus = 4;
            HHT_LOG(EN_INFO, "======Write status(%d)===== ",g_nWriteStatus);
            return g_nWriteStatus;
        }
    }
#else

#endif
    HHT_LOG(EN_INFO, " ++++++++++++++Start sending stop command");

    /*OPS向Android传送文件列表结束*/
    acSndBuf[0] = COM_HEADER;
    acSndBuf[1] = 0x0B; //YY数据长度
    memcpy(&acSndBuf[2], acCmdSign, 6);
    acSndBuf[8] = COM_TRANS_FILELIST_STOP;
    acSndBuf[9] = 0xA0;
    acSndBuf[10] = g_nFileNum % 0x100;
    acSndBuf[11] = g_nFileNum / 0x100;
    ucChecksum = 0;
    for (i=1; i<12; i++)
    {
        ucChecksum += acSndBuf[i];
    }
    acSndBuf[12] = ucChecksum;
    acSndBuf[13] = COM_ENDER;

    while (nWriteMaxCount > nRetrytimes)
    {
        nRet = Com_Write(pstComDev, acSndBuf, 14);
        //        HHT_LOG(EN_INFO, "  STOP SEND COMMAND (%s)", Pub_ConvertHexToStr(acSndBuf,14));
        if (RET_SUCCESS != nRet)
        {
            nRetrytimes++;
            Pub_MSleep(1);
            continue;
        }
        else
        {
            break;
        }
    }
    if (RET_SUCCESS != nRet)
    {
        g_nWriteStatus = 5;
        HHT_LOG(EN_INFO, "======Write status(%d)===== ",g_nWriteStatus);
        return g_nWriteStatus;
    }
    g_nWriteStatus = 0; //success
    HHT_LOG(EN_INFO, "======Write status(%d)===== ",g_nWriteStatus);
    return g_nWriteStatus;
}

int Ops_WriteStatus()
{
    return g_nWriteStatus;
}

void ReadConfig()
{
    QString strCfgPath = "Settings/Config/config.dat";
    QFile file(strCfgPath);
    if (!file.exists())//default
    {
        g_stComportInfo.nComNo = 1;
        g_stComportInfo.nBaudRate = 115200;
        return;
    }
    else
    {
        QSettings settings(strCfgPath, QSettings::IniFormat);
        settings.setIniCodec(QTextCodec::codecForName("System"));

        g_stComportInfo.nComNo = settings.value("COMMON/comno", 1).toInt();
        g_stComportInfo.nBaudRate = settings.value("COMMON/baud", 115200).toInt();
        HHT_LOG(EN_INFO, "  comNo(%d), baudRate(%d)", g_stComportInfo.nComNo, g_stComportInfo.nBaudRate);
    }
}

int OPS_DeleteSingleAppFromAndroid(QString appName)
{
    QList<unsigned char> fileNameDataList;
    UINT8 ucChecksum = 0;
    INT32  nRetrytimes = 0;
    INT32  nTotalPkgNum=0,
                nLastPkgDataNum=0;
    int       nRet = -1;
    INT32  nDataLen     = 0;
    UINT8 acSndBuf[COM_DATA_PKG_LEN] = {0,};
    UINT8 acCmdSign[6] = {0x99, 0xA2, 0xB3, 0xC4, 0x02, 0xFF};    //命令标识

    if(appName.isEmpty())
    {
        return  -1;
    }
    QByteArray fileNameByteArray =appName.toUtf8();//utf8
    MakeByte2UCharList(fileNameByteArray,fileNameDataList);

    qDebug()<<Q_FUNC_INFO<<"---->fileName(Asic): "<<fileNameByteArray;
    qDebug()<<Q_FUNC_INFO<<"---->fileName(Hex): "<<fileNameByteArray.toHex();

    memset(acSndBuf, 0, sizeof(acSndBuf));
    acSndBuf[0] = COM_HEADER;

    HHT_LOG(EN_INFO,"1.///////////////////////////////////////////file name length:[%d]////////////////////////////////////",fileNameDataList.size());
    if(fileNameDataList.size()>COM_VALID_DATA_LEN)
    {
            PUB_SS.PostFileNameTooLong();
            Pub_MSleep(100);
            return -100;
    }

    nDataLen = 6 + 2 + 2 + fileNameDataList.size() + 1; //YY数据长度(命令标识+命令码+总包数+文件名+校验位)
    acSndBuf[1] = nDataLen;
    memcpy(&acSndBuf[2], acCmdSign, 6);
    acSndBuf[8] = 0x91;
    acSndBuf[9] = 0xA0;

    if (fileNameDataList.size()% COM_VALID_DATA_LEN)
    {
        nTotalPkgNum = fileNameDataList.size() / COM_VALID_DATA_LEN+1;
        nLastPkgDataNum = fileNameDataList.size() % COM_VALID_DATA_LEN;
    }
    else
    {
        nTotalPkgNum = fileNameDataList.size() / COM_VALID_DATA_LEN;
        nLastPkgDataNum = COM_VALID_DATA_LEN;
    }
#if 1
    acSndBuf[10] = nTotalPkgNum % 0x100;
    acSndBuf[11] = nTotalPkgNum / 0x100;
#else
    acSndBuf[10] = nTotalPkgNum / 0x100;
    acSndBuf[11] = nTotalPkgNum % 0x100;
#endif
    for (int j=0; j<fileNameDataList.size(); j++)
    {
        acSndBuf[12+j] = fileNameDataList[j];
    }
    ucChecksum = 0;
    for (int j=1; j<nDataLen+1; j++)
    {
        ucChecksum += acSndBuf[j];
    }
    acSndBuf[nDataLen+1] = ucChecksum;
    acSndBuf[nDataLen+2] = COM_ENDER;

     HHT_LOG(EN_INFO, "  Send Delete An App Commands Data(%s)", Pub_ConvertHexToStr(acSndBuf, COM_DATA_PKG_LEN));
     qDebug("  Send Delete An App Commands Data(%s)",Pub_ConvertHexToStr(acSndBuf, COM_DATA_PKG_LEN/2));
    while (1> nRetrytimes)//one
    {
        nRet = Com_Write(&g_stComDev, acSndBuf, nDataLen+3);
        if (RET_SUCCESS != nRet)
        {
            nRetrytimes++;
            Pub_MSleep(1);
            HHT_LOG(EN_ERR, "Delete An App Commands to Com failed (%d)", nRetrytimes);
            continue;
        }
        else
        {
             HHT_LOG(EN_ERR, "Delete An App Commands to Com success(%s)", Pub_ConvertHexToStr(acSndBuf, COM_DATA_PKG_LEN/2));
            return 0;
        }
    }
    return -1;
}

void OPS_SndAppToX5X7(int nFileNum, QList<QList<unsigned char> > listAllFilesName)
{
        qDebug("===>Syncing [%d] apps to Compatible Mode Device.\n",nFileNum);
        HHT_LOG(EN_INFO,"===>Sync [%d] apps to Compatible Mode Device.\n",nFileNum);
        g_nFileNum = nFileNum;
        g_listFilesName.clear();
        g_listFilesName = listAllFilesName;
#if 0
        for(int i=0;i< g_listFilesName.size();i++)
        {
            qDebug("====>The [%d] app \n",i);
            for(int j=0;j< g_listFilesName.at(i).length();j++)
            {
                qDebug("->0x%02x ", g_listFilesName.at(i).at(j));
            }
            qDebug("\n");
        }
#endif
        g_nWriteX5X7FileFlag =1;
}

int Ops_WriteX5X7Files(COMDEV *pstComDev)
{
    UINT16 checksum = 0;
    UINT16 cmdlength = 0;
    int nRet=-1;
    const INT32 nWriteMaxCount = 3;
    UINT8 acSndBuf[256] = {0,};
    UINT8 acCmdHeader[2] = {0xaa,0x55};
    UINT8 acCmdEnder[2] = {0x55,0xaa};
    UINT8 acCmdSnderOps[3] = {0x02, 0x00, 0x03};            //发送者标识 发送给Android
    UINT8 acCmdSnderID[1]={0x00};
    UINT8 acCmdSnderAndroid[3] = {0x01, 0x00, 0x03};    //发送者标识 发送给OPS
    if(pstComDev==NULL)
    {
        HHT_LOG(EN_INFO, "++++++++Com dev not found++++++\n");
        return RET_INVALID;
    }
    //[aa 55] (26 00) (02 00 03) {30 34 31/A  30 36 33/c  30 36 33/c  30 36 35/e 30 37 33/s  30 37 33/s 30 32 30/空格  30 33 32/2  30 33 30/0  30 33 31/1 30 33 33/3 } (10 06) [55 aa]
    //TODO
    //向Android发送数据
    g_nTotalPkgNum = g_nFileNum;
    PUB_SS.PostTotalPkgNum(g_nTotalPkgNum);
    HHT_LOG(EN_INFO, "PUB_SS->>PostTotalPkgNum: (%d)",g_nTotalPkgNum);
    qDebug("PUB_SS->>PostTotalPkgNum: (%d) [size:%d]",g_nTotalPkgNum,g_listFilesName.size());
    g_nCurrentPkgNum = 0;
    Pub_MSleep(1000);//睡眠一段时间等待设置上传最大值
    PUB_SS.PostTotalPkgNum(g_nTotalPkgNum);
    for(int i=0;i<g_nTotalPkgNum;i++)
    {
        //逐个应用名字数据写入串口
        UINT8 acSndBufTemp[256] = {0,};
        cmdlength= g_listFilesName.at(i).length()+6;
        //(checksum)&0xFF,(checksum>>8)&0xFF
        UINT8 acCmdLength[2] ={0,};
        acCmdLength[0]= cmdlength&0xFF;
        acCmdLength[1]= (cmdlength>>8)&0xFF;
        qDebug("cmdlength: [0x%02x 0x%02x]",acCmdLength[0],acCmdLength[1]);
        checksum =acCmdLength[0]+acCmdLength[1]+acCmdSnderOps[0]+acCmdSnderOps[1]+acCmdSnderOps[2];
        for(int j=0;j<g_listFilesName.at(i).length();j++)
        {
            checksum+=g_listFilesName.at(i).at(j);
            acSndBufTemp[j]=g_listFilesName.at(i).at(j);
            //qDebug("0x%02x ",g_listFilesName.at(i).at(j));
        }
        //qDebug("===========================================================================\n");
        UINT8 acCmdCheckSum[2]={0,};
        acCmdCheckSum[0]=checksum&0xFF;
        acCmdCheckSum[1]=(checksum>>8)&0xFF;
        qDebug("checksum: [0x%02x 0x%02x]",acCmdCheckSum[0],acCmdCheckSum[1]);
        memcpy(acSndBuf,acCmdHeader,2);
        memcpy(&acSndBuf[2],acCmdLength,2);
        memcpy(&acSndBuf[3],"\x00",1);
        memcpy(&acSndBuf[5],acCmdSnderOps,3);
        memcpy(&acSndBuf[8],acSndBufTemp,g_listFilesName.at(i).length());
        memcpy(&acSndBuf[8+g_listFilesName.at(i).length()],acCmdCheckSum,2);
        memcpy(&acSndBuf[10+g_listFilesName.at(i).length()],acCmdEnder,2);
        int one_msg_length=2+2+1+3+g_listFilesName.at(i).length()+2+2;
        HHT_LOG(EN_INFO,"===Snder data to uart(%d)[%s]",one_msg_length,Pub_ConvertHexToStr(acSndBuf,one_msg_length));
        qDebug("===Snder data to uart(%d):[%s]",one_msg_length,Pub_ConvertHexToStr(acSndBuf,one_msg_length));
        //开始写入
#if 0
        int nRetrytimes =0;
        while (nWriteMaxCount > nRetrytimes)
        {
            nRet = X5X7Com_Write(pstComDev, acSndBuf, one_msg_length);
            HHT_LOG(EN_INFO, "START SENDE SNDBUF TO UART[%d] (%s)",one_msg_length, Pub_ConvertHexToStr(acSndBuf, one_msg_length));
            qDebug("START SENDE SNDBUF TO UART[%d] (%s)",one_msg_length, Pub_ConvertHexToStr(acSndBuf, one_msg_length));
            if (RET_SUCCESS != nRet)
            {
                nRetrytimes++;
                Pub_MSleep(1);
                continue;
            }
            else
            {
                break;
            }
            Pub_MSleep(100);
        }
#else
        for(int i=0;i<one_msg_length;i++)
        {
            nRet = X5X7Com_Write(pstComDev, &acSndBuf[i], 1);
            if (RET_SUCCESS != nRet)
            {
                Pub_MSleep(1);
                continue;
            }
            else
            {
                HHT_LOG(EN_INFO, "START SENDE SNDBUF TO UART BY BYTE[%d] (0x%02x)",1, acSndBuf[i]);
                qDebug("START SENDE SNDBUF TO UART BY BYTE[%d] (0x%02x)",i, acSndBuf[i]);
            }
            Pub_MSleep(10);
        }
        HHT_LOG(EN_INFO, "===================SENDER END==========================");
        qDebug("===================SENDER END==========================");
#endif
        if (RET_SUCCESS != nRet)
        {
            g_nWriteStatus = -100;
            HHT_LOG(EN_INFO, "======X5 X7 Write status(%d)===== ",g_nWriteStatus);
            qDebug("======X5 X7 Write status(%d)===== ",g_nWriteStatus);
            g_nTotalPkgNum = 0;
            return g_nWriteStatus;
        }
        else
        {
            g_nCurrentPkgNum++;
            PUB_SS.PostCurrentPkgNum(g_nCurrentPkgNum);
            HHT_LOG(EN_INFO, "PUB_SS->>PostCurrentPkgNum: (%d)\n",g_nCurrentPkgNum);
            qDebug("PUB_SS->>PostCurrentPkgNum: (%d)\n",g_nCurrentPkgNum);
        }
        checksum =0;
        cmdlength =0;
        memset(acSndBuf,0,sizeof(acSndBuf));
        Pub_MSleep(100);
    }
    g_nCurrentPkgNum =0;
    g_nTotalPkgNum = 0;
    g_listFilesName.clear();
    g_nWriteStatus = 0; //success
    return g_nWriteStatus; //default -1
}

THREADRETURN X5X7OpsCom_ThreadFn(void *pParam)
{
     UINT8 acRcvBuf[256] = {0,};
     UINT8 acAppName[128] = {0,};
     UINT8 acCmdSnderOps[3] = {0x02, 0x00, 0x03};            //发送者标识 发送给Android
     UINT8 acCmdSnderAndroid[3] = {0x10, 0x00, 0x03};      //发送者标识  发送给OPS
     UINT8 acHandShakeBuf[]={0xaa,0x55,0x1a,0x00,0x4a,0x10,0x00,0x03,0x30,0x30,0x36,0x38 ,0x30,0x30,
                0x36,0x35,0x30,0x30,0x36,0x31,0x30,0x30,0x37,0x32,0x30,0x30,0x37,0x34 ,0x6b,0x04,0x55,0xaa };//类似x5x7握手指令Android->OPS
     UINT8 acHandShakeSndBuf[]={0xaa,0x55,0x1a,0x00,0x00,0x02,0x00,0x03,0x30,0x30,0x36,0x38,0x30,0x30,
                0x36,0x35,0x30,0x30,0x36,0x31,0x30,0x30,0x37,0x32,0x30,0x30,0x37,0x34 ,0x13,0x04,0x55,0xaa};//类似x5x7握手指令OPS->Android
     int        nDataLen =0 ,nAppNameLen =0,nShakeTimes=0;
    while (1)
    {
        if(nShakeTimes>=1)
        {
            g_nX5X7HandShakeFlag=-1;
        }
        if(1==g_nX5X7HandShakeFlag)
        {
            //0x68 0x65 0x61 0x72 0x74 心跳包指令->heart
#if 1
            int nRet = g_stComDev.WriteCom(acHandShakeSndBuf,sizeof(acHandShakeSndBuf));
            if(nRet<=0)
            {
                HHT_LOG(EN_INFO, "[Compatible Mode]: Ops write handshake to Android failed");
                qDebug("[Compatible Mode]: Ops write handshake to Android failed");
            }
            nShakeTimes++;
#endif
        }
        if(1==g_nWriteX5X7FileFlag)
        {
            //X5 X7 Frimware status
           int  nRet = Ops_WriteX5X7Files(&g_stComDev);
            if(RET_SUCCESS!=nRet)
            {
                HHT_LOG(EN_ERR, "[Compatible Mode]: Ops write files to Android failed");
                qDebug("[Compatible Mode]: Ops write files to Android failed");
                PUB_SS.PostSendFileToAndroidFailed();//
            }
            else
            {
                HHT_LOG(EN_INFO, "[Compatible Mode]: Ops write files to Android success");
                qDebug("[Compatible Mode]: Ops write files to Android success");
            }
            g_nWriteX5X7FileFlag=-1;
        }
        //read data from com[data from x5/x7 android]
        memset(acRcvBuf,0,sizeof(acRcvBuf));
        int nRet = g_stComDev.ReadX5X7ComByHeader(acRcvBuf,sizeof(acRcvBuf));
        if(nRet>0)
        {
            HHT_LOG(EN_INFO, "===>ReadX5X7ComByHeader status:[%d]\n",nRet);
            HHT_LOG(EN_INFO, "===>Get Android data through serial port (%d)[%s]\n",nRet, Pub_ConvertHexToStr(acRcvBuf,nRet));
            if((0==memcmp(&acRcvBuf[0],"\xaa\x55",2))&&(0==memcmp(&acRcvBuf[5],"\x10\x00\x03",3))&&
               (0==memcmp(&acRcvBuf[8],&acHandShakeBuf[8],20))&&(0==memcmp(&acRcvBuf[30],"\x55\xaa",2)))
            {
                g_nX5X7HandShakeFlag =-1;
                HHT_LOG(EN_INFO, "===>Get Android Handshake return success\n");
                continue;
            }
            int length = (int)((acRcvBuf[2]&0x00FF)|(acRcvBuf[3]<<8&0xFF00));
            HHT_LOG(EN_INFO, " ++++++acRvBuf[0]:0x%02x acRvBuf[1]:0x%02x  length:[%d] acRcvBuf[ender-1]:0x%02x acRcvBuf[ender]:0x%02x \n",
                   acRcvBuf[0],acRcvBuf[1],length,acRcvBuf[length+4],acRcvBuf[length+5]);
            if((0==memcmp(&acRcvBuf[0],"\xaa\x55",2))&&(0==memcmp(&acRcvBuf[5],acCmdSnderAndroid,3))
                    &&(0==memcmp(&acRcvBuf[length+4],"\x55\xaa",2)))
            {
                //[aa 55] (32 00)  (43) (10 00 03) {30 30 34 31/A  30 30 36 33/c  30 30 36 33/c  30 30 36 35/e 30 30 37 33/s
                //30 30 37 33/s 30 30 32 30/空格  30 30 33 32/2  30 30 33 30/0  30 30 33 31/1 30 30 33 33/3 }(12 09) [55 aa]
                nDataLen = (int)((acRcvBuf[2]&0x00FF)|(acRcvBuf[3]<<8&0xFF00));
                HHT_LOG(EN_INFO, "  X5X7 sendbuf  [0x%x 0x%x]=(%d)[%s]\n",acRcvBuf[2],acRcvBuf[3],nDataLen, Pub_ConvertHexToStr(acRcvBuf,nDataLen+6));
                nAppNameLen = (nDataLen -6)/4;    //应用名称长度=(数据长度-6)/4
#if 0
                unsigned char* dealAppName = hhtHelper::data_decrypt_default(&acRcvBuf[8],(nDataLen -6));
                HHT_LOG(EN_INFO, "  X5X7 sendbuf app (%d)[%s]\n",nDataLen, dealAppName);
                memcpy(acAppName,dealAppName, nAppNameLen);

                QByteArray baAppName;
                baAppName.clear();
                for (int i=0; i<nAppNameLen; i++)
                {
                    baAppName.append(acAppName[i]);
                }
                QString strAppName = QString::fromLocal8Bit(baAppName);
                HHT_LOG(EN_INFO, "  app name (%s)", strAppName.toLocal8Bit().data());
                HHT_LOG(EN_INFO, "  app byte array name(%s)", baAppName.data());
                /*notice gui to start app*/
                PUB_SS.PostOpenAppX5X7(strAppName);//qstring
                memset(acAppName,0,sizeof(acAppName));
                if(dealAppName!=NULL)
                {
                    free(dealAppName);
                    dealAppName==NULL;
                }
#else
                QString uAppName = hhtHelper::data_decrypt(&acRcvBuf[8],(nDataLen -6));
                QByteArray uApp = uAppName.toLatin1();
                HHT_LOG(EN_INFO, "  get uApp Name (%s)", uApp.data());
                QString  openAppName = hhtHelper::fromUtf16(uAppName);
                HHT_LOG(EN_INFO, "  emit to open App Name (%s)", openAppName.toStdString().c_str());
                /*notice gui to start app*/
                PUB_SS.PostOpenAppX5X7(openAppName);    //qstring
#endif
            }
        }
        Pub_MSleep(10);
    }
}

THREADRETURN distinguish_device(void *pParam)
{
    Q_UNUSED(pParam);
#if 1
    int *handshake_return;
    VOS_INT8 acDevName[DEV_MAX_PATH_LEN] = {0,};
    handshake_return = get_handshake_status();
    qDebug("Handshake Status:[%d] [%d]\n",handshake_return[0],handshake_return[1]);
    HHT_LOG(EN_INFO,"Handshake Status:[%d] [%d]\n",handshake_return[0],handshake_return[1]);
    switch (handshake_return[0])
    {
    case -1:// NOT X5/X7 X9
    {
        g_stComportInfo.nComNo = 1;
        g_stComportInfo.nBaudRate =115200;
        COM_GetComPortNameFromIndex(g_stComportInfo.nComNo, acDevName);
        if (0 != g_stComDev.IsOpenCom())
        {
            g_stComDev.CloseCom();
            Pub_MSleep(COM_CMD_DELAY_TIME/2);
        }
        int nRet = g_stComDev.OpenCom(acDevName, g_stComportInfo.nBaudRate);
        if(nRet ==RET_SUCCESS)
        {
            PUB_SS.PostOpenComSuccess();
            qDebug("NOT X5/X7 X9 SELECT COM[%d](%s)",g_stComportInfo.nBaudRate ,acDevName);
            HHT_LOG(EN_INFO,"NOT X5/X7 X9 SELECT COM[%d](%s)",g_stComportInfo.nBaudRate ,acDevName);
            Pub_MSleep(COM_CMD_DELAY_TIME/2);
        }
        else
        {
            PUB_SS.PostOpenComFailed();
            Pub_MSleep(COM_CMD_DELAY_TIME/2);
        }
        vos_pthread_t opsThreadID;
        nRet = VOS_CreateThread(&opsThreadID, X5X7OpsCom_ThreadFn, NULL);
        if (RET_SUCCESS != nRet)
        {
            HHT_LOG(EN_ERR, "+++++++++++create Compatible Mode ops_com_thread failed++++++++++");
            qDebug("+++++++++++create Compatible Mode ops_com_thread failed++++++++++");
        }
        else
        {
            HHT_LOG(EN_ERR, "++++++++++create Compatible Mode ops_com_thread success++++++++++");
            qDebug("++++++++++create Compatible Mode ops_com_thread success+++++++++++");
        }
    }
        break;
    case 0://GET X9
    {
        PUB_SS.PostOpenComSuccess();
        Pub_MSleep(COM_CMD_DELAY_TIME/2);
        PUB_SS.PostOpenComSuccess();
        vos_pthread_t opsThreadID;
        int nRet = VOS_CreateThread(&opsThreadID, OpsCom_ThreadFn, NULL);
        if (RET_SUCCESS != nRet)
        {
            HHT_LOG(EN_ERR, "++++++++++++++create x9 ops_com_thread failed++++++++++++++");
            qDebug("++++++++++++++create x9 ops_com_thread failed++++++++++++++");
        }
        else
        {
            HHT_LOG(EN_ERR, "+++++++++++++++create x9 ops_com_thread success+++++++++++++++");
            qDebug("+++++++++++++++create x9 ops_com_thread success+++++++++++++++");
        }
    }
        break;
    case 1://GET X5/X7
    {
        PUB_SS.PostOpenComSuccess();
        Pub_MSleep(COM_CMD_DELAY_TIME/2);
        vos_pthread_t opsThreadID;
        PUB_SS.PostOpenComSuccess();
        int nRet = VOS_CreateThread(&opsThreadID, X5X7OpsCom_ThreadFn, NULL);
        if (RET_SUCCESS != nRet)
        {
            HHT_LOG(EN_ERR, "+++++++++++create Compatible Mode ops_com_thread failed++++++++++");
            qDebug("+++++++++++create Compatible Mode ops_com_thread failed++++++++++");
        }
        else
        {
            HHT_LOG(EN_ERR, "++++++++++create Compatible Mode ops_com_thread success++++++++++");
            qDebug("++++++++++create Compatible Mode ops_com_thread success+++++++++++");
        }
    }
        break;
    default://GET X5/X7
    {
        g_stComportInfo.nComNo = 2;
        g_stComportInfo.nBaudRate =115200;
        COM_GetComPortNameFromIndex(g_stComportInfo.nComNo, acDevName);
        if (0 != g_stComDev.IsOpenCom())
        {
            g_stComDev.CloseCom();
            Pub_MSleep(COM_CMD_DELAY_TIME/2);
        }
        int nRet = g_stComDev.OpenCom(acDevName, g_stComportInfo.nBaudRate);
        if(nRet ==RET_SUCCESS)
        {
            PUB_SS.PostOpenComSuccess();
            qDebug("DEFAULT SELECT COM[%d](%s)",g_stComportInfo.nBaudRate ,acDevName);
            HHT_LOG(EN_INFO,"DEFAULT X5/X7 X9 SELECT COM[%d](%s)",g_stComportInfo.nBaudRate ,acDevName);
            Pub_MSleep(COM_CMD_DELAY_TIME/2);
        }
        else
        {
            PUB_SS.PostOpenComFailed();
            Pub_MSleep(COM_CMD_DELAY_TIME/2);
        }
        vos_pthread_t opsThreadID;
        nRet = VOS_CreateThread(&opsThreadID, X5X7OpsCom_ThreadFn, NULL);
        if (RET_SUCCESS != nRet)
        {
            HHT_LOG(EN_ERR, "+++++++++++create Compatible Mode ops_com_thread failed++++++++++");
            qDebug("+++++++++++create Compatible Mode ops_com_thread failed++++++++++");
        }
        else
        {
            HHT_LOG(EN_ERR, "++++++++++create Compatible Mode ops_com_thread success++++++++++");
            qDebug("++++++++++create Compatible Mode ops_com_thread success+++++++++++");
        }
    }
        break;
    }
    if(handshake_return!=NULL)//动态分配内存,使用完后立即释放
    {
        free(handshake_return);
        handshake_return =NULL;
    }
#else
    INT32 nRet = -1;
    INT32 nRetrytimes = 0;
    VOS_INT8 acDevName[DEV_MAX_PATH_LEN] = {0,};
    UINT8 acCmdSign[6] = {0x99, 0xA2, 0xB3, 0xC4, 0x02, 0xFF};    //命令标识
    UINT8 acRcvBuf[COM_DATA_PKG_LEN] = {0,};
    //
    memset(acDevName, 0, sizeof(acDevName));
    nRet = COM_GetComPortNameFromIndex(g_stComportInfo.nComNo, acDevName);
    if (RET_SUCCESS != nRet)
    {
        HHT_LOG(EN_ERR, "Get comport(%d) name failed", g_stComportInfo.nComNo);
        return (THREADRETURN)1;
    }
    if (0 != g_stComDev.IsOpenCom())
    {
        g_stComDev.CloseCom();
        Pub_MSleep(COM_CMD_DELAY_TIME);
    }
    //
    for (;;)
    {
        if(1==g_nIsAppQuit)
        {
            HHT_LOG(EN_INFO," Exit Ops com Thread");
            break;
        }
        nRet = g_stComDev.OpenCom(acDevName, g_stComportInfo.nBaudRate);
        if (RET_SUCCESS != nRet)
        {
            HHT_LOG(EN_ERR, "Open com (%s) failed", acDevName);
            PUB_SS.PostOpenComFailed();
            Pub_MSleep(1);
            continue;
        }
        else
        {
            PUB_SS.PostOpenComSuccess();
            break;
        }
    }
    HHT_LOG(EN_ERR, "Open com (%s) [%d] success", acDevName,g_stComportInfo.nBaudRate);
    while (1)
    {
        //X9 firmware check commands
#if 1
        if(g_nX9FirmwareCheckFlag ==1)
        {
            UINT8 mSndBuf[13]= {0x7F,0x09,0x99,0xA2,0xB3,0xC4,0x02,0xFF,0x93,0xA0,0xEF,0xCF};
            int Ret= g_stComDev.WriteCom(mSndBuf,13);
            if(Ret==0)
            {
                continue;
                HHT_LOG(EN_INFO, "[%d]Write request check x9 firmware status commands failed.",nRetrytimes+1);
                qDebug("[%d]Write request check x9 firmware status commands failed.",nRetrytimes+1);
            }
            else
            {
                HHT_LOG(EN_INFO, "[%d]Write request check x9 firmware status commands success.",nRetrytimes+1);
                qDebug("[%d]Write request check x9 firmware status commands success.",nRetrytimes+1);
            }
            Pub_MSleep(200);
        }
#endif
        //
        memset(acRcvBuf, 0, sizeof(acRcvBuf));
        nRet = g_stComDev.ReadComByHeader(acRcvBuf,COM_DATA_PKG_LEN);//read commands
        if (0 >= nRet)
        {
            nRetrytimes++;
            if(nRetrytimes>=10)//超过10次还没有收到消息
                break;
            Pub_MSleep(10);
            continue;
        }

        HHT_LOG(EN_INFO, " ==> distinguish get data(%s)", Pub_ConvertHexToStr(acRcvBuf, COM_DATA_PKG_LEN/4));
        qDebug(" ==> distinguish get data(%s)", Pub_ConvertHexToStr(acRcvBuf, COM_DATA_PKG_LEN/4));

        //接受来自Android的X9 firmware响应
        if((COM_HEADER ==acRcvBuf[0])&&(0 == memcmp(&acRcvBuf[2], acCmdSign, 6))
                &&(0x93 == acRcvBuf[8]) && (0xA1 == acRcvBuf[9])&&(0x00 == acRcvBuf[10]))
        {
            HHT_LOG(EN_INFO, "Received X9 Firmware check echo from Android.");
            PUB_SS.PostX9FirmwareCheck();//发送X9固件确认信号
            g_nX9FirmwareCheckFlag =-1;
            break;
        }
    }//while(1)

    HHT_LOG(EN_INFO,"+++++++++++++++++++select thread to run+++++++++++++++++++++++");
    qDebug()<<"+++++++++++++++++++select thread to run+++++++++++++++++++++++";
    HHT_LOG(EN_INFO,"status:[%d],flags[%d]",g_nX9FirmwareCheckStatus,g_nX9FirmwareCheckFlag);
    qDebug("=======>status:[%d],flags[%d]",g_nX9FirmwareCheckStatus,g_nX9FirmwareCheckFlag);
    if(g_nX9FirmwareCheckStatus||(g_nX9FirmwareCheckFlag==-1))//X9
    {
        //X9/X8/X6
        //TODO thread
        vos_pthread_t opsThreadID;
        nRet = VOS_CreateThread(&opsThreadID, OpsCom_ThreadFn, NULL);
        if (RET_SUCCESS != nRet)
        {
            HHT_LOG(EN_ERR, "++++++++++++++create x9 ops_com_thread failed++++++++++++++");
            qDebug("++++++++++++++create x9 ops_com_thread failed++++++++++++++");
        }
        else
        {
            HHT_LOG(EN_ERR, "+++++++++++++++create x9 ops_com_thread success+++++++++++++++");
            qDebug("+++++++++++++++create x9 ops_com_thread success+++++++++++++++");
        }
    }
    else
    {
        //X5/X7
        //TODO thread
        //关闭X9配置文件串口,打开X5/X7通讯串口设备
        if(g_stComDev.IsOpenCom())
        {
            int result = g_stComDev.CloseCom();
            if(RET_SUCCESS==result)
            {
                HHT_LOG(EN_ERR, "Select Compatible Mode device,Try to close X9 serial device.");
                qDebug("Select Compatible Mode device,Try to close X9 serial device.");
                g_stComportInfo.nComNo = 2;
                g_stComportInfo.nBaudRate = 115200;
                Pub_MSleep(COM_CMD_DELAY_TIME);

                memset(acDevName, 0, sizeof(acDevName));
                nRet = COM_GetComPortNameFromIndex(g_stComportInfo.nComNo, acDevName);
                if (RET_SUCCESS != nRet)
                {
                    HHT_LOG(EN_ERR, "X5/X7 Get comport(%d) name failed", g_stComportInfo.nComNo);
                    return (THREADRETURN)1;
                }
                if (0 != g_stComDev.IsOpenCom())
                {
                    g_stComDev.CloseCom();
                    Pub_MSleep(COM_CMD_DELAY_TIME);
                }
                int  error_count=0;
                for (;;)
                {
                    if(1==g_nIsAppQuit)
                    {
                        HHT_LOG(EN_INFO," Exit Ops com Thread");
                        break;
                    }
                    nRet = g_stComDev.OpenCom(acDevName, g_stComportInfo.nBaudRate);
                    if (RET_SUCCESS != nRet)
                    {
                        PUB_SS.PostOpenComFailed();
                        error_count++;
                        if(error_count<10)
                        {
                            HHT_LOG(EN_ERR, "Compatible Mode Open com (%s) [%d] failed", acDevName,g_stComportInfo.nBaudRate);
                            qDebug("Compatible Mode Open com (%s) [%d] failed", acDevName,g_stComportInfo.nBaudRate);
                        }
                        if(error_count==10)
                        {
                            HHT_LOG(EN_ERR, "+++++++++I am ganna fucking terminate to record errors.++++++++");
                            qDebug("+++++++++I am ganna fucking terminate to record errors.+++++++++");
                        }
                        Pub_MSleep(1);
                        continue;
                    }
                    else
                    {
                        PUB_SS.PostOpenComSuccess();
                        break;
                    }
                }
                HHT_LOG(EN_ERR, "Compatible Mode Open com (%s) [%d] success", acDevName,g_stComportInfo.nBaudRate);
                qDebug("Compatible Mode Open com (%s)[%d] success", acDevName,g_stComportInfo.nBaudRate);
            }
        }
        //===================================================================================
        vos_pthread_t opsThreadID;
        nRet = VOS_CreateThread(&opsThreadID, X5X7OpsCom_ThreadFn, NULL);
        if (RET_SUCCESS != nRet)
        {
            HHT_LOG(EN_ERR, "+++++++++++create Compatible Mode ops_com_thread failed++++++++++");
            qDebug("+++++++++++create Compatible Mode ops_com_thread failed++++++++++");
        }
        else
        {
            HHT_LOG(EN_ERR, "++++++++++create Compatible Mode ops_com_thread success++++++++++");
            qDebug("++++++++++create Compatible Mode ops_com_thread success+++++++++++");
        }
    }

#endif
    return 0;
}
/**
 * @brief get_handshake_status
 * @return
 * handshake_return[0]
 * 握手状态 [0]:X9   [1]:X5/X7   [-1]:ERROR
 * handshake_return[1]
 * 串口index
 */
int* get_handshake_status()
{
    //握手状态 [0]:X9 [1]:X5/X7 [-1]:ERROR
    static int handshake_status =-1;
    static int com_index = -1; //串口序列号
    //动态分配内存,使用完后强制释放
    int  *handshake_return=(int*)malloc(2*sizeof(int)); //handshake_return[0]: 握手状态  handshake_return[1]:串口序列号
    memset(handshake_return,-1,sizeof(handshake_return));
    INT32 nRetrytimes = 0;
    INT32 nRet = -1;
    VOS_INT8 acDevName[DEV_MAX_PATH_LEN] = {0,};
    UINT8 acCmdSign[6] = {0x99, 0xA2, 0xB3, 0xC4, 0x02, 0xFF};    //命令标识
    UINT8 acRcvBuf[COM_DATA_PKG_LEN] = {0,};
    UINT8 acRcvBuf2[256] = {0,};
    //UINT8 acCmdSnderOps[3] = {0x02, 0x00, 0x03};            //发送者标识 发送给Android
    //UINT8 acCmdSnderAndroid[3] = {0x10, 0x00, 0x03};      //发送者标识  发送给OPS
    UINT8 acHandShakeBuf[]={0xaa,0x55,0x1a,0x00,0x4a,0x10,0x00,0x03,0x30,0x30,0x36,0x38 ,0x30,0x30,
                            0x36,0x35,0x30,0x30,0x36,0x31,0x30,0x30,0x37,0x32,0x30,0x30,0x37,0x34 ,0x6b,0x04,0x55,0xaa };//类似x5x7握手指令Android->OPS
    UINT8 acHandShakeSndBuf[]={0xaa,0x55,0x1a,0x00,0x00,0x02,0x00,0x03,0x30,0x30,0x36,0x38,0x30,0x30,
                               0x36,0x35,0x30,0x30,0x36,0x31,0x30,0x30,0x37,0x32,0x30,0x30,0x37,0x34 ,0x13,0x04,0x55,0xaa};//类似x5x7握手指令OPS->Android
    //获取系统所有串口设备
    QStringList com_list = hhtHelper::get_local_serial_devices();
    for(int i=0;i<com_list.size();i++)
    {
        memset(acDevName, 0, sizeof(acDevName));
        QString index_string=com_list.at(i).mid(com_list.at(i).length()-1,1);
        int index = index_string.toInt(0,10);
        qDebug()<<"==>INDEX["<<index<<"]: "<<"SERIAL DEVICE ("<<com_list.at(i)<<")";
        QString device = com_list.at(i);
        HHT_LOG(EN_INFO,"==>INDEX[%d]: SERIAL DEVICE (%s)",index,device.toStdString().c_str());
        g_stComportInfo.nComNo = index;
        g_stComportInfo.nBaudRate =115200;
        nRet = COM_GetComPortNameFromIndex(g_stComportInfo.nComNo, acDevName);
        if (RET_SUCCESS != nRet)
        {
            HHT_LOG(EN_ERR, "Get comport(%d) name failed", g_stComportInfo.nComNo);
            continue;
        }
        else//GET设备成功
        {
            if (0 != g_stComDev.IsOpenCom())
            {
                g_stComDev.CloseCom();
                Pub_MSleep(COM_CMD_DELAY_TIME);
            }
            nRet = g_stComDev.OpenCom(acDevName, g_stComportInfo.nBaudRate);
            if(nRet ==RET_SUCCESS)//每一个串口发送两套握手指令
            {
                //TODO X9设备握手
                HHT_LOG(EN_ERR, "==>Open com (%s) [%d] success", acDevName,g_stComportInfo.nBaudRate);
                while (1)
                {
                    //X9 firmware check commands
                    if(g_nX9FirmwareCheckFlag ==1)
                    {
                        UINT8 mSndBuf[13]= {0x7F,0x09,0x99,0xA2,0xB3,0xC4,0x02,0xFF,0x93,0xA0,0xEF,0xCF};
                        int Ret= g_stComDev.WriteCom(mSndBuf,13);
                        if(Ret==0)
                        {
                            continue;
                            HHT_LOG(EN_INFO, "[%d]Write request check x9 firmware status commands failed.",nRetrytimes+1);
                            qDebug("[%d]Write request check x9 firmware status commands failed.",nRetrytimes+1);
                        }
                        else
                        {
                            HHT_LOG(EN_INFO, "[%d]Write request check x9 firmware status commands success.",nRetrytimes+1);
                            qDebug("[%d]Write request check x9 firmware status commands success.",nRetrytimes+1);
                        }
                        Pub_MSleep(100);
                    }
                    //
                    memset(acRcvBuf, 0, sizeof(acRcvBuf));
                    nRet = g_stComDev.ReadComByHeader(acRcvBuf,COM_DATA_PKG_LEN);//read commands
                    if (0 >= nRet)
                    {
                        nRetrytimes++;
                        if(nRetrytimes>=5)//超过5次还没有收到消息
                            break;
                        Pub_MSleep(10);
                        continue;
                    }

                    HHT_LOG(EN_INFO, " ==> handshake_status get data(%s)", Pub_ConvertHexToStr(acRcvBuf, COM_DATA_PKG_LEN/4));
                    qDebug(" ==> handshake_status get data(%s)", Pub_ConvertHexToStr(acRcvBuf, COM_DATA_PKG_LEN/4));

                    //接受来自Android的X9 firmware响应
                    if((COM_HEADER ==acRcvBuf[0])&&(0 == memcmp(&acRcvBuf[2], acCmdSign, 6))
                            &&(0x93 == acRcvBuf[8]) && (0xA1 == acRcvBuf[9])&&(0x00 == acRcvBuf[10]))
                    {
                        HHT_LOG(EN_INFO, "Received X9 Firmware check echo from Android.");
                        PUB_SS.PostX9FirmwareCheck();//发送X9固件确认信号
                        g_nX9FirmwareCheckFlag =-1;
                        break;
                    }
                }//while(1) //X9 check
                if(g_nX9FirmwareCheckStatus||(g_nX9FirmwareCheckFlag==-1))//X9
                {
                    handshake_status = 0; //X9
                    com_index = g_stComportInfo.nComNo; //serial device
                    handshake_return[0]=handshake_status;
                    handshake_return[1]=com_index;
                    qDebug("X9+++Handshake Status:[%d] [%d]",handshake_return[0],handshake_return[1]);
                    HHT_LOG(EN_INFO,"X9+++Handshake Status:[%d] [%d]",handshake_return[0],handshake_return[1]);
                    return handshake_return;
                }

                nRetrytimes=0;//清零
                //TODO X5/X7设备握手
                while (1)
                {
                    if(1==g_nX5X7HandShakeFlag)
                    {
                        //0x68 0x65 0x61 0x72 0x74 心跳包指令->heart
                        nRet = g_stComDev.WriteCom(acHandShakeSndBuf,sizeof(acHandShakeSndBuf));
                        if(nRet<=0)
                        {
                            continue;
                            HHT_LOG(EN_INFO, "[Compatible Mode]: Ops write handshake to Android failed[%d].",nRetrytimes+1);
                            qDebug("[Compatible Mode]: Ops write handshake to Android failed[%d].",nRetrytimes+1);
                        }
                        else
                        {
                            HHT_LOG(EN_INFO, "[Compatible Mode]: Ops write handshake to Android failed[%d].",nRetrytimes+1);
                            qDebug("[Compatible Mode]: Ops write handshake to Android failed[%d].",nRetrytimes+1);
                        }
                        Pub_MSleep(100);
                    }
                    //read data from com[data from x5/x7 android]
                    memset(acRcvBuf2,0,sizeof(acRcvBuf2));
                    int nRet = g_stComDev.ReadX5X7ComByHeader(acRcvBuf2,sizeof(acRcvBuf2));
                    if(nRet>0)
                    {
                        HHT_LOG(EN_INFO, "===>ReadX5X7ComByHeader status:[%d]\n",nRet);
                        HHT_LOG(EN_INFO, "===>Get Android data through serial port (%d)[%s]\n",nRet, Pub_ConvertHexToStr(acRcvBuf2,nRet));
                        if((0==memcmp(&acRcvBuf2[0],"\xaa\x55",2))&&(0==memcmp(&acRcvBuf2[5],"\x10\x00\x03",3))&&
                                (0==memcmp(&acRcvBuf2[8],&acHandShakeBuf[8],20))&&(0==memcmp(&acRcvBuf2[30],"\x55\xaa",2)))
                        {
                            g_nX5X7HandShakeFlag =-1;
                            HHT_LOG(EN_INFO, "===>Get Android Handshake return success\n");
                            break;
                        }
                    }
                    else
                    {
                        nRetrytimes++;
                        if(nRetrytimes>=5)//超过5次还没有收到消息
                            break;
                        Pub_MSleep(10);
                        continue;
                    }
                    Pub_MSleep(20);
                }//while(1) //X5/X7 check
                if(g_nX5X7HandShakeFlag ==-1)
                {
                    handshake_status = 1; //X5/X7
                    com_index = g_stComportInfo.nComNo; //serial device
                    handshake_return[0]=handshake_status;
                    handshake_return[1]=com_index;
                    qDebug("X5/X7+++Handshake Status:[%d] [%d]",handshake_return[0],handshake_return[1]);
                    HHT_LOG(EN_INFO,"X5/X7+++Handshake Status:[%d] [%d]",handshake_return[0],handshake_return[1]);
                    return handshake_return;
                }
            }
            else
            {
                HHT_LOG(EN_ERR, "==>Open comport(%d)[%s] failed", g_stComportInfo.nComNo,g_stComportInfo.nBaudRate);
                g_stComDev.CloseCom();
                Pub_MSleep(COM_CMD_DELAY_TIME/2);
                continue;
            }
            g_stComDev.CloseCom();
            Pub_MSleep(COM_CMD_DELAY_TIME/2);
        }
    }
    handshake_return[0]=-1;
    handshake_return[1]=-1;
    qDebug("ERROR+++Handshake Status:[%d] [%d]",handshake_return[0],handshake_return[1]);
    HHT_LOG(EN_INFO,"ERROR+++Handshake Status:[%d] [%d]",handshake_return[0],handshake_return[1]);
    qDebug()<<"++++++++++++++++++FUCK YOU ALL+++++++++++++++++";
    return  handshake_return;
}
