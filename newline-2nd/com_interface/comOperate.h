#ifndef COMOPERATE_H
#define COMOPERATE_H
#include "comdev.h"


extern COMDEV g_stComDev;

#define COM_RSP_CMD_LEN (64)
#define COM_CMD_DELAY_TIME          (200)

INT32 COM_GetComPortNameFromIndex(const INT32 nPortIndex, VOS_INT8 *pcComportName);
INT32 COM_SndCmdPkg(COMDEV * pstComDev, const UINT8 * pcBuf, UINT8 * pRcvBuf, INT32 nLen, const INT32 nDelayMs);
INT32 Com_Write(COMDEV * pstComDev, UINT8* pcSndBuf, INT32 nLen);

#endif // COMOPERATE_H
