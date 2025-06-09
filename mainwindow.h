#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QLabel>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    struct StructRect   //Структура с габаритами деталей
    {
        quint32 ui32RectW;
        quint32 ui32RectH;
        QRgb rgbColor;
        bool bAllocate;
    };
    QList<StructRect> qlistRects;   //Список деталей

    QList<StructRect> qlistRectsSourceVert;   //Список источник - детали повёрнуты вертикально
    QList<StructRect> qlistRectsSourceHor;   //Список источник - детали повёрнуты горизонтально
    struct StructRectDest   //Структура с параметрами размещения
    {
        quint32 ui32X;
        quint32 ui32Y;
        quint32 ui32W;
        quint32 ui32H;
        QRgb rgbColor;
    };
    QList<StructRectDest> qlistRectsDestinationFFDH;   //Список с параметрами размещения FFDH
    QList<StructRectDest> qlistRectsDestinationFFDHV;   //Список с параметрами размещения FFDHV
    QList<StructRectDest> qlistRectsDestinationFFDHH;   //Список с параметрами размещения FFDHH
    QList<StructRectDest> qlistRectsDestinationFCNR;   //Список с параметрами размещения FCNR
    QList<StructRectDest> qlistRectsDestinationFCV;   //Список с параметрами размещения FCV

public slots:
    void slotMetSet();
    void slotRectSet();
    void slotRectsLoad();
    void slotRectsGenerate();
    void slotRectsClear();
    void slotComboBoxIndexCh();

private:
    Ui::MainWindow *ui;

    QString qtstrApplicationPath;

    quint32 ui32MetW;
    quint32 ui32MetH;

    QFile qtclMetFile;
    QFile qtclRectsFile;

    quint32 ui32TableRowsTotal;
    quint32 ui32TableRowsCurr;

    quint32 ui32MaxHeight;
    QLabel *pqtlbMaxHeight;
    quint32 ui32FFDHHeight;
    QLabel *pqtlbFFDHHeight;
    quint32 ui32FFDHVHeight;
    QLabel *pqtlbFFDHVHeight;
    quint32 ui32FFDHHHeight;
    QLabel *pqtlbFFDHHHeight;
    quint32 ui32FCNRHeight;
    QLabel *pqtlbFCNRHeight;
    quint32 ui32FCVHeight;
    QLabel *pqtlbFCVHeight;

    qint32 TestValue(QString strValue, quint32 ui32Border);
    void ErrorMessageBox(QString strMessageText);
    void TableAddRow();
    void MetToXml();
    void PlaceRects();
    void RectsToXML();
    void RectsXmlLoad(QString strFileName);
    void ClearTable();
    QRgb ColorGenerator();
//    bool IsAllAllocated(QList<StructRect> stctInc);

    struct StructFloor   //Структура с параметрами уровня
    {
        quint32 ui32FloorW;
        quint32 ui32FloorH;
        quint32 ui32FloorHF;
    };

protected:
    void paintEvent(QPaintEvent *);
};

#endif // MAINWINDOW_H
