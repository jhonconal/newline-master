#ifndef HHTHEADER_H
#define HHTHEADER_H
#include <QString>
#include <QIcon>
#include <QVector>
//OPS向android发送的数据信息
typedef struct __HHTAPPINFO
{
    QString  _fileName;//文件名
    QString  _lnkPath;//快捷方式文件路径
    QIcon   _appIcon;//图标
}HHTAPPINFO;

#define  HHT_2ND_PROJECT_SUPPORT  //newline 二期支持

#endif // HHTHEADER_H
