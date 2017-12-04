#include "uploadwidget.h"
#include "ui_uploadwidget.h"
#include "Helper/iconhelper.h"
#include "Helper/hhthelper.h"
#include "Pub/ss_pub.h"
#include "Pub/global.h"
extern double      g_fontPixelRatio;
//UploadWidget* UploadWidget::_instance = 0;
bool  g_uploadStatus = false;//上传模块上传成功标识 全局

UploadWidget::UploadWidget(QDialog *parent) :
  QDialog(parent),m_value(0),m_min(0),m_max(100)
  ,m_nullPosition(PositionTop),
    ui(new Ui::UploadWidget)
{
    ui->setupUi(this);
    isExist = true;
    initValue();
    ui->statusLabel->setStyleSheet("background-color: rgb(56, 166, 223);"
                                                              "color: rgb(255, 255, 255);"
                                                             "border-image: url(:/Resource/icons/upload.png);");
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);//窗口无边框，置顶
    this->setWindowModality(Qt::ApplicationModal);//阻塞除当前窗体之外的所有的窗体
    setAttribute(Qt::WA_TranslucentBackground);
    //成功发送包数量信号与槽函数
    connect(&PUB_SS,SIGNAL(SignalCurrentPkgNum(int)),this,SLOT(SoltCurrentPkgNum(int)));
    connect(&PUB_SS,SIGNAL(SignalTotalPkgNum(int)),this,SLOT(SoltTotalPkgNum(int)));
    connect(&PUB_SS,SIGNAL(SignalSendFileToAndroidFailed()),this,SLOT(SoltSendFileToAndroidFailed()));
}

UploadWidget::~UploadWidget()
{
    delete ui;
}

void UploadWidget::showParentCenter(QWidget *parentFrm)
{
    this->setGeometry((parentFrm->pos().x()+parentFrm->width()/2)- (this ->width()/2),
                                     (parentFrm->pos().y()+parentFrm->height()/2)-(this->height()/2),
                                    this->width(),this->height());
}

double UploadWidget::nullPosition() const
{
     return m_nullPosition;
}

void UploadWidget::setNullPosition(double position)
{
    if (position != m_nullPosition)
    {
        m_nullPosition = position;
        update();
    }
}

double UploadWidget::value() const
{
    return m_value;
}

double UploadWidget::minimum() const
{
    return m_min;
}

double UploadWidget::maximum() const
{
    return m_max;
}

void UploadWidget::setRange(double min, double max)
{
    m_min =min;
    m_max =max;
    if (m_max < m_min)
        qSwap(m_max, m_min);

    if (m_value < m_min)
        m_value = m_min;
    else if (m_value > m_max)
        m_value = m_max;
    update();
}

void UploadWidget::setMinimum(double min)
{
    setRange(min, m_max);
}

void UploadWidget::setMaximum(double max)
{
    setRange(m_min, max);
}

void UploadWidget::setValue(double val)
{
    if(val>=m_min&&val<=m_max)
    {
        m_value = val;
        m_currentValue =val;
    }
    update();
}

void UploadWidget::setValue(int val)
{
    if(val>=m_min&&val<=m_max)
    {
        m_value =(double) val;
        m_currentValue =(double)val;
    }
    update();
}
/**总包数槽函数设置Range
 * @brief UploadWidget::SoltTotalPkgNum
 * @param counts
 */
void UploadWidget::SoltTotalPkgNum(int counts)
{
//    HHT_LOG(EN_INFO, "SET UPLOAD RANGE:(%d) ", counts);
    setRange(0,counts);
    setMaximum(counts);
}
/**线程信号响应函数
 * @brief UploadWidget::SoltCurrentPkgNum
 * @param counts
 */
void UploadWidget::SoltCurrentPkgNum(int currentCounts)
{
//    HHT_LOG(EN_INFO, "SET CURRENT UPLOAD: (%d)", currentCounts);
    setValue(currentCounts);
}

void UploadWidget::SoltSendFileToAndroidFailed()
{
    HHT_LOG(EN_INFO, "SEND FILE TO ANDROID FAILED: (SLOT)");
    qDebug()<<"SEND FILE TO ANDROID FAILED: (SLOT)";

    if(!m_failedClose){//仅仅接受一次发送失败信号
        HintDialog *hint = new HintDialog();
        hint->setMassage(tr("Newline assistant failed to send the data to the smart system .  "),-2);
        hint->resize(hint->width()-60,hint->height());
        hint->showParentCenter(this);
        hint->show();
    }
    m_failedClose =true;
    this->close();
}

void UploadWidget::closeTimerEvent()
{
//    HHT_LOG(EN_INFO,"Send file failed to close.");
    qDebug()<<"uploading moudle: Send file failed to close.";
    this->close();
}

void UploadWidget::closeTimer2Event()
{
//    HHT_LOG(EN_INFO,"Send file success to close.");
    qDebug()<<"uploading moudle:Send file success to close.";
    hhtHelper::Sleep(1);
    g_uploadStatus = true;//发送文件成功修改全局上传状态标识
    qDebug()<<"uploading moudle: upload status-> "<<g_uploadStatus;
    closeTimer2->stop();
    this->close();
}

