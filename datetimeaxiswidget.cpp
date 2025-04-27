#include "datetimeaxiswidget.h"

#include <QChart>
#include <QDateTime>
#include <QDateTimeAxis>
#include <QFile>
#include <QLineSeries>
#include <QTextStream>
#include <QValueAxis>

DateTimeAxisWidget::DateTimeAxisWidget(QWidget *parent)
    : ContentWidget(parent)
{
}

void DateTimeAxisWidget::myWidget(std::vector<QString> &vectorOfDates, std::vector<double>& valuesForGraph, QString * mainTitle , QString * titleOfVerticalBar)
{
    auto series = new QLineSeries;
    auto chart = new QChart;
    for(int i =0;i<vectorOfDates.size();++i)
    {
        QStringList value = vectorOfDates[i].split(QLatin1Char(' '),Qt::SkipEmptyParts);
        //QStringList value = vectorOfDates[i].split(QLatin1Char('-'),Qt::SkipEmptyParts);
        QStringList value1 = value[0].split(QLatin1Char('-'),Qt::SkipEmptyParts);
        QDateTime momentInTime;
        momentInTime.setDate(QDate(value1[0].toInt(),value1[1].toInt(),value1[2].toInt()));
        series->append(momentInTime.toMSecsSinceEpoch()+(value[1].toInt()*3600000),valuesForGraph[i]);
    }

    //![3]
    //auto chart = new QChart;
    chart->addSeries(series);
    chart->legend()->hide();
    if(mainTitle)
        chart->setTitle(*mainTitle);
    else
        chart->setTitle("Data from selected sensor");
    //![3]

    //![4]
    auto axisX = new QDateTimeAxis;
    axisX->setTickCount(5);
    axisX->setFormat("dd MMM yyyy");
    axisX->setTitleText("Date");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto axisY = new QValueAxis;
    axisY->setLabelFormat("%i");
    if(titleOfVerticalBar)
        axisY->setTitleText(*titleOfVerticalBar);
    else
        axisY->setTitleText("Sunspots count");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    //![4]

    //![5]
    createDefaultChartView(chart);
    //![5]
}

bool DateTimeAxisWidget::doLoad()
{
    //![1]
    auto series = new QLineSeries;
    //![1]

    //![2]
    // data from http://www.swpc.noaa.gov/ftpdir/weekly/RecentIndices.txt
    // http://www.swpc.noaa.gov/ftpdir/weekly/README
    // http://www.weather.gov/disclaimer
    QFile sunSpots(":sun_spots");
    if (!sunSpots.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_loadError = QStringLiteral("Failed to load '%1' file.").arg(sunSpots.fileName());
        return false;
    }

    QTextStream stream(&sunSpots);
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (line.startsWith("#") || line.startsWith(":"))
            continue;
        QStringList values = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        QDateTime momentInTime;
        momentInTime.setDate(QDate(values[0].toInt(), values[1].toInt() , 15));
        series->append(momentInTime.toMSecsSinceEpoch(), values[2].toDouble());
    }
    sunSpots.close();
    //![2]

    //![3]
    auto chart = new QChart;
    chart->addSeries(series);
    chart->legend()->hide();
    chart->setTitle("Sunspots count (by Space Weather Prediction Center)");
    //![3]

    //![4]
    auto axisX = new QDateTimeAxis;
    axisX->setTickCount(10);
    axisX->setFormat("MMM yyyy");
    axisX->setTitleText("Date");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto axisY = new QValueAxis;
    axisY->setLabelFormat("%i");
    axisY->setTitleText("Sunspots count");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    //![4]

    //![5]
    createDefaultChartView(chart);
    //![5]

    return true;
}
