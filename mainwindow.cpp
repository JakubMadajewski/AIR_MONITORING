#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "API_CALL.h"
#include <filesystem>
#include <QInputDialog>

//#include <QChart>
//#include <QValueAxis>
//#include <qlineseries.h>
//#include <QtCharts/qlineseries.h>
//#include <QtCharts/QDateTimeAxis>
//#include <QValueAxis>

/**
 * @brief MainWindow::MainWindow
 * @param parent
 * This is our constructor of class MainWindow which is our main class in this project.
 * Only important thing here are these connect statements, they are our core multithreading solution.
 * Update General Database is QFutureWatcher object which is used for updating main database of all stations across our country
 * Update sensor for certain station is QFutureWatcher object associated later with function that updates list of sensors for certain selected station
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(&updateGeneralDatabase,&QFutureWatcher<CURLcode>::finished,this,&MainWindow::handleUpdateDatabasePartTwo);
    connect(&updateSensorsForCertainStation,&QFutureWatcher<CURLcode>::finished,this,&MainWindow::handleSensorListUpdatePart2);
    connect(&getDataFromThisSensor,&QFutureWatcher<Json::Value>::finished,this,&MainWindow::prepareDataAndDraw);

    /*model = new QStringListModel();
    QStringList tmp;
    tmp<<"DATA FROM WHICH SENSOR TO SHOW?";
    model->setStringList(tmp);
    ui->ListOfSensors->setModel(model);*/
}

MainWindow::~MainWindow()
{
    delete ui;
    //delete model;
}


/**
 * @brief MainWindow::on_Download_Database_clicked
 * This function is used to prepare things for updating database of stations like blocking certain buttons which when clicked could make conflict
 * if database update wasn't finished.
 * We can also see that function will start another thread associated with our updateGeneralDatabase QFutureWatcher object with function directly responsible for downloading database from
 * servers of GIOŚ.
 * Also that function sets program in online mode automatically.
 */
void MainWindow::on_Download_Database_clicked()
{
    ui->Download_Database->blockSignals(true);
    ui->loadOfflineDatabase->blockSignals(true);
    ui->saveData->blockSignals(true);
    WORKING_MODE = ONLINE_MODE;
    //experimentalUpdateDatabase();
    ui->logsData->append("Downloading database please wait...");
    QFuture<CURLcode> updateListOfLocationsDatabase = QtConcurrent::run([this]{return updateDatabase();}); //It is same as normal function call but concurrent
    //experimentalUpdateDatabase();
    updateGeneralDatabase.setFuture(updateListOfLocationsDatabase);
}

/**
 * @brief MainWindow::on_ListOfLocations_itemClicked
 * @param item
 * @param column
 * That function is responsible for updating list of sensors for selected station.
 * This is doing in really similar manner as main database update.
 * This function has two diffrent modes of work depending on working modes of this app (ONLINE_MODE or OFFLINE_MODE).
 * For Offline mode thar function just scanning dir entries which are named after stations and which are stored in our local save dir
 */
void MainWindow::on_ListOfLocations_itemClicked(QTreeWidgetItem *item, int column)
{
    if(WORKING_MODE == ONLINE_MODE)
    {
        //updateDetailedDatabase(item,column);
        //ui->ListOfSensorsNew->blockSignals(true);
        ui->saveData->blockSignals(true);
        QString stationId = item->text(1);
        QString stationName = item->text(0);
        currentSelectedStationId = stationId;
        currentSelectedStationName = stationName;
        ui->logsData->append("Updating Sensor list For Station: " + stationName);
        QFuture<CURLcode> update= QtConcurrent::run([=]{return updateDetailedDatabase(stationId);});
        updateSensorsForCertainStation.setFuture(update);
    }
    else
    {
        ui->saveData->blockSignals(true);
        ui->ListOfSensorsNew->clear();
        QStringList getNameOfStation = item->text(0).split(QLatin1Char('/'));
        currentSelectedStationName = getNameOfStation.at(1);
        currentSelectedStationId = item->text(1);
        //qDebug()<<getNameOfStation.at(1);
        filesystem::directory_entry pathToSave(item->text(0).toStdString());
        QTreeWidgetItem * sensorItem = nullptr;
        //int index = 0;
        for(auto const & saveFile: filesystem::directory_iterator{pathToSave})
        {
            if(saveFile.path().string().find("stationId")!=string::npos)
            {
                //index++;
                continue;
            }
            sensorItem = new QTreeWidgetItem(ui->ListOfSensorsNew);
            sensorItem->setText(0,saveFile.path().string().c_str());
            ui->ListOfSensorsNew->addTopLevelItem(sensorItem);
            //index++;
        }
        //ui->saveData->blockSignals(false);
    }
}

/*
void MainWindow::updateDatabase()
{
    if(threadWorkingOnBuffer)
        return;
    threadWorkingOnBuffer = true;
    QString url = "https://api.gios.gov.pl/pjp-api/rest/station/findAll";
    Json::Value root= apiRequest(url);
    ui->ListOfLocations->blockSignals(true);
    ui->ListOfLocations->clear();
    QTreeWidgetItem *LocalizationsList = new QTreeWidgetItem(ui->ListOfLocations);
    for(auto iter = root.begin();iter !=root.end();++iter)
    {

        QTreeWidgetItem *localizationItem = new QTreeWidgetItem(ui->ListOfLocations,LocalizationsList);
        localizationItem->setText(0,QString(root[iter.index()]["stationName"].asCString()));
        LocalizationsList->insertChild(iter.index(),localizationItem);
        //qDebug()<<"DONE!!!!";
    }
    //for(int i =0;i<10;++i)
    //{
    //    QTreeWidgetItem * obj =new QTreeWidgetItem(ui->ListOfLocations);
    //    obj->setText(0,QString::number(i));
    //    //LocalizationsList.addChild(&localizationItem);
    //    ui->ListOfLocations->addTopLevelItem(obj);
    //    qDebug()<<QString::number(i);
    //}
    ui->ListOfLocations->addTopLevelItem(LocalizationsList);
    privateRoot = root;
    ui->ListOfLocations->blockSignals(false);
    threadWorkingOnBuffer = false;
}*/

