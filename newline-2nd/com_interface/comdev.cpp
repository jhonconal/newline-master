#include <QDebug>
#include "comdev.h"
#include "opsoperate.h"

INT32 g_com_nHLThreadFlag  = 1;    //0---线程退出 1---正在运行


COMDEV::COMDEV()
{
}

//INT32 COMDEV::OpenCom(char *pcPortName,INT32 nBaudRate,INT32 nDataBit,char cStopBit,char cParity)
INT32 COMDEV::OpenCom(char *pcPortName,INT32 nBaudRate)
{
    QString strCom;
    DCB stCurDCB;
    COMMTIMEOUTS stTimeOuts;

    if (NULL == pcPortName)
    {
        return -8;
    }

//    Q_UNUSED(cParity);
//    Q_UNUSED(cStopBit);
//    Q_UNUSED(nDataBit);

    strCom = pcPortName;
    g_com_nHLThreadFlag = 0;

    /* 关闭已经打开的句柄 */
    if ((NULL != m_hHandle) && (INVALID_HANDLE_VALUE != m_hHandle))
    {
        CloseHandle(m_hHandle);
    }

    m_hHandle = CreateFile(strCom.toStdWString().c_str(),
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
    if ((INVALID_HANDLE_VALUE == m_hHandle) || (NULL == m_hHandle))
    {
        //HHT_LOG(EN_ERR, "create read file (%s) failed.", strCom.toStdString().c_str());
        CloseHandle(m_hHandle);
        return -9;
    }
    HHT_LOG(EN_INFO, "create read file (%s) OK.", strCom.toStdString().c_str());

    /* 串口只能打开一个,故不需要再创建一个写数据句柄 */

    GetCommState(m_hHandle, &stCurDCB);
    stCurDCB.BaudRate = nBaudRate;  //修改波特率
    stCurDCB.fParity = NOPARITY;    //n
    stCurDCB.ByteSize = 8;          //8个数据位//8
    stCurDCB.StopBits = ONESTOPBIT; //1个停止位//1s
    SetCommState(m_hHandle, &stCurDCB); //设置com端口的状态

    GetCommTimeouts(m_hHandle, &stTimeOuts);
    stTimeOuts.ReadIntervalTimeout = 50;
    stTimeOuts.ReadTotalTimeoutConstant = 300;
    SetCommTimeouts(m_hHandle, &stTimeOuts);

    m_nBaudRate = nBaudRate;
    m_cParity = NOPARITY;
    memset(m_acComBuf, 0, sizeof(m_acComBuf));
    m_nStartPos = 0;
    m_nEndPos = 0;

    g_com_nHLThreadFlag = 1;
    m_hReadThread = (HANDLE)_beginthreadex(NULL, 0,
                            (unsigned int (__stdcall *)(void *))ReadThread,
                            this, 0, &m_ulReadThdID);
    if (NULL != m_hReadThread)
    {
        SetThreadPriority(m_hReadThread, THREAD_PRIORITY_HIGHEST);
    }
    HHT_LOG(EN_INFO, "create readthread success.");

    return RET_SUCCESS;
}

ULONG __stdcall COMDEV::ReadThread(LPVOID pParam)
{
    INT32       nReadOnceLen    = 120;    //需要读取的长度
    DWORD       dwRealReadLen   = 0;    //实际读取长度
    INT32       nDataRealLen    = 0;     //数据实际长度
    UINT8       acRcvBuf[300]   = {0,};
    INT32       nRet            = 0;
    UINT32      ulErr           = 0;
    COMSTAT     stComStat;

    COMDEV *pstCom      = (COMDEV*)pParam;

    if (NULL == pstCom)
    {
        HHT_LOG(EN_ERR, "invalid input parameter!");
        return 0;
    }

    while (0 != g_com_nHLThreadFlag)
    {
        if (NULL == pstCom->m_hHandle)
        {
            HHT_LOG(EN_DEBUG, "read handle is null, so quit");
            break;
        }

        nRet = ::ClearCommError(pstCom->m_hHandle, (LPDWORD)&ulErr, &stComStat);
        if (nRet && (0 < stComStat.cbInQue))
        {
            memset(acRcvBuf, 0, sizeof(acRcvBuf));
            if ((DWORD)nReadOnceLen < stComStat.cbInQue)
            {
                nRet = ReadFile(pstCom->m_hHandle, (PBYTE)acRcvBuf,
                                nReadOnceLen, &dwRealReadLen, NULL);
            }
            else
            {
                nRet = ReadFile(pstCom->m_hHandle, (PBYTE)acRcvBuf,
                                stComStat.cbInQue, &dwRealReadLen, NULL);
            }
            HHT_LOG(EN_DEBUG, "get[%d],start[%d],end[%d] (%02x %02x %02x %02x %02x %02x)",
                    dwRealReadLen, pstCom->m_nStartPos, pstCom->m_nEndPos, acRcvBuf[0], acRcvBuf[1], acRcvBuf[2],
                    acRcvBuf[3], acRcvBuf[4], acRcvBuf[5]);
            if (0 < dwRealReadLen)
            {
                nDataRealLen = dwRealReadLen;
                /*nDataRealLen = Pub_CheckComData(acRcvBuf);
                if (0 >= nDataRealLen)
                {//abandon
                    continue;
                }*/
                if (COM_BUF_LEN> (pstCom->m_nEndPos + nDataRealLen))
                {//缓冲区未满
                    memcpy((pstCom->m_acComBuf + pstCom->m_nEndPos), acRcvBuf, nDataRealLen);
                    pstCom->m_nEndPos += nDataRealLen;
                }
                else//缓冲区已满
                {
                    int nTmpLen = pstCom->m_nEndPos + nDataRealLen - COM_BUF_LEN;
                    memcpy((pstCom->m_acComBuf + pstCom->m_nEndPos), acRcvBuf, (nDataRealLen - nTmpLen));
                    memcpy((pstCom->m_acComBuf + 0), (acRcvBuf + nDataRealLen - nTmpLen), nTmpLen);
                    pstCom->m_nEndPos = nTmpLen;
                }
                pstCom->m_nRealBufLen += nDataRealLen;
                HHT_LOG(EN_DEBUG, "reallen[%d], start[%d], end[%d]", pstCom->m_nRealBufLen, pstCom->m_nStartPos, pstCom->m_nEndPos);
            }
        }

        Pub_MSleep(1);
    }
    Pub_MSleep(1);
    _endthreadex(0);

    return 0;
}

INT32 COMDEV::ReadCom(UINT8 *pcBuf, const INT32 nLen)
{
    INT32   nReadLen  = 0;

    INT64   nPreTime    = 0;

    if ((NULL == pcBuf) || (0 >= nLen))
    {
        return -1;
    }

    if ((INVALID_HANDLE_VALUE == m_hHandle) || (NULL == m_hHandle))
    {
        HHT_LOG(EN_ERR, "read handle[%d] is invalid", m_hHandle);
        return -1;
    }

//    HHT_LOG(EN_DEBUG, "startpos [%d], endpost[%d], num[%d](%02x %02x %02x %02x %02x %02x)",
//                    m_nStartPos, m_nEndPos, m_nRealBufLen, m_acComBuf[m_nStartPos],
//                    m_acComBuf[m_nStartPos+1], m_acComBuf[m_nStartPos+2],
//                    m_acComBuf[m_nStartPos+3], m_acComBuf[m_nStartPos+4],
//                    m_acComBuf[m_nStartPos+5]);

    nPreTime = Pub_GetCurrentTimeClick();
    while (10 > (Pub_GetCurrentTimeClick() - nPreTime))//20ms超时
    {
        if ((0 >= m_nRealBufLen) || (5 > m_nRealBufLen)) //没有有效数据
        {
            Pub_MSleep(1);         //防止死循环CPU过高
            continue;
        }
        nReadLen = nLen;
        if (NULL != m_acComBuf)
        {
            /* 数据头正确但整条数据可能未读完 */
            if (nReadLen > m_nRealBufLen)
            {
                HHT_LOG(EN_DEBUG, "nReadLen[%d],reallen[%d]", nReadLen, m_nRealBufLen);
                //continue;
                nReadLen = m_nRealBufLen;
            }


            /* 开始读取数据 */
            if (m_nStartPos < m_nEndPos)    //buffer正常顺序
            {
                memcpy(pcBuf, (m_acComBuf + m_nStartPos), nReadLen);
                m_nStartPos += nReadLen;
                m_nRealBufLen -= nReadLen;
                return nReadLen;
            }
            else                           //buffer头大于尾
            {
                if ((m_nStartPos + nReadLen) < COM_BUF_LEN)//尾部可供读取数据
                {
                    memcpy(pcBuf, (m_acComBuf + m_nStartPos), nReadLen);
                    m_nStartPos += nReadLen;
                    m_nRealBufLen -= nReadLen;
                    return nReadLen;
                }
                else//尾部不够读取,需要移位到头部
                {
                    int nTmpLen = m_nStartPos + nReadLen - COM_BUF_LEN;
                    memcpy(pcBuf, (m_acComBuf + m_nStartPos), (nReadLen - nTmpLen));
                    m_nStartPos = 0;
                    memcpy(&pcBuf[nReadLen-nTmpLen], (m_acComBuf + m_nStartPos), nTmpLen);
                    m_nStartPos = nTmpLen;
                    m_nRealBufLen -= nReadLen;
                    return nReadLen;
                }
            }
        }
    }

    return -1;
}

INT32 COMDEV::ReadComByHeader(UINT8 * pcBuf, const INT32 nLen)
{
    INT32 nReadLen = 0;
    INT32 nDataLen = 0;
    INT32 nEndPos = 0;

    INT64 nPreTime = 0;

    if ((NULL == pcBuf) || (0 >= nLen))
    {
        return -1;
    }

    if ((INVALID_HANDLE_VALUE == m_hHandle) || (NULL == m_hHandle))
    {
        HHT_LOG(EN_ERR, "read handle[%d] is invalid", m_hHandle);
        return -1;
    }

//    HHT_LOG(EN_DEBUG, "startpos [%d], endpost[%d], num[%d](%02x %02x %02x %02x %02x %02x)",
//                    m_nStartPos, m_nEndPos, m_nRealBufLen, m_acComBuf[m_nStartPos],
//                    m_acComBuf[m_nStartPos+1], m_acComBuf[m_nStartPos+2],
//                    m_acComBuf[m_nStartPos+3], m_acComBuf[m_nStartPos+4],
//                    m_acComBuf[m_nStartPos+5]);

    nPreTime = Pub_GetCurrentTimeClick();
    while (10 > (Pub_GetCurrentTimeClick() - nPreTime))//20ms超时
    {
        if ((0 >= m_nRealBufLen) || (5 > m_nRealBufLen)) //没有有效数据
        {
            Pub_MSleep(1);         //防止死循环CPU过高
            continue;
        }
        nReadLen = nLen;
        if (NULL != m_acComBuf)
        {
            /* 数据头正确但整条数据可能未读完 */
            if (nReadLen > m_nRealBufLen)
            {
                HHT_LOG(EN_DEBUG, "nReadLen[%d],reallen[%d]", nReadLen, m_nRealBufLen);
                //continue;
                nReadLen = m_nRealBufLen;
            }

            //7F YY 99 A2 B3 C4 02 FF 70 A1 ZZ CHK CF
            /* 开始读取数据 */
            if (m_nStartPos < m_nEndPos)    //buffer正常顺序
            {
                /*先找到正确的包头位置*/
                while (m_nStartPos < m_nEndPos)
                {
                    if (COM_HEADER == m_acComBuf[m_nStartPos])
                    {
                        break;
                    }
                    m_nStartPos++;
                    m_nRealBufLen--;
                }
                if (m_nEndPos == m_nStartPos)   //缓冲区数据为空,没有找到合法的数据包
                {
                    return -2;
                }
                nDataLen = m_acComBuf[m_nStartPos+1];
                nEndPos = m_nStartPos+nDataLen+2;   //包尾位置
//                HHT_LOG(EN_INFO, "huang, datalen:%d, EndPos:%d, startPos:%d", nDataLen, m_nEndPos, m_nStartPos);
                if (nDataLen+3 > m_nEndPos-m_nStartPos)
                {
                    return -5;
                }
                if (nEndPos < COM_BUF_LEN)
                {
                    nReadLen = nDataLen+3;
                    if (COM_ENDER == m_acComBuf[nEndPos])
                    {
                        memcpy(pcBuf, (m_acComBuf + m_nStartPos), nReadLen);
                        m_nStartPos += nReadLen;
                        m_nRealBufLen -= nReadLen;
                        return nReadLen;
                    }
                    else
                    {
                        m_nStartPos += nReadLen;
                        m_nRealBufLen -= nReadLen;
                        return -3;
                    }
                }
                else
                {
                    return -4;
                }
            }
            else                           //buffer头大于尾
            {
                if ((m_nStartPos + nReadLen) < COM_BUF_LEN)//尾部可供读取数据
                {
                    memcpy(pcBuf, (m_acComBuf + m_nStartPos), nReadLen);
                    m_nStartPos += nReadLen;
                    m_nRealBufLen -= nReadLen;
                    return nReadLen;
                }
                else//尾部不够读取,需要移位到头部
                {
                    int nTmpLen = m_nStartPos + nReadLen - COM_BUF_LEN;
                    memcpy(pcBuf, (m_acComBuf + m_nStartPos), (nReadLen - nTmpLen));
                    m_nStartPos = 0;
                    memcpy(&pcBuf[nReadLen-nTmpLen], (m_acComBuf + m_nStartPos), nTmpLen);
                    m_nStartPos = nTmpLen;
                    m_nRealBufLen -= nReadLen;
                    return nReadLen;
                }
            }
        }
    }

    return -1;
}

INT32 COMDEV::ReadX5X7ComByHeader(UINT8 *pcBuf, const INT32 nLen)
{
    INT32 nReadLen = 0;
    INT32 nDataLen = 0;
    INT32 nEndPos = 0;
    UINT8 acCmdSnderOps[3] = {0x02, 0x00, 0x03};            //发送者标识 发送给Android
    UINT8 acCmdSnderAndroid[3] = {0x01, 0x00, 0x03};     //发送者标识 发送给OPS
    INT64 nPreTime = 0;

    if ((NULL == pcBuf) || (0 >= nLen))
    {
        return -1;
    }

    if ((INVALID_HANDLE_VALUE == m_hHandle) || (NULL == m_hHandle))
    {
        HHT_LOG(EN_ERR, "read handle[%d] is invalid", m_hHandle);
        return -2;
    }

//    HHT_LOG(EN_DEBUG, "\n startpos [%d]0x%02x 0x%02x, endpos[%d] 0x%02x 0x%02x , num[%d]",
//            m_nStartPos,  m_acComBuf[m_nStartPos],m_acComBuf[m_nStartPos+1],
//            m_nEndPos,m_acComBuf[m_nEndPos], m_acComBuf[m_nEndPos-1],
//            m_nRealBufLen);

    nPreTime = Pub_GetCurrentTimeClick();
    while (50 > (Pub_GetCurrentTimeClick() - nPreTime))//50ms超时
    {
        if ((0 >= m_nRealBufLen) || (12 > m_nRealBufLen)) //没有有效数据
        {
            Pub_MSleep(1);         //防止死循环CPU过高
            continue;
        }
        nReadLen = nLen;
        if (NULL != m_acComBuf)
        {
            /* 数据头正确但整条数据可能未读完 */
            if (nReadLen > m_nRealBufLen)
            {
                HHT_LOG(EN_DEBUG, "nReadLen[%d],reallen[%d]", nReadLen, m_nRealBufLen);
                //continue;
                nReadLen = m_nRealBufLen;
            }

            //aa 55 (1a 00)长度  4a/序列号 (10 00 03)/发送方标识  [30 30 36 38  30 30 36 35 30 30 36 31  30 30 37 32  30 30 37 34]/数据  (6b 04)校验和 55 aa
            //7F YY 99 A2 B3 C4 02 FF 70 A1 ZZ CHK CF
            /* 开始读取数据 */
            if (m_nStartPos < m_nEndPos)    //buffer正常顺序
            {
                /*先找到正确的包头位置*/  //header 0xaa 0x55  ender 0x55 0xaa
                while (m_nStartPos < m_nEndPos)
                {
                    if ( (COM_X5X7_HEADER == m_acComBuf[m_nStartPos])&&
                         (COM_X5X7_HEADER2 == m_acComBuf[m_nStartPos+1]))
                    {
                        break;
                    }
                    m_nStartPos++;
                    m_nRealBufLen--;
                }
                if (m_nEndPos == m_nStartPos)   //缓冲区数据为空,没有找到合法的数据包
                {
                    return -3;
                }
                nDataLen =(int)(m_acComBuf[m_nStartPos+2]|((m_acComBuf[m_nStartPos+3]<<8)&0xFFFF));//0x1a 0x00->0x001a 计算长度
                nEndPos = 5+m_nStartPos+nDataLen;   //包尾位置
                HHT_LOG(EN_INFO, "X5.X7===>, datalen:%d, EndPos:%d, startPos:%d  header:[0x%02x 0x%02x] ender:[0x%02x 0x%02x]",
                nDataLen, m_nEndPos, m_nStartPos,m_acComBuf[m_nStartPos],m_acComBuf[m_nStartPos+1],m_acComBuf[nEndPos-1],m_acComBuf[nEndPos]);
                if ((nDataLen+5) > (m_nEndPos-m_nStartPos))
                {
                    return -4;
                }
                if (nEndPos < COM_BUF_LEN)
                {
                    nReadLen = nDataLen+6;
                    HHT_LOG(EN_INFO, "X5.X7===>[nEndPos-1]:0x%02x [nEndPos]0x%02x",m_acComBuf[nEndPos-1],m_acComBuf[nEndPos]);
                    if ((COM_X5X7_ENDER == m_acComBuf[nEndPos-1])&&
                        (COM_X5X7_ENDER2 == m_acComBuf[nEndPos]))
                    {
                        memcpy(pcBuf, (m_acComBuf + m_nStartPos), nReadLen);
                        m_nStartPos += nReadLen;
                        m_nRealBufLen -= nReadLen;
                        return nReadLen;
                    }
                    else
                    {
                        m_nStartPos += nReadLen;
                        m_nRealBufLen -= nReadLen;
                        return -5;
                    }
                }
                else
                {
                    return -6;
                }
            }
            else                           //buffer头大于尾
            {
                if ((m_nStartPos + nReadLen) < COM_BUF_LEN)//尾部可供读取数据
                {
                    memcpy(pcBuf, (m_acComBuf + m_nStartPos), nReadLen);
                    m_nStartPos += nReadLen;
                    m_nRealBufLen -= nReadLen;
                    return nReadLen;
                }
                else//尾部不够读取,需要移位到头部
                {
                    int nTmpLen = m_nStartPos + nReadLen - COM_BUF_LEN;
                    memcpy(pcBuf, (m_acComBuf + m_nStartPos), (nReadLen - nTmpLen));
                    m_nStartPos = 0;
                    memcpy(&pcBuf[nReadLen-nTmpLen], (m_acComBuf + m_nStartPos), nTmpLen);
                    m_nStartPos = nTmpLen;
                    m_nRealBufLen -= nReadLen;
                    return nReadLen;
                }
            }
        }
    }
    return -100;
}


INT32 COMDEV::WriteCom(UINT8 *pBuf, const INT32 nLen)
{
    INT32   nWriteLen   = 0;
    //INT32   nIndex      = 0;

    if ((NULL == pBuf) || (0 >= nLen) || (NULL == m_hHandle))
    {
        return RET_INVALID;
    }

//    for (nIndex=0; nIndex<nLen; nIndex++)
//    {
//        if (FALSE == WriteFile(m_hHandle, (PBYTE)&pBuf[nIndex], 1, (LPDWORD)&nWriteLen, NULL))
//        {
//            return 0;
//        }
//        //Pub_MSleep(1);
//    }
    if (FALSE == WriteFile(m_hHandle, pBuf, nLen, (LPDWORD)&nWriteLen, NULL))
    {
        return RET_INVALID;
    }

    return nWriteLen;
}

INT32 COMDEV::IsOpenCom()
{
    if (NULL != m_hHandle)
    {
        return RET_FAILURE;
    }
    
    return RET_SUCCESS;
}


INT32 COMDEV::CloseCom()
{
    g_com_nHLThreadFlag = 0;
    if ((NULL != m_hHandle) && (INVALID_HANDLE_VALUE != m_hHandle))
    {
        CloseHandle(m_hHandle);
    }
    if ((NULL != m_hReadThread) && (INVALID_HANDLE_VALUE != m_hReadThread))
    {
        CloseHandle(m_hReadThread);
    }

    m_hHandle       = NULL;
    m_hReadThread   = NULL;

    m_nBaudRate     = COM_BAUDRATE;
    m_nDataBit      = 8;
    m_cStopBit      = '1';
    m_cParity       = 'N';

    return RET_SUCCESS;
}
