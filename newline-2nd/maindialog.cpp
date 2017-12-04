#include "maindialog.h"
#include "ui_maindialog.h"
#include "nlistwidget.h"
#include "trashwidget.h"
#include <QMenuBar>
#include <QMenu>
#include<QGraphicsDropShadowEffect>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
#include "Helper/iconhelper.h"
#include "Pub/ss_pub.h"
#include "Helper/hhthelper.h"
#include "Helper/fontscaleratio.h"
#include "ops_operate/opsoperate.h"
#include "maskwidget.h"

#define  HHT_WIDTH   735
#define  HHT_HEIGHT 840
//#define  HHT_DEBUG
#define  HHT_SUPPORT_DIR //支持文件夹拖拽

NListWidget     *g_listWidget;//全局的ListWidgetextern
MainDialog      *g_mainDialog;
QWidget            *g_TrashWidget;
QString               g_tryDeleteAppName;//尝试从列表删除的APP
char                     g_Version[100];
bool                      g_SerialStatus;//全局串口状态
int                        g_PubUsbOpertationStatus=-1 ;//操作状态
extern double     g_fontPixelRatio;
extern int            g_nCameraCommandsFlag;//摄像头状态标识
extern  int           g_nDeleteSingleAppFlag;//接收已经删除一个APP指令
extern  int           g_nWriteDeleteSingleAppCommandsFlag;//写请求删除一个APP指令
extern  int           g_nClearAllAppCommandsFlag;//写请求清除APP列表指令
extern bool          g_uploadStatus;//上传模块 上传成功标识
extern int            g_nPubUSBCommandsFlag;//切换PubUSB指令标识

MainDialog::MainDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MainDialog)
{
    HHT_LOG(EN_INFO,"[System font scales ratios (%f)]",g_fontPixelRatio);
    ui->setupUi(this);
    init();

    connect(&PUB_SS,SIGNAL(signal_openProcFromFileName(QString)),this,SLOT(slot_openProcFromFileName(QString)));
    connect(&PUB_SS,SIGNAL(SignalOpenComFailed()),this,SLOT(slot_RS232isDisabled()));
    connect(&PUB_SS,SIGNAL(SignalOpenComSuccess()),this,SLOT(slot_RS232isAvaliable()));
    //摄像头启用指令信号与槽
    connect(&PUB_SS,SIGNAL(SignalCameraStatusCheck()),this,SLOT(slot_cameraStatusCheck()));
    //PusbUSB切换
    connect(&PUB_SS,SIGNAL(SignalPubUsbStatusCheck()),this,SLOT(slot_pubUsbStatusCheck()));
    //清除APP列表
    connect(&PUB_SS,SIGNAL(SignalClearAllApp()),this,SLOT(slot_clearAllApp()));
    //垃圾桶类删除APP信号与槽
    connect(ui->Trash,SIGNAL(signal_deleteAppFromVector(QString)),this,SLOT(slot_deleteAppFromVector(QString)));
    connect(ui->Trash,SIGNAL(signal_deleteAppFromVectorFailed()),this,SLOT(slot_deleteAppFromVectorFailed()));
    connect(ui->Trash,SIGNAL(signal_TrashOpenCOMFailed()),this,SLOT(slot_TrashOpenCOMFailed()));
    //Montage弹窗
    montageDialog = new MontageDialog();
    connect(montageDialog,SIGNAL(signal_showMainDialog()),this,SLOT(slot_showMainDialog()));

    ReadRecords();
    //--------------------------------------------------托盘相关--------------------------------------------------
    QIcon icon = QIcon(":/Resource/images/logo.png");
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(icon);
    trayIcon->setToolTip("Newline assistant");
    trayIcon->show();
    //添加单/双击鼠标相应
    connect(trayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this,SLOT(trayiconActivated(QSystemTrayIcon::ActivationReason)));

    //创建右键弹出菜单
    trayIconMenu = new QMenu(this);
    trayIconMenu->setStyleSheet("QMenu {background-color: white;border: 1px solid white;}"
                                "QMenu::item {background-color: transparent;padding:12px 56px;"
                                "margin:0px 4px;border-bottom:1px solid #DBDBDB;}"
                                "QMenu::item:selected { background-color: #2dabf9;}");
    FontScaleRatio::Instance()->setGuiFont("Helvetica",12,trayIconMenu);
    QAction *quitAction = new QAction(tr("Exit"), this);
    quitAction->setIcon(QIcon(":/Resource/icons/Exit_25px.png"));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(slot_quitAction()));
    QAction  *syncAction = new QAction(tr("Manual sync"),this);
    syncAction->setIcon(QIcon(":/Resource/icons/Synchronize_25px.png"));
    connect(syncAction, SIGNAL(triggered()), this, SLOT(slot_syncAction()));
    QAction  *clearAction = new QAction(tr("Manual clear"),this);
    clearAction->setIcon(QIcon(":/Resource/icons/Clear_25px.png"));
    connect(clearAction, SIGNAL(triggered()), this, SLOT(slot_clearAction()));
    //    clearAction->setVisible(false);
    trayIconMenu->addAction(syncAction);
    trayIconMenu->addAction(clearAction);
    trayIconMenu->addAction(quitAction);
    trayIcon->setContextMenu(trayIconMenu);
    //--------------------------------------------------------------------------------------------------------------
    //设置屏蔽罩
    MaskWidget::Instance()->setMainWidget(this);
    QStringList dialogNames;
    dialogNames << "UploadWidget";
    MaskWidget::Instance()->setDialogNames(dialogNames);
    //slot_pubUsbStatusCheck();
}

MainDialog::~MainDialog()
{
    if(!g_appInfoVector.isEmpty())
    {
        WriteRecords();
        g_appInfoVector.clear();
    }
    delete ui;
}

int MainDialog::isSysCameraStatus()
{
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    QCameraInfo cameraInfo;
    foreach (/*const QCameraInfo &*/cameraInfo, cameras)
    {
        HHT_LOG(EN_INFO, "---List of Camera devices : (%s)",cameraInfo.deviceName().toStdString().c_str());
        qDebug()<<"List of Camera :"<<cameraInfo.deviceName();
    }
    QCamera *camera = new QCamera(cameraInfo);
    QCameraImageCapture *cameraViewCapture;
    cameraViewCapture = new QCameraImageCapture(camera);
    camera->start();
    qDebug()<<camera->status();
    if((camera->status()==QCamera::Status::LoadedStatus))
    {
        if(hhtHelper::isAppRunning("Qt5QWindowOwnDCIcon",NULL))
        {
            HHT_LOG(EN_INFO, "Camera is occupied by Montage . ");
            qDebug()<<"Camera is occupied  by Montage . ";
            camera->stop();
            camera->unload();
            delete camera;
            return 2;//Montage 打开摄像头
        }
        else
        {
            HHT_LOG(EN_INFO, "Camera is occupied failed to open it. ");
            qDebug()<<"Camera is occupied failed to open it. ";
            camera->stop();
            camera->unload();
            delete camera;
            return 0;//其他程序打开摄像头
        }
    }
    else if ((camera->status()==QCamera::Status::ActiveStatus))
    {
        HHT_LOG(EN_INFO, "Camera is occupied failed to open it. ");
        qDebug()<<"Camera is avaliable,start it and then stop it. ";
        camera->stop();
        camera->unload();
        delete camera;
        return 1;//摄像头可用
    }
    else if(camera->status()==QCamera::Status::UnavailableStatus)
    {
        qDebug()<<"Camera not found. ";
        HHT_LOG(EN_INFO, "Camera not found. ");
        camera->stop();
        camera->unload();
        delete camera;
        return -1;//摄像头未知原因不可用
    }
    return -2;

}

void MainDialog::init()
{
    ui->Trash->setAcceptDrops(true);
    ui->listWidget->setAcceptDrops(false);
    ui->widget->setAcceptDrops(true);
    this->setAcceptDrops(true);
    this->recvivedFailedOpenCom =false;
    this->isFailedOpenCOM = false;
    g_listWidget = ui->listWidget;
    m_widget   = ui->widget;
    g_TrashWidget = ui->Trash;
    upload =0;
    ui->BottomWidget->setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint |Qt::WindowStaysOnTopHint);//窗口无边框，置顶
