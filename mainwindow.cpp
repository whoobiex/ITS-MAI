#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QtXml>
#include <QTextStream>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QPainter>
#include <QFileDialog>
#include <QColor>
#include <QScreen>
#include <algorithm>
#include <QVector>

#define MET_W 286
#define MET_H 15

#define MET_MAX_W 700
#define MET_MAX_H 1000000

#define MULTIPLIER 0.5   //Множитель в генераторе случайных размеров деталей
//------------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QRect rect = frameGeometry();
    QScreen *screen = QGuiApplication::primaryScreen();
    rect.moveCenter(screen->geometry().center());
    move(rect.topLeft());

    qsrand(QTime::currentTime().msec());
    qtstrApplicationPath = QApplication::applicationDirPath();

    // Загрузка габаритов материала
    qtclMetFile.setFileName(qtstrApplicationPath + "/met.xml");
    ui32MetW = MET_W;
    ui32MetH = MET_H;
    if (!qtclMetFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "Ошибка не удалось открыть xml-файл для чтения!" << endl;

        //сохраняем в xml
        MetToXml();

        ui->lineEdit_met_w_value->setText(QString("%1").arg(ui32MetW));
        ui->lineEdit_met_h_value->setText(QString("%1").arg(ui32MetH));
    }
    else
    {
        qDebug() << "Удалось открыть xml-файл для чтения!" << endl;
        QDomDocument docMet;
        if (docMet.setContent(&qtclMetFile))
        {
            QDomElement domEl = docMet.documentElement();
            QDomNode domNode = domEl.firstChild();
            QDomElement domGetEl = domNode.toElement();
            QString strTmp = "";
            if (!domGetEl.isNull())
            {
                qDebug() << "\tШирина: " << domGetEl.tagName() << domGetEl.text();
                qint32 i32Res = TestValue(domGetEl.text(), MET_MAX_W); //domGetEl.text().toUInt();
                if (i32Res != -1)
                    ui32MetW = i32Res;
            }
            domNode = domNode.nextSibling();
            domGetEl = domNode.toElement();
            if (!domGetEl.isNull())
            {
                qDebug() << "\tДлина: " << domGetEl.tagName() << domGetEl.text() << endl;
                qint32 i32Res = TestValue(domGetEl.text(), MET_MAX_H); //domGetEl.text().toUInt();
                if (i32Res != -1)
                    ui32MetH = i32Res;
            }
        }
        ui->lineEdit_met_w_value->setText(QString("%1").arg(ui32MetW));
        ui->lineEdit_met_h_value->setText(QString("%1").arg(ui32MetH));
        qtclMetFile.close();
    }
    qDebug() << "Ширина листа: " << ui32MetW << "мм. Длина листа: " << ui32MetH << " м." << endl;

    // Подключаем кнопки
    connect(ui->pushButton_met_set, SIGNAL(pressed()), SLOT(slotMetSet()));
    connect(ui->pushButton_rect_set, SIGNAL(pressed()), SLOT(slotRectSet()));
    connect(ui->pushButton_rects_load, SIGNAL(pressed()), SLOT(slotRectsLoad()));
    connect(ui->pushButton_rects_generate, SIGNAL(pressed()), SLOT(slotRectsGenerate()));
    connect(ui->pushButton_rects_clear, SIGNAL(pressed()), SLOT(slotRectsClear()));
    connect(ui->comboBox_algo, SIGNAL(activated(int )), SLOT(slotComboBoxIndexCh()));

    ui->comboBox_algo->addItem("FFDH (First Fit Decreasing High)");
    ui->comboBox_algo->addItem("FFDHV (First Fit Decreasing High Vert)");
    ui->comboBox_algo->addItem("FFDHH (First Fit Decreasing High Hor)");
//    ui->comboBox_algo->addItem("FCNR (Floor Ceiling No Rotation)");
//    ui->comboBox_algo->addItem("FCV (Floor Ceiling Vert)");

    ui32MaxHeight = 0;
    ui32FFDHHeight = 0;
    ui32FFDHVHeight = 0;
    ui32FFDHHHeight = 0;
