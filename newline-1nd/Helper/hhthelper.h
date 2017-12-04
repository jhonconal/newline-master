#ifndef HHTHELPER_H
#define HHTHELPER_H

#include <QtCore>
#include <QtGui>
#include <QApplication>
#include <iostream>
#include <QFont>
#include <QCamera>
#include <QCameraInfo>
#include <QCameraControl>
#include <QCameraImageCapture>
#include <QCameraViewfinder>
#include <QDesktopWidget>
#include "Gui/frmmessagebox.h"
#include "string.h"
#include "stdio.h"
#include "Pub/global.h"
class hhtHelper: public QObject
{
public:
    //设置为开机启动
    static void AutoRunWithSystem(bool IsAutoRun, QString AppName, QString AppPath)
    {
        QSettings *reg = new QSettings(
                    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                    QSettings::NativeFormat);

        if (IsAutoRun) {
            reg->setValue(AppName, AppPath);
        } else {
            reg->setValue(AppName, "");
        }
    }
    /**
     * @brief PUBComClass::Pub_GetFontPixelRatio
     * @return
     * 例如，100% 缩放比例下，1000 号的 Courier New 字体，
     * 一个字母所占的宽度是 800 像素。进而计算缩放比例的代码如下：
     */
    static double GetFontPixelRatio()
    {
         //# the width of a character when dpi = 96
        int base =800;
        QFont font ;
        font.setFamily("Courier New");
        font.setPointSize(1000);
        QFontMetrics font_metrics = QFontMetrics(font);
        double width = font_metrics.width('Q');
        return width/base;
    }

    //设置编码为UTF8
    static void SetUTF8Code()
    {
#if (QT_VERSION <= QT_VERSION_CHECK(5,0,0))
        QTextCodec *codec = QTextCodec::codecForName("UTF-8");
        QTextCodec::setCodecForLocale(codec);
        QTextCodec::setCodecForCStrings(codec);
        QTextCodec::setCodecForTr(codec);
#endif
    }
    //设置皮肤样式
    static void SetStyle(const QString &styleName)
    {
        QFile file(QString(":Resource/images/%1.css").arg(styleName));
        file.open(QFile::ReadOnly);
        QString qss = QLatin1String(file.readAll());
        qApp->setStyleSheet(qss);
        qApp->setPalette(QPalette(QColor("#F0F0F0")));
    }

    //加载中文字符
    static void SetChinese()
    {
        QTranslator *translator = new QTranslator(qApp);
        translator->load(":/image/qt_zh_CN.qm");
        qApp->installTranslator(translator);
    }

    //判断是否是IP地址
    static bool IsIP(QString IP)
    {
        QRegExp RegExp("((2[0-4]\\d|25[0-5]|[01]?\\d\\d?)\\.){3}(2[0-4]\\d|25[0-5]|[01]?\\d\\d?)");
        return RegExp.exactMatch(IP);
    }

    //显示信息框,仅确定按钮
    static void ShowMessageBoxInfo(QString info)
    {
        frmMessageBox *msg = new frmMessageBox;
        msg->SetMessage(info, 0);
        msg->exec();
    }

//    显示错误框,仅确定按钮
    static void ShowMessageBoxError(QString info)
    {
        frmMessageBox *msg = new frmMessageBox;
        msg->SetMessage(info, 2);
        msg->exec();
    }

//    显示询问框,确定和取消按钮
    static int ShowMessageBoxQuesion(QString info)
    {
        frmMessageBox *msg = new frmMessageBox;
        msg->SetMessage(info, 1);
        return msg->exec();
    }

    //延时
    static void Sleep(int sec)
    {
        QTime dieTime = QTime::currentTime().addMSecs(sec);
        while ( QTime::currentTime() < dieTime ) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }
    }

    //窗体居中显示
    static void FormInCenter(QWidget *frm)
    {
        int frmX = frm->width();
        int frmY = frm->height();
        QDesktopWidget w;
        int deskWidth = w.width();
        int deskHeight = w.height();
        QPoint movePoint(deskWidth / 2 - frmX / 2, deskHeight / 2 - frmY / 2);
        frm->move(movePoint);
    }