#ifdef HHT_2ND_PROJECT_SUPPORT
    ui->checkButton->setVisible(false);//隐藏取消按钮
#else
    ui->closeButton->setVisible(false);
#endif
    ui->maxButton->setVisible(false);
    ui->minButton->setVisible(false);
    FontScaleRatio::Instance()->setGuiFont("Helvetica",12,ui->label);
    ui->Trash->setStyleSheet("image: url(:/Resource/icons/trash_close.png);");
    m_widget->installEventFilter(this);//groupBox过滤事件
    ui->groupWidget->setStyleSheet("background-image: url(:/Resource/images/Background.png);");
    //listwidget 样式
    QScrollBar *verticalScrollBar=new QScrollBar(this);
    verticalScrollBar->setStyleSheet("QScrollBar:vertical {"
                                     "background-color:transparent; "
                                     //                                     "border-image: url(:/Resource/images/vertical.png);"
                                     " width: 15px;"
                                     " margin: 0px 0 0px 0;"
                                     "border-radius: 6px;"
                                     "border: none;"
                                     "}"
                                     "QScrollBar::handle:vertical {"
                                     "background: rgb(255,255,255);"
                                     "min-height: 100px;"
                                     "min-width:10px;"
                                     "margin: 2 px 2 px 2px 2px;"
                                     "border-radius:6px;"
                                     "border: solid;"
                                     "}"
                                     "QScrollBar::add-line:vertical {"
                                     "height: 0px;"
                                     "subcontrol-position: bottom;"
                                     "subcontrol-origin: margin;"
                                     "}"
                                     " QScrollBar::sub-line:vertical {"
                                     "height: 1px;"
                                     "subcontrol-position: top;"
                                     "subcontrol-origin: margin;"
                                     "}"
                                     "QScrollBar::up-arrow:vertical, QScrollBar::down-arrow:vertical {"
                                     " border: 0px solid blue;"
                                     "width: 3px;"
                                     "height: 0px;"
                                     "}"
                                     "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
                                     "background: none;"
                                     "}"
                                     );
    ui->listWidget->setVerticalScrollBar(verticalScrollBar);

    if(g_fontPixelRatio>=3)//支持300%缩放比例
    {
        ui->listWidget->setStyleSheet("QListWidget{padding-left:30px;padding-top:5px; padding-bottom:15px; "
                                      "padding-right:15px;border-image: url(:/Resource/images/background_5.png);}"
                                      "QListWidget::Item{padding-top:12px; padding-bottom:10px; border:1px solid white; border-style:solid;border-radius:10px;}"
                                      "QListWidget::Item:hover{background:skyblue; }"
                                      "QListWidget::item:selected{color:white; border:2px solid white; border-style:solid;}"
                                      "QListWidget::item:selected:!active{border-width:2px; background-color:transparent }"
                                      );
    }
    else
    {
#if 0
        ui->listWidget->setStyleSheet("QListWidget{padding-left:30px;padding-top:5px; padding-bottom:15px; "
                                      "padding-right:15px;border-image: url(:/Resource/images/background_5.png);}"
                                      "QListWidget::Item{padding-top:7px; padding-bottom:10px; border:1px solid white; border-style:solid;border-radius:10px;}"
                                      "QListWidget::Item:hover{background:skyblue; }"
                                      "QListWidget::item:selected{color:white; border:2px solid white; border-style:solid;}"
                                      "QListWidget::item:selected:!active{border-width:2px; background-color:transparent }"
                                      );
#else
        ui->listWidget->setStyleSheet("QListWidget{padding-left:30px;padding-top:5px; padding-bottom:15px; "
                                      "padding-right:15px;border-image: url(:/Resource/images/background_5.png);}"
                                      "QListWidget::Item{padding-top:15px; padding-bottom:10px; border:1px solid white; border-style:solid;border-radius:10px;}"
                                      "QListWidget::Item:hover{background:skyblue; }"
                                      "QListWidget::item:selected{color:white; border:2px solid white; border-style:solid;}"
                                      "QListWidget::item:selected:!active{border-width:2px; background-color:transparent }"
                                      );
#endif
    }
    this->setWindowTitle(APPLICATION_NAME);//设置程序窗体标题,保障main.cpp中单利运行
    if(g_fontPixelRatio>=3)
    {
        this->setFixedSize(QSize(HHT_WIDTH+140,HHT_HEIGHT+160));
    }
    else
    {
        this->setFixedSize(QSize(HHT_WIDTH,HHT_HEIGHT));
    }
}

void MainDialog::WriteRecords()
{
    //    QString settingsPath = "Settings/Records.txt";
    QString settingsPath =PUB_SS.Pub_GetUserDataPath()+"/Records.txt";
    qDebug()<<"WriteRecords()------>"<<settingsPath;
    QFile file(settingsPath);
    if(!file.exists())
    {
        file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
        file.close();
    }
    else
    {
        bool result = file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
        if(result)
        {
            QTextCodec *code =QTextCodec::codecForName("UTF-8");
            QTextStream out(&file);
            out.setCodec(code);
            for(int i=0;i<g_appInfoVector.count();i++)
            {
                QString appinfo = "APP";
                appinfo.append(tr("[%1]=").arg(QString::number(i,10)));
                appinfo.append(tr("%1#").arg(g_appInfoVector.at(i)._fileName));
                appinfo.append(tr("%1").arg(g_appInfoVector.at(i)._lnkPath));
                out<<appinfo<<endl;
                qDebug()<<"FileName["<<i<<"] :"<<g_appInfoVector.at(i)._fileName;
                qDebug()<<"FilePath["<<i<<"] :"<<g_appInfoVector.at(i)._lnkPath;
                out.flush();
            }
            file.close();
        }
        else
        {
            file.close();
        }
    }
}

void MainDialog::ReadSettings()
{
    QString settingsPath = "Settings/Settings.ini";
    QFile file(settingsPath);
    if(!file.exists())
    {
        return;
    }
    QSettings settings(settingsPath,QSettings::IniFormat);
    QString color = settings.value("/SETTINGS/theme").toString();
    //    qDebug()<<"获取到设置颜色:"<<color;
    this->setStyleSheet(tr("background-color:%1").arg(color));
    //读取配置文件
}

void MainDialog::ReadRecords()
{
    QString settingsPath =PUB_SS.Pub_GetUserDataPath()+"/Records.txt";
    QFile file(settingsPath);
    if(!file.exists())
    {
        file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
        file.close();
        return;
    }
    bool result = file.open(QIODevice::ReadOnly);
    if(result)
    {
        QTextCodec *code =QTextCodec::codecForName("UTF-8");
        QTextStream in(&file);
        in.setCodec(code);
        while (!in.atEnd())
        {
            QString lineStr = in.readLine();
            if(!lineStr.contains("APP"))
            {
                ;
            }
            else
            {
                HHTAPPINFO hhtAppInfo;
                QStringList sections =lineStr.split(QRegExp("[=,#]"));
                QString fileName = sections.at(1);//获取名字
                QString lnkPath =sections.at(2);//获取路径
                hhtAppInfo._fileName = fileName;
                hhtAppInfo._lnkPath=lnkPath;
                //                QFileIconProvider icon_provider;
                //                hhtAppInfo._appIcon = icon_provider.icon(QFileInfo(lnkPath));
                QFileInfo fileInfo(lnkPath);
#ifdef HHT_SUPPORT_DIR
                if(fileInfo.isDir())
                {
                    QFileIconProvider icon_provider;
                    hhtAppInfo._appIcon = icon_provider.icon(fileInfo);
                }
#endif
                if(!fileInfo.isDir())
                {
                    QFileInfo file_info(lnkPath);//lnk or exe
                    if(file_info.filePath().contains(".lnk"))
                    {
                        if(file_info.symLinkTarget().isEmpty())
                        {
                            //UWP 快捷方式支持 2017/8/22
                            QFileIconProvider icon_provider;
                            QIcon icon= icon_provider.icon(file_info);
                            QPixmap pixmap = icon.pixmap(QSize(100,100));
                            hhtAppInfo._appIcon = QIcon(pixmap);
                            //==============================
                        }
                        else
                        {
                            hhtAppInfo._appIcon = QIcon(getMaxPixmap(file_info.symLinkTarget()));
                        }
                    }
                    else
                    {//exe
                        hhtAppInfo._appIcon = QIcon(getMaxPixmap(file_info.filePath()));
                    }
                }
                g_appInfoVector.append(hhtAppInfo);
                //==============加载APP======================
                QListWidgetItem *item = new QListWidgetItem();
                QFont font;
                font.setFamily("Helvetica");
                if(g_fontPixelRatio>=3)
                {
                    item->setSizeHint(QSize(150,150));
                }
                else
                {
                    item->setSizeHint(QSize(120,120));
                }
                font.setPointSize(qRound(11/g_fontPixelRatio));
                item->setFont(font);
                item->setTextColor(Qt::white);
                item->setTextAlignment(Qt::AlignVCenter|Qt::AlignHCenter);
                //----------------------------------------------------------------------------------
                item->setIcon(hhtAppInfo._appIcon);
                item->setText(hhtAppInfo._fileName);
                item->setToolTip(hhtAppInfo._lnkPath);
                ui->listWidget->addItem(item);
            }
            in.flush();
        }
        file.close();
    }
    else
    {
        file.close();
    }
}

