#include "fontscaleratio.h"
FontScaleRatio *FontScaleRatio::self =0;

FontScaleRatio::FontScaleRatio(QObject *parent) : QObject(parent)
  ,m_fontScaleRatio(getFontScaleRatio())
{
    m_fontScaleRatio = getFontScaleRatio();
}

double FontScaleRatio::getFontScaleRatio()
{
        //# the width of a character when dpi = 96
        int base =800;
        QFont font ;
        font.setFamily("Courier New");
        font.setPointSize(1000);
        QFontMetrics font_metrics = QFontMetrics(font);
        double width = font_metrics.width('Q');
        m_fontScaleRatio = width/base;
//        qDebug()<<"System font scales ratio: "<<m_fontScaleRatio;
        return m_fontScaleRatio;
}

void FontScaleRatio::setGuiFont(int fontPointSize,QWidget *widget)
{
    QFont font;
    font.setFamily(font.defaultFamily());
    if(m_fontScaleRatio>0)
        font.setPointSize(qRound(fontPointSize/m_fontScaleRatio));
    widget->setFont(font);
}

void FontScaleRatio::setGuiFont(QFont font, QWidget *widget)
{
    widget->setFont(font);
    qDebug()<<"1----->"<<font.toString();
}
/**
 * @brief FontScaleRatio::setGuiFont
 * @param font
 * @param fontPointSize
 * @param widget
 * 程序自定义字体
 */
void FontScaleRatio::setGuiFont(QFont font, int fontPointSize, QWidget *widget)
{
    font.setPixelSize(fontPointSize/m_fontScaleRatio);
    qDebug()<<"2----->"<<font.toString();
    widget->setFont(font);
}

void FontScaleRatio::setGuiFont(int fontPointSize, QMainWindow *window)
{
    QFont font;
    font.setFamily(font.defaultFamily());
    if(m_fontScaleRatio>0)
        font.setPointSize(qRound(fontPointSize/m_fontScaleRatio));
    window->setFont(font);
}

void FontScaleRatio::setGuiFont(const QString fontName,int fontPointSize,QWidget *widget)
{
    QFont font;
    font.setFamily(tr("%1").arg(fontName));
    if(m_fontScaleRatio>0)
        font.setPointSize(qRound(fontPointSize/m_fontScaleRatio));
    widget->setFont(font);
}

void FontScaleRatio::setGuiFont(const QString fontName, int fontPointSize, QMainWindow *window)
{
    QFont font;
    font.setFamily(tr("%1").arg(fontName));
    if(m_fontScaleRatio>0)
        font.setPointSize(qRound(fontPointSize/m_fontScaleRatio));
    window->setFont(font);
}

