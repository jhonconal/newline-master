#include "hintdialog.h"
#include "ui_hintdialog.h"
#include "Helper/iconhelper.h"
#include "Helper/fontscaleratio.h"
extern double      g_fontPixelRatio;
HintDialog::HintDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HintDialog)
{
    ui->setupUi(this);
    FontScaleRatio::Instance()->setGuiFont("Helvetica",12,ui->titleLabel);
    initUiFontSize(g_fontPixelRatio);
    ui->cancelButton->setVisible(false);
    ui->iconLabel->setStyleSheet("border-image: url(:Resource/images/info.png);");
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint|Qt::FramelessWindowHint);//窗口置顶
    //this->setWindowModality(Qt::ApplicationModal);//阻塞除当前窗体之外的所有的窗体
}

HintDialog::~HintDialog()
{
    delete ui;
}

void HintDialog::showParentCenter(QWidget *parentFrm)
{
    this->setGeometry((parentFrm->pos().x()+parentFrm->width()/2)- (this ->width()/2),
                                         (parentFrm->pos().y()+parentFrm->height()/2)-(this->height()/2),
                      this->width(),this->height());
}

void HintDialog::setBackground(const QString &color)
{
    this->setStyleSheet(tr("background-color:%1;color:#FFFFFF;").arg(color));
}
//0 提示 1 询问 2 错误
void HintDialog::setMassage(const QString &msg, int type)
{
    if (type == 0)
    {
        ui->iconLabel->setStyleSheet("border-image: url(:Resource/images/info.png);");
       setWindowTitle("Prompt");
    }
    else if (type == 1)
    {
        ui->iconLabel->setStyleSheet("border-image: url(:Resource/images/question.png);");
        setWindowTitle("");
    }
    else if (type == 2)
    {
        ui->iconLabel->setStyleSheet("border-image: url(:Resource/images/warning.png);");
        setWindowTitle("Error");
    }
    else
    {
        ui->iconLabel->setStyleSheet("border-image: url(:Resource/images/warning.png);");
        setWindowTitle("Newline assistant");
    }
    ui->textLabel->setText(" "+msg);
    QFont font;
//    font.setFamily("Microsoft YaHei UI");
    font.setPointSize(qRound(16/g_fontPixelRatio));
    //font.setFamily(font.defaultFamily());
    QFontMetrics fm = QFontMetrics(font);
    int textWidth = fm.width(msg);
    this->resize(QSize(50+textWidth+50,this->height()));
}

void HintDialog::setTitle(const  QString &title)
{
    this->setWindowTitle(title);
}

void HintDialog::initUiFontSize(double fontSizeRatio)
{
    QFont font;
    font.setFamily("Helvetica");
    font.setPointSize(qRound(14/g_fontPixelRatio));
    ui->textLabel->setFont(font);
    ui->okButton->setFont(font);
//    FontScaleRatio::Instance()->setGuiFont("Helvetica",16,this);
//    FontScaleRatio::Instance()->setGuiFont(QApplication::font(),16,ui->textLabel);
//    FontScaleRatio::Instance()->setGuiFont(QApplication::font(),16,ui->buttonBox);
}

void HintDialog::setCancelButtonEnable(bool flag)
{
        ui->cancelButton->setVisible(flag);
        FontScaleRatio::Instance()->setGuiFont("Helvetica",14/g_fontPixelRatio,ui->cancelButton);
}

void HintDialog::mousePressEvent(QMouseEvent *event)
{
    this->windowPos = this->pos();
    this->mousePos = event->globalPos();
    this->dPos = mousePos - windowPos;
}

void HintDialog::mouseMoveEvent(QMouseEvent *event)
{
    this->move(event->globalPos() - this->dPos);
}

void HintDialog::on_okButton_clicked()
{

    this->close();
}

void HintDialog::on_closeButton_clicked()
{

    this->close();
}

void HintDialog::on_cancelButton_clicked()
{
    this->close();
}