QByteArray MainDialog::QIcon2QByteArray(QIcon icon)
{
    if(icon.isNull())
        return NULL;
    QList<QSize>sizes = icon.availableSizes();
    int maxinum = sizes[0].width();
    for(int i=1;i<sizes.size();++i)
    {
        maxinum = qMax(maxinum,sizes[i].width());
    }
    QPixmap pixmap = icon.pixmap(icon.actualSize(QSize(maxinum,maxinum)));//QIcon 转QPixmap
    //    QPixmap pixmap = icon.pixmap(QSize(32,32));//QIcon 转QPixmap
    QByteArray byteArray,hexByteArray;
    //    //方法一：
    //    QDataStream ds(&byteArray,QIODevice::WriteOnly);
    //    ds<<pixmap;
    //方法二:
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer,"PNG",20);
    //对数据进行压缩，读数据需要解压缩,下位机没有对应函数解压不了
    hexByteArray= qCompress(byteArray, 1).toHex(); // better just open file with QFile, load data, compress and toHex?
    return byteArray/*.toHex()*/;
}

int MainDialog::sendSingleAppInfoToAndroid(HHTAPPINFO hhtAppInfo)
{
    if(isFailedOpenCOM)
    {//打开串口失败
        HintDialog *hint = new HintDialog();
        hint->setMassage(tr("Serial port open failed .  "),-2);
        hint->resize(hint->width()-40,hint->height());
        hint->showParentCenter(this);
        hint->show();
        return  -1;
    }
    else
    {
        QList<QList<unsigned char>>listFileNameDataUCharList,listIconDataUCharList;
        QList<unsigned char> fileNameDataUChar,fileIconDataUChar;
        if(hhtAppInfo._fileName==APPLICATION_NAME)
        {//屏蔽向Android端发送自己信息
            //           直接添加APP不上传至newline assistant
            qDebug()<<"Prevent send "<<hhtAppInfo._fileName<<" to Android";
            HHT_LOG(EN_INFO, "Prevent send (%s) to Android",hhtAppInfo._fileName.toStdString().c_str());
            return 1;
        }
        else
        {
            QByteArray fileNameByteArray =hhtAppInfo._fileName.toUtf8();//utf8
            QByteArray iconDataByteArray = QIcon2QByteArray(hhtAppInfo._appIcon);//16进制
            //单个APP的数据
            MakeByte2UCharList(fileNameByteArray,fileNameDataUChar);
            MakeByte2UCharList(iconDataByteArray,fileIconDataUChar);

            listFileNameDataUCharList.append(fileNameDataUChar);
            listIconDataUCharList.append(fileIconDataUChar);

            OPS_SendFilesToAndroid(1, listFileNameDataUCharList,listIconDataUCharList);//发送1个APP数据至android
            UploadWidget *Upload= new UploadWidget();
            Upload->showParentCenter(this);
            Upload->exec();
            qDebug()<<"uploading moudle: upload app status: "<<g_uploadStatus;
            HHT_LOG(EN_INFO,"uploading moudle: upload app status:[%d]",g_uploadStatus);
            return 0;
        }
    }
    return -1;
}

QList<QString> MainDialog::enumUSBDevices()
{
    QList<QString> usb_device;
    HDEVINFO hDevInfo;
    SP_DEVICE_INTERFACE_DATA spDevData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pDetail;
    BOOL bRes = TRUE;
    int nCount = 0;
    hDevInfo = ::SetupDiGetClassDevs((LPGUID)&UsbClassGuid,NULL,NULL,DIGCF_PRESENT|DIGCF_INTERFACEDEVICE);
    if (hDevInfo != INVALID_HANDLE_VALUE)
    {
        pDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)::GlobalAlloc(LMEM_ZEROINIT,1024);
        pDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        while (bRes)
        {
            spDevData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
            bRes = ::SetupDiEnumDeviceInterfaces(hDevInfo,NULL,(LPGUID)&UsbClassGuid,nCount,&spDevData);
            if (bRes)
            {
                bRes = ::SetupDiGetInterfaceDeviceDetail(hDevInfo,&spDevData,pDetail,1024,NULL,NULL);
                if (bRes)
                {
                    wstring  szStr = pDetail->DevicePath;
#if 0
                    wcout<<nCount<<": "<<szStr<<endl;
                    //\\?\usb#vid_0e0f&pid_0001#5&106a1f17&0&2#{a5dcbf10-6530-11d2-901f-00c04fb951ed}
#endif
                    QString string = QString::fromStdWString(szStr);
                    QString device_str =string .mid(8,string.length()-39-8).toUpper();
                    usb_device.append(device_str);
                    nCount ++;
                }
            }
        }
        ::GlobalFree(pDetail);
        ::SetupDiDestroyDeviceInfoList(hDevInfo);
    }
#if 1
    for(int i=0;i<usb_device.size();i++)
    {
        qDebug()<<i<<usb_device.at(i);
    }
#endif
    return usb_device;
}

bool MainDialog::isPublicUSBStatus(QList<QString> deviceList)
{
    //\HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Enum\USB\VID_0951&PID_1666\60A44C3FAC2DBEC1A98A0014
    // "VID_0951&PID_1666#60A44C3FAC2DBEC1A98A0014"
    const QString rootUsbRegeditPath = "HKEY_LOCAL_MACHINE/SYSTEM/ControlSet001/Enum/USB/";
    if(deviceList.isEmpty())
    {
        return false;
    }
    for(int i=0;i<deviceList.size();i++)
    {
        QString string = deviceList.at(i);
        QString usbDevice=rootUsbRegeditPath+string.replace('#',"/");
        QString  usbDeviceReg = usbDevice.replace('/',"\\");
        qDebug()<<i<<"--------->"<<usbDeviceReg;
        QSettings reg(usbDeviceReg,QSettings::NativeFormat);
#if 0
        QStringList keyList = reg.childKeys();
        foreach (QString key, keyList)
        {
            QString location = reg.value(key).toString();
            qDebug()<<key<<"======"<<location;
        }
#endif
        QString usbService = reg.value("Service").toString();
        qDebug()<<"[Service]: "<<usbService;
        QString  usbLocationInfo = reg.value("LocationInformation").toString();
        qDebug()<<"[Location info]: "<<usbLocationInfo;
        if(usbService=="USBSTOR")//usb设备是存储设备
        {
            if(usbLocationInfo.contains("Hub_#0003")||usbLocationInfo.contains("Hub_#0004"))//usb存储设备挂在特定的HUSB上面
            {
                qDebug()<<"Public USB Port is busy now..... ";
                HHT_LOG(EN_INFO, "Public USB Port is busy now..... ");
                return true;
            }
        }
    }
    return false;
}

bool MainDialog::nativeEvent(const QByteArray &eventType, void *message, long *lResult)
{
    Q_UNUSED(eventType);
    Q_UNUSED(lResult);
    MSG *pMsg = reinterpret_cast<MSG*>(message);
    if (NULL == pMsg)
    {
        qDebug()<<"Msg is null";
    }
    if(pMsg->message==SHOWNORNAL)
    {//自定义Msg
        qDebug()<<"Msg: WM_SHOWNORNAL";
        //        this->trayiconActivated(QSystemTrayIcon::Trigger);
        this->resize(HHT_WIDTH,HHT_HEIGHT);
        this->showNormal();
        return true;
    }
    return false;
}

