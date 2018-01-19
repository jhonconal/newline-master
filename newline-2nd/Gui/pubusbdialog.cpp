#include "pubusbdialog.h"
#include "ui_pubusbdialog.h"
#include "Helper/fontscaleratio.h"
#include "Helper/iconhelper.h"
extern double      g_fontPixelRatio;
extern int           g_PubUsbOpertationStatus;//切换PubUSB指令标识
PubUSBDialog::PubUSBDialog(QDialog *parent) :
    QDialog(parent),
    ui(new Ui::PubUSBDialog)
{
    ui->setupUi(this);
    m_operation_status =-1;
    FontScaleRatio::Instance()->setGuiFont("Helvetica",12,ui->TitleLabel);
    initUiFontSize(g_fontPixelRatio);
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint|Qt::FramelessWindowHint);//窗口置顶
    //this->setWindowModality(Qt::ApplicationModal);//阻塞除当前窗体之外的所有的窗体
    QScrollBar *verticalScrollBar=new QScrollBar(this);
    verticalScrollBar->setStyleSheet("QScrollBar:vertical { \
                                     max-width: 16px; \
                                     background: transparent; \
                                     padding-top: 2px;\
                                     padding-bottom:2px; \
                                     }\
                                     QScrollBar::handle:vertical { \
                                     width: 16px; \
                                     min-height: 30px; \
                                     background: rgb(190, 190, 190); \
                                     } \
                                     QScrollBar::handle:vertical:hover { \
                                     background: rgb(170, 170, 170); \
                                     } \
                                     QScrollBar::sub-line:vertical { \
                                     height: 1px; \
                                     width: 1px; \
                                     background: rgb(220, 220, 220); \
                                     subcontrol-position: top; \
                                     } \
                                     QScrollBar::add-line:vertical { \
                                     height: 1px;\
                                     width: 1px; \
                                     background: rgb(220, 220, 220); \
                                     subcontrol-position: bottom; \
                                     }\
                                     QScrollBar::sub-line:vertical:hover { \
                                     background: rgb(190, 190, 190);\
                                     } \
                                     QScrollBar::add-line:vertical:hover { \
                                     background: rgb(190, 190, 190); \
                                     } \
                                     QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical { \
                                     background: rgb(220, 220, 220); ");
           ui->PlainTextEdit->setVerticalScrollBar(verticalScrollBar);
           ui->PlainTextEdit->setPlainText("USB port is currently in use. Switching the source may "
                                                "cause unexpected errore. Do you really want to switch the source ?");
}

PubUSBDialog::~PubUSBDialog()
{
    delete ui;
}

void PubUSBDialog::showParentCenter(QDialog *parentFrm)
{
    this->setGeometry((parentFrm->pos().x()+parentFrm->width()/2)- (this ->width()/2),
                                         (parentFrm->pos().y()+parentFrm->height()/2)-(this->height()/2),
                      this->width(),this->height());
}

void PubUSBDialog::initUiFontSize(double fontSizeRatio)
{
    FontScaleRatio::Instance()->setGuiFont("Helvetica",12,this);//"Helvetica"
    FontScaleRatio::Instance()->setGuiFont("Helvetica",13,ui->PlainTextEdit);
    FontScaleRatio::Instance()->setGuiFont("Helvetica",13,ui->CancelButton);
    FontScaleRatio::Instance()->setGuiFont("Helvetica",13,ui->SwitchButton);
}

const int PubUSBDialog::get_operation_status()
{
    return m_operation_status;
}

void PubUSBDialog::mousePressEvent(QMouseEvent *event)
{
    this->windowPos = this->pos();
    this->mousePos = event->globalPos();
    this->dPos = mousePos - windowPos;
}

void PubUSBDialog::mouseMoveEvent(QMouseEvent *event)
{
    this->move(event->globalPos() - this->dPos);
}

void PubUSBDialog::on_SwitchButton_clicked()
{
    //切换信号源
    g_PubUsbOpertationStatus =1;
    m_operation_status = 1;
    accept();
    //this->close();
}

void PubUSBDialog::on_CancelButton_clicked()
{
    //不切换信号源
     g_PubUsbOpertationStatus =0;
     m_operation_status = 0;
     reject();
    //this->close();
}

void PubUSBDialog::on_CloseButton_clicked()
{
    //不切换信号源
    g_PubUsbOpertationStatus =0;
    m_operation_status = 0;
    reject();
    //this->close();
}
