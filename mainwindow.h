#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <json/json.h>
#include <curl/curl.h>
#include <QTreeWidgetItem>
#include <QtConcurrent>
#include <QJsonObject>
#include <QJsonValue>
#include <QMessageBox>
#include <fstream>
#include <vector>
#include <QFutureWatcher>
#include <string>
#include "contentwidget.h"
#include "datetimeaxiswidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_Download_Database_clicked();

    void on_ListOfLocations_itemClicked(QTreeWidgetItem *item, int column);

    //void on_ListOfSensors_doubleClicked(const QModelIndex &index);

    void on_ListOfSensorsNew_itemClicked(QTreeWidgetItem *item, int column);

    void on_saveData_clicked();

    void on_loadOfflineDatabase_clicked();

private:
    enum mode{OFFLINE_MODE,ONLINE_MODE}; //Sets application in offline or online mode
    mode WORKING_MODE = ONLINE_MODE; //FOR ALL FUNCTIONS THAT NEEDS WORK DIFFRENTLY DEPENDING ON MODE!!!!!!
    Ui::MainWindow *ui;
    Json::Value privateRoot,bufferRoot; //mainDatabaseRoot
    QString bufferRootAsString; //main database root
    CURLcode updateDatabase();
    void handleUpdateDatabasePartTwo();
    //int experimentalUpdateDatabase();

    CURLcode updateDetailedDatabase(QString nameOfStation);

    QString bufferOfSensorsForParse; //List of sensors before parse;
    int handleSensorListUpdateErrors(CURLcode error);
    void handleSensorListUpdatePart2();

    QString filepathForPrepareDataAndDraw = "";
    Json::Value getDataFromCertainSensor(int indexOfSensorIdInBuffer, int dayNumber = 0);
    void prepareDataAndDraw();
    void calculateMeanValueFromDataDayByDay(std::vector<QString>*vectorOfDatesResult = nullptr, std::vector<double>*meanValuesForCorespondingDates = nullptr, Json::Value * rootExternal = nullptr);

    int handleSelectionWindow();

    void drawGraph(std::vector<QString>&vectorOfDates, std::vector<double>&valuesForGraph);

    bool lastThreeDaysSoDoNotUseMeanValues = false;
    std::vector<std::vector<QString>> bufferOfSensors; //index = 0 buffer of sensors Ids and index = 1 buffer of Names of sensors
    bool threadWorkingOnBuffer = false;
    QFutureWatcher<CURLcode>updateGeneralDatabase;
    QFutureWatcher<CURLcode>updateSensorsForCertainStation; //For getting list of sensors for certain station selected by user data are get via api call
    QFutureWatcher<Json::Value>getDataFromThisSensor; //For getting data from certain sensor via another api call
    //QStringListModel * model = nullptr;

    //For Graph display and saving
    DateTimeAxisWidget * displayWidget = nullptr;
    QString currentSelectedStationName = "", currentSelectedStationId = "", currentSelectedSensorName = "", currentSelectedSensorId = "";
    std::vector<QString>toSaveDates;
    std::vector<double>toSaveValues;
    enum NUMBER_OF_SELECTED_DAYS{ZERO,THREE,WEEK,MONTH,YEAR,CUSTOM};
    NUMBER_OF_SELECTED_DAYS CURRENT_DAYS = ZERO;
    int howManyDaysSelected = 0;//,customDaysSelected = 0;

    //For Data Analysis
    void meanValueDisplay(std::vector<double>&valuesForGraph);
    void maxValueDisplay(std::vector<QString>&vectorOfDates, std::vector<double>&valuesForGraph);
    void minValueDiplay(std::vector<QString>&vectorOfDates, std::vector<double>&valuesForGraph);

    //Error handle
    void showWindowWithError(QString error);
    int handleConnectionError(bool forListOfSensors = false);

    //void showWindowWithError(QString error);

};
#endif // MAINWINDOW_H