QPixmap MainDialog::getMaxPixmap(const QString sourceFile)
{
    QPixmap max;
    // ExtractIconEx 从限定的可执行文件、动态链接库（DLL）、或者图标文件中生成图标句柄数组
    const UINT iconCount = ExtractIconEx((wchar_t *)sourceFile.utf16(), -1, 0, 0, 0);
    //    qDebug()<<"Max icon count: "<<iconCount;
    if (!iconCount)
    {
        //没有图标exe文件是加载自定义默认icon
        max = QPixmap(":/Resource/icons/default.png");
        return max;
    }
    QScopedArrayPointer<HICON> icons(new HICON[iconCount]);
    ExtractIconEx((wchar_t *)sourceFile.utf16(), 0, icons.data(), 0, iconCount) ;//最大HICON
    max =QtWin::fromHICON(icons[0]);//获取第一个所谓最佳
    return max;
}

void MainDialog::mousePressEvent(QMouseEvent *event)
{
    this->windowPos = this->pos();
    this->mousePos = event->globalPos();
    this->dPos = mousePos - windowPos;
}

void MainDialog::mouseMoveEvent(QMouseEvent *event)
{
    this->move(event->globalPos() - this->dPos);
    if(upload!=0)
    {
        upload->showParentCenter(this);//上传界面一直停留再MainDialog中心位置
    }
}

void MainDialog::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void MainDialog::dropEvent(QDropEvent *event)
{
    // [[3]]: 当放操作发生后, 取得拖放的数据
    QList<QUrl>urls = event->mimeData()->urls();
    if (urls.isEmpty()) { return ; }
    QString path = urls.first().toLocalFile();
    qDebug()<<"[Path]:"<<path;
    if(!path.isEmpty())
    {
        foreach (QUrl url, urls)
        {
            QString fileName = url.toLocalFile();//文件名
            QString recodeAbsPath;//记录APP绝对路径
            QString recodeFileName;//记录APP名字
            QIcon    recodeFileIcon;//记录APP图标
            HHTAPPINFO hhtAppInfo;//HHT app信息
            QFileInfo file_info(fileName);

            if (file_info.isDir())
            {
#ifdef HHT_SUPPORT_DIR
                qDebug()<<"The drop content is dir";
                QIcon icon;
                QListWidgetItem *item = new QListWidgetItem();
                QFont font;
                font.setFamily("Helvetica");
                font.setPointSize(qRound(11/g_fontPixelRatio));
                item->setFont(font);
                item->setIcon(icon);
                if(g_fontPixelRatio>=3)
                {
                    item->setSizeHint(QSize(150,150));
                }
                else
                {
                    item->setSizeHint(QSize(120,120));
                }
                item->setTextColor(Qt::white);
                item->setTextAlignment(Qt::AlignVCenter|Qt::AlignHCenter);
                if(file_info.fileName().contains(".lnk"))
                {
                    QFileInfo fileInfo(file_info.symLinkTarget());
                    QFileIconProvider icon_provider;
                    icon = icon_provider.icon(fileInfo);
                    recodeFileIcon = icon;//图标
                    item->setIcon(icon);
                    item->setText(fileInfo.fileName());
                    recodeFileName =fileInfo.fileName();//名字
                    recodeAbsPath =fileInfo.filePath();//dir绝对路径
                    item->setToolTip(recodeAbsPath);
                }
                else
                {
                    QFileIconProvider icon_provider;
                    icon = icon_provider.icon(file_info);
                    recodeFileIcon = icon;//图标
                    item->setIcon(icon);
                    item->setText(file_info.fileName());
                    recodeFileName =file_info.fileName();//名字
                    recodeAbsPath =file_info.filePath();//dir绝对路径
                    item->setToolTip(recodeAbsPath);
                }
                //----------------------------------------------------------------------------------------------------------------------
                if(ui->listWidget->count()==0)
                {
                    hhtAppInfo._fileName = recodeFileName;
                    hhtAppInfo._lnkPath = recodeAbsPath;
                    hhtAppInfo._appIcon = recodeFileIcon;
#ifdef HHT_2ND_PROJECT_SUPPORT
                    int result = sendSingleAppInfoToAndroid(hhtAppInfo);
                    if(result ==0)
                    {
                        if(g_uploadStatus)
                        {
                            ui->listWidget->addItem(item);
                            g_appInfoVector.append(hhtAppInfo);
                            g_uploadStatus =false;
                        }
                    }
                    else if (result ==1)//newline assistant
                    {
                        ui->listWidget->addItem(item);
                        g_appInfoVector.append(hhtAppInfo);
                    }
#else
                    ui->listWidget->addItem(item);
                    g_appInfoVector.append(hhtAppInfo);
#endif
                    WriteRecords();
                }
                else
                {
                    bool isExist = false;
                    for(int i=0;i<ui->listWidget->count();i++)
                    {
                        if(ui->listWidget->item(i)->toolTip()==recodeAbsPath)
                        {
                            isExist = true;
                            break;
                        }
                    }
                    if(isExist)
                    {
                        HintDialog *hint = new HintDialog();
                        hint->setMassage(tr("The item is already in the list .  "),-1);
                        hint->showParentCenter(this);
                        hint->show();
                    }
                    else
                    {
                        hhtAppInfo._fileName = recodeFileName;
                        hhtAppInfo._lnkPath = recodeAbsPath;
                        hhtAppInfo._appIcon = recodeFileIcon;
#ifdef HHT_2ND_PROJECT_SUPPORT      
                        int result = sendSingleAppInfoToAndroid(hhtAppInfo);
                        if(result ==0)
                        {
                            if(g_uploadStatus)
                            {
                                ui->listWidget->addItem(item);
                                g_appInfoVector.append(hhtAppInfo);
                                g_uploadStatus =false;
                            }
                        }
                        else if (result ==1)//newline assistant
                        {
                            ui->listWidget->addItem(item);
                            g_appInfoVector.append(hhtAppInfo);
                        }
#else
                        ui->listWidget->addItem(item);
                        g_appInfoVector.append(hhtAppInfo);
#endif
                        WriteRecords();
                    }
                }
                //----------------------------------------------------------------------------------------------------------------------------
#else
                HintDialog *hint = new HintDialog();
                hint->setMassage(tr("Format not supported. You can add .exe and .lnk files only .  ").arg(file_info.fileName()),-2);
                hint->resize(hint->width()-60,hint->height());
                hint->showParentCenter(this);
                hint->show();
#endif
            }
            else if(file_info.fileName().contains(".exe")||file_info.fileName().contains(".lnk")||file_info.fileName().contains(".EXE"))
            {
                QIcon icon;
                QListWidgetItem *item = new QListWidgetItem();
                QFont font;
                font.setFamily("Helvetica");
                font.setPointSize(qRound(11/g_fontPixelRatio));
                item->setFont(font);
                item->setIcon(icon);
                if(g_fontPixelRatio>=3)
                {
                    item->setSizeHint(QSize(150,150));
                }
                else
                {
                    item->setSizeHint(QSize(120,120));
                }
                item->setTextColor(Qt::white);
                item->setTextAlignment(Qt::AlignVCenter|Qt::AlignHCenter);
                if(file_info.fileName().contains(".exe")||file_info.fileName().contains(".EXE"))
                {//获取原文件exe绝对路径
                    // QFileIconProvider icon_provider;
                    // icon = icon_provider.icon(file_info);
                    icon =QIcon(getMaxPixmap(file_info.filePath()));
                    recodeFileIcon = icon;//图标
                    item->setIcon(icon);
                    item->setText(file_info.completeBaseName());
                    recodeFileName = file_info.completeBaseName();
                    //修改为拖入文件路径而非目标路径
                    qDebug()<<"EXE====>filePath: "<<file_info.filePath();
                    recodeAbsPath = file_info.filePath();
                    item->setToolTip(recodeAbsPath);
                }
                else if (file_info.fileName().contains(".lnk"))
                {//获取lnk对应的exe绝对路径
                    qDebug()<<"LNK.====>filePath: "<<file_info.filePath();
                    QString target = file_info.symLinkTarget();
                    qDebug()<<"------>"<<target;
                    // 支持桌面UWP快捷方式软件 2017/8/22添加
                    if(target.isEmpty()){
                        qDebug()<<"LNK.====>UWP type file lnk";
                        QFileIconProvider icon_provider;
                        QIcon icon= icon_provider.icon(file_info);
                        QPixmap pixmap = icon.pixmap(QSize(100,100));
                        QIcon appIcon=QIcon(pixmap);
                        recodeFileIcon = appIcon;
                        item->setIcon(appIcon);
                        recodeFileName = file_info.completeBaseName();
                        item->setText(file_info.completeBaseName());
                        recodeAbsPath =file_info.filePath();//lnkpath
                        item->setToolTip(file_info.filePath());
                    }
                    // =====================================
#ifdef  HHT_SUPPORT_DIR
                    QFileInfo fileInfo(target);
                    if(fileInfo.isDir())
                    {//快捷方式目标文件
                        //文件夹
                        QFileIconProvider icon_provider;
                        icon = icon_provider.icon(QFileInfo(target));
                        recodeFileIcon = icon;//图标
                        item->setIcon(icon);
                        QFileInfo targetDir = QFileInfo(target);
                        item->setText(targetDir.fileName());
                        recodeFileName =targetDir.fileName();//名字
                        recodeAbsPath =target;//dir绝对路径
                        item->setToolTip(recodeAbsPath);
                    }
#endif
                    if (target.contains(".exe")||target.contains(".EXE"))
                    {
                        qDebug()<<"LNK 2 =====> "<<file_info.filePath();
                        // QFileIconProvider icon_provider;
                        // icon = icon_provider.icon(QFileInfo(file_info.symLinkTarget()));
                        icon =QIcon(getMaxPixmap(file_info.symLinkTarget()));
                        recodeFileIcon = icon;//图标
                        item->setIcon(icon);
                        item->setText(file_info.completeBaseName());
                        recodeFileName = file_info.completeBaseName();
                        recodeAbsPath = file_info.filePath();//lnk绝对路径
                        item->setToolTip(recodeAbsPath);
                    }
                }
                //----------------------------------------------------------------------------------------------------------------------
                if(ui->listWidget->count()==0)
                {
                    hhtAppInfo._fileName = recodeFileName;
                    hhtAppInfo._lnkPath = recodeAbsPath;
                    hhtAppInfo._appIcon = recodeFileIcon;
#ifdef HHT_2ND_PROJECT_SUPPORT
                    int result = sendSingleAppInfoToAndroid(hhtAppInfo);
                    if(result ==0)
                    {
                        if(g_uploadStatus)
                        {
                            ui->listWidget->addItem(item);
                            g_appInfoVector.append(hhtAppInfo);
                            g_uploadStatus =false;
                        }
                    }
                    else if (result ==1)//newline assistant
                    {
                        ui->listWidget->addItem(item);
                        g_appInfoVector.append(hhtAppInfo);
                    }
#else
                    ui->listWidget->addItem(item);
                    g_appInfoVector.append(hhtAppInfo);
#endif
                    WriteRecords();
                }
                else
                {
                    bool isExist = false;
                    for(int i=0;i<ui->listWidget->count();i++)
                    {
                        if(ui->listWidget->item(i)->toolTip()==recodeAbsPath)
                        {
                            isExist = true;
                            break;
                        }
                    }
                    if(isExist)
                    {
                        HintDialog *hint = new HintDialog();
                        hint->setMassage(tr("The item is already in the list .  "),-1);
                        hint->showParentCenter(this);
                        hint->show();
                    }
                    else
                    {
                        hhtAppInfo._fileName = recodeFileName;
                        hhtAppInfo._lnkPath = recodeAbsPath;
                        hhtAppInfo._appIcon = recodeFileIcon;
#ifdef HHT_2ND_PROJECT_SUPPORT
                        int result = sendSingleAppInfoToAndroid(hhtAppInfo);
                        if(result ==0)
                        {
                            if(g_uploadStatus)
                            {
                                ui->listWidget->addItem(item);
                                g_appInfoVector.append(hhtAppInfo);
                                g_uploadStatus =false;
                            }
                        }
                        else if (result ==1)//newline assistant
                        {
                            ui->listWidget->addItem(item);
                            g_appInfoVector.append(hhtAppInfo);
                        }
#else
                        ui->listWidget->addItem(item);
                        g_appInfoVector.append(hhtAppInfo);
#endif
                        WriteRecords();
                    }
                }
                //----------------------------------------------------------------------------------------------------------------------------
            }
            else
            {
                HintDialog *hint = new HintDialog();
#ifdef  HHT_SUPPORT_DIR
                hint->setMassage(tr("Format not supported. You can add .exe and file folder only .  ").arg(file_info.fileName()),-2);
#else
                hint->setMassage(tr("Format not supported. You can add .exe and .lnk files only .  ").arg(file_info.fileName()),-2);
#endif
                hint->resize(hint->width()-60,hint->height());
                hint->showParentCenter(this);
                hint->show();
            }
        }
    }//
    ui->listWidget->setCurrentRow(ui->listWidget->count()-1);//自动滚动到最底层
}

