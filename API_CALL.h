#ifndef API_CALL_H
#define API_CALL_H
#include <iostream>
#include <json/json.h>
#include <curl/curl.h>
#include <QMainWindow>
using namespace std;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    /*
    * userp - pointer to user data, we cast void ptr to string and append the content
    * to the userp - user data. Finally the number of bytes that we taken care of is returned
    */
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to perform CURL request
bool performCurlRequest(const string& url, string& response,CURLcode * errorCode = nullptr, bool * errorOccured = nullptr, QString * errorMessage = nullptr) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        cerr << "Failed to initialize CURL" << endl;
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        if(errorCode)
            *errorCode = res;
        if(errorMessage)
            *errorMessage = curl_easy_strerror(res);
        if(errorOccured)
            *errorOccured = true;
        return false;
    }

    return true;
}

// Function to parse JSON response
bool parseJsonResponse(const string& jsonResponse, Json::Value& parsedRoot) {
    Json::Reader reader;

    bool parsingSuccessful = reader.parse(jsonResponse, parsedRoot);

    if (!parsingSuccessful) {
        cerr << "Failed to parse JSON: " << endl;
        return false;
    }

    return true;
}

Json::Value apiRequest(QString & url,QString * getDataAsQString = nullptr,CURLcode * errorCode = nullptr, bool * errorOccured = nullptr, QString * errorMessage = nullptr) { //default request "https://api.gios.gov.pl/pjp-api/rest/station/findAll"

    setlocale(LC_CTYPE, "Polish"); // for Polish characters
    qDebug()<<"API call to: "<<url;
    string api_url = url.toStdString();
    string response;
    Json::Value root;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    if (performCurlRequest(api_url, response,errorCode,errorOccured,errorMessage)) {
        if (!parseJsonResponse(response, root)) {
            //throw exception("corrupted json");
        }
    }
    curl_global_cleanup();
    if(getDataAsQString!=nullptr)
        *getDataAsQString = QString(response.c_str());
    //qDebug()<<root.asString();
    return root;
}

#endif // API_CALL_H
