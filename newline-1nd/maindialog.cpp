#include "maindialog.h"
#include "ui_maindialog.h"
#include "nlistwidget.h"
#include "trashwidget.h"
#include <QMenuBar>
#include <QMenu>
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

NListWidget      *g_listWidget;//全局的ListWidgetextern
MainDialog        *g_mainDialog;
QWidget             *g_TrashWidget;
char                       g_Version[100];
extern double      g_fontPixelRatio;
extern int             g_nCameraCommandsFlag;//摄像头状态标识

MainDialog::MainDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MainDialog)
{
    ui->setupUi(this);
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));//中文乱码
    init();
    connect(&PUB_SS,SIGNAL(signal_openProcFromFileName(QString)),this,SLOT(slot_openProcFromFileName(QString)));
    connect(&PUB_SS,SIGNAL(SignalOpenComFailed()),this,SLOT(slot_RS232isDisabled()));
    connect(&PUB_SS,SIGNAL(SignalOpenComSuccess()),this,SLOT(slot_RS232isAvaliable()));
    //摄像头启用指令信号与槽
    connect(&PUB_SS,SIGNAL(SignalCameraStatusCheck()),this,SLOT(slot_cameraStatusCheck()));
    //垃圾桶类删除APP信号与槽
    connect(ui->Trash,SIGNAL(signal_deleteAppFromVector(QString)),this,SLOT(slot_deleteAppFromVector(QString)));

    //Montage弹窗
    montageDialog = new MontageDialog();
    connect(montageDialog,SIGNAL(signal_showMainDialog()),this,SLOT(slot_showMainDialog()));
    //     ReadSettings();
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

    QAction *quitAction = new QAction(tr("Exit"), this);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(slot_forceExit()));
    //创建右键弹出菜单
    trayIconMenu = new QMenu(this);
    trayIconMenu->setStyleSheet("QMenu {background-color: white;border: 1px solid white;}"
                                "QMenu::item {background-color: transparent;padding:8px 64px;"
                                "margin:0px 8px;border-bottom:1px solid #DBDBDB;}"
                                "QMenu::item:selected { background-color: #2dabf9;}");
    FontScaleRatio::Instance()->setGuiFont("Helvetica",12,trayIconMenu);
    trayIconMenu->addAction(quitAction);
    trayIcon->setContextMenu(trayIconMenu);
    //--------------------------------------------------------------------------------------------------------------
    //设置屏蔽罩
    MaskWidget::Instance()->setMainWidget(this);
    QStringList dialogNames;
    dialogNames << "UploadWidget";
    MaskWidget::Instance()->setDialogNames(dialogNames);

}

