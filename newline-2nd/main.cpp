#include "maindialog.h"
#include <QApplication>
#include <QFontDatabase>
#include "Helper/hhthelper.h"
#include <QSharedMemory>
#include <QMessageBox>
#include "opsoperate.h"
#include "Helper/hhthelper.h"

extern char g_Version[100];

double g_fontPixelRatio; //全局字体缩放比例

int main(int argc, char *argv[])
{  
    int nRet = 0;
//    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#if (QT_VERSION >= QT_VERSION_CHECK(5,6,0))
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);//Hight DPI support
#endif
    QApplication a(argc, argv);
    QApplication::setApplicationName(APPLICATION_NAME);
    int index= QFontDatabase::addApplicationFont(":/Resource/Helvetica.ttf");
    //qDebug()<<"---------->"<<index;
    if(index!=-1)
    {
        QStringList strList(QFontDatabase::applicationFontFamilies(index));
        if(strList.count()>0)
        {
            qDebug()<<"fontName:"<<strList.at(0);
            QFont  appFont(strList.at(0));
            appFont.setPixelSize(9);
            a.setFont(appFont);
        }
    }
     //获取系统字体缩放比例
     g_fontPixelRatio =hhtHelper::GetFontPixelRatio();
     qDebug()<<"[System font scales ratios: "<<g_fontPixelRatio<<"]";
//     HHT_LOG(EN_INFO,"[System font scales ratios (%f)]\n",g_fontPixelRatio);
     //获取软件版本号
     memset(g_Version,0,100);
     hhtHelper::GetAppVersion(g_Version);
     //执行程序单例子
     QString appName = QCoreApplication::applicationName();
     qDebug ()<<"Application Name --->"<<appName;
     //An instacne has already running
     const QString   windowTitle = APPLICATION_NAME;
     const QString  windowClass = "Qt5QWindowIcon"; //find it out this name with "Microsoft Spy++"
     const wchar_t *WInTitle = reinterpret_cast<const wchar_t*>(windowTitle.utf16());
     const wchar_t *WInClass = reinterpret_cast<const wchar_t*>(windowClass.utf16());
     HWND hwnd = FindWindow(WInClass,WInTitle);
     //HWND hwnd = ::FindWindowA(NULL, "Newline assistant");
     qDebug()<<"[window hwnd---->"<<hwnd<<"]";
     if(hwnd)
     {//
         qDebug()<<"An instance is running";
         SendMessage(hwnd,SHOWNORNAL,0,0);
         return 0;
     }
//frist run the instance
    MainDialog w;
    qDebug()<<"MainDialog Title: "<<w.windowTitle();
    {
        //
        ReadConfig();
        vos_pthread_t opsThreadID;
#if    SUPORT_X5X7_DEVICE
        nRet = VOS_CreateThread(&opsThreadID, distinguish_device, NULL);
        if (RET_SUCCESS != nRet)
        {
            HHT_LOG(EN_ERR, "   create distinguish_device failed");
        }
        else
        {
            HHT_LOG(EN_ERR, "   create distinguish_device success");
        }
#else
        ReadConfig();
        nRet = VOS_CreateThread(&opsThreadID, OpsCom_ThreadFn, NULL);
        if (RET_SUCCESS != nRet)
        {
            HHT_LOG(EN_ERR, "   create OpsCom_Thread failed");
        }
        else
        {
            HHT_LOG(EN_ERR, "   create OpsCom_Thread success");
        }
#endif

#if 1
       w.showMinimized();
       w.hide();
#else
        w.showNormal();
#endif
        return a.exec();
    }

}