/**
 * @brief MainWindow::updateDatabase
 * @return
 * This on first glance simple looking function is used for getting data from GIOŚ servers, but if we look at constructor of our class then we will understand that it is
 * only small piece of bigger group of functions which are connected via signals for handling whole process of updating our database.
 */
CURLcode MainWindow::updateDatabase()
{
    Json::Value root;
    QString url = "https://api.gios.gov.pl/pjp-api/v1/rest/station/findAll?size=500&sort=Id";
    QString result;
    CURLcode errorCode = CURLE_OK;
    apiRequest(url, &result, &errorCode);
    bufferRootAsString = result;
    return errorCode;
}

/**
 * @brief MainWindow::handleUpdateDatabasePartTwo
 * That function handles possible errors during update process in case something goes wrong.
 * That function uses also handleConnectionError function for asking user what to do in case of errors and for direct actions which user wants to do in case of errors.
 * If there isn't any error function is responsible for calling dedicated function for parsing json.
 * After that list of station named listOfLocation in gui is updated.
 */
void MainWindow::handleUpdateDatabasePartTwo()
{
    ui->logsData->append("Done!!!");
    switch(handleConnectionError())
    {
        case 0:
        {
            Json::Value root;
            ui->ListOfLocations->clear();
            ui->ListOfLocations->blockSignals(true);
            //bufferRootAsString.remove(0,371);
            parseJsonResponse(bufferRootAsString.toStdString(),root);
            QTreeWidgetItem *localizationItem = nullptr;
            Json::Value rootTmp = root["Lista stacji pomiarowych"];
            for(auto iter = rootTmp.begin();iter !=rootTmp.end();++iter)
            {
                localizationItem = new QTreeWidgetItem(ui->ListOfLocations);

                QString state,city,street,id;
                //Trying get state where station is...
                try {
                    state = QString(rootTmp[iter.index()]["Województwo"].asCString());
                } catch (...) {
                    state = "";
                    //continue;
                }
                //Try get a city where station is...
                try {
                    city = QString(rootTmp[iter.index()]["Nazwa miasta"].asCString());
                } catch (...) {
                    city = "";
                }
                //Try get a street where station is...
                try {
                    street = QString(rootTmp[iter.index()]["Ulica"].asCString());
                } catch (...) {
                    street = "";
                }
                //Try get a station id...
                try {
                    id = QString::number(rootTmp[iter.index()]["Identyfikator stacji"].asInt());
                    //qDebug()<<QString::number(rootTmp[iter.index()]["Identyfikator stacji"].asInt()) + " " + id;
                } catch (...) {
                    id = "000000000000";
                }
                //qDebug()<<id;
                localizationItem->setText(0,state + " " + city + " " +street);
                localizationItem->setText(1,id);
                //LocalizationsList->insertChild(iter.index(),localizationItem);
                ui->ListOfLocations->addTopLevelItem(localizationItem);
            }
            ui->ListOfLocations->sortItems(0,Qt::AscendingOrder);
            privateRoot = root;
            ui->ListOfLocations->blockSignals(false);
            ui->Download_Database->blockSignals(false);
            ui->loadOfflineDatabase->blockSignals(false);
            break;
        }
        case 5: //Trying again because of multithreading do nothing here!!!
        {
            return;
            break;
        }
        case 10: //User just accepted error do nothing!!!
        {
            ui->Download_Database->blockSignals(false);
            ui->loadOfflineDatabase->blockSignals(false);
            return;
            break;
        }
        case 15: //User decided to load offline database!!!
        {
            on_loadOfflineDatabase_clicked();
            return;
            break;
        }
    }
}

/*
QJsonObject ObjectFromString(const QString& in)
{
    QJsonObject obj;

    QJsonDocument doc = QJsonDocument::fromJson(in.toLocal8Bit());

    // check validity of the document
    if(!doc.isNull())
    {
        if(doc.isObject())
        {
            obj = doc.object();
        }
        else
        {
            qDebug() << "Document is not an object";
        }
    }
    else
    {
        qDebug() << "Invalid JSON...\n" << in;
    }

    return obj;
}*/