#ifdef Q_OS_WIN32
    static char *WcharToChar(wchar_t*wc){
        char *m_char;
        int len= WideCharToMultiByte(CP_ACP,0,wc,wcslen(wc),NULL,0,NULL,NULL);
        m_char=new char[len+1];
        WideCharToMultiByte(CP_ACP,0,wc,wcslen(wc),m_char,len,NULL,NULL);
        m_char[len]='\0';
        return m_char;
    }
    static wchar_t* CharToWchar(char* c){
        wchar_t *m_wchar;
        int len = MultiByteToWideChar(CP_ACP,0,c,strlen(c),NULL,0);
        m_wchar=new wchar_t[len+1];
        MultiByteToWideChar(CP_ACP,0,c,strlen(c),m_wchar,len);
        m_wchar[len]='\0';
        return m_wchar;
    }
#endif
    static char *QStringToChar(const QString&str){
        char *mm;
        QByteArray ba = str.toLatin1();
        mm = ba.data();
        return mm;
    }
    static QString CharToQString(char *m){
        return QString(QLatin1String(m));
    }
    static QString ByteArryToQString(QByteArray ba){
        return QString(ba);
    }
    static QByteArray QStringToQByteArry(QString str){
        return str.toLatin1();
    }
    //将1-9 a-f字符转化为对应的整数
    static char ConvertHexChar(char ch)
    {
        if((ch >= '0') && (ch <= '9'))
            return ch-0x30;
        else if((ch >= 'A') && (ch <= 'F'))
            return ch-'A'+10;
        else if((ch >= 'a') && (ch <= 'f'))
            return ch-'a'+10;
        else return (-1);
    }
    //将字符型进制转化为16进制
    static QByteArray QString2Hex(QString str)
    {
        QByteArray senddata;
        int hexdata,lowhexdata;
        int hexdatalen = 0;
        int len = str.length();
        senddata.resize(len/2);
        char lstr,hstr;
        for(int i=0; i<len; )
        {
            hstr=str[i].toLatin1();   //字符型
            if(hstr == ' ')
            {
                i++;
                continue;
            }
            i++;
            if(i >= len)
                break;
            lstr = str[i].toLatin1();
            hexdata = ConvertHexChar(hstr);
            lowhexdata = ConvertHexChar(lstr);
            if((hexdata == 16) || (lowhexdata == 16))
                break;
            else
                hexdata = hexdata*16+lowhexdata;
            i++;
            senddata[hexdatalen] = (char)hexdata;
            hexdatalen++;
        }
        senddata.resize(hexdatalen);
        return senddata;
    }
    static char *Hex2Char(const unsigned char*pBuf,const unsigned nLen){
        static char acBuf[20000]={0,};
        char acTmpBuf[10]={0,};
        unsigned int ulIndex =0;
        unsigned int ulBufLen = 0;
        if((pBuf ==NULL)||nLen<=0){
            return NULL;
        }
        ulBufLen = sizeof(acBuf)/sizeof(acTmpBuf);
        if(ulBufLen>=nLen){
            ulBufLen = nLen;
        }
        memset(acBuf,0,sizeof(acBuf));
        memset(acTmpBuf,0,sizeof(acTmpBuf));
        for(ulIndex=0;ulIndex<ulBufLen;ulIndex++){
            sprintf_s(acTmpBuf,sizeof(acTmpBuf),"%02X ",*(pBuf+ulIndex));
            strcat(acBuf,acTmpBuf);
        }
        return acBuf;
    }
     static QString FormatHex2Uncide(QString hexString){
         //00 41 00 64 00 6F 00 62 00 65 00 20 00 50 00 68 00 6F 00 74 00 6F [from]
         //00410064006F00620065002000500068006F0074006F00730068006F [to]
         QString unicodeString = hexString.replace(" ","");
         return unicodeString;
     }
    //将接收的一串QByteArray类型的16进制,转化为对应的字符串16进制
    static QString ShowHex(QByteArray str)
    {
        QDataStream out(&str,QIODevice::ReadWrite);   //将str的数据 读到out里面去
        QString buf;
        while(!out.atEnd())
        {
            qint8 outChar = 0;
            out >> outChar;   //每次一个字节的填充到 outchar
            QString str = QString("%1").arg(outChar&0xFF,2,16,QLatin1Char('0')).toUpper() + QString(" ");   //2 字符宽度
            buf += str;
        }
        return buf;
    }
    //将串口接收的16进制数据，每2个字节合并成1个字显示出来
    static QString Convert4Hex(QByteArray str)
    {
        QDataStream out(&str,QIODevice::ReadWrite);   //将str的数据 读到out里面去
        QString buf;
        while(!out.atEnd())
        {
            qint16 outChar = 0;
            out>>outChar;   //每次一个字的填充到 outchar
            QString str = QString("%1").arg(outChar&0xFFFF,4,16,QLatin1Char('0')).toUpper() + QString(" ");   //4 字符宽度
            buf += str;
        }
        return buf;
    }

    static QString  QString2HexString(QString str){
        bool ok =true;
        QByteArray a = QByteArray::number(str.toLatin1().toHex().toInt(&ok, 16), 16);
        QByteArray b= QByteArray::fromHex(a);
        QString HexString= QString(b);
        return  HexString;
    }
   static unsigned char CheckSum(QByteArray ba){
        int result = 0;
        for(int i=0;i<ba.size();i++){
            result +=ba.at(i);
        }
        return  (unsigned char)(result%256);
    }

    static QString QString2Unicode(QString str)
    {
        QString str_code = "";
        QChar qch='0';
        int len = str.length();
        for(int m = 0; m < len;m++)
        {
            qch = str.at(m);
            int code = (int)(qch.unicode());
            QString hex_code_str = QString::number(code,16);
            switch(hex_code_str.length())
            {
            case 1:
                hex_code_str = "000"+hex_code_str;
                break;
            case 2:
                hex_code_str = "00"+hex_code_str;
                break;
            case 3:
                hex_code_str = "0"+hex_code_str;
                break;
            }
            str_code += hex_code_str;
        }
        return str_code;
    }

    static QString Unicode2QString(QString str)
    {
        int temp[400];
        QChar qchar[100];
        QString strOut;
        bool ok;
        int count=str.count();
        int len=count/4;
        for(int i=0;i<count;i+=4)
        {
            temp[i]=str.mid(i,4).toInt(&ok,16);//每四位转化为16进制整型
            qchar[i/4]=temp[i];
            QString str0(qchar, len);
            strOut=str0;
        }
        return strOut;
    }

    static QString Int2HexStringA(int arg){
        QString WW;
        QString temp =QString::number(arg,16);
        switch (temp.length()) {
        case 1:
             WW="0"+temp;
            break;
        case 2:
             WW=temp;
            break;
        default:
            break;
        }
        return WW;
    }

   static QString Int2HexString(int arg){
       QString WW;
       QString temp =QString::number(arg,16);
       switch (temp.length()) {
       case 1:
            WW="000"+temp;
           break;
       case 2:
            WW="00"+temp;
           break;
       case 3:
            WW="0"+temp;
           break;
       default:
           break;
       }
       return WW;
   }
