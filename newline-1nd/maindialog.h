#ifndef MAINDIALOG_H
#define MAINDIALOG_H
#include <QDialog>
#include <QFileDialog>
#include <QDebug>
#include <QGroupBox>
#include <QFile>
#include <QPainter>
#include <QPaintEvent>
#include <QFileInfo>
#include <QDropEvent>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QUrl>
#include <QTimer>
#include<QBuffer>
#include<QScrollBar>
#include <QFileIconProvider>
#include <QProcess>
#include <QSettings>
#include <QPaintEvent>
#include <QSystemTrayIcon>
#include <QtWin>
#include "nlistwidget.h"
#include "Helper/iconhelper.h"
#include "Gui/frmmessagebox.h"
#include "Gui/aboutdialog.h"
#include "Gui/hintdialog.h"
#include "Gui/uploadwidget.h"
#include "Gui/montagedialog.h"
#include "hhtheader.h"
#include "windows.h"

#define  APPLICATION_NAME  "Newline assistant"
#define  MONTAGE  "Montage Receiver" //定义特殊第三方软件Montage
#define SHOWNORNAL   (WM_USER+0x0004)//自定义SendMessageA 的Msg参数
class NListWidget;

namespace Ui {
class MainDialog;
}

class MainDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MainDialog(QWidget *parent = 0);
    ~MainDialog();
    QVector<HHTAPPINFO>g_appInfoVector;
    int isSysCameraStatus();//摄像头检测
    void init();
    //用于记录程序关闭前设置的APP信息
    void WriteRecords();
    void ReadSettings();
    void  ReadRecords();
    //    //QIcon转数据
    QByteArray QIcon2QByteArray(QIcon icon);

#ifdef  Q_OS_WIN32
    //bool winEvent(MSG * pMsg, long * lResult);
    bool nativeEvent(const QByteArray & eventType, void *message, long * lResult);
#endif //Q_OS_WIN32

    QPixmap getMaxPixmap(const QString sourceFile);
protected:
    void mousePressEvent(QMouseEvent *event);

    void mouseMoveEvent(QMouseEvent *event);

    void dragEnterEvent(QDragEnterEvent *event);

    void dropEvent(QDropEvent *event);

public slots:

    void slot_backgroundColorChanged(QString &color);
    //     //接收线程检测串口不可用信号
    void  slot_RS232isDisabled();

    void slot_RS232isAvaliable();

    void  slot_openProcFromFileName(QString fileName);
    //TrashWIdget删除APP响应信号
     void slot_deleteAppFromVector(QString appName);

    void  trayiconActivated(QSystemTrayIcon::ActivationReason reason);

    void  slot_forceExit();

    void slot_showMainDialog();//连接至MontageDialog类

    void slot_cameraStatusCheck();//摄像头启用操函数-->ss_pub.h
private slots:
    void on_canncelButton_clicked();

    void on_checkButton_clicked();

    void on_closeButton_clicked();

    void on_aboutButton_clicked();

    void on_minButton_clicked();

private:
    Ui::MainDialog *ui;

    QWidget *m_widget;
    bool groupBoxFliter;
    QPoint windowPos;
    QPoint mousePos;
    QPoint dPos;
    AboutDialog *aboutDialog;
    MontageDialog *montageDialog;
    //Menu
    QString shortcutPath;
    UploadWidget *upload;
    QMenu *trayIconMenu;
    QSystemTrayIcon *trayIcon;
    bool  recvivedFailedOpenCom;
    bool  isFailedOpenCOM;
};

#endif // MAINDIALOG_H