/*
void MainWindow::experimentalUpdateDatabase()
{
    //ui->ListOfSensors->update();
    Json::Value root;
    QString url = "https://api.gios.gov.pl/pjp-api/v1/rest/station/findAll?size=500&sort=Id";
    QString result,errorMessage = "";
    //bool errorOccured = false;
    CURLcode errorCode = CURLE_OK;
    apiRequest(url, &result, &errorCode);
    if(errorCode!=CURLE_OK)
    {
        switch (handleConnectionError(errorCode)) {
        case 3: //Exit from this fun
            break;
        case 4: //Load offline database
            break;
        case 5: //Try Again
            experimentalUpdateDatabase();
            break;
        default:
            break;
        }
        return;
    }
    ui->ListOfLocations->blockSignals(true);
    //ui->ListOfLocations->removeItemWidget(ui->ListOfLocations->currentItem(),0);
    //ui->ListOfLocations->removeItemWidget(ui->ListOfLocations->currentItem(),1);
    //ui->ListOfLocations->clear(); //FIGURE OUT HOW TO CLEAR THIS SHIT!!!!!!!!
    result.remove(0,371); //IMPORTANT REMOVES USELESS INFO FROM BEGINNING FOR EASY PARSE
    //qDebug()<<"done";
    parseJsonResponse(result.toStdString(),root);
    //qDebug()<<root.size();
    //QTreeWidgetItem *LocalizationsList = new QTreeWidgetItem(ui->ListOfLocations);
    QTreeWidgetItem *localizationItem = nullptr;
    for(auto iter = root.begin();iter !=root.end();++iter)
    {
        localizationItem = new QTreeWidgetItem(ui->ListOfLocations);

        QString state,city,street,id;
        //Trying get state where station is...
        try {
            state = QString(root[iter.index()]["Województwo"].asCString());
        } catch (...) {
            state = "ERROR";
            //continue;
        }
        //Try get a city where station is...
        try {
            city = QString(root[iter.index()]["Nazwa miasta"].asCString());
        } catch (...) {
            city = "ERROR";
        }
        //Try get a street where station is...
        try {
            street = QString(root[iter.index()]["Ulica"].asCString());
        } catch (...) {
            street = "ERROR";
        }
        //Try get a station id...
        try {
            id = QString::number(root[iter.index()]["Identyfikator stacji"].asInt());
            qDebug()<<QString::number(root[iter.index()]["Identyfikator stacji"].asInt()) + " " + id;
        } catch (...) {
            id = "000000000000";
        }
        qDebug()<<id;
        localizationItem->setText(0,state + " " + city + " " +street);
        localizationItem->setText(1,id);
        //LocalizationsList->insertChild(iter.index(),localizationItem);
        ui->ListOfLocations->addTopLevelItem(localizationItem);
    }
    //LocalizationsList->sortChildren(0,Qt::AscendingOrder);
    //qDebug()<<root.size();
    //ui->ListOfLocations->addTopLevelItem(LocalizationsList);
    privateRoot = root;
    ui->ListOfLocations->blockSignals(false);
    //qDebug()<<root.size();
}

*/

/*
int MainWindow::experimentalUpdateDatabase()
{
    //ui->ListOfSensors->update();
    Json::Value root;
    QString url = "https://api.gios.gov.pl/pjp-api/v1/rest/station/findAll?size=500&sort=Id";
    QString result,errorMessage = "";
    //bool errorOccured = false;
    CURLcode errorCode = CURLE_OK;
    apiRequest(url, &result, &errorCode);
    if(errorCode!=CURLE_OK)
    {
        switch (handleConnectionError(errorCode)) {
        case 3: //Exit from this fun
            break;
        case 4: //Load offline database
            break;
        case 5: //Try Again
            experimentalUpdateDatabase();
            break;
        default:
            break;
        }
        return 0;
    }
    ui->ListOfLocations->blockSignals(true);
    //ui->ListOfLocations->removeItemWidget(ui->ListOfLocations->currentItem(),0);
    //ui->ListOfLocations->removeItemWidget(ui->ListOfLocations->currentItem(),1);
    //ui->ListOfLocations->clear(); //FIGURE OUT HOW TO CLEAR THIS SHIT!!!!!!!!
    result.remove(0,371); //IMPORTANT REMOVES USELESS INFO FROM BEGINNING FOR EASY PARSE
    //qDebug()<<"done";
    parseJsonResponse(result.toStdString(),root);
    //qDebug()<<root.size();
    //QTreeWidgetItem *LocalizationsList = new QTreeWidgetItem(ui->ListOfLocations);
    QTreeWidgetItem *localizationItem = nullptr;
    for(auto iter = root.begin();iter !=root.end();++iter)
    {
        localizationItem = new QTreeWidgetItem(ui->ListOfLocations);

        QString state,city,street,id;
        //Trying get state where station is...
        try {
            state = QString(root[iter.index()]["Województwo"].asCString());
        } catch (...) {
            state = "ERROR";
            //continue;
        }
        //Try get a city where station is...
        try {
            city = QString(root[iter.index()]["Nazwa miasta"].asCString());
        } catch (...) {
            city = "ERROR";
        }
        //Try get a street where station is...
        try {
            street = QString(root[iter.index()]["Ulica"].asCString());
        } catch (...) {
            street = "ERROR";
        }
        //Try get a station id...
        try {
            id = QString::number(root[iter.index()]["Identyfikator stacji"].asInt());
            qDebug()<<QString::number(root[iter.index()]["Identyfikator stacji"].asInt()) + " " + id;
        } catch (...) {
            id = "000000000000";
        }
        qDebug()<<id;
        localizationItem->setText(0,state + " " + city + " " +street);
        localizationItem->setText(1,id);
        //LocalizationsList->insertChild(iter.index(),localizationItem);
        ui->ListOfLocations->addTopLevelItem(localizationItem);
    }
    //LocalizationsList->sortChildren(0,Qt::AscendingOrder);
    //qDebug()<<root.size();
    //ui->ListOfLocations->addTopLevelItem(LocalizationsList);
    privateRoot = root;
    ui->ListOfLocations->blockSignals(false);
    //qDebug()<<root.size();
}*/


/**
 * @brief MainWindow::updateDetailedDatabase
 * @param nameOfStation
 * @return
 * This functions just taking data from GIOŚ servers and letting handleSensorListUpdatePart2 do the rest like parsing updating GUI etc.
 * This function is never used in main application thread.
 * This functions is getting data about sensors from selected station.
 */
CURLcode MainWindow::updateDetailedDatabase(QString nameOfStation)
{
    CURLcode potentialError;
    //QString result;
    QString url = "https://api.gios.gov.pl/pjp-api/v1/rest/station/sensors/"+ nameOfStation + "?size=500";// QString::number(privateRoot[ui->ListOfLocations->indexFromItem(item).row()]["Identyfikator stacji"].asInt()); //current data for clicked sensor https://api.gios.gov.pl/pjp-api/v1/rest/data/getData/{idSensor}
    Json::Value tmp;
    apiRequest(url,&bufferOfSensorsForParse,&potentialError);
    //bufferOfSensorsForParse.remove(0,393);
    //parseJsonResponse(result.toStdString(),tmp);
    //qDebug()<<QString::number(privateRoot[ui->ListOfLocations->indexFromItem(item).row()]["id"].asInt();
    //ui->DetailedData->clear();
    //ui->DetailedData->setPlainText(result);
    //bool bug = false;
    //ofstream file;
    //file.open("test2.json");
    //file<<tmp;
    //file.close();
    return potentialError;
}

