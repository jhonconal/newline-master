#ifndef MONTAGEDIALOG_H
#define MONTAGEDIALOG_H

#include <QDialog>
#include <QDebug>
#include <QScrollBar>
#include <QDesktopServices>
namespace Ui {
class MontageDialog;
}

class MontageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MontageDialog(QWidget *parent = 0);
    ~MontageDialog();

    /**界面居中父窗口
     * @brief showParentCenter
     * @param parentFrm
     */
    void showParentCenter(QWidget *parentFrm);
    void initUiFontSize(double fontSizeRatio);
private slots:
    void on_activeButton_clicked();

    void on_installButton_clicked();
    void on_cancelButton_clicked();

    void on_closeButton_clicked();

signals:
    void signal_showMainDialog();
private:
    Ui::MontageDialog *ui;
};

#endif // MONTAGEDIALOG_H