void MainDialog::slot_RS232isDisabled()
{
    if(!this->recvivedFailedOpenCom)
    {//仅仅弹窗一次
        HintDialog *hint = new HintDialog();
        hint->setMassage(tr(" Serial port open failed .  "),-2);
        hint->showParentCenter(this);
        this->showNormal();
        hint->show();
    }
    this->recvivedFailedOpenCom = true;
    this->isFailedOpenCOM = true;
    g_SerialStatus = false;
}

void MainDialog::slot_TrashOpenCOMFailed()
{
    HintDialog *hint = new HintDialog();
    hint->setMassage(tr(" Serial port open failed .  "),-2);
    hint->showParentCenter(this);
    hint->show();
}

void MainDialog::slot_RS232isAvaliable()
{
    this->isFailedOpenCOM = false;
    g_SerialStatus = true;
}

void MainDialog::slot_openProcFromFileName(QString fileName)
{

#if 1
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));//中文乱码
    HHT_LOG(EN_INFO, "---Android commands to open program: (%s)",fileName.toLocal8Bit().data());
    if(fileName!=NULL)
    {
        if(fileName==APPLICATION_NAME)
        {//Android want to open the "Newline assistant"
            HHT_LOG(EN_INFO,"Android try to open: (%s)",fileName.toLocal8Bit().data());
            this->trayiconActivated(QSystemTrayIcon::Trigger);//不打开自己，直接show出自己
            //       this->show();//不打开自己，直接show出自己
        }
        else if (fileName==MONTAGE)
        {//处理第三方软件Montage
            bool isMontage = false;
            //查询列表，没有Montage就弹出窗口提示
            for(int i=0;i<g_appInfoVector.count();i++)
            {
                if(g_appInfoVector.at(i)._fileName==MONTAGE)
                {
                    isMontage =true;
                    LPCWSTR program = (LPCWSTR)g_appInfoVector.at(i)._lnkPath.utf16();
                    HINSTANCE hInstance;
                    //使用ShellExecute
                    hInstance = ShellExecute(NULL,NULL,program,NULL,NULL,SW_NORMAL);
                }
            }
            if(fileName == MONTAGE&&isMontage==false)
            {
                //弹窗提示
                montageDialog->showParentCenter(this);
                montageDialog->show();
            }
        }

#if  HHT_GERMANY  德国固件支持
        else if (fileName.contains("Germany#"))//德国xxx固件home键和return键,音量+-键
        {
            const QString IE_HOME= "C:/Program Files/Internet Explorer/iexplore.exe";
            HHT_LOG(EN_INFO, "---Android commands on Germany firwmare. ");
            if(fileName =="Germany#IE-home")//home键
            {
                HHT_LOG(EN_INFO, "---Android commands to open IE-HOME");
                const QString IEWInClass  = "IEFrame";
                const  wchar_t *WInClass = reinterpret_cast<const wchar_t*>(IEWInClass.utf16());
                HWND hWnd = FindWindow(WInClass,NULL);
                qDebug()<<"IE hwnd: "<<hWnd;
                if(hWnd)
                {
                    HHT_LOG(EN_INFO, "---Find IE hWnd: success(home)");
                    PostMessage(hWnd,WM_SYSCOMMAND ,SC_MAXIMIZE,0);
                    keybd_event(VK_MENU, 0, 0, 0);
                    //qDebug()<<"==(ALT)Down status: "<<GetKeyState(VK_MENU);

                    PostMessage(hWnd,WM_SYSKEYDOWN,VK_HOME,1<<29);//
                    //qDebug()<<"==(HOME)Down status: "<<GetKeyState(VK_HOME);

                    Sleep(100);
                    keybd_event(VK_MENU,0,KEYEVENTF_KEYUP, 0);
                    //qDebug()<<"==(ALT)Up status: "<<GetKeyState(VK_MENU);

                    PostMessage(hWnd,WM_SYSKEYUP,VK_HOME,0);//
                    //qDebug()<<"==(HOME)Up status: "<<GetKeyState(VK_HOME);
                }
                else
                {
                    LPCWSTR program = (LPCWSTR)IE_HOME.utf16();
                    HINSTANCE hInstance;
                    //使用ShellExecute
                    hInstance = ShellExecute(NULL,NULL,program,NULL,NULL,SW_MAXIMIZE);
                }
            }
            else if (fileName=="Germany#IE-back")//back键
            {
                HHT_LOG(EN_INFO, "---Android commands to open IE-BACK");
                const QString IEWInClass  = "IEFrame";
                const  wchar_t *WInClass = reinterpret_cast<const wchar_t*>(IEWInClass.utf16());
                HWND hWnd = FindWindow(WInClass,NULL);
                qDebug()<<"IE hwnd: "<<hWnd;
                if(hWnd)
                {
                    HHT_LOG(EN_INFO, "---Find IE hWnd: success(back)");
                    PostMessage(hWnd,WM_SYSCOMMAND ,SC_MAXIMIZE,0);
                    keybd_event(VK_MENU, 0, 0, 0);
                    //qDebug()<<"==(ALT)Down status: "<<GetKeyState(VK_MENU);

                    PostMessage(hWnd,WM_SYSKEYDOWN,VK_LEFT,1<<29);//
                    //qDebug()<<"==(Left) down status: "<<GetKeyState(VK_LEFT);

                    Sleep(100);
                    keybd_event(VK_MENU,0,KEYEVENTF_KEYUP, 0);
                    //qDebug()<<"==(ALT)Up status: "<<GetKeyState(VK_MENU);

                    PostMessage(hWnd,WM_SYSKEYUP,VK_LEFT,0);//
                    //qDebug()<<"==(Left) up status: "<<GetKeyState(VK_LEFT);
                }
                else
                {
                    HHT_LOG(EN_INFO, "---Find IE hWnd: failed(back)");
                }
            }
            else if (fileName=="Germany#volume-plus")//音量加
            {
                keybd_event(VK_VOLUME_UP,MapVirtualKey(VK_VOLUME_UP,0),KEYEVENTF_EXTENDEDKEY,0);
                keybd_event(VK_VOLUME_UP,MapVirtualKey(VK_VOLUME_UP,0),KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,0);
            }
            else if (fileName=="Germany#volume-minus")//音量减
            {
                keybd_event(VK_VOLUME_DOWN,MapVirtualKey(VK_VOLUME_DOWN,0),KEYEVENTF_EXTENDEDKEY,0);
                keybd_event(VK_VOLUME_DOWN,MapVirtualKey(VK_VOLUME_DOWN,0),KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,0);
            }
        }
#endif
        else
        {
            HHT_LOG(EN_INFO, "---Android commands to open fileName[QString] : (%s)", fileName.toLocal8Bit().data());
            for(int  i=0;i<g_appInfoVector.count();i++)
            {
                HHT_LOG(EN_INFO, "---Android commands to open fileName[%d] : (%s)",i, g_appInfoVector.at(i)._fileName.toLocal8Bit().data());
                if(g_appInfoVector.at(i)._fileName==fileName)
                {
                    HHT_LOG(EN_INFO,"g_appInfoVector.at(%d)._fileName(%s)=?=fileName(%s) .",i,g_appInfoVector.at(i)._fileName.toLocal8Bit().data(),fileName.toLocal8Bit().data());
                    LPCWSTR program = (LPCWSTR)g_appInfoVector.at(i)._lnkPath.utf16();
                    HINSTANCE hInstance;
                    //使用ShellExecute
                    //直接调用Windows api执行程序可以避免路径空格不识别从而执行程序失败
                    //hInstance = ShellExecute(NULL,NULL,program,NULL,NULL,SW_MAXIMIZE);
                    hInstance = ShellExecute(NULL,NULL,program,NULL,NULL,SW_NORMAL);
                    if(hInstance!=NULL)
                    {
                        HHT_LOG(EN_INFO,"Android try to open: (%s) success .",fileName.toLocal8Bit().data());
                    }
                    else
                    {
                        HintDialog *hint = new HintDialog();
                        hint->setMassage(tr("Excute %1 failed .  ").arg(fileName),-2);
                        hint->showParentCenter(this);
                        this->showNormal();
                        hint->show();
                    }
                }
            }
        }
    }
    else
    {
        HintDialog *hint = new HintDialog();
        hint->setMassage(tr("Receive empty commands from smart system .  "),-2);
        hint->showParentCenter(this);
        this->showNormal();
        hint->show();
    }