/**
 * @brief MainWindow::handleSensorListUpdateErrors
 * @param error
 * @return
 * That function ask user what to do in case of errors in updating list of sensors for selected station and also triggers selected action.
 *
 */
int MainWindow::handleSensorListUpdateErrors(CURLcode error)
{
    qDebug()<<error;
    if(error != CURLE_OK && error != CURLE_UNSUPPORTED_PROTOCOL)
    {
        QMessageBox connectionErrorBox;
        connectionErrorBox.setText(curl_easy_strerror(error));
        switch (error) {
        case 5:
        {
            connectionErrorBox.setDetailedText("This error means there is something with connection to proxy host go wrong.");
            break;
        }
        case 6:
        {
            connectionErrorBox.setDetailedText("This means there is something wrong with your DNS setting or your internet connection or DNS server iteself.");
            break;
        }
        case 7:
        {
            connectionErrorBox.setDetailedText("This means something is wrong with your internet connection or GIOŚ server itself.");
            break;
        }
        default:
            break;
        }
        connectionErrorBox.addButton("OK",QMessageBox::ActionRole);
        connectionErrorBox.addButton("Try Load Offline Database", QMessageBox::ActionRole);
        connectionErrorBox.addButton("Try Again",QMessageBox::ActionRole);
        switch(connectionErrorBox.exec())
        {
        case 3: //Exit from this fun
        {
            return 10;
            break;
        }
        case 4: //Load offline database
        {
            return 15;
            break;
        }
        case 5: //Try Again
        {
            QFuture<CURLcode> updateListOfSensorDatabase = QtConcurrent::run([=]{return updateDetailedDatabase(currentSelectedStationId);});
            updateSensorsForCertainStation.setFuture(updateListOfSensorDatabase);
            return 5;
            break;
        }
        default:
            return 100;
            break;
        }
    }
    return 0;
}

/**
 * @brief MainWindow::handleSensorListUpdatePart2
 * That function uses function of handleSensorListUpdateErrors for handling potential errors and also in case when no error occured
 * is responsible for updating list of sensors in GUI.
 */
void MainWindow::handleSensorListUpdatePart2()
{
    /*if(model)
        delete model;
    model = new QStringListModel();
    QStringList listID,listName;
    listID<<"DATA FROM WHICH SENSOR TO SHOW?";
    listName<<"DATA FROM WHICH SENSOR TO SHOW?";
    for(int i =0;i<bufferOfSensors[0].size();++i)
    {
        listID<<bufferOfSensors[0][i];
        listName<<bufferOfSensors[1][i];
    }
    //model->setStringList(listID);
    //model->insertColumn(2);
    //model->col
    model->setStringList(listName);
    ui->ListOfSensors->setModelColumn(0);
    ui->ListOfSensors->setModel(model);*/
    //ui->ListOfSensorsNew->blockSignals(true);
    ui->logsData->append("Done!!!");
    CURLcode potentialError = updateSensorsForCertainStation.result();
    switch(handleSensorListUpdateErrors(potentialError))
    {
        case 0:
        {
            ui->ListOfSensorsNew->clear();
            bufferOfSensors.clear();
            Json::Value tmp;
            vector<QString>buffID,buffName;
            parseJsonResponse(bufferOfSensorsForParse.toStdString(),tmp);
            tmp = tmp["Lista stanowisk pomiarowych dla podanej stacji"];
            for(auto iter = tmp.begin();iter!= tmp.end();++iter )
            {
                try {
                    buffID.push_back(QString::number(tmp[iter.index()]["Identyfikator stanowiska"].asInt()));
                    buffName.push_back(tmp[iter.index()]["Wskaźnik"].asCString());
                } catch (...) {
                    buffID.push_back("EMPTY");
                    buffName.push_back("EMPTY");
                    //bug = true;
                    //qDebug()<<tmp[iter.index()]["Identyfikator stanowiska"].asCString();
                }
            }
            bufferOfSensors.push_back(buffID);
            bufferOfSensors.push_back(buffName);
            QTreeWidgetItem * sensorItem = nullptr;
            for(int i =0;i<bufferOfSensors[0].size();++i)
            {
                sensorItem = new QTreeWidgetItem(ui->ListOfSensorsNew);
                sensorItem->setText(0,bufferOfSensors[1][i]);
                sensorItem->setText(1,bufferOfSensors[0][i]);
                ui->ListOfSensorsNew->addTopLevelItem(sensorItem);
            }
            ui->ListOfSensorsNew->blockSignals(false);
        }
        case 5: // user trying again so exit from this fun
        {
            return;
            break;
        }
        case 10: //do nothing user just accepted error
        {
            ui->ListOfSensorsNew->blockSignals(false);
            return;
            break;
        }
        case 15: //User wants to load offline database
        {
            on_loadOfflineDatabase_clicked();
            return;
            break;
        }
    }
}

/**
 * @brief MainWindow::getDataFromCertainSensor
 * @param indexOfSensorIdInBuffer
 * @param dayNumber
 * @return
 * This function is responsible for getting data from selected by user range of time from selected sensor.
 */
