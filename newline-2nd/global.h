#ifndef GLOBAL_H
#define GLOBAL_H
#include <QObject>
#include <QVector>
#include <QLabel>
#include <QStringList>
#include <QListWidget>
#include "nlistwidget.h"
#include "hhtheader.h"
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#define RX_TX_PKG_LENTH  64
NListWidget *g_listWidget;//全局的ListWidgetextern
QString           g_backgroundColor;
QLabel           *g_TrashLabel;
QWidget        *g_TrashWidget;
//COM口发送接收数据

//unsigned char g_RS232TxData[RX_TX_PKG_LENTH];//发送接收到数据的返回码
//unsigned char g_RS232RxData[RX_TX_PKG_LENTH];//接收Andorid断发送的控制码
//全局串口信息
QSerialPort       *g_RS232Port;
QSerialPortInfo *g_RS232PortInfo;
//判断串口是否打开
bool g_RS232isOpen;
QVector<HHTAPPINFO>g_appInfoVector;
char g_Version[50];
#endif // GLOBAL_H
