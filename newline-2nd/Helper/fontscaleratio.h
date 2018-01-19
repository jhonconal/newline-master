#ifndef FONTSCALERATIO_H
#define FONTSCALERATIO_H

#include <QObject>
#include <QWidget>
#include <QMainWindow>
#include <QFont>
#include <QApplication>
#include <QDebug>
#include <QFontMetrics>
class FontScaleRatio : public QObject
{
    Q_OBJECT
public:
    explicit FontScaleRatio(QObject *parent = 0);
    static FontScaleRatio*Instance()
    {
        static QMutex mutex;
        if(!self)
        {
            QMutexLocker locker(&mutex);
            if(!self)
            {
                self = new FontScaleRatio;
            }
        }
        return self;
    }

    /**
     * @brief PUBComClass::Pub_GetFontPixelRatio
     * @return
     * 例如，100% 缩放比例下，1000 号的 Courier New 字体，
     * 一个字母所占的宽度是 800 像素。进而计算缩放比例的代码如下：
     */
    double getFontScaleRatio();

    void setGuiFont(int fontPointSize,QWidget*widget);
    void setGuiFont(QFont font,QWidget *widget);
    void setGuiFont(QFont font,int fontPointSize,QWidget *widget);
    void setGuiFont(int fontPointSize,QMainWindow*window);
    void setGuiFont(const QString fontName,int fontPointSize,QWidget *widget);
    void setGuiFont(const QString fontName,int fontPointSize,QMainWindow*window);
private:
    static FontScaleRatio*self;    //对象自身
    double m_fontScaleRatio;//系统缩放比例

};

#endif // FONTSCALERATIO_H
