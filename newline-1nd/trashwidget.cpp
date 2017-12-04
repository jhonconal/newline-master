#include "trashwidget.h"
#include "Helper/iconhelper.h"
#include "hhtheader.h"
#include "nlistwidget.h"
extern NListWidget      *g_listWidget;//全局的ListWidget
extern QWidget            *g_TrashWidget;

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
        isTrydelete = true;
        g_TrashWidget->setStyleSheet("border-image: url(:/Resource/icons/trash_open.png);background-color:transparent ");
        timer->start(1000);
    }
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
            emit signal_deleteAppFromVector(g_listWidget->currentItem()->text());
            qDebug()<<g_listWidget->currentItem()->text()<<"---->selected to delete.";
            delete g_listWidget->currentItem();
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
