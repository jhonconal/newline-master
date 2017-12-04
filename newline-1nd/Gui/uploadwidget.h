#ifndef UPLOADWIDGET_H
#define UPLOADWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QMouseEvent>
#include <QPaintEvent>
#include "Gui/hintdialog.h"
namespace Ui {
class UploadWidget;
}

class UploadWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UploadWidget(QWidget *parent = 0);
    ~UploadWidget();
//    static UploadWidget* _instance;
//    static UploadWidget* Instance()
//    {
//        static QMutex mutex;
//        if (!_instance) {
//            QMutexLocker locker(&mutex);
//            if (!_instance) {
//                _instance = new UploadWidget;
//            }
//        }
//        return _instance;
//    }
    static const int PositionLeft = 180;
    static const int PositionTop = 90;
    static const int PositionRight = 0;
    static const int PositionBottom = -90;
    /**界面居中父窗口
     * @brief showParentCenter
     * @param parentFrm
     */
    void showParentCenter(QWidget *parentFrm);

    double nullPosition() const;
    void setNullPosition(double position);

    double value() const;
    double minimum() const;
    double maximum() const;

    bool     isExist;
    double m_currentValue;

    void setRange(double min, double max);
    void setMinimum(double min);
    void setMaximum(double max);
    void setValue(double val);
    void setValue(int val);
signals:
    void signal_closeUpload();
    void signal_failedToUpload();
public slots:
     void SoltTotalPkgNum(int counts);//线程信号，成功发送数据包数量
     void SoltCurrentPkgNum(int currentCounts);
     void SoltSendFileToAndroidFailed();//接收线程发送的向Android发送文件失败信号

     void closeTimerEvent();
     void closeTimer2Event();
     void slot_closeUpload();
protected:
    virtual  void paintEvent(QPaintEvent *event);
    virtual  void mousePressEvent(QMouseEvent *event);
private:
    Ui::UploadWidget *ui;
    QTimer   *closeTimer ,*closeTimer2;
    bool  m_successClose;//发送成功
    bool  m_failedClose;//发送失败
    double m_min, m_max;
    double m_nullPosition;
    QPointF m_center;
    double  m_value;
    qreal  m_innerRadious,m_outRadious,m_perAngle;
    qreal  m_colorRadious,m_coverColorRadious;
    QTimer *updateTimer;
    void initValue();
    void drawOuterCircle(QPainter* painter);
    void drawInnerCircle(QPainter* painter);
    void drawRoundBarProgress(QPainter* painter);
    void drawTextValue(QPainter *painter);

private slots:
    void updateWidget();

};

#endif // UPLOADWIDGET_H