Json::Value MainWindow::getDataFromCertainSensor(int indexOfSensorIdInBuffer,int dayNumber)
{
    switch (CURRENT_DAYS) {
    case THREE:
    {
        QString result;
        QString url = "https://api.gios.gov.pl/pjp-api/v1/rest/data/getData/" + QString(this->bufferOfSensors[0][indexOfSensorIdInBuffer]) + "?size=500&sort=Data";
        Json::Value root;
        apiRequest(url,&result);
        //result.remove(0,346);
        parseJsonResponse(result.toStdString(),root);
        root = root["Lista danych pomiarowych"];
        /*ofstream file;
        file.open("test.json");
        file<<result.toStdString();
        file.close();*/
        //lastThreeDaysSoDoNotUseMeanValues = true;
        return root;
        break;
    }
    case WEEK:
    {
        QString result;
        QString url = "https://api.gios.gov.pl/pjp-api/v1/rest/archivalData/getDataBySensor/" + QString(this->bufferOfSensors[0][indexOfSensorIdInBuffer]) + "?size=500&sort=Data&dayNumber=7";
        Json::Value root;
        apiRequest(url,&result);
        //result.remove(0,383);
        parseJsonResponse(result.toStdString(),root);
        root = root["Lista archiwalnych wyników pomiarów"];
        /*ofstream file;
        file.open("test.json");
        file<<result.toStdString();
        file.close();*/
        return root;
        break;
    }
    case YEAR:
    {
        QString result;
        QString url = "https://api.gios.gov.pl/pjp-api/v1/rest/archivalData/getDataBySensor/" + QString(this->bufferOfSensors[0][indexOfSensorIdInBuffer]) + "?size=10000&sort=Data&dayNumber=365";
        Json::Value root;
        apiRequest(url,&result);
        //result.remove(0,383);
        parseJsonResponse(result.toStdString(),root);
        root = root["Lista archiwalnych wyników pomiarów"];
        /*ofstream file;
        file.open("test.json");
        file<<root;
        file.close();*/
        return root;
        break;
    }
    case CUSTOM:
    {
        QString result;
        QString url = "https://api.gios.gov.pl/pjp-api/v1/rest/archivalData/getDataBySensor/" + QString(this->bufferOfSensors[0][indexOfSensorIdInBuffer]) + "?size=10000&sort=Data&dayNumber=" + QString::number(dayNumber);
        Json::Value root;
        apiRequest(url,&result);
        //result.remove(0,383);
        parseJsonResponse(result.toStdString(),root);
        root = root["Lista archiwalnych wyników pomiarów"];
        /*ofstream file;
        file.open("test.json");
        file<<root;
        file.close();*/
        return root;
        break;
    }
    default:
        throw exception("BUG HAS OCCURED DURING SELECTION OF TIME RANGE, CLOSING APP!!!!!!!");
        break;
    }
}

/**
 * @brief MainWindow::prepareDataAndDraw
 * This function if used in online mode doing basic checking if data which we got from GIOŚ are correct.
 * and if they seem ok we are preparing them to be used for displaying in chart and for basic analysis.
 * We also making possible to save those data here.
 */
void MainWindow::prepareDataAndDraw()
{
    if(WORKING_MODE == ONLINE_MODE)
    {
        //qDebug()<<"Size of data = "<<getDataFromThisSensor.result().size();
        ui->logsData->append("Done!!!");
        if(getDataFromThisSensor.result().size()==0)
        {
            showWindowWithError("Sensor is unaviable currently or is manual. If sensor is manual you can try get data from last year.");
            ui->ListOfSensorsNew->blockSignals(false);
            return;
        }
        vector<QString>getDates;
        vector<double>getValues;
        calculateMeanValueFromDataDayByDay(&getDates, &getValues);
        drawGraph(getDates,getValues);
        meanValueDisplay(getValues);
        maxValueDisplay(getDates,getValues);
        minValueDiplay(getDates, getValues);
        toSaveDates = getDates;
        toSaveValues = getValues;
        ui->ListOfSensorsNew->blockSignals(false);
        ui->saveData->blockSignals(false);
    }
    else
    {
        Json::Value root;
        vector<QString>getDates;
        vector<double>getValues;
        ifstream file;
        //file.open();
        string date1 = "",date2 = "";
        double tmp = 0.0, sensorId;
        string sensorName;
        auto size = filesystem::file_size(filepathForPrepareDataAndDraw.toStdString());
        string buffer(size,'\0');
        file.open(filepathForPrepareDataAndDraw.toStdString());
        file.read(&buffer[0],size);
        file.close();
        qDebug()<<"After reading file.";
        try {
            parseJsonResponse(buffer,root);
        } catch (...) {
            qDebug()<<"ERROR PARSING";
        }
        /*file >> sensorId;
        getline(file,sensorName);
        currentSelectedSensorName = sensorName.c_str();
        currentSelectedSensorId = QString::number(sensorId);
        while(!file.eof())
        {
            file >> date1 >> date2;
            file>>tmp;
            getDates.push_back(date1.c_str() + QString(" ") + date2.c_str());
            getValues.push_back(tmp);
        }*/
        try {
            calculateMeanValueFromDataDayByDay(&getDates, &getValues, &root);
        } catch (...) {
            qDebug()<<"ERROR";
        }
        //ofstream debFile;
        //debFile.open("debug.json");
        //debFile<<root;
        //debFile.close();
        //qDebug()<<"DRAWING GRAPH";
        drawGraph(getDates,getValues);
        //qDebug()<<"DONE DRAWING GRAPH";
        meanValueDisplay(getValues);
        maxValueDisplay(getDates,getValues);
        minValueDiplay(getDates, getValues);
        //toSaveDates = getDates;
        //toSaveValues = getValues;
        ui->ListOfSensorsNew->blockSignals(false);
        ui->saveData->blockSignals(true);
    }
}