#else
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));//中文乱码
    HHT_LOG(EN_INFO, "---Android commands to open program: (%s)",fileName.toLocal8Bit().data());
    if(fileName!=NULL)
    {
        if(fileName==APPLICATION_NAME)
        {//Android want to open the "Newline assistant"
            HHT_LOG(EN_INFO,"Android try to open: (%s)",fileName.toLocal8Bit().data());
            this->trayiconActivated(QSystemTrayIcon::Trigger);//不打开自己，直接show出自己
            //       this->show();//不打开自己，直接show出自己
        }
        else if (fileName==MONTAGE)
        {//处理第三方软件Montage
            bool isMontage = false;
            //查询列表，没有Montage就弹出窗口提示
            for(int i=0;i<g_listWidget->count();i++)
            {
                if(g_listWidget->item(i)->text()==MONTAGE)
                {
                    isMontage =true;
                    LPCWSTR program = (LPCWSTR)g_listWidget->item(i)->toolTip().utf16();
                    HINSTANCE hInstance;
                    //使用ShellExecute
                    hInstance = ShellExecute(NULL,NULL,program,NULL,NULL,SW_NORMAL);
                }
            }
            if(fileName == MONTAGE&&isMontage==false)
            {
                //弹窗提示
                montageDialog->showParentCenter(this);
                montageDialog->show();
            }
        }
#if  1  //德国固件支持
        else if (fileName.contains("Germany#"))//德国xxx固件home键和return键,音量加减键
        {
            const QString IE_HOME= "C:/Program Files/Internet Explorer/iexplore.exe";
            HHT_LOG(EN_INFO, "---Android commands on Germany firwmare. ");
            if(fileName =="Germany#IE-home")//home键 alt+home
            {
                HHT_LOG(EN_INFO, "---Android commands to open IE-HOME");
                const QString IEWInClass  = "IEFrame";
                const  wchar_t *WInClass = reinterpret_cast<const wchar_t*>(IEWInClass.utf16());
                HWND hWnd = FindWindow(WInClass,NULL);
                qDebug()<<"IE hwnd: "<<hWnd;
                if(hWnd)
                {
                    HHT_LOG(EN_INFO, "---Find IE hWnd: success(home)");
                    PostMessage(hWnd,WM_SYSCOMMAND ,SC_MAXIMIZE,0);
                    keybd_event(VK_MENU, 0, 0, 0);
                    qDebug()<<"==(ALT)Down status: "<<GetKeyState(VK_MENU);

                    PostMessage(hWnd,WM_SYSKEYDOWN,VK_HOME,1<<29);//
                    qDebug()<<"==(HOME)Down status: "<<GetKeyState(VK_HOME);

                    Sleep(100);
                    keybd_event(VK_MENU,0,KEYEVENTF_KEYUP, 0);
                    qDebug()<<"==(ALT)Up status: "<<GetKeyState(VK_MENU);

                    PostMessage(hWnd,WM_SYSKEYUP,VK_HOME,0);//
                    qDebug()<<"==(HOME)Up status: "<<GetKeyState(VK_HOME);
                }
                else
                {
                    LPCWSTR program = (LPCWSTR)IE_HOME.utf16();
                    HINSTANCE hInstance;
                    //使用ShellExecute
                    hInstance = ShellExecute(NULL,NULL,program,NULL,NULL,SW_MAXIMIZE);
                }
            }
            else if (fileName=="Germany#IE-back")//back键 alt+left
            {
                HHT_LOG(EN_INFO, "---Android commands to open IE-BACK");
                const QString IEWInClass  = "IEFrame";
                const  wchar_t *WInClass = reinterpret_cast<const wchar_t*>(IEWInClass.utf16());
                HWND hWnd = FindWindow(WInClass,NULL);
                qDebug()<<"IE hwnd: "<<hWnd;
                if(hWnd)
                {
                    HHT_LOG(EN_INFO, "---Find IE hWnd: success(back)");
                    PostMessage(hWnd,WM_SYSCOMMAND ,SC_MAXIMIZE,0);
                    keybd_event(VK_MENU, 0, 0, 0);
                    qDebug()<<"==(ALT)Down status: "<<GetKeyState(VK_MENU);

                    PostMessage(hWnd,WM_SYSKEYDOWN,VK_LEFT,1<<29);//
                    qDebug()<<"==(Left) down status: "<<GetKeyState(VK_LEFT);

                    Sleep(100);
                    keybd_event(VK_MENU,0,KEYEVENTF_KEYUP, 0);
                    qDebug()<<"==(ALT)Up status: "<<GetKeyState(VK_MENU);

                    PostMessage(hWnd,WM_SYSKEYUP,VK_LEFT,0);//
                    qDebug()<<"==(Left) up status: "<<GetKeyState(VK_LEFT);
                }
                else
                {
                    HHT_LOG(EN_INFO, "---Find IE hWnd: failed(back)");
                }
            }
            else if (fileName=="Germany#volume-plus")//音量加+
            {
                keybd_event(VK_VOLUME_UP,MapVirtualKey(VK_VOLUME_UP,0),KEYEVENTF_EXTENDEDKEY,0);
                keybd_event(VK_VOLUME_UP,MapVirtualKey(VK_VOLUME_UP,0),KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,0);
            }
            else if (fileName=="Germany#volume-minus")//音量减-
            {
                keybd_event(VK_VOLUME_DOWN,MapVirtualKey(VK_VOLUME_DOWN,0),KEYEVENTF_EXTENDEDKEY,0);
                keybd_event(VK_VOLUME_DOWN,MapVirtualKey(VK_VOLUME_DOWN,0),KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,0);
            }
        }
    }