MainDialog::~MainDialog()
{
    if(!g_appInfoVector.isEmpty()){
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
        else {
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
    ui->canncelButton->setVisible(false);//隐藏取消按钮
    ui->maxButton->setVisible(false);
    ui->menuButton->setVisible(false);
    FontScaleRatio::Instance()->setGuiFont("Helvetica",12,ui->label);
    IconHelper::Instance()->SetIcon(ui->closeButton, QChar(0xf00d), qRound(12/g_fontPixelRatio));
    IconHelper::Instance()->SetIcon(ui->aboutButton, QChar(0xf128), qRound(12/g_fontPixelRatio));
    IconHelper::Instance()->SetIcon(ui->maxButton, QChar(0xf096), qRound(12/g_fontPixelRatio));
    IconHelper::Instance()->SetIcon(ui->minButton, QChar(0xf068), qRound(12/g_fontPixelRatio));
    IconHelper::Instance()->SetIcon(ui->menuButton, QChar(0xf0c9), qRound(12/g_fontPixelRatio));
    ui->Trash->setStyleSheet("image: url(:/Resource/icons/trash_close.png);");
    m_widget->installEventFilter(this);//groupBox过滤事件
    ui->groupWidget->setStyleSheet("background-image: url(:/Resource/images/Background.png);");
    //listwidget 样式
    QScrollBar *verticalScrollBar=new QScrollBar(this);
    verticalScrollBar->setStyleSheet("QScrollBar:vertical {"
                                     "background-color:transparent; "
                                     // "border-image: url(:/Resource/images/vertical.png);"
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

    if(g_fontPixelRatio>=3)
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
            QTextCodec *code=QTextCodec::codecForName("UTF-8");
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
        QTextCodec *code=QTextCodec::codecForName("UTF-8");
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
                    //UWP 目标文件不可获取
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
        this->trayiconActivated(QSystemTrayIcon::Trigger);
//        this->activateWindow();
//        this->resize(HHT_WIDTH,HHT_HEIGHT);
//        this->showNormal();
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
        //没有图标exe文件是加载zi'ding'y自定义默认icon
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
                    ui->listWidget->addItem(item);
                    g_appInfoVector.append(hhtAppInfo);
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
                        hint->showParentCenter(this);
                        hint->setMassage(tr("The item is already in the list .  "),-1);
                        hint->showParentCenter(this);
                        hint->show();
                    }
                    else
                    {
                        hhtAppInfo._fileName = recodeFileName;
                        hhtAppInfo._lnkPath = recodeAbsPath;
                        hhtAppInfo._appIcon = recodeFileIcon;
                        ui->listWidget->addItem(item);
                       g_appInfoVector.append(hhtAppInfo);
                    }
                }
            //----------------------------------------------------------------------------------------------------------------------------
#else
            HintDialog *hint = new HintDialog();
            hint->showParentCenter(this);
            hint->setMassage(tr("Format not supported. You can add .exe and .lnk files only .  ").arg(file_info.fileName()),-2);
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
//                   QFileIconProvider icon_provider;
//                   icon = icon_provider.icon(file_info);
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
                    //                    支持桌面UWP快捷方式软件 2017/8/22添加
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
                    //=====================================
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
    //                    QFileIconProvider icon_provider;
    //                   icon = icon_provider.icon(QFileInfo(file_info.symLinkTarget()));
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
                    ui->listWidget->addItem(item);
                    g_appInfoVector.append(hhtAppInfo);
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
                        hint->showParentCenter(this);
                        hint->setMassage(tr("The item is already in the list .  "),-1);
                        hint->showParentCenter(this);
                        hint->show();
                    }
                    else
                    {
                        hhtAppInfo._fileName = recodeFileName;
                        hhtAppInfo._lnkPath = recodeAbsPath;
                        hhtAppInfo._appIcon = recodeFileIcon;
                        ui->listWidget->addItem(item);
                        g_appInfoVector.append(hhtAppInfo);
                    }
                }
            //----------------------------------------------------------------------------------------------------------------------------
            }
        else
        {
                HintDialog *hint = new HintDialog();
                hint->showParentCenter(this);
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

void MainDialog::slot_backgroundColorChanged(QString &color)
{
    this->setStyleSheet(tr("background-color:%1").arg(color));
}

void MainDialog::slot_RS232isDisabled()
{
    if(!this->recvivedFailedOpenCom)
    {//仅仅弹窗一次
        HintDialog *hint = new HintDialog();
//        hint->showParentCenter(this);
        hint->setGeometry((this->pos().x()+this->width()/2)- (hint->width()/3-6),
                                             (this->pos().y()+this->height()/2)-(hint->height()/2),
                                            hint->width(),hint->height());
        hint->setMassage(tr(" Serial port open failed .  "),-2);
        this->showNormal();
        hint->show();
    }
    this->recvivedFailedOpenCom = true;
    this->isFailedOpenCOM = true;
}

void MainDialog::slot_RS232isAvaliable()
{
    this->isFailedOpenCOM = false;
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
#endif
        else
        {
//            HHT_LOG(EN_INFO, "---Android commands to open fileName[QString] : (%s)", fileName.toLocal8Bit().data());
            for(int  i=0;i<g_appInfoVector.count();i++)
            {
                HHT_LOG(EN_INFO, "---Newline assistant list fileName[%d] : (%s)",i, g_appInfoVector.at(i)._fileName.toLocal8Bit().data());
                if(g_appInfoVector.at(i)._fileName==fileName)
                {
                    HHT_LOG(EN_INFO,"g_appInfoVector.at(%d)._fileName(%s)=?=fileName(%s) .",i,g_appInfoVector.at(i)._fileName.toLocal8Bit().data(),fileName.toLocal8Bit().data());;
                    LPCWSTR program = (LPCWSTR)g_appInfoVector.at(i)._lnkPath.utf16();
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
}

void MainDialog::trayiconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
    {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        //单击托盘图标
        //双击托盘图标
    {
//        this->setVisible(true);
//        QDesktopWidget *desktop = QApplication::desktop();
//        this->move(desktop->screenGeometry().width()-this->width(),
//                   desktop->screenGeometry().height()-this->height()-70);
        this->activateWindow();
        this->resize(HHT_WIDTH,HHT_HEIGHT);
        this->showNormal();
    }
        break;
    default:
        break;
    }
}

void MainDialog::slot_forceExit(){
    qDebug()<<"close signal";
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
//    HHT_LOG(EN_INFO, " --------slot_cameraStatusCheck. -------->");
    int result = isSysCameraStatus();
    if(result == 1)
    {//可用
        g_nCameraCommandsFlag =1;
        qDebug()<<"Camera write commands flag: "<<g_nCameraCommandsFlag;
        HHT_LOG(EN_INFO, " --------CAMERA IS AVALIABLE,TRY TO OPEN IT AND IMMEDIATELY CLOSE IT. -------->");
    }
    else if(result ==2)
    {//被Montage占用摄像头可用状态
        g_nCameraCommandsFlag =1;
        qDebug()<<"Camera write commands flag: "<<g_nCameraCommandsFlag;
        HHT_LOG(EN_INFO, " --------CAMERA IS OCUPPIED BY MONTAGE . -------->");
    }
    else if(result == 0)
    {//被占用
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
    else if(result ==-1)
    {//摄像头不存在 不可用
//        HintDialog *cameraDialog = new HintDialog();
//        cameraDialog->setMassage(tr("Camera is not found.\n For changing the camera please make sure camera is exist first .  "),-2);
//        cameraDialog->setCancelButtonEnable(false);
//        cameraDialog->resize(QSize(215,cameraDialog->height()));
//        cameraDialog->setGeometry((this->pos().x()+this->width()/2)- (cameraDialog ->width()/2)-210,
//                                             (this->pos().y()+this->height()/2)-(cameraDialog->height()/2),
//                          cameraDialog->width(),cameraDialog->height());
//        this->showNormal();
        g_nCameraCommandsFlag = 1;//摄像头不存在 不可用 #强制向Android发送可以切换状态
//        cameraDialog->exec();
        qDebug()<<"Camera write commands flag: "<<g_nCameraCommandsFlag;
        HHT_LOG(EN_INFO, " --------CAMERA IS NOT FOUND. -------->");
    }
}

void MainDialog::on_canncelButton_clicked()
{
#ifdef HHT_DEBUG
    montageDialog->showParentCenter(this);
    montageDialog->show();
    //取消上传 g_nWriteFileFlag=0写文件标识
    // PUB_SS.PostSendFileToAndroidFailed();//发送取消(失败)信号
#endif

}

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
//                 UploadWidget::Instance()->showParentCenter(this);
//                 UploadWidget::Instance()->show();//单例显示
//                 HHT_LOG(EN_INFO, "UPLOAD RANGE---->", UploadWidget::Instance()->maximum());
             }
         }
     }
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

void MainDialog::on_aboutButton_clicked()
{
    AboutDialog *about = new AboutDialog();
    about->showParentCenter(this);
    about->show();
}

void MainDialog::on_minButton_clicked()
{
    if(!g_appInfoVector.isEmpty())
    {
        WriteRecords();
    }
    this->showMinimized();
//    this->hide();
}