/*
void MainWindow::calculateMeanValueFromDataDayByDay(std::vector<QString>*vectorOfDatesResult, std::vector<double>*meanValuesForCorespondingDates)
{
    auto root = getDataFromThisSensor.result();
    //ofstream file;
    //file.open("test4.json");
    QString previousDate = "",date = "";
    vector<vector<double>> data;
    //vector<QString>vectorOfDates;
    vector<double>tmp;
    for(auto iter = root.begin();iter!=root.end();++iter)
    {
        date  = QString(root[iter.index()]["Data"].toStyledString().c_str());
        date = date.slice(1,10);
        //qDebug()<<date;
        if(iter == root.begin())
            previousDate=date;
        if(previousDate==date)
        {
            try {
                tmp.push_back(root[iter.index()]["Wartość"].asDouble());
            } catch (...) {
                previousDate=date;
                //qDebug()<<date;
                //continue;
            }
        }
        else
        {
            qDebug()<<date;
            data.push_back(tmp);
            vectorOfDatesResult->push_back(previousDate);
            tmp = vector<double>();
            try {
                tmp.push_back(root[iter.index()]["Wartość"].asDouble());
            } catch (...) {
                previousDate=date;
                //qDebug()<<date;
                //continue;
            }
        }
        previousDate=date;
    }
    data.push_back(tmp);
    vectorOfDatesResult->push_back(date);
    qDebug()<<"data[0] size = "<<data[0].size();
    for(int i =0;i<data.size();++i)
    {
        double meanResult = 0;
        for(int j = 0;j<data[i].size();++j)
        {
            meanResult += data[i][j];
        }
        meanResult/=static_cast<double>(data[i].size());
        meanValuesForCorespondingDates->push_back(meanResult);
        qDebug()<<i+1<<" "<<meanResult<<" "<<(*vectorOfDatesResult)[i];
    }
    /*
    if(lastThreeDaysSoDoNotUseMeanValues==false)
    {
        for(int i =0;i<data.size();++i)
        {
            double meanResult = 0;
            for(int j = 0;j<data[i].size();++j)
            {
                meanResult += data[i][j];
            }
            meanResult/=static_cast<double>(data[i].size());
            meanValuesForCorespondingDates->push_back(meanResult);
            qDebug()<<i+1<<" "<<meanResult<<" "<<(*vectorOfDatesResult)[i];
        }
    }
    else
    {
        for(int i =0;i<data.size();++i)
        {
            double meanResult = 0;
            for(int j = 0;j<data[i].size();++j)
            {
                meanResult += data[i][j];
            }
            meanResult/=static_cast<double>(data[i].size());
            meanValuesForCorespondingDates->push_back(meanResult);
            qDebug()<<i+1<<" "<<meanResult<<" "<<(*vectorOfDatesResult)[i];
        }
    }

    //file<<root;
    //file.close();
}*/

/**
 * @brief MainWindow::calculateMeanValueFromDataDayByDay
 * @param vectorOfDatesResult
 * @param meanValuesForCorespondingDates
 * @param rootExternal
 * This function takes Json::Value from result of getDataFromCertainSensor and putting this data into two vectors which are later used for stuff like display our graph.
 * One vector stores dates and second one storing corresponding values.
 */
void MainWindow::calculateMeanValueFromDataDayByDay(std::vector<QString>*vectorOfDatesResult, std::vector<double>*meanValuesForCorespondingDates, Json::Value *rootExternal)
{
    Json::Value root;
    if(rootExternal)
        root = *rootExternal;
    else
        root = getDataFromThisSensor.result();
    for(auto iter = root.begin();iter!=root.end();++iter)
    {
        try {
            QString date = root[iter.index()]["Data"].toStyledString().c_str();
            date = date.slice(1,13);
            vectorOfDatesResult->push_back(date);
        } catch (...) {
            continue;
        }
        try {
            meanValuesForCorespondingDates->push_back(root[iter.index()]["Wartość"].asDouble());
            //qDebug()<<"Done";
        } catch (...) {
            meanValuesForCorespondingDates->push_back(0);
        }
    }
}

/**
 * @brief MainWindow::handleSelectionWindow
 * @return
 * This function uses QComboBox to allow user to select from what period of time he want values.
 * function returns integer from 2 to 6 in order coresponding to code here.
 */
int MainWindow::handleSelectionWindow()
{
    //QWidget * selectionWindow = new QWidget(this);
    //selectionWindow->setWindowFlags(Qt::Window);
    //selectionWindow->show();
    //selectionWindow->resize(480,320);
    //selectionWindow->raise();
    //QPushButton * newButton = new QPushButton("my button",selectionWindow);
    QMessageBox messageBox;
    //QPushButton lastDay("Select Data from last 24H",&messageBox),lastWeek("Select Data from last week",&messageBox),LastYear("Select Data from last year",&messageBox);
    messageBox.addButton("Select Data from last 3 days",QMessageBox::ActionRole);
    messageBox.addButton("Select Data from last week",QMessageBox::ActionRole);
    messageBox.addButton("Select Data from last year",QMessageBox::ActionRole);
    messageBox.addButton("Select Data from custom time",QMessageBox::ActionRole);
    messageBox.addButton("CANCEL",QMessageBox::ActionRole);
    return messageBox.exec();
}

/**
 * @brief MainWindow::drawGraph
 * @param vectorOfDates
 * @param valuesForGraph
 * This function creating new graph object and calling proper function to prepare that,
 * and lastly when object is ready to display function drawGraph making it visible.
 */
void MainWindow::drawGraph(std::vector<QString> &vectorOfDates, std::vector<double>& valuesForGraph)
{
    displayWidget = new DateTimeAxisWidget(ui->WidgetForDisplayOfGraphs);
    displayWidget->myWidget(vectorOfDates,valuesForGraph,&currentSelectedStationName,&currentSelectedSensorName);
    displayWidget->resize(ui->WidgetForDisplayOfGraphs->size());
    displayWidget->setVisible(true);
}