//    ui32FCNRHeight = 0;
//    ui32FCVHeight = 0;

    pqtlbMaxHeight = new QLabel(this);
    pqtlbMaxHeight->setText("Максимальный расход: ");
    ui->statusBar->addWidget(pqtlbMaxHeight);
    pqtlbFFDHHeight = new QLabel(this);
    pqtlbFFDHHeight->setText(" FFDH: ");
    ui->statusBar->addWidget(pqtlbFFDHHeight);
    pqtlbFFDHVHeight = new QLabel(this);
    pqtlbFFDHVHeight->setText(" FFDHV: ");
    ui->statusBar->addWidget(pqtlbFFDHVHeight);
    pqtlbFFDHHHeight = new QLabel(this);
    pqtlbFFDHHHeight->setText(" FFDHH: ");
    ui->statusBar->addWidget(pqtlbFFDHHHeight);
//    pqtlbFCNRHeight = new QLabel(this);
//    pqtlbFCNRHeight->setText(" FCNR: ");
//    ui->statusBar->addWidget(pqtlbFCNRHeight);
//    pqtlbFCVHeight = new QLabel(this);
//    pqtlbFCVHeight->setText(" FCV: ");
//    ui->statusBar->addWidget(pqtlbFCVHeight);

    ui32TableRowsTotal = 20;
    ui32TableRowsCurr = 0;
    QStringList horizontalHeader;
    horizontalHeader.append("Ширина");
    horizontalHeader.append("Длина");
    ui->tableWidget->setHorizontalHeaderLabels(horizontalHeader);

    for (quint32 i = 0; i < ui32TableRowsTotal; i++)
        for (int j = 0; j < 2; j++)
        {
            QTableWidgetItem *ptWi = new QTableWidgetItem("");
            ui->tableWidget->setItem(i, j, ptWi);
        }
    ui->tableWidget->verticalHeader()->setVisible(true);

    qlistRects.clear();
    qlistRectsSourceVert.clear();
    qlistRectsSourceHor.clear();
    qlistRectsDestinationFFDH.clear();
    qlistRectsDestinationFFDHV.clear();
    qlistRectsDestinationFFDHH.clear();
    qlistRectsDestinationFCNR.clear();
    qlistRectsDestinationFCV.clear();
    QWidget::repaint();
    // Загружаем список деталей
    RectsXmlLoad(qtstrApplicationPath + "/rects.xml");
}
//------------------------------------------------------------------------------
qint32 MainWindow::TestValue(QString strValue, quint32 ui32Border)
{
    //Проверка корректности вводимого размера
    quint32 ui32Res = strValue.toUInt();

    if ((ui32Res)&&(ui32Res <= ui32Border))
        return ui32Res;
    else
    {
        qDebug() << "Некорректный размер: " << strValue << endl;
        ErrorMessageBox("Некорректный размер!");
        return -1;
    }
}
//------------------------------------------------------------------------------
void MainWindow::MetToXml()
{
    //Сохраняем параметры листа в XML файл
    qtclMetFile.open(QIODevice::WriteOnly);
    QDomDocument doc("met");
    QDomElement metGeom = doc.createElement("geometry");
    doc.appendChild(metGeom);

    QDomElement metW = doc.createElement("metW");
//        QDomAttr metAtrW = doc.createAttribute("metW");
//        metAtrW.setValue("metW");
//        metW.setAttributeNode(metAtrW);
    QDomText domTextW = doc.createTextNode(QString("%1").arg(ui32MetW));
    metW.appendChild(domTextW);
    metGeom.appendChild(metW);

    QDomElement metH = doc.createElement("metH");
//        QDomAttr metAtrH = doc.createAttribute("metH");
//        metAtrH.setValue("metH");
//        metH.setAttributeNode(metAtrH);
    QDomText domTextH = doc.createTextNode(QString("%1").arg(ui32MetH));
    metH.appendChild(domTextH);
    metGeom.appendChild(metH);

    doc.appendChild(metGeom);
    QTextStream(&qtclMetFile) << doc.toString();
    qtclMetFile.close();
}
//------------------------------------------------------------------------------
void MainWindow::slotMetSet()
{
    //Обработчик кнопки: Изменить размер листа металла
    qint32 i32Res = TestValue(ui->lineEdit_met_w_value->text(), MET_MAX_W);
    if (i32Res != -1)
        ui32MetW = i32Res;
    else
    {
        ui32MetW = MET_W;
        ui->lineEdit_met_w_value->setText(QString("%1").arg(ui32MetW));
    }

    i32Res = TestValue(ui->lineEdit_met_h_value->text(), MET_MAX_H);
    if (i32Res != -1)
        ui32MetH = i32Res;
    else
    {
        ui32MetH = MET_H;
        ui->lineEdit_met_h_value->setText(QString("%1").arg(ui32MetH));
    }

    qDebug() << "Ширина листа: " << ui32MetW << " мм. Длина листа: " << ui32MetH << " м." << endl;

    //сохраняем в xml
    MetToXml();

    // запускаем размещение
    PlaceRects();
}
//------------------------------------------------------------------------------
void MainWindow::ErrorMessageBox(QString strMessageText)
{
    //Сообщение об ошибке
    QMessageBox *pqtclErrorMessage = new QMessageBox(QMessageBox::Critical,
                                                     "Ошибка",
                                                     strMessageText,
                                                     QMessageBox::Ok);
    pqtclErrorMessage->exec();
    delete pqtclErrorMessage;
}
//------------------------------------------------------------------------------
void MainWindow::slotRectSet()
{
    //Обработчик кнопки: Добавить новую деталь
    qint32 i32Res = TestValue(ui->lineEdit_rect_w_value->text(), ui32MetW);
    if (i32Res != -1)
    {
        StructRect stctRect;
        stctRect.ui32RectW = i32Res;

        i32Res = TestValue(ui->lineEdit_rect_h_value->text(), ui32MetW*1000);
        if (i32Res != -1)
        {
            stctRect.ui32RectH = i32Res;
            stctRect.rgbColor = ColorGenerator();
            qlistRects.append(stctRect);
            qDebug() << "Ширина детали: " << ui32MetW << " мм. Длина детали: " << ui32MetH << " мм." << endl;
            if (ui32TableRowsCurr == ui32TableRowsTotal)
                TableAddRow();
            ui->tableWidget->item(ui32TableRowsCurr, 0)->setText(QString("%1").arg(stctRect.ui32RectW));
            ui->tableWidget->item(ui32TableRowsCurr++, 1)->setText(QString("%1").arg(stctRect.ui32RectH));
            RectsToXML();
            PlaceRects();
        }
        else
            ErrorMessageBox("Длина детали больше длины листа!");
    }
    else
        ErrorMessageBox("Ширина детали больше ширины листа!");
}
//------------------------------------------------------------------------------
void MainWindow::TableAddRow()
{
    ui->tableWidget->insertRow(ui32TableRowsTotal);
    for (int j = 0; j < 2; j++)
    {
        QTableWidgetItem *ptWi = new QTableWidgetItem("");
        ui->tableWidget->setItem(ui32TableRowsTotal, j, ptWi);
    }
    ui32TableRowsTotal += 1;
}
//------------------------------------------------------------------------------
void MainWindow::slotRectsLoad()
{
    //Обработчик кнопки: Загрузить файл с размерами деталей
    QString srtFileName = QFileDialog::getOpenFileName(this, QString(""), qtstrApplicationPath,
                                            "Objects (*.xml)");
    RectsXmlLoad(srtFileName);
}
//------------------------------------------------------------------------------
void MainWindow::RectsXmlLoad(QString strFileName)
{   //Загружаем детали из XML файла
    QFile qtclRectsCustFile;
    qtclRectsCustFile.setFileName(strFileName);

    if (qtclRectsCustFile.open(QIODevice::ReadOnly))
    {
        QDomDocument docRects;
        if (docRects.setContent(&qtclRectsCustFile))
        {
            QDomElement domEl = docRects.documentElement();
            QDomNode domNode = domEl.firstChild();

            StructRect stctRect;
            while(!domNode.isNull())
            {
                QDomNode domRect = domNode.firstChild();
                QDomElement domGetEl = domRect.toElement();

                QString strTmp = "";
                if (!domGetEl.isNull())
                {
                    qint32 i32Res = TestValue(domGetEl.text(), ui32MetW);
                    if (i32Res != -1)
                        stctRect.ui32RectW = i32Res;
                }
                domRect = domRect.nextSibling();
                domGetEl = domRect.toElement();
                if (!domGetEl.isNull())
                {
                    qint32 i32Res = TestValue(domGetEl.text(), ui32MetH*1000);
                    if (i32Res != -1)
                        stctRect.ui32RectH = i32Res;
                }
                domRect = domRect.nextSibling();
                domGetEl = domRect.toElement();
                if (!domGetEl.isNull())
                {
                    stctRect.rgbColor = QRgb(domGetEl.text().toLongLong());
                }
                qlistRects.append(stctRect);
                if (ui32TableRowsCurr == ui32TableRowsTotal)
                    TableAddRow();
                ui->tableWidget->item(ui32TableRowsCurr, 0)->setText(QString("%1").arg(stctRect.ui32RectW));
                ui->tableWidget->item(ui32TableRowsCurr++, 1)->setText(QString("%1").arg(stctRect.ui32RectH));
                qDebug() << "Новая деталь - ширина: " << stctRect.ui32RectW << " мм. длина: " << stctRect.ui32RectH << " мм.";

                domNode = domNode.nextSibling();
            }
        }
        qtclRectsCustFile.close();
    }
    qDebug() << "Загрузили файл с размерами деталей" << endl; //Отладочное сообщение
    RectsToXML();

    if (qlistRects.size())
        PlaceRects();
}
//------------------------------------------------------------------------------
void MainWindow::slotRectsGenerate()
{
    //Обработчик кнопки: Генерировать 10 деталей с произвольными размерами
    qDebug() << "Генерируем 10 деталей с произвольными размерами";
    StructRect stctRect;
    for(int i=0; i<10; i++)
    {
        stctRect.ui32RectW = qrand() % int(ui32MetW * MULTIPLIER - 10) + 10;
        stctRect.ui32RectH = qrand() % int(ui32MetW * MULTIPLIER - 10) + 10;
        stctRect.rgbColor = ColorGenerator();
        qlistRects.append(stctRect);
        qDebug() << "\tШирина детали: " << stctRect.ui32RectW << " мм. Длина детали: " << stctRect.ui32RectH << " мм.";
        if (ui32TableRowsCurr == ui32TableRowsTotal)
            TableAddRow();
        ui->tableWidget->item(ui32TableRowsCurr, 0)->setText(QString("%1").arg(stctRect.ui32RectW));
        ui->tableWidget->item(ui32TableRowsCurr++, 1)->setText(QString("%1").arg(stctRect.ui32RectH));
    }
    RectsToXML();
    PlaceRects();
}
//------------------------------------------------------------------------------
void MainWindow::slotRectsClear()
{
    //Обработчик кнопки: Очистить список деталей
    qDebug() << "Очистить список деталей" << endl; //Отладочное сообщение

    qlistRects.clear();
    qlistRectsSourceVert.clear();
    qlistRectsSourceHor.clear();
    qlistRectsDestinationFFDH.clear();
    qlistRectsDestinationFFDHV.clear();
    qlistRectsDestinationFFDHH.clear();
    qlistRectsDestinationFCNR.clear();
    qlistRectsDestinationFCV.clear();
    ClearTable(); //Очищаем таблицу
    RectsToXML(); //Очищаем XML файл
    QWidget::repaint(); //Очищаем изображение
}
//------------------------------------------------------------------------------
void MainWindow::RectsToXML()
{
    //Сохраняем детали в файл
    qtclRectsFile.setFileName(qtstrApplicationPath + "/rects.xml");
    qtclRectsFile.open(QIODevice::WriteOnly);
    QDomDocument doc("rects");
    QDomElement rects = doc.createElement("rects");
    doc.appendChild(rects);

    for (qsizetype i = 0; i < qlistRects.size(); ++i)
    {
//        qDebug() << "\tШирина: " << qlistRects.at(i).ui32RectW << "Длина: " << qlistRects.at(i).ui32RectH;

        QDomElement rect = doc.createElement("rect");
        QDomAttr rectAtr = doc.createAttribute("number");
        rectAtr.setValue(QString("%1").arg(i));
        rect.setAttributeNode(rectAtr);
        rects.appendChild(rect);

        QDomElement rectW = doc.createElement("rectW");
        QDomText domTextW = doc.createTextNode(QString("%1").arg(qlistRects.at(i).ui32RectW));
        rectW.appendChild(domTextW);
        rect.appendChild(rectW);

        QDomElement rectH = doc.createElement("rectH");
        QDomText domTextH = doc.createTextNode(QString("%1").arg(qlistRects.at(i).ui32RectH));
        rectH.appendChild(domTextH);
        rect.appendChild(rectH);

        QDomElement rectC = doc.createElement("rectC");
        QDomText domTextC = doc.createTextNode(QString("%1").arg(qlistRects.at(i).rgbColor));
        rectC.appendChild(domTextC);
        rect.appendChild(rectC);
    }

    doc.appendChild(rects);
    QTextStream(&qtclRectsFile) << doc.toString();
    qtclRectsFile.close();

    qDebug() << "Сохранили детали в файл" << endl; //Отладочное сообщение
}
//------------------------------------------------------------------------------
void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    //Рисуем чистый лист
    QBrush brush(Qt::white, Qt::SolidPattern);
    painter.fillRect(ui->widget_left_top->width() + 8,
                     3,
                     ui32MetW+2,
                     ui->widget_viz->height()-3,
                     brush);
    painter.setPen(Qt::black);
    painter.drawRect(QRect(ui->widget_left_top->width() + 8,
                           3,
                           ui32MetW+2,
                           ui->widget_viz->height()-3));

    int iInd = ui->comboBox_algo->currentIndex();
    QList<StructRectDest> *ptDest;
    switch (iInd) {
    case 0: ptDest = &qlistRectsDestinationFFDH;
        break;
    case 1: ptDest = &qlistRectsDestinationFFDHV;
        break;
    case 2: ptDest = &qlistRectsDestinationFFDHH;
        break;
    default:
        ptDest = &qlistRectsDestinationFFDH;
    }

    if ((*ptDest).size())
    {
        for (qsizetype i = 0; i < (*ptDest).size(); ++i)
        {
            QBrush brush((*ptDest).at(i).rgbColor, Qt::Dense3Pattern);   //Qt::SolidPattern);
            if (3 + (*ptDest).at(i).ui32Y < (unsigned int)ui->widget_viz->height())
            {
                quint32 ui32H;
                if (3 + (*ptDest).at(i).ui32Y + (*ptDest).at(i).ui32H > (unsigned int)ui->widget_viz->height())
                    ui32H = (unsigned int)ui->widget_viz->height() - (3 + 1 + (*ptDest).at(i).ui32Y);
                else
                    ui32H = (*ptDest).at(i).ui32H;

                painter.fillRect(ui->widget_left_top->width() + 8 + 1 + (*ptDest).at(i).ui32X,
                                 3 + 1 + (*ptDest).at(i).ui32Y,
                                 (*ptDest).at(i).ui32W,
                                 ui32H,
                                 brush);
                painter.setPen(Qt::black); //qlistRectsDestination.at(i).rgbColor);
                painter.drawRect(QRect(ui->widget_left_top->width() + 8 + 1 + (*ptDest).at(i).ui32X,
                                       3 + 1 + (*ptDest).at(i).ui32Y,
                                       (*ptDest).at(i).ui32W,
                                       ui32H));
            }
        }
    }
}
//------------------------------------------------------------------------------
void MainWindow::ClearTable()
{
    //Очищаем таблицу
    for(quint32 i = 0; i < ui32TableRowsTotal; i++)
        for(int j = 0; j < 2; j++)
        {
            ui->tableWidget->item(i, 0)->setText("");
            ui->tableWidget->item(i, 1)->setText("");
        }
    ui32TableRowsCurr = 0;
}
//------------------------------------------------------------------------------
bool WidthGreater(const MainWindow::StructRect& El_1, const MainWindow::StructRect& El_2)
{//Функция для сортировки по невозрастнию
    return El_1.ui32RectH > El_2.ui32RectH;
}
//------------------------------------------------------------------------------
//bool MainWindow::IsAllAllocated(QList<StructRect> stctInc)
//{
//    bool bAllAllocated = true;
//    for (qsizetype i = 0; i < stctInc.size(); ++i)
//        if (!stctInc.at(i).bAllocate)
//        {
//            bAllAllocated = false;
//            break;
//        }

