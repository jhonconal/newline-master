#include <QMetaType>
#include "ss_pub.h"

PUBComClass  PUB_SS;

PUBComClass::PUBComClass()
{

}

PUBComClass::~PUBComClass()
{

}

QString PUBComClass::Pub_GetUserDataPath()
{
    /// 得到application data目录 programdata
    char szPath[256];
    QString UserDataPath=NULL;
    TCHAR path[300];
    SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, path);
    QString strPath = QString::fromWCharArray(path);
    strcpy(szPath, strPath.toStdString().c_str());
    strcat(szPath, "\\Newline assistant\\Settings");

    QDir dir;
    QString  dataPath = hhtHelper::CharToQString(szPath);
    qDebug()<<"dataPath: "<<dataPath;
    UserDataPath = dataPath.replace(QRegExp("\\\\"),"/");
    if (false == dir.exists(UserDataPath))
    {
        bool result = dir.mkpath(UserDataPath);
        if(!result)
        {
            return NULL;
        }
    }
    qDebug()<<"UserDataPath: "<<UserDataPath;
    return UserDataPath;
}

QString PUBComClass::Pub_GetUserConfigPath()
{
    /// 得到application data目录 programdata
    char szPath[256];
    QString UserConfigPath=NULL;
    TCHAR path[300];
    SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, path);
    QString strPath = QString::fromWCharArray(path);
    strcpy(szPath, strPath.toStdString().c_str());
    strcat(szPath, "\\Newline assistant\\Settings\\Config");

    QDir dir;
    QString  dataPath = hhtHelper::CharToQString(szPath);
    qDebug()<<"dataPath: "<<dataPath;
    UserConfigPath = dataPath.replace(QRegExp("\\\\"),"/");
    if (false == dir.exists(UserConfigPath))
    {
        bool result = dir.mkpath(UserConfigPath);
        if(!result)
        {
            return NULL;
        }
    }
    qDebug()<<"UserConfigPath: "<<UserConfigPath;
    return UserConfigPath;
}

int PUBComClass::Pub_CheckAppRunning()
{
    HANDLE hMutex;
    //创建句柄
    hMutex = ::CreateMutex(NULL, FALSE, (LPCWSTR)"PanelInspector");
    if (NULL == hMutex)
    {
        return 1;
    }
    if (ERROR_ALREADY_EXISTS == ::GetLastError())
    {
        QMessageBox::warning(NULL, "Warning", "The program is already running.");
        CloseHandle(hMutex);
        return 1;
    }
    return 0;
}

void PUBComClass::PostOpenApp(QString strAppName)
{
    emit signal_openProcFromFileName(strAppName);
}

void PUBComClass::PostOpenComFailed()
{
    emit SignalOpenComFailed();
}

void PUBComClass::PostOpenComSuccess()
{
    emit SignalOpenComSuccess();
}

void PUBComClass::PostTotalPkgNum(int nTotalPkgNum)
{
    emit SignalTotalPkgNum(nTotalPkgNum);
}

void PUBComClass::PostCurrentPkgNum(int nCurrentPkgNum)
{
    emit SignalCurrentPkgNum(nCurrentPkgNum);
}

void PUBComClass::PostSendFileToAndroidFailed()
{
    emit SignalSendFileToAndroidFailed();
}

void PUBComClass::PostCameraStatusCheck()
{
    emit SignalCameraStatusCheck();
}
