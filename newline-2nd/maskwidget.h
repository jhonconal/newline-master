#ifndef MASKWIDGET_H
#define MASKWIDGET_H

/**
 * 弹窗遮罩层控件
 * 1:可设置需要遮罩的主窗体,自动跟随主窗体位置显示遮罩面积
 * 2:只需要将弹窗窗体的名称一开始传入队列即可,足够简单
 * 3:可设置透明度
 * 4:可设置遮罩层颜色
 * 5:不阻塞消息循坏
 */
//第一步,设置需要遮罩的父窗体
//MaskWidget::Instance()->setMainWidget(this);
//第二步,设置哪些弹窗窗体需要被遮罩
//QStringList dialogNames;
//dialogNames << "窗口1" << "窗口2";
//MaskWidget::Instance()->setDialogNames(dialogNames);

#include <QWidget>
#include <QMutex>
#include <QDebug>
#include <QMouseEvent>
class MaskWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MaskWidget(QWidget *parent = 0);
    static MaskWidget *Instance()
    {
        static QMutex mutex;

        if (!self) {
            QMutexLocker locker(&mutex);

            if (!self) {
                self = new MaskWidget;
            }
        }
        return self;
    }
   bool isExist;
protected:
    void showEvent(QShowEvent *);
    bool eventFilter(QObject *obj, QEvent *event);
    void mousePressEvent(QMouseEvent *event);
private:
    static MaskWidget *self;
    QWidget *mainWidget;        //需要遮罩的主窗体
    QStringList dialogNames;    //可能弹窗的窗体对象名称集合链表

public slots:
    void setMainWidget(QWidget *mainWidget);
    void setDialogNames(const QStringList &dialogNames);
    void setBgColor(const QColor &bgColor);
    void setOpacity(double opacity);
};

#endif // MASKWIDGET_H