#endif
    else
    {
        //            HHT_LOG(EN_INFO, "---Android commands to open fileName[QString] : (%s)", fileName.toLocal8Bit().data());
        for(int  i=0;i<g_listWidget->count();i++)
        {
            HHT_LOG(EN_INFO, "---Android commands to open fileName[%d] : (%s)",i, g_listWidget->item(i)->text().toLocal8Bit().data());
            if(g_listWidget->item(i)->text()==fileName)
            {
                HHT_LOG(EN_INFO,"g_listWidget.item(%d)._fileName(%s)=?=fileName(%s) .",i,g_listWidget->item(i)->text().toLocal8Bit().data(),fileName.toLocal8Bit().data());;
                LPCWSTR program = (LPCWSTR)g_listWidget->item(i)->toolTip().utf16();
                HINSTANCE hInstance;
                //使用ShellExecute
                //直接调用Windows api执行程序可以避免路径空格不识别从而执行程序失败
                //hInstance = ShellExecute(NULL,NULL,program,NULL,NULL,SW_MAXIMIZE);
                hInstance = ShellExecute(NULL,NULL,program,NULL,NULL,SW_NORMAL);
                if(hInstance)
                {
                    HHT_LOG(EN_INFO,"Android try to open: (%s) success .",fileName.toLocal8Bit().data());;
                }
                else
                {
                    HintDialog *hint = new HintDialog();
                    hint->setMassage(tr("Excute %1 failed .  ").arg(fileName),-2);
                    hint->showParentCenter(this);
                    this->showNormal();
                    hint->show();
                }
            }
        }
    }
}
else
{
HintDialog *hint = new HintDialog();
hint->setMassage(tr("Receive empty commands from smart system .  "),-2);
hint->showParentCenter(this);
this->showNormal();
hint->show();
}
#endif
}

void MainDialog::slot_deleteAppFromVector(QString appName)
{
    //    HHT_LOG(EN_INFO,"[%s] IS GOING TO DELETE",appName.toStdString().c_str());
#ifdef HHT_2ND_PROJECT_SUPPORT
    if(g_SerialStatus)//串口存在
    {
        //向Android写删除指令
        g_tryDeleteAppName = appName;
        g_nWriteDeleteSingleAppCommandsFlag = 1;// OPS_DeleteSingleAppFromAndroid(appName);
        hhtHelper::Sleep(500);
        if(g_appInfoVector.count()>0)
        {
            for(int i=0;i<g_appInfoVector.count();i++)
            {
                if(g_appInfoVector.at(i)._fileName==appName)
                {
                    if(g_nDeleteSingleAppFlag==1)//删除标识
                    {
                        delete g_listWidget->currentItem();
                        g_appInfoVector.remove(i);
                        WriteRecords();
                        g_nDeleteSingleAppFlag = 0;//删除完复位删除标识
                    }
                    else
                    {
                        slot_deleteAppFromVectorFailed();
                        qDebug()<<g_listWidget->currentItem()->text()<<": [selected to delete failed.]";
                    }
                }
            }
        }
    }
    else
    {
        HintDialog *hint = new HintDialog();
        hint->setMassage(tr("serial port open failed .  "),-2);
        hint->resize(hint->width()-40,hint->height());
        hint->showParentCenter(this);
        hint->show();
        qDebug()<<"serial port open failed .";
    }
#else
    //    HHT_LOG(EN_INFO,"[%s] IS GOING TO DELETE",appName.toStdString().c_str());
    if(g_appInfoVector.count()>0)
    {
        for(int i=0;i<g_appInfoVector.count();i++)
        {
            if(g_appInfoVector.at(i)._fileName==appName)
            {
                g_appInfoVector.remove(i);
            }
        }
    }
#endif
}

//删除APP同步Android失败槽函数
void MainDialog::slot_deleteAppFromVectorFailed()
{
    HintDialog *hint = new HintDialog();
    hint->setMassage(tr("Delete app and sync with smart system failed .  "),-2);
    hint->resize(hint->width()-40,hint->height());
    hint->showParentCenter(this);
    hint->show();
}

void MainDialog::trayiconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason){
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        //单击托盘图标
        //双击托盘图标
    {
        this->setVisible(true);
        this->resize(HHT_WIDTH,HHT_HEIGHT);
        this->showNormal();
    }
        break;
    default:
        break;
    }
}

//同步APP
void MainDialog::slot_syncAction()
{
    this->showNormal();
    g_nClearAllAppCommandsFlag=1; //同步APP时先发送清除Android  App指令
    hhtHelper::Sleep(100);//睡眠100ms
    on_checkButton_clicked();
}

//清除APP
void MainDialog::slot_clearAction()
{
    qDebug()<<Q_FUNC_INFO;
    this->showNormal();
#ifdef HHT_2ND_PROJECT_SUPPORT
    if(g_SerialStatus)//串口存在
    {
        if(g_listWidget->count()>0)
        {
            //向Android写清除指令
            g_nClearAllAppCommandsFlag =1;
        }
    }
    else
    {
        HintDialog *hint = new HintDialog();
        hint->setMassage(tr("serial port open failed .  "),-2);
        hint->resize(hint->width()-40,hint->height());
        hint->showParentCenter(this);
        hint->show();
        qDebug()<<"serial port open failed .";
    }
#endif
}

void MainDialog::slot_quitAction(){
    qDebug()<<"quit signal";
    WriteRecords();
    if(!g_appInfoVector.isEmpty())
    {
        g_appInfoVector.clear();
    }
    done(1);
    this->close();
}

void MainDialog::slot_showMainDialog()
{
    this->trayiconActivated(QSystemTrayIcon::Trigger);
}

