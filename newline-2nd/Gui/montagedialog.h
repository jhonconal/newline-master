#ifndef MONTAGEDIALOG_H
#define MONTAGEDIALOG_H

#include <QDialog>
#include <QDebug>
#include <QPoint>
#include <QMouseEvent>
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

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private slots:
    void on_activeButton_clicked();

    void on_installButton_clicked();
    void on_cancelButton_clicked();

    void on_closeButton_clicked();

signals:
    void signal_showMainDialog();
private:
    Ui::MontageDialog *ui;
    QPoint windowPos;
    QPoint mousePos;
    QPoint dPos;
};

#endif // MONTAGEDIALOG_H