//-----------------------------------------------如下是获取程序版本函数-------------------------------------------------------------------------
   // 将缓冲区中，最前面和最后面的空格和制表符去掉
   static void trim(char *buffer)
   {
       char *ptr1, *ptr2;
       int len;
       unsigned char c;

       len = strlen(buffer);
       if (len == 0 ) return;

       // 去掉最前面的空格
       for(ptr1 = buffer; *ptr1 != 0; ptr1++)
       {
           c = *ptr1;
           if ( c > ' ') break;
       }
       if( *ptr1 == '\0') // 空行
       {
           buffer[0] = '\0';
           return;
       }

       // 去掉最后面的空格
       for(ptr2 = buffer + len - 1; ptr2 != ptr1; ptr2--)
       {
           c = *ptr2;
           if ( c > ' ' ) break;
       }

       len = ptr2 - ptr1 + 1;
       memcpy(buffer, ptr1, len);
       *(buffer + len) = '\0';
   }

   static   void GetYMD(const char *src, int &y, int &m, int &d)
   {
       int len = strlen(src);
       char temp[10];

       //------------------------------
       memset(temp, 0, sizeof(temp));
       memcpy(temp, src + len  - 4, 4);  // 获取最后4位
       y = atoi(temp) - 2000;

       //------------------------------
       if     (memcmp(src, "Jan", 3) == 0 ) m = 1;
       else if(memcmp(src, "Feb", 3) == 0 ) m = 2;
       else if(memcmp(src, "Mar", 3) == 0 ) m = 3;
       else if(memcmp(src, "Apr", 3) == 0 ) m = 4;
       else if(memcmp(src, "May", 3) == 0 ) m = 5;
       else if(memcmp(src, "Jun", 3) == 0 ) m = 6;
       else if(memcmp(src, "Jul", 3) == 0 ) m = 7;
       else if(memcmp(src, "Aug", 3) == 0 ) m = 8;
       else if(memcmp(src, "Sep", 3) == 0 ) m = 9;
       else if(memcmp(src, "Oct", 3) == 0 ) m = 10;
       else if(memcmp(src, "Nov", 3) == 0 ) m = 11;
       else if(memcmp(src, "Dec", 3) == 0 ) m = 12;

       //------------------------------
       memset(temp, 0, sizeof(temp));
       memcpy(temp, src + 3, len - 7);
       trim(temp);
       d = atoi(temp);
   }

   static  void GetAppVersion(char *version)
   {
       int y, m, d;
       //QDate date = QDate::currentDate();  //  运行时的时间
       //y = date.year() - 2000;
       //m = date.month();
       //d = date.day();

       //   Standard Predefined Macros
       // __DATE__   :编译时的日期
       // __FIlE__      :编译时的文件名字
       // __LINE__   :编译时所在的行
       // __TIME__   :编译时的时间

       char today[30];
       sprintf(today, "%s", __DATE__);   // 输出格式如下："Jun  8 2012"
       // "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
       GetYMD(today, y, m, d);

//       g_nVersion = y;
//       g_nVersion <<= 8;
//       g_nVersion += m;
//       g_nVersion <<= 8;
//       g_nVersion += d;

       sprintf(version, "1.%02d.%02d.%02d-D", y, m, d);
   }

   static bool isCameraLoaded()
   {
       QCamera *camera = new QCamera();
       QCameraImageCapture *cameraViewCapture;
       cameraViewCapture = new QCameraImageCapture(camera);
       camera->start();
       qDebug()<<camera->status();
       if((camera->status()==QCamera::Status::LoadedStatus)){
           qDebug()<<"Camera is occupied failed to open it";
           return true;
       }else if ((camera->status()==QCamera::Status::ActiveStatus)) {
           camera->stop();
           qDebug()<<"Camera is avaliable,start it and then stop it";
           return false;
       }else {
           camera->stop();
           return false;
       }
       return false;
   }

   /**
    * @brief Widget::isSysCameraStatus
    * @return
    * 1 :camera is avaliable
    * 0 :camera is occupied
    * -1:camera not found
    */
   static  int isSysCameraStatus()
   {
       QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
       foreach (const QCameraInfo &cameraInfo, cameras)
       {
           HHT_LOG(EN_INFO, "---List of Camera devices : (%s)",cameraInfo.deviceName().toStdString().c_str());
           qDebug()<<"List of Camera :"<<cameraInfo.deviceName();
       }
       QCamera *camera = new QCamera();
       QCameraImageCapture *cameraViewCapture;
       cameraViewCapture = new QCameraImageCapture(camera);
       camera->start();
       qDebug()<<camera->status();
       if((camera->status()==QCamera::Status::LoadedStatus))
       {
           HHT_LOG(EN_INFO, "Camera is occupied failed to open it. ");
           qDebug()<<"Camera is occupied failed to open it. ";
           camera->stop();
           camera->unload();
           delete camera;
           return 0;
       }
       else if ((camera->status()==QCamera::Status::ActiveStatus))
       {
           HHT_LOG(EN_INFO, "Camera is occupied failed to open it. ");
           qDebug()<<"Camera is avaliable,start it and then stop it. ";
           camera->stop();
           camera->unload();
           delete camera;
           return 1;
       }
       else if(camera->status()==QCamera::Status::UnavailableStatus)
       {
           qDebug()<<"Camera not found. ";
           HHT_LOG(EN_INFO, "Camera not found. ");
           camera->stop();
           camera->unload();
           delete camera;
           return -1;
       }
       return -1;
   }
   static bool isAppRunning(const QString windowClass,const QString windowTitle)
   {
       if(!windowTitle.isEmpty()||!windowClass.isEmpty())
       {
           const wchar_t *WInTitle;
           const wchar_t *WInClass;
           if(windowTitle==NULL)
           {
               WInTitle =NULL;
           }
           else
           {
               WInTitle = reinterpret_cast<const wchar_t*>(windowTitle.utf16());
           }
           if(windowClass==NULL)
           {
               WInClass =NULL;
           }
           else
           {

               WInClass = reinterpret_cast<const wchar_t*>(windowClass.utf16());
           }
           HWND hwnd = FindWindow(WInClass,WInTitle);
           if(hwnd)
           {//
               qDebug()<<windowTitle<<"An instance is running";
               return  true;
           }
           else
           {
               return false;
           }
       }
       else
       {
           return false;
       }
       return false;
   }
};

#endif // HHTHELPER_H