void UploadWidget::slot_closeUpload()
{
   if(!m_successClose){//仅仅接受一次发送成功信号
        closeTimer2->start(1000);
   }
    m_successClose = true;
}

void UploadWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing);
    //绘制上传百分比
    drawOuterCircle(&painter);
    drawRoundBarProgress(&painter);
    drawInnerCircle(&painter);
    drawTextValue(&painter);
    painter.end();
    QWidget::paintEvent(event);
}

void UploadWidget::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
}

void UploadWidget::initValue()
{
    m_max =100;
    m_min =0;
    m_value =0;
    m_currentValue =0;
    m_successClose=false;
    m_failedClose =false;
    m_outRadious =this->width()<=this->height()?(this->width()/2-2):(this->height()/2-2);
    m_innerRadious =m_outRadious-7;
    m_colorRadious = m_innerRadious;
    m_coverColorRadious = m_colorRadious-5;
    m_center = this->rect().center();
    //定时更新
    updateTimer = new QTimer(this);
    updateTimer->setInterval(10);
    connect(updateTimer,SIGNAL(timeout()),this,SLOT(updateWidget()));
    updateTimer->start();

    closeTimer = new QTimer(this);
    connect(closeTimer,SIGNAL(timeout()),this,SLOT(closeTimerEvent()));//定时关闭

    closeTimer2 = new QTimer(this);
    connect(closeTimer2,SIGNAL(timeout()),this,SLOT(closeTimer2Event()));//定时关闭

    connect(this,SIGNAL(signal_closeUpload()),this,SLOT(slot_closeUpload()));
}

void UploadWidget::drawOuterCircle(QPainter *painter)
{
    painter->save();
    painter->setPen(Qt::NoPen);
    QPointF pieRectTopLeftPot(m_center.x()-m_outRadious,this->rect().center().y()-m_outRadious);
    QPointF pieRectBottomRightPot(m_center.x()+m_outRadious,this->rect().center().ry()+m_outRadious);

    QRectF m_pieRect=QRectF(pieRectTopLeftPot,pieRectBottomRightPot);
    painter->setBrush(Qt::gray);
    painter->drawPie(m_pieRect,0*16,360*16);
    painter->restore();

}

void UploadWidget::drawInnerCircle(QPainter *painter)
{
    painter->save();
    painter->setPen(Qt::NoPen);

    QRadialGradient coverGradient(this->rect().center(),m_coverColorRadious,this->rect().center());
    coverGradient.setColorAt(1.0,QColor("#38a6df"));
    painter->setBrush(coverGradient);
    painter->drawEllipse(m_center,m_innerRadious,m_innerRadious);

    painter->restore();
}

void UploadWidget::drawRoundBarProgress(QPainter *painter)
{
    painter->save();
    m_perAngle = (qreal)360 / m_max;
    painter->setPen(Qt::NoPen);
    QPointF pieRectTopLeftPot(m_center.x()-m_colorRadious*1.1,m_center.y()-m_colorRadious*1.1);
    QPointF pieRectBottomRightPot(m_center.x()+m_colorRadious*1.1,m_center.y()+m_colorRadious*1.1);

    QRectF m_pieRect=QRectF(pieRectTopLeftPot,pieRectBottomRightPot);
    QRadialGradient graphGradient(m_center,m_colorRadious,m_center);

//    graphGradient.setColorAt(1.0,QColor(255,0,128));
    graphGradient.setColorAt(1.0,Qt::white);
    painter->setBrush(graphGradient);
    painter->drawPie(m_pieRect,m_nullPosition*16,-m_value*m_perAngle*16-0.1*m_perAngle*16);
//    painter->drawPie(m_pieRect,90*16,-m_value*m_perAngle*16-0.5*m_perAngle*16);
    //从逻辑0值（270度）开始，绘制 显示 百分数的饼状图
    painter->restore();
}

void UploadWidget::drawTextValue(QPainter *painter)
{
    painter->save();
    QFont font;
    QPen pen;
    pen.setBrush(Qt::white);
    font.setFamily("Helvetica");
    if(g_fontPixelRatio>=2.5)
    {
        font.setPixelSize(qRound(48/g_fontPixelRatio));
    }
    else
    {
        font.setPixelSize(qRound(36/g_fontPixelRatio));
    }
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(pen);
    QString strValue;
    strValue=tr("%1").arg((int)((m_value/m_max)*100));
    //draw precent ％
    QPointF topLeftDolt(m_center.x()-this->width()/3+5,m_center.y()-this->height()/4);
    QPointF bottomRightDolt(m_center.x()+this->width()/3,m_center.y());
    QRectF textRect(topLeftDolt,bottomRightDolt);//
    if(m_value==m_max)
    {//upload complete.
        painter->drawText(textRect,Qt::AlignCenter,tr("OK"));
        ui->statusLabel->setStyleSheet("background-color: rgb(56, 166, 223);"
                                                                  "color: rgb(255, 255, 255);"
                                                                 "border-image: url(:/Resource/icons/success.png);");
        update();
        emit signal_closeUpload();//关闭上传模块
    }
    else
    {
        painter->drawText(textRect,Qt::AlignCenter,strValue+tr("%"));
    }
    painter->restore();
}

void UploadWidget::updateWidget()
{
    update();
}

