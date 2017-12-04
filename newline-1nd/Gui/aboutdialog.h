#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QFont>
namespace Ui {
class AboutDialog;
}

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = 0);
    ~AboutDialog();
    /**界面居中父窗口
     * @brief showParentCenter
     * @param parentFrm
     */
    void showParentCenter(QWidget *parentFrm);

    void initUiFontSize(double fontSizeRatio);
private slots:
    void on_closeButton_clicked();

    void on_okButton_clicked();

private:
    Ui::AboutDialog *ui;
};

#endif // ABOUTDIALOG_H
