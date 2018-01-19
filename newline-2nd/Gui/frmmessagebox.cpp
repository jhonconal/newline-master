#include "frmmessagebox.h"
#include "ui_frmmessagebox.h"
#include "Helper/iconhelper.h"
#include "Helper/hhthelper.h"
frmMessageBox::frmMessageBox(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::frmMessageBox)
{
    ui->setupUi(this);
    this->mousePressed = false;
    //设置窗体标题栏隐藏
    this->setWindowFlags(Qt::FramelessWindowHint);
    //设置窗体关闭时自动释放内存
    this->setAttribute(Qt::WA_DeleteOnClose);
    //设置图形字体
    IconHelper::Instance()->SetIcon(ui->lab_Ico, QChar(0xf015), 12);
    IconHelper::Instance()->SetIcon(ui->btnMenu_Close, QChar(0xf00d), 10);
    //关联关闭按钮
    connect(ui->btnMenu_Close, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->btnCancel, SIGNAL(clicked()), this, SLOT(close()));
    //窗体居中显示
    hhtHelper::FormInCenter(this);
    setWindowFlags(Qt::FramelessWindowHint|Qt::Tool | Qt::WindowStaysOnTopHint);//窗口置顶
}

frmMessageBox::~frmMessageBox()
{
    delete ui;
}

void frmMessageBox::showParentCenter(QWidget *parentFrm)
{
    this->setGeometry((parentFrm->pos().x()+parentFrm->width()/2)- (this ->width()/2),
                                         (parentFrm->pos().y()+parentFrm->height()/2)-(this->height()/2),
                      this->width(),this->height());
}

void frmMessageBox::SetMessage(const QString &msg, int type)
{
    if (type == 0)
    {
        ui->labIcoMain->setStyleSheet("border-image: url(:Resource/images/info.png);");
        ui->btnCancel->setVisible(false);
        //        ui->lab_Title->setText("提示");
        ui->lab_Title->setText("Prompt");
    }
    else if (type == 1)
    {
        ui->labIcoMain->setStyleSheet("border-image: url(:Resource/images/question.png);");
        //        ui->lab_Title->setText("询问");
        ui->lab_Title->setText("Ask");
    }
    else if (type == 2)
    {
        ui->labIcoMain->setStyleSheet("border-image: url(:Resource/images/error.png);");
        ui->btnCancel->setVisible(false);
        //        ui->lab_Title->setText("错误");
        ui->lab_Title->setText("Error");
    }

    ui->labInfo->setText(" "+msg);
}

void frmMessageBox::setBackground(const QString &color)
{
    this->setStyleSheet(tr("background-color:%1;color:#FFFFFF;").arg(color));
}

void frmMessageBox::mouseMoveEvent(QMouseEvent *e)
{
    if (mousePressed && (e->buttons() && Qt::LeftButton))
    {
        this->move(e->globalPos() - mousePoint);
        e->accept();
    }
}

void frmMessageBox::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        mousePressed = true;
        mousePoint = e->globalPos() - this->pos();
        e->accept();
    }
}

void frmMessageBox::mouseReleaseEvent(QMouseEvent *)
{
    mousePressed = false;
}

void frmMessageBox::on_btnOk_clicked()
{
    done(1);
    this->close();
}
