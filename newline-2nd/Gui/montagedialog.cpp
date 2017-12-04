#include "montagedialog.h"
#include "ui_montagedialog.h"
#include "Helper/fontscaleratio.h"
#include "Helper/iconhelper.h"
extern double      g_fontPixelRatio;
MontageDialog::MontageDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MontageDialog)
{
    ui->setupUi(this);
    FontScaleRatio::Instance()->setGuiFont("Helvetica",12,ui->titleLabel);
    initUiFontSize(g_fontPixelRatio);
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint|Qt::FramelessWindowHint);//窗口置顶
    this->setWindowModality(Qt::ApplicationModal);//阻塞除当前窗体之外的所有的窗体
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
           ui->plainTextEdit->setVerticalScrollBar(verticalScrollBar);
}

MontageDialog::~MontageDialog()
{
    delete ui;
}

void MontageDialog::showParentCenter(QWidget *parentFrm)
{
    this->setGeometry((parentFrm->pos().x()+parentFrm->width()/2)- (this ->width()/2),
                                         (parentFrm->pos().y()+parentFrm->height()/2)-(this->height()/2),
                      this->width(),this->height());
}

void MontageDialog::initUiFontSize(double fontSizeRatio)
{
    FontScaleRatio::Instance()->setGuiFont("Helvetica",12,this);//"Helvetica"
    FontScaleRatio::Instance()->setGuiFont("Helvetica",13,ui->plainTextEdit);
    FontScaleRatio::Instance()->setGuiFont("Helvetica",13,ui->cancelButton);
    FontScaleRatio::Instance()->setGuiFont("Helvetica",13,ui->activeButton);
    FontScaleRatio::Instance()->setGuiFont("Helvetica",13,ui->installButton);
}

void MontageDialog::on_activeButton_clicked()
{
    emit signal_showMainDialog();
//    qDebug()<<"send show main dialog signal";
    this->close();
}

void MontageDialog::on_installButton_clicked()
{
    const QString NewlineURL ="http://www.displaynote.com/newline/";
    const QString BaiduURL = "https://www.baidu.com/";
    bool result = QDesktopServices::openUrl(NewlineURL);
    if(result)
    {
        qDebug()<<"Open Success.";
        this->close();
    }
    else
    {
        qDebug()<<"Open Failed.";
    }
}

void MontageDialog::on_cancelButton_clicked()
{
    this->close();
}

void MontageDialog::on_closeButton_clicked()
{
    this->close();
}
