#ifndef PUBUSBDIALOG_H
#define PUBUSBDIALOG_H

#include <QDialog>
#include <QDebug>
#include <QScrollBar>
#include <QDesktopServices>

namespace Ui {
class PubUSBDialog;
}

class PubUSBDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PubUSBDialog(QDialog *parent = 0);
    ~PubUSBDialog();
    /**界面居中父窗口
     * @brief showParentCenter
     * @param parentFrm
     */

    void showParentCenter(QDialog *parentFrm);

    void initUiFontSize(double fontSizeRatio);

    const  int get_operation_status();
signals:

private slots:
    void on_SwitchButton_clicked();

    void on_CancelButton_clicked();

    void on_CloseButton_clicked();

private:
    Ui::PubUSBDialog *ui;
    int  m_operation_status;
};

#endif // PUBUSBDIALOG_H
