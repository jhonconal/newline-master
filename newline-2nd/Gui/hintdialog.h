#ifndef HINTDIALOG_H
#define HINTDIALOG_H

#include <QDialog>
#include <QDebug>
#include <QPoint>
#include <QMouseEvent>
namespace Ui {
class HintDialog;
}

class HintDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HintDialog(QWidget *parent = 0);
    ~HintDialog();

    /**界面居中父窗口
     * @brief showParentCenter
     * @param parentFrm
     */
    void showParentCenter(QWidget *parentFrm);

    void setBackground(const QString &color);

    void setMassage(const QString &msg,int type);

    void setTitle(const QString &title);

    void initUiFontSize(double fontSizeRatio);

    void setCancelButtonEnable(bool flag);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private slots:
    void on_okButton_clicked();

    void on_closeButton_clicked();

    void on_cancelButton_clicked();

private:
    Ui::HintDialog *ui;
    QPoint windowPos;
    QPoint mousePos;
    QPoint dPos;
};

#endif // HINTDIALOG_H
