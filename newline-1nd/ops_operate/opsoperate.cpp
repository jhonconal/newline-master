#include "opsoperate.h"
#include "comOperate.h"
#include "ss_pub.h"

int g_nWriteStatus = -1;
int g_nWriteFileFlag = 0;
int g_nCameraCommandsFlag=0;
int g_nFileNum = 0;
int g_nTotalPkgNum = 0;
int g_nCurrentPkgNum = 0;
//QList<unsigned char> g_listFileData;
QList<QList<unsigned char>> g_listFilesName;
QList<QList<unsigned char>> g_listAllFilesData;

/***********************************************************
Function     :   OpsCom_ThreadFn
Description  :   OPS���ڿ������߳�
Input        :   pParm �̲߳���
Output       :   ��
Return       :   ��
Others       :   ��
Author       :
History      :   1. ��������2017.05.23
***********************************************************/

THREADRETURN OpsCom_ThreadFn(void *pParam)
{
    INT32 nRet = -1;
    INT32 i = 0;
    INT32 nDataLen = 0;    
    INT32 nRetrytimes = 0;
    INT32 nAppNameLen = 0;  //Ҫ�򿪵�Ӧ�����Ƴ���    
    const INT32 nWriteMaxCount = 3;
    VOS_INT8 acDevName[DEV_MAX_PATH_LEN] = {0,};    
    UINT8 ucChecksum = 0;
    UINT8 acCmdSign[6] = {0x99, 0xA2, 0xB3, 0xC4, 0x02, 0xFF};    //�����ʶ
    //������ͷָ��: 7F YY 99 A2 B3 C4 02 FF 80 A0 CHK CF   7F YY 99 A2 B3 C4 02 FF 80 A1 ZZ CHK CF
    //������ͷָ��: 7F YY 99 A2 B3 C4 02 FF 81 A0 CHK CF   7F YY 99 A2 B3 C4 02 FF 81 A1 ZZ CHK CF
    UINT8 acCameraUpSign[2]={0x80,0xA0}; //������ͷ����
    UINT8 acCameraDownSign[2]={0x81,0xA0};//������ͷ����
    UINT8 acRecvCameraSign[2]={0,};//����ͷ����
    //ZOOM INָ��: 7F YY 99 A2 B3 C4 02 FF 60 A0 CHK CF   7F YY 99 A2 B3 C4 02 FF 60 A1 ZZ CHK CF
    //ZOOM OUTָ��: 7F YY 99 A2 B3 C4 02 FF 61 A0 CHK CF   7F YY 99 A2 B3 C4 02 FF 61 A1 ZZ CHK CF
    UINT8 acZoomInSign[2]={0x60,0xA0};//Zoom in����
    UINT8 acZoomOutSign[2]={0x61,0xA0};//Zoom out����
    //PgUpָ��: 7F YY 99 A2 B3 C4 02 FF 70 A0 CHK CF   7F YY 99 A2 B3 C4 02 FF 70 A1 ZZ CHK CF
    //PgDnָ��: 7F YY 99 A2 B3 C4 02 FF 71 A0 CHK CF   7F YY 99 A2 B3 C4 02 FF 71 A1 ZZ CHK CF
    UINT8 acPgUpSign[2]={0x70,0xA0};//PgUp����
    UINT8 acPgDnSign[2]={0x71,0xA0};//PgDn����


    //UINT8 acSndBuf[COM_DATA_PKG_LEN] = {0,};
    UINT8 acRcvBuf[COM_DATA_PKG_LEN] = {0,};
    UINT8 acAppName[COM_DATA_PKG_LEN] = {0,};
    UINT8 acSndBuf[COM_DATA_PKG_LEN] = {0,};
    Q_UNUSED(pParam);

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
        if(1==g_nIsAppQuit){
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

//    PUB_SS.PostCameraStatusCheck();//��������ͷ����

    for (;;)
    {
        if (1 == g_nIsAppQuit)
        {
            HHT_LOG(EN_INFO, "  Exit Ops com Thread");
            break;
        }

        // --------------------------------------------------------------------------------------------------------
        //        ����򴮿�д������ͷ״ָ̬��
        if(g_nCameraCommandsFlag==1)
        {//����״̬
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
        {//������״̬
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
                HHT_LOG(EN_INFO, "Ops write files to Android success");
            }
            g_nWriteFileFlag = 0;
        }

        memset(acRcvBuf, 0, sizeof(acRcvBuf));
//        nRet = g_stComDev.ReadCom(acRcvBuf, COM_DATA_PKG_LEN);
        nRet = g_stComDev.ReadComByHeader(acRcvBuf,COM_DATA_PKG_LEN);//read commands
        if (0 >= nRet)
        {
            Pub_MSleep(10);
            continue;
        }
        HHT_LOG(EN_INFO, "  get data(%s)", Pub_ConvertHexToStr(acRcvBuf, COM_DATA_PKG_LEN));
        if ((COM_HEADER == acRcvBuf[0]) && (0 == memcmp(&acRcvBuf[2], acCmdSign, 6))
                &&(0xA1 == acRcvBuf[8]) && (0xA0 == acRcvBuf[9]))   //��׿�´�windowsϵͳӦ��ָ��
        {
            nDataLen = acRcvBuf[1];
            nAppNameLen = nDataLen -6-2-1;    //Ӧ�����Ƴ���=���ݳ���-�����ʶ����-�����볤��-У��ͳ���
            memcpy(acAppName, &acRcvBuf[10], nAppNameLen);

            QByteArray baAppName;
            baAppName.clear();
            for (i=0; i<nAppNameLen; i++)
            {
                baAppName.append(acAppName[i]);
            }
            QString strAppName = QString::fromLocal8Bit(baAppName);
            HHT_LOG(EN_INFO, "  app name (%s)", strAppName.toStdString().c_str());
            /*notice gui to start app*/
            PUB_SS.PostOpenApp(strAppName);

            nRetrytimes = 0;
            while (nWriteMaxCount > nRetrytimes)
            {
                memset(acSndBuf, 0, sizeof(acSndBuf));
                acSndBuf[0] = COM_HEADER;
                acSndBuf[1] = acRcvBuf[1]+1;    //���ݳ���
                memcpy(&acSndBuf[2], acCmdSign, 6);
                acSndBuf[8] = 0xA1;
                acSndBuf[9] = 0xA1;
                acSndBuf[10] = 0x00;    //״̬ZZ
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
        //----------------------------------------------------------------------------------------------------------
        // ��Ӷ�ȡ�ײ��������ͷָ����Ϣ
        //upper 7F YY 99 A2 B3 C4 02 FF 80 A0 CHK CF   7F YY 99 A2 B3 C4 02 FF 80 A1 ZZ CHK CF
        //lower 7F YY 99 A2 B3 C4 02 FF 81 A0 CHK CF   7F YY 99 A2 B3 C4 02 FF 81 A1 ZZ CHK CF
//        HHT_LOG(EN_INFO, "  Receive Cemera Commands Data(%s)", Pub_ConvertHexToStr(acRcvBuf, COM_DATA_PKG_LEN));
        if(((COM_HEADER ==acRcvBuf[0])&&(0==memcmp(&acRcvBuf[2],acCmdSign,6))
            &&(COM_ENDER==acRcvBuf[11]))&&((0==memcmp(&acRcvBuf[8],acCameraUpSign,2))
                                           ||(0==memcmp(&acRcvBuf[8],acCameraDownSign,2)))){
            if(0==memcmp(&acRcvBuf[8],acCameraUpSign,2))
            {
                    memcpy(&acRecvCameraSign[0],acCameraUpSign,2);
            }
            else if (0==memcmp(&acRcvBuf[8],acCameraDownSign,2))
            {
                    memcpy(&acRecvCameraSign[0],acCameraDownSign,2);
            }
            PUB_SS.PostCameraStatusCheck();
            qDebug()<<"Receive calling camera commands success.";
            HHT_LOG(EN_INFO, "Receive calling camera commands success.");
        }
        else
        {
            qDebug()<<"Receive calling camera commands failed.";
            HHT_LOG(EN_INFO, "Receive calling camera commands failed.");
        }
        //  -----------------------------------------------------------------------------------------------------------
        //�������ң��������ָ��
        if ((COM_HEADER == acRcvBuf[0]) && (0 == memcmp(&acRcvBuf[2], acCmdSign, 6))
                &&(0==memcmp(&acRcvBuf[8],acPgUpSign,2)))
        {
                HHT_LOG(EN_INFO, "Receive PgUp commands success.");
                keybd_event(VK_PRIOR,0,0,0);//����ϵͳpgUp�������ֵ
                Sleep(100);
                keybd_event(VK_PRIOR,0,KEYEVENTF_KEYUP,0);
        }
        else if ((COM_HEADER == acRcvBuf[0]) && (0 == memcmp(&acRcvBuf[2], acCmdSign, 6))
                &&(0==memcmp(&acRcvBuf[8],acPgDnSign,2)))
        {
                HHT_LOG(EN_INFO, "ReceivePgDn commands success.");
                keybd_event(VK_NEXT,0,0,0);//����ϵͳpgDn�������ֵ
                Sleep(100);
                keybd_event(VK_NEXT,0,KEYEVENTF_KEYUP,0);
        }
        if ((COM_HEADER == acRcvBuf[0]) && (0 == memcmp(&acRcvBuf[2], acCmdSign, 6))
                &&(0==memcmp(&acRcvBuf[8],acZoomInSign,2)))
        {
                HHT_LOG(EN_INFO, "Receive ZoomIn commands success.");
                keybd_event(VK_CONTROL,0,0,0);
                keybd_event(VK_OEM_MINUS,0,0,1<<29);//Ctrl -
                Sleep(100);
                keybd_event(VK_OEM_MINUS,0,KEYEVENTF_KEYUP,0);
                keybd_event(VK_CONTROL,0,KEYEVENTF_KEYUP,0);
        }
        else if ((COM_HEADER == acRcvBuf[0]) && (0 == memcmp(&acRcvBuf[2], acCmdSign, 6))
                 &&(0==memcmp(&acRcvBuf[8],acZoomOutSign,2)))
        {
                HHT_LOG(EN_INFO, "Receive ZoomOut commands success.");
                keybd_event(VK_CONTROL,0,0,0);
                keybd_event(VK_OEM_PLUS,0,0,1<<29);//Ctrl +
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
Description  :   �˳�OPS���ڿ������߳�
Input        :   ��
Output       :   ��
Return       :   ��
Others       :   ��
Author       :
History      :   1. ��������2017.05.24
***********************************************************/

void OPS_QuitThread()
{
    g_nIsAppQuit = 1;
}

/***********************************************************
Function     :   OPS_SendFilesToAndroid
Description  :   OPS�����ļ���Android�ӿڹ�GUI����
Input        :  nFileNum �ļ���, listAllFilesNameҪд����ļ����б�, listAllFilesData Ҫд����ļ������б�
Output       :   ��
Return       :   ��
Others       :   ��
Author       :
History      :   1. ��������2017.05.23
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
Description  :   OPS�����ļ���Android����ʵ�ֽӿ�
Input        :   pstComDev �����豸
Output       :   ��
Return       :   0 success, others failure
Others       :   ��
Author       :
History      :   1. ��������2017.05.23
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
    INT32 nLastPkgDataNum = 0;  //����һ�����ݰ��ĳ��ȿ���Ϊ����(50)Ҳ���ܲ���
    const INT32 nWriteMaxCount = 3;
    UINT8 ucChecksum = 0;
    UINT8 acSndBuf[COM_DATA_PKG_LEN] = {0,};
    //UINT8 acRcvBuf[COM_DATA_PKG_LEN] = {0,};
    UINT8 acCmdSign[6] = {0x99, 0xA2, 0xB3, 0xC4, 0x02, 0xFF};    //�����ʶ

    if (NULL == pstComDev)
    {
        HHT_LOG(EN_INFO, "======Com dev not found===== ");
        return RET_INVALID;
    }
    /*OPS��Android�����ļ��б�ʼ*/
    acSndBuf[0] = COM_HEADER;
    acSndBuf[1] = 0x0B; //YY���ݳ���
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
//    HHT_LOG(EN_INFO, " -----------OPS SEND FILE NAME INFO-----------");
    g_nCurrentPkgNum = 0;
    for (i=0; i<g_nFileNum; i++)
    {
//         HHT_LOG(EN_INFO, "  OPS START SEND TO ANDROID--> (%d)",i);
        /*OPS��Android�����ļ����ҿ�ʼ�����ļ�*/
        memset(acSndBuf, 0, sizeof(acSndBuf));
        acSndBuf[0] = COM_HEADER;
        nDataLen = 6 + 2 + 2 + g_listFilesName[i].size() + 1; //YY���ݳ���(�����ʶ+������+�ܰ���+�ļ���+У��λ)
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
//              HHT_LOG(EN_INFO, "  START WRITE FILE COMMAND (%s)", Pub_ConvertHexToStr(acSndBuf, nDataLen+3));
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
        // HHT_LOG(EN_INFO, " --------------OPS SEND FILE CONTANT INFO-------------");
        /*OPS��Android�����ļ�����*/
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
                nDataLen = 6 + 2 + 2 + COM_VALID_DATA_LEN + 1; //YY���ݳ���(�����ʶ+������+������+�ð���Ч����+У��λ)
                for (k=0; k<COM_VALID_DATA_LEN; k++)
                {
                    acSndBuf[12+k] = listFileData[k+j*COM_VALID_DATA_LEN];
                }
            }
            else
            {
                nDataLen = 6 + 2 + 2 + nLastPkgDataNum + 1; //YY���ݳ���(�����ʶ+������+������+�ð���Ч����+У��λ)
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
//        HHT_LOG(EN_INFO, " --------OPS SEND FILE NAME INFO END--------");
        /*OPS��Android�����ļ�����ֹͣ�����ļ�*/
        memset(acSndBuf, 0, sizeof(acSndBuf));
        acSndBuf[0] = COM_HEADER;
        nDataLen = 6 + 2 + 2 + g_listFilesName[i].size() + 1; //YY���ݳ���(�����ʶ+������+�ܰ���+�ļ���+У��λ)
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
// HHT_LOG(EN_INFO, " --------OPS SEND FILE INFO LIST END--------");
    /*OPS��Android�����ļ��б����*/
    acSndBuf[0] = COM_HEADER;
    acSndBuf[1] = 0x0B; //YY���ݳ���
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
//    HHT_LOG(EN_INFO, "  STOP------------>1 (%d)",nRet );
    if (RET_SUCCESS != nRet)
    {
        g_nWriteStatus = 5;
        HHT_LOG(EN_INFO, "======Write status(%d)===== ",g_nWriteStatus);
        return g_nWriteStatus;
    }
//    HHT_LOG(EN_INFO, "  STOP------------>2 (%d)",nRet );
    g_nWriteStatus = 0; //success
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