/**
 * @brief MainWindow::meanValueDisplay
 * @param valuesForGraph
 * This function only calculates mean value and displays it as normal text.
 */
void MainWindow::meanValueDisplay(std::vector<double>&valuesForGraph)
{
    double result = 0;
    for(int i = 0;i<valuesForGraph.size();++i)
    {
        result +=valuesForGraph[i];
    }
    result /=static_cast<double>(valuesForGraph.size());
    ui->meanValue->setText(QString::number(result));
}

/**
 * @brief MainWindow::maxValueDisplay
 * @param vectorOfDates
 * @param valuesForGraph
 * This function searches maximal value and corresponding to it date.
 * And prints both as normal text.
 */
void MainWindow::maxValueDisplay(std::vector<QString> &vectorOfDates, std::vector<double> &valuesForGraph)
{
    auto it = max_element(valuesForGraph.begin(),valuesForGraph.end());
    QString MaxValue = QString::number(*it);
    auto date = vectorOfDates.begin() + distance(valuesForGraph.begin(),it);
    QString MaxValueDate = *date;
    ui->maxValue->setText(MaxValue + "\n" + MaxValueDate + ":00:00");
}

/**
 * @brief MainWindow::minValueDiplay
 * @param vectorOfDates
 * @param valuesForGraph
 * This function searches minimal value and corresponding to it date.
 * And prints both as normal text.
 */
void MainWindow::minValueDiplay(std::vector<QString> &vectorOfDates, std::vector<double> &valuesForGraph)
{
    auto it = min_element(valuesForGraph.begin(),valuesForGraph.end());
    QString minValue = QString::number(*it);
    auto date = vectorOfDates.begin() + distance(valuesForGraph.begin(),it);
    QString minValueDate = *date;
    ui->minValue->setText(minValue + "\n" + minValueDate + ":00:00");
}

/**
 * @brief MainWindow::showWindowWithError
 * @param error
 * This function is used to print user error messages with only option to just confirm.
 * It is used in only one place which is basic info something went wrong when reading data from sensor.
 */
void MainWindow::showWindowWithError(QString error)
{
    QMessageBox errorBox;
    errorBox.setText(error);
    errorBox.addButton("Ok",QMessageBox::ActionRole);
    errorBox.exec();
}

/**
 * @brief MainWindow::handleConnectionError
 * @param forListOfSensors
 * @return
 * This Function handles potential errors when getting list of station from GIOŚ.
 */
int MainWindow::handleConnectionError(bool forListOfSensors) //0 = Everything is OK, 5 = Error occured and user trying another time so we Waiting for another database update, 10 = User just accepted error so DO NOTHING
{
    CURLcode errorCode = updateGeneralDatabase.result();
    if( errorCode != CURLE_OK)
    {
        QMessageBox connectionErrorBox;
        connectionErrorBox.setText(curl_easy_strerror(errorCode));
        switch (errorCode) {
        case 5:
        {
            connectionErrorBox.setDetailedText("This error means there is something with connection to proxy host go wrong.");
            break;
        }
        case 6:
        {
            connectionErrorBox.setDetailedText("This means there is something wrong with your DNS setting or your internet connection or DNS server iteself.");
            break;
        }
        case 7:
        {
            connectionErrorBox.setDetailedText("This means something is wrong with your internet connection or GIOŚ server itself.");
            break;
        }
        default:
            break;
        }
        connectionErrorBox.addButton("OK",QMessageBox::ActionRole);
        connectionErrorBox.addButton("Try Load Offline Database", QMessageBox::ActionRole);
        connectionErrorBox.addButton("Try Again",QMessageBox::ActionRole);

        switch(connectionErrorBox.exec())
        {
        case 3: //Exit from this fun
        {
            return 10;
            break;
        }
        case 4: //Load offline database
        {
            return 15;
            break;
        }
        case 5: //Try Again
        {
            QFuture<CURLcode> updateListOfLocationsDatabase = QtConcurrent::run([this]{return updateDatabase();});
            updateGeneralDatabase.setFuture(updateListOfLocationsDatabase);
            return 5;
            break;
        }
        default:
            return 100;
            break;
        }
    }
    return 0;
}

/*
void MainWindow::on_ListOfSensors_doubleClicked(const QModelIndex &index)
{
    //QString myData = ui->ListOfSensors->model()->data(index).toString();
    if(index.row()==0)
        return; //0 is just description not a sensor launch app and see yourself
    int readyIndex = index.row()-1;
    int selection = handleSelectionWindow();
    qDebug()<<"DONE";
    currentSelectedSensorId = bufferOfSensors[0][readyIndex];
    currentSelectedSensorName = bufferOfSensors[1][readyIndex];
    ui->ListOfSensors->blockSignals(true);
    QFuture<Json::Value> getData = QtConcurrent::run([&,this]{return getDataFromCertainSensor(readyIndex,selection);});
    getDataFromThisSensor.setFuture(getData);
}*/

/**
 * @brief MainWindow::on_ListOfSensorsNew_itemClicked
 * @param item
 * @param column
 * This is function is used when user selects item from sensor list, it is used to handle diffrent user choices in case of period of data they want.
 */