//摄像头启用指令响应
void MainDialog::slot_cameraStatusCheck()
{
    int result = isSysCameraStatus();
    if(result == 1)
    {//可用
        g_nCameraCommandsFlag =1;
        qDebug()<<"Camera write commands flag: "<<g_nCameraCommandsFlag;
        HHT_LOG(EN_INFO, " --------CAMERA IS AVALIABLE,TRY TO OPEN IT. -------->");
    }
    else if(result ==2)
    {//被Montage占用摄像头可用状态
        g_nCameraCommandsFlag =1;
        qDebug()<<"Camera write commands flag: "<<g_nCameraCommandsFlag;
        HHT_LOG(EN_INFO, " --------CAMERA IS OCUPPIED BY MONTAGE . -------->");
    }
    else if(result == 0)
    {
        HintDialog *cameraDialog = new HintDialog();
        cameraDialog->setMassage(tr("UC program is currently running.\n For changing the camera please stop the UC program first .  "),-2);
        cameraDialog->setCancelButtonEnable(false);
        cameraDialog->resize(QSize(215,cameraDialog->height()));
        cameraDialog->setGeometry((this->pos().x()+this->width()/2)- (cameraDialog ->width()/2)-180,
                                  (this->pos().y()+this->height()/2)-(cameraDialog->height()/2),
                                  cameraDialog->width(),cameraDialog->height());
        this->showNormal();
        g_nCameraCommandsFlag = -1;//摄像头被占用
        cameraDialog->exec();
        qDebug()<<"Camera write commands flag: "<<g_nCameraCommandsFlag;
        HHT_LOG(EN_INFO, " --------CAMERA IS OCCUPIED BY OTHER PROGRAM. -------->");
    }
    else if(result==-1)
    {//摄像头不存在 不可用
        /*
        HintDialog *cameraDialog = new HintDialog();
        cameraDialog->setMassage(tr("Camera is not found.\n For changing the camera please make sure camera is exist first .  "),-2);
        cameraDialog->setCancelButtonEnable(false);
        cameraDialog->resize(QSize(215,cameraDialog->height()));
        cameraDialog->setGeometry((this->pos().x()+this->width()/2)- (cameraDialog ->width()/2)-210,
                                  (this->pos().y()+this->height()/2)-(cameraDialog->height()/2),
                                  cameraDialog->width(),cameraDialog->height());
        this->showNormal();
        */
        g_nCameraCommandsFlag = 1;//摄像头不存在 不可用 #强制向Android发送可以切换状态
        //        cameraDialog->exec();
        qDebug()<<"Camera write commands flag: "<<g_nCameraCommandsFlag;
        HHT_LOG(EN_INFO, " --------CAMERA IS NOT FOUND. -------->");
    }
}

void MainDialog::slot_pubUsbStatusCheck()
{
    bool result = isPublicUSBStatus(enumUSBDevices());//
    HHT_LOG(EN_INFO,"===>public status:[%d]",result);
#if 0
    if(result)
    {
        this->showNormal();
        PubUSBDialog *pubUSB = new PubUSBDialog();
        pubUSB->showParentCenter(this);
        if(pubUSB->exec()==QDialog::Accepted)
        {
            HHT_LOG(EN_INFO,"===>usb operstation status:[%d]",pubUSB->get_operation_status());
            //if(pubUSB->get_operation_status() ==1)
            {
                g_nPubUSBCommandsFlag =1;
                qDebug()<<"Switch : Permission allowed...";
                HHT_LOG(EN_INFO, "Switch : Permission allowed...");
            }
        }
        else {
            //else if(pubUSB->get_operation_status() ==0)
            {
                g_nPubUSBCommandsFlag =0;
                qDebug()<<"Switch : Permission denied...";
                HHT_LOG(EN_INFO, "Switch : Permission denied...");
            }
        }
    }
    g_PubUsbOpertationStatus = -1;
#else
        if(result)//USB已经挂载
        {
                g_nPubUSBCommandsFlag =1;
                qDebug()<<"Switch : Permission allowed flags...";
                HHT_LOG(EN_INFO, "Switch : Permission allowed flags...");
        }
        else
        {
                g_nPubUSBCommandsFlag =0;
                qDebug()<<"Switch : Permission denied flags...";
                HHT_LOG(EN_INFO, "Switch : Permission denied flags...");
        }
#endif
}

void MainDialog::slot_clearAllApp()
{
    if(g_listWidget>0)
    {
        g_appInfoVector.clear();
        g_listWidget->clear();
        WriteRecords();
    }
}

//全部同步按钮，屏蔽该按钮实现，其他函数可调用该接口实现全部APP上传
void MainDialog::on_checkButton_clicked()
{
    QList<QList<unsigned char>>listAllFilesNameDataUCharList,listAllFilesDataUCharList;
    //写配置文件
    WriteRecords();
    if(isFailedOpenCOM)
    {//打开串口失败
        HintDialog *hint = new HintDialog();
        //         hint->showParentCenter(this);
        hint->setGeometry((this->pos().x()+this->width()/2)- (hint->width()/3),
                          (this->pos().y()+this->height()/2)-(hint->height()/2),
                          hint->width(),hint->height());
        hint->setMassage(tr("Serial port open failed .  "),-2);
        hint->show();
    }
    else
    {//打开串口成功
        int aviliableAppNum=0;//实质发送apps数目
        if(g_listWidget->count()>0)
        {
            for(int i=0;i<g_appInfoVector.count();i++)
            {
                //         MakeByte2UCharList
                QList<unsigned char> fileNameDataUChar,iconDataUChar;
                //         QByteArray fileNameByteArray,iconDataByteArray;
                if(g_appInfoVector.at(i)._fileName==APPLICATION_NAME)
                {//屏蔽向Android端发送自己信息
                    qDebug()<<"Prevent send "<<g_appInfoVector.at(i)._fileName<<" to Android";
                    HHT_LOG(EN_INFO, "Prevent send (%s) to Android", g_appInfoVector.at(i)._fileName.toStdString().c_str());
                }
                else
                {
                    QByteArray fileNameByteArray =g_appInfoVector.at(i)._fileName.toUtf8();//utf8
                    QByteArray iconDataByteArray = QIcon2QByteArray(g_appInfoVector.at(i)._appIcon);//16进制
                    //单个APP的数据
                    MakeByte2UCharList(fileNameByteArray,fileNameDataUChar);
                    MakeByte2UCharList(iconDataByteArray,iconDataUChar);
                    //组合所有数据
                    listAllFilesNameDataUCharList.append(fileNameDataUChar);
                    listAllFilesDataUCharList.append(iconDataUChar);
                    qDebug()<<"---->fileName(Asic): "<<fileNameByteArray;
                    qDebug()<<"---->fileName(Hex): "<<fileNameByteArray.toHex();
                    HHT_LOG(EN_INFO, "  SEND APP INFO (%s)", fileNameByteArray.data());
                    aviliableAppNum = listAllFilesNameDataUCharList.count();//获取实质上向android发送的APP数目，除去本身
                }
            }
            qDebug()<<"[send "<<aviliableAppNum<<"apps to Android]";
            HHT_LOG(EN_INFO, "  [Send %d apps to Android .]", aviliableAppNum);
            if(aviliableAppNum!=0)
            {
                OPS_SendFilesToAndroid(aviliableAppNum, listAllFilesNameDataUCharList, listAllFilesDataUCharList);
                UploadWidget *Upload= new UploadWidget();
                Upload->showParentCenter(this);
                Upload->show();
            }
        }
    }
}

//隐藏至托盘
void MainDialog::on_trayButton_clicked()
{
    if(!g_appInfoVector.isEmpty())
    {
        WriteRecords();
    }
    this->showMinimized();
    this->hide();
}

void MainDialog::on_aboutButton_clicked()
{
    AboutDialog *about = new AboutDialog();
    about->showParentCenter(this);
    about->show();
}
//已经屏蔽该按钮
void MainDialog::on_minButton_clicked()
{
    if(!g_appInfoVector.isEmpty())
    {
        WriteRecords();
    }
    this->showMinimized();
}

//同步上传，调用on_checkButton_clicked()接口
void MainDialog::on_syncButton_clicked()
{
    on_checkButton_clicked();//全部上传同步
}

void MainDialog::on_closeButton_clicked()
{
    if(!g_appInfoVector.isEmpty())
    {
        WriteRecords();
    }
    this->showMinimized();
    this->hide();
}
