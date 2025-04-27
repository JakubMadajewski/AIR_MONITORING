#ifndef DATETIMEAXISWIDGET_H
#define DATETIMEAXISWIDGET_H

#include "contentwidget.h"

class DateTimeAxisWidget : public ContentWidget
{
    Q_OBJECT
public:
    explicit DateTimeAxisWidget(QWidget *parent = nullptr);
    void myWidget(std::vector<QString> &vectorOfDates, std::vector<double>& valuesForGraph, QString * mainTitle = nullptr, QString * titleOfVerticalBar = nullptr);

protected:
    bool doLoad() override;
};

#endif // DATETIMEAXISWIDGET_H
