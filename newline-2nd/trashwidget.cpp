#include "trashwidget.h"
#include "Helper/iconhelper.h"
#include "Pub/ss_pub.h"
#include "hhtheader.h"
#include "hhtheader.h"
#include "nlistwidget.h"
#include "hintdialog.h"
extern NListWidget      *g_listWidget;//全局的ListWidget
extern QWidget            *g_TrashWidget;
extern bool          g_SerialStatus;
extern int            g_nCameraCommandsFlag;//摄像头状态标识
extern int            g_nDeleteSingleAppFlag;//删除一个APP 发送指令标识

TrashWIdget::TrashWIdget(QWidget *parent) : QWidget(parent)
{
    this->setAttribute(Qt::WA_TranslucentBackground);
    timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(slot_timeout()));
}

void TrashWIdget::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug()<<"dialog drag enter";
    event->acceptProposedAction();
    if(g_listWidget->currentItem())
    {
//        qDebug()<<"将要删除";
        isTrydelete = true;
        g_TrashWidget->setStyleSheet("border-image: url(:/Resource/icons/trash_open.png);background-color:transparent ");
        timer->start(1000);
    }
}

void TrashWIdget::deleteSingleAppCommandsToAndroid(QString appName)
{
   //删除指令 +APP 名字
}

void TrashWIdget::dropEvent(QDropEvent *event)
{
    Q_UNUSED(event);
    int  appCount = g_listWidget->count();
    qDebug()<<"APP_COUNT:"<<appCount;
    if(g_listWidget->currentItem())
    {
        if(g_listWidget->currentItem()->isSelected())
        {

#ifdef HHT_2ND_PROJECT_SUPPORT
            //Android 返回删除成功标识位
            emit signal_deleteAppFromVector(g_listWidget->currentItem()->text());
#else
            emit signal_deleteAppFromVector(g_listWidget->currentItem()->text());
            qDebug()<<g_listWidget->currentItem()->text()<<": [selected to delete success.]";
            delete g_listWidget->currentItem();
#endif
        }
        //        //删除列表
        if(isTrydelete&&appCount>g_listWidget->count())
        {
            g_TrashWidget->setStyleSheet("border-image: url(:/Resource/icons/trash_close.png);background-color:transparent ");
        }
    }
}

void TrashWIdget::slot_timeout()
{
    g_TrashWidget->setStyleSheet("border-image: url(:/Resource/icons/trash_close.png);background-color:transparent ");
}