void MainWindow::on_ListOfSensorsNew_itemClicked(QTreeWidgetItem *item, int column)
{
    if(WORKING_MODE == ONLINE_MODE)
    {
        ui->ListOfSensorsNew->blockSignals(true);
        switch(handleSelectionWindow())
        {
            case 1:
            {
                return;
                break;
            }
            case 2:
            {
                CURRENT_DAYS = THREE;
                break;
            }
            case 3:
            {
                CURRENT_DAYS = WEEK;
                break;
            }
            case 4:
            {
                CURRENT_DAYS = YEAR;
                break;
            }
            case 5:
            {
                CURRENT_DAYS = CUSTOM;
                howManyDaysSelected = QInputDialog::getInt(this,"From how many days you want data? Acceptable Range (1-366).","Select day range: ",1,1,366);
                break;
            }
            default:
            {
                CURRENT_DAYS = ZERO;
                ui->ListOfSensorsNew->blockSignals(false);
                return;
                break;
            }
        }
        //qDebug()<<ui->ListOfSensorsNew->currentIndex().row();
        int readyIndex = ui->ListOfSensorsNew->currentIndex().row();
        currentSelectedSensorId = bufferOfSensors[0][readyIndex];
        currentSelectedSensorName = bufferOfSensors[1][readyIndex];
        //qDebug()<<currentSelectedSensorName<<" "<<currentSelectedSensorId;

        //qDebug()<<howManyDaysSelected;
        ui->logsData->append("Getting data from GIOŚ for station: " + currentSelectedStationName + " and for sensor: " + currentSelectedSensorName);
        QFuture<Json::Value> getData = QtConcurrent::run([=]{return getDataFromCertainSensor(readyIndex,howManyDaysSelected);});
        getDataFromThisSensor.setFuture(getData);
    }
    else
    {
        ui->ListOfSensorsNew->blockSignals(true);
        filepathForPrepareDataAndDraw = item->text(0);
        prepareDataAndDraw();
    }
}

/**
 * @brief MainWindow::on_saveData_clicked
 * This function saves data from selected sensor and selected period of time.
 * It is only possible when you just have clicked selected sensor.
 */
void MainWindow::on_saveData_clicked()
{
    qDebug()<<"SAVING...";
    currentSelectedStationName.replace(QLatin1Char('/'),QLatin1Char('_'));
    currentSelectedStationName.replace(QLatin1Char('\\'),QLatin1Char('_'));
    ui->logsData->append("Saving data from: " + currentSelectedStationName + " " + currentSelectedSensorName);
    ofstream file;
    filesystem::directory_entry mainSave("save");
    if(!mainSave.exists())
        filesystem::create_directory(mainSave.path());
    filesystem::directory_entry detailedEntry("save/"+currentSelectedStationName.toStdString());
    if(!detailedEntry.exists())
        filesystem::create_directory(detailedEntry.path());
    switch(CURRENT_DAYS)
    {
        case ZERO:
        {
            return;
            break;
        }
        case THREE:
        {
            file.open("save/"+currentSelectedStationName.toStdString()+"/"+currentSelectedSensorName.toStdString()+"_last_three days.json");
            break;
        }
        case WEEK:
        {
            file.open("save/"+currentSelectedStationName.toStdString()+"/"+currentSelectedSensorName.toStdString()+"_last_week.json");
            break;
        }
        case YEAR:
        {
            file.open("save/"+currentSelectedStationName.toStdString()+"/"+currentSelectedSensorName.toStdString()+"_last_year.json");
            break;
        }
        case CUSTOM:
        {
            file.open("save/"+currentSelectedStationName.toStdString()+"/"+currentSelectedSensorName.toStdString()+"_last_"+QString::number(howManyDaysSelected).toStdString()+"_days.json");
            break;
        }
        default:
        {
            file.open("save/"+currentSelectedStationName.toStdString()+"/"+currentSelectedSensorName.toStdString()+".json");
            break;
        }
    }
    //file<<currentSelectedSensorId.toStdString()<<" "<<currentSelectedSensorName.toStdString()<<"\n";
    file<<"[";
    for(int i =0;i<toSaveDates.size();++i)
    {
        file<<"{\n";
        file<<"   \"Nazwa Stacji\": ";
        file<<"\""<<currentSelectedStationName.toStdString()<<"\",\n";
        file<<"   \"Id\": ";
        file<<""<<currentSelectedStationId.toStdString()<<",\n";
        file<<"   \"Nazwa Sensora\": ";
        file<<"\""<<currentSelectedSensorName.toStdString()<<"\",\n";
        file<<"   \"Sensor Id\": ";
        file<<""<<currentSelectedSensorId.toStdString()<<",\n";
        file<<"   \"Data\": ";
        file<<"\""<<toSaveDates[i].toStdString()<<"\",\n";
        file<<"   \"Wartość\": ";
        file<<""<<toSaveValues[i]<<"\n";
        if(i==toSaveDates.size()-1)
            file<<"}\n";
        else
            file<<"},\n";
    }
    file<<"]";
    file.close();
    ofstream stationIdAndName;
    stationIdAndName.open("save/"+currentSelectedStationName.toStdString()+"/" + "stationId.json");
    stationIdAndName<<currentSelectedStationId.toStdString()<<" "<<currentSelectedStationName.toStdString();
    stationIdAndName.close();
    ui->logsData->append("Done!!!");
}

/**
 * @brief MainWindow::on_loadOfflineDatabase_clicked
 * This function is used to load offline database from files instead of online database from GIOŚ servers.
 *
 */
void MainWindow::on_loadOfflineDatabase_clicked()
{
    WORKING_MODE = OFFLINE_MODE;
    ui->Download_Database->blockSignals(true);
    ui->saveData->blockSignals(true);
    ui->ListOfLocations->clear();
    filesystem::directory_entry saveDir("save");
    QTreeWidgetItem * localizationItem = nullptr;
    for(auto const &dir: filesystem::directory_iterator{saveDir})
    {
        localizationItem = new QTreeWidgetItem(ui->ListOfLocations);
        QString tmp = dir.path().generic_string().c_str();
        int stationId;
        //tmp.remove()
        ifstream temporary;
        temporary.open(dir.path().generic_string() + "/stationId.json");
        temporary>>stationId;
        temporary.close();
        localizationItem->setText(0,tmp);
        localizationItem->setText(1,QString::number(stationId));
    }
    ui->ListOfLocations->sortItems(0,Qt::AscendingOrder);
    ui->Download_Database->blockSignals(false);
}

