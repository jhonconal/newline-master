#ifndef TRASHWIDGET_H
#define TRASHWIDGET_H

#include <QWidget>
#include <QDebug>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QTimer>
#include "nlistwidget.h"
class TrashWIdget : public QWidget
{
    Q_OBJECT
public:
    explicit TrashWIdget(QWidget *parent = 0);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
signals:
    void signal_deleteAppFromVector(QString appName);//删除APP信号-->连接至MainDialog
public slots:
    void slot_timeout();
private:
     bool isTrydelete;
     QTimer *timer;
};

#endif // TRASHWIDGET_H