//    return bAllAllocated;
//}
//------------------------------------------------------------------------------
void MainWindow::PlaceRects()
{
    //Размещаем детали
    qDebug() << "Размещаем детали";

//Подготовка: готовим списки источники, рассчитываем максимальный расход
    ui32MaxHeight = 0;
    qlistRectsSourceVert.clear();
    qlistRectsSourceHor.clear();

    StructRect stctRectsSourceVert;
    StructRect stctRectsSourceHor;
    for (qsizetype i = 0; i < qlistRects.size(); i++)
    {
        qlistRects[i].bAllocate = false;
        //Поворачиваем детали вертикально
        if(qlistRects.at(i).ui32RectH >= qlistRects.at(i).ui32RectW)
        {
            ui32MaxHeight += qlistRects.at(i).ui32RectH;
            stctRectsSourceVert.ui32RectW = qlistRects.at(i).ui32RectW;
            stctRectsSourceVert.ui32RectH = qlistRects.at(i).ui32RectH;
        }
        else
        {
            ui32MaxHeight += qlistRects.at(i).ui32RectW;
            stctRectsSourceVert.ui32RectW = qlistRects.at(i).ui32RectH;
            stctRectsSourceVert.ui32RectH = qlistRects.at(i).ui32RectW;
        }
        stctRectsSourceVert.rgbColor = qlistRects.at(i).rgbColor;
        stctRectsSourceVert.bAllocate = false;
        qlistRectsSourceVert.append(stctRectsSourceVert);

        //Поворачиваем детали горизонтально
        if((qlistRects.at(i).ui32RectW >= qlistRects.at(i).ui32RectH)||(qlistRects.at(i).ui32RectH > ui32MetW))
        {
            stctRectsSourceHor.ui32RectW = qlistRects.at(i).ui32RectW;
            stctRectsSourceHor.ui32RectH = qlistRects.at(i).ui32RectH;
        }
        else
        {
            stctRectsSourceHor.ui32RectW = qlistRects.at(i).ui32RectH;
            stctRectsSourceHor.ui32RectH = qlistRects.at(i).ui32RectW;
        }
        stctRectsSourceHor.rgbColor = qlistRects.at(i).rgbColor;
        stctRectsSourceHor.bAllocate = false;
        qlistRectsSourceHor.append(stctRectsSourceHor);
    }
    qDebug() << "\tМаксимальный расход: " << ui32MaxHeight << " мм.";
    pqtlbMaxHeight->setText(QString("Максимальный расход: %1 мм").arg(ui32MaxHeight));

    if (qlistRects.size())
    {
        std::sort(qlistRects.begin(), qlistRects.end(), &WidthGreater);
        std::sort(qlistRectsSourceVert.begin(), qlistRectsSourceVert.end(), &WidthGreater);
        std::sort(qlistRectsSourceHor.begin(), qlistRectsSourceHor.end(), &WidthGreater);
    }

//FFDH (First Fit Decreasing High)
    qlistRectsDestinationFFDH.clear();
    //Формируем список назначения (с параметрами размещения)
    ui32FFDHHeight = 0;
    if(qlistRects.size())
    {
        StructRectDest stctRectsDestination;
        StructFloor stctFloor;
        stctFloor.ui32FloorW = 0;
        stctFloor.ui32FloorH = 0;
        stctFloor.ui32FloorHF = qlistRects.at(0).ui32RectH;
        ui32FFDHHeight += qlistRects.at(0).ui32RectH;
        QList<StructFloor> qlistFloor;
        qlistFloor.append(stctFloor);

        for (qsizetype i = 0; i < qlistRects.size(); i++)
        {
            int FloorNumber = 0;
            for (; FloorNumber < qlistFloor.size(); FloorNumber++)
            {
                if (qlistRects.at(i).ui32RectW <= ui32MetW - qlistFloor.at(FloorNumber).ui32FloorW)
                {
                    qlistRects[i].bAllocate = true;
                    break;
                }
            }
            if (!qlistRects.at(i).bAllocate)
            {
                stctFloor.ui32FloorW = 0;
                stctFloor.ui32FloorH = qlistFloor.at(FloorNumber-1).ui32FloorHF;
                stctFloor.ui32FloorHF = qlistFloor.at(FloorNumber-1).ui32FloorHF + qlistRects.at(i).ui32RectH;
                ui32FFDHHeight += qlistRects.at(i).ui32RectH;
                qlistFloor.append(stctFloor);
            }

            stctRectsDestination.ui32X = qlistFloor.at(FloorNumber).ui32FloorW;
            stctRectsDestination.ui32Y = qlistFloor.at(FloorNumber).ui32FloorH;
            stctRectsDestination.ui32W = qlistRects.at(i).ui32RectW;
            stctRectsDestination.ui32H = qlistRects.at(i).ui32RectH;
            stctRectsDestination.rgbColor = qlistRects.at(i).rgbColor;
            qlistFloor[FloorNumber].ui32FloorW += stctRectsDestination.ui32W;
            qlistRectsDestinationFFDH.append(stctRectsDestination);
        }
    }
    qDebug() << "\tFFDH расход: " << ui32FFDHHeight << " мм.";
    pqtlbFFDHHeight->setText(QString(" FFDH: %1 ").arg(ui32FFDHHeight));

//FFDHV (First Fit Decreasing High Vert)
    qlistRectsDestinationFFDHV.clear();
    //Формируем список назначения (с параметрами размещения)
    ui32FFDHVHeight = 0;
    if(qlistRectsSourceVert.size())
    {
        StructRectDest stctRectsDestination;
        StructFloor stctFloor;
        stctFloor.ui32FloorW = 0;
        stctFloor.ui32FloorH = 0;
        stctFloor.ui32FloorHF = qlistRectsSourceVert.at(0).ui32RectH;
        ui32FFDHVHeight += qlistRectsSourceVert.at(0).ui32RectH;
        QList<StructFloor> qlistFloor;
        qlistFloor.append(stctFloor);

        for (qsizetype i = 0; i < qlistRectsSourceVert.size(); i++)
        {
            int FloorNumber = 0;
            for (; FloorNumber < qlistFloor.size(); FloorNumber++)
            {
                if (qlistRectsSourceVert.at(i).ui32RectW <= ui32MetW - qlistFloor.at(FloorNumber).ui32FloorW)
                {
                    qlistRectsSourceVert[i].bAllocate = true;
                    break;
                }
            }
            if (!qlistRectsSourceVert.at(i).bAllocate)
            {
                stctFloor.ui32FloorW = 0;
                stctFloor.ui32FloorH = qlistFloor.at(FloorNumber-1).ui32FloorHF;
                stctFloor.ui32FloorHF = qlistFloor.at(FloorNumber-1).ui32FloorHF + qlistRectsSourceVert.at(i).ui32RectH;
                ui32FFDHVHeight += qlistRectsSourceVert.at(i).ui32RectH;
                qlistFloor.append(stctFloor);
            }

            stctRectsDestination.ui32X = qlistFloor.at(FloorNumber).ui32FloorW;
            stctRectsDestination.ui32Y = qlistFloor.at(FloorNumber).ui32FloorH;
            stctRectsDestination.ui32W = qlistRectsSourceVert.at(i).ui32RectW;
            stctRectsDestination.ui32H = qlistRectsSourceVert.at(i).ui32RectH;
            stctRectsDestination.rgbColor = qlistRectsSourceVert.at(i).rgbColor;
            qlistFloor[FloorNumber].ui32FloorW += stctRectsDestination.ui32W;
            qlistRectsDestinationFFDHV.append(stctRectsDestination);
        }
    }
    qDebug() << "\tFFDHV расход: " << ui32FFDHVHeight << " мм.";
    pqtlbFFDHVHeight->setText(QString(" FFDHV: %1 ").arg(ui32FFDHVHeight));

//FFDHH (First Fit Decreasing High Hor)
    qlistRectsDestinationFFDHH.clear();
    //Формируем список назначения (с параметрами размещения)
    ui32FFDHHHeight = 0;
    if(qlistRectsSourceHor.size())
    {
        StructRectDest stctRectsDestination;
        StructFloor stctFloor;
        stctFloor.ui32FloorW = 0;
        stctFloor.ui32FloorH = 0;
        stctFloor.ui32FloorHF = qlistRectsSourceHor.at(0).ui32RectH;
        ui32FFDHHHeight += qlistRectsSourceHor.at(0).ui32RectH;
        QList<StructFloor> qlistFloor;
        qlistFloor.append(stctFloor);

        for (qsizetype i = 0; i < qlistRectsSourceHor.size(); i++)
        {
            int FloorNumber = 0;
            for (; FloorNumber < qlistFloor.size(); FloorNumber++)
            {
                if (qlistRectsSourceHor.at(i).ui32RectW <= ui32MetW - qlistFloor.at(FloorNumber).ui32FloorW)
                {
                    qlistRectsSourceHor[i].bAllocate = true;
                    break;
                }
            }
            if (!qlistRectsSourceHor.at(i).bAllocate)
            {
                stctFloor.ui32FloorW = 0;
                stctFloor.ui32FloorH = qlistFloor.at(FloorNumber-1).ui32FloorHF;
                stctFloor.ui32FloorHF = qlistFloor.at(FloorNumber-1).ui32FloorHF + qlistRectsSourceHor.at(i).ui32RectH;
                ui32FFDHHHeight += qlistRectsSourceHor.at(i).ui32RectH;
                qlistFloor.append(stctFloor);
            }

            stctRectsDestination.ui32X = qlistFloor.at(FloorNumber).ui32FloorW;
            stctRectsDestination.ui32Y = qlistFloor.at(FloorNumber).ui32FloorH;
            stctRectsDestination.ui32W = qlistRectsSourceHor.at(i).ui32RectW;
            stctRectsDestination.ui32H = qlistRectsSourceHor.at(i).ui32RectH;
            stctRectsDestination.rgbColor = qlistRectsSourceHor.at(i).rgbColor;
            qlistFloor[FloorNumber].ui32FloorW += stctRectsDestination.ui32W;
            qlistRectsDestinationFFDHH.append(stctRectsDestination);
        }
    }
    qDebug() << "\tFFDHH расход: " << ui32FFDHHHeight << " мм." << endl;
    pqtlbFFDHHHeight->setText(QString(" FFDHH: %1 ").arg(ui32FFDHHHeight));

//FCNR (Floor Ceiling No Rotation)
//    pass

//FCV (Floor Ceiling Vert)
//    pass

    QVector<quint32> vInd;
    vInd.push_back(ui32FFDHHeight);
    vInd.push_back(ui32FFDHVHeight);
    vInd.push_back(ui32FFDHHHeight);
    int ind = 0;
    quint32 ui32MinV = ui32MaxHeight;
    for(int i = 0; i < vInd.size(); i++)
        if (ui32MinV >= vInd[i])
        {
            ui32MinV = vInd[i];
            ind = i;
        }

    ui->comboBox_algo->setCurrentIndex(ind);
    QWidget::repaint();
}
//------------------------------------------------------------------------------
QRgb MainWindow::ColorGenerator()
{
    //Генератор цвета  //    QRgb rgb;
    int r, g, b;
    do
    {
        r = rand() % 255;
        g = rand() % 255;
        b = rand() % 255;
    }
    while((r+g+b > 650)||(r+g+b < 300));

//    qDebug() << "Генерируем случайный цвет" << rgb << QColor::fromRgb(rgb) << endl; //Отладочное сообщение
    return qRgba(r, g, b, 255);
}
//------------------------------------------------------------------------------
void MainWindow::slotComboBoxIndexCh()
{
    //Перерисовываем при изменении алгоритма
    QWidget::repaint();
}
//------------------------------------------------------------------------------
MainWindow::~MainWindow()
{
    delete ui;
}
//------------------------------------------------------------------------------
