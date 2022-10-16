#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "HostAPI.h"

using json = nlohmann::json;

// TBD: Unify host resposes, currntly used in unpairHostClients
class HostResponse {
public:
    std::string status;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(HostResponse, status)
};

// libcurl helper function to read result value
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void HostAPI::setHostSessionKey(QString hostIp, int apiPort, QString hostSessionKey)
{
    CURL *handle;
    CURLcode res;
    std::string readBuffer;

    // address to perform post
    QString serverAddress = "https://" + hostIp + ":" + QString::number(apiPort) + "/api/pin";
    // use string cause QString intoduces unwanted symbol at start.
    std::string postData = "{\"pin\":\"" + hostSessionKey.toStdString() + "\"}\0";

    handle = curl_easy_init();
    if(handle) {
//        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
//                  "HostAPI::setHostSessionKey():host:%s\n",
//                  serverAddress.toStdString().c_str());
//        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
//                  "HostAPI::setHostSessionKey():postData:%s\n",
//                  postData.c_str());

        curl_easy_setopt(handle, CURLOPT_URL, serverAddress.toStdString().c_str());
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, postData.size());
        curl_easy_setopt(handle, CURLOPT_POST, 1L);

        // configure output
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, &readBuffer);

        // TBD: investigate how to deal with certificates properly
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);

        // fill headers
        struct curl_slist *headers=NULL;
        headers = curl_slist_append(headers, "Content-Type:text/plain;charset=UTF-8");
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);

        res = curl_easy_perform(handle); /* post away! */
        if(res != CURLE_OK) {
                   SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "curl_easy_perform() failed: %s\n",
                                         curl_easy_strerror(res));
        }

        // cleanup
        curl_slist_free_all(headers); /* free the header list */
        curl_easy_cleanup(handle);

        // parse output
//        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
//                  "HostAPI::host_response:%s\n",
//                  readBuffer.c_str());
    }
}

bool HostAPI::unpairHostClients(QString hostIp, int apiPort)
{
    CURL *handle;
    CURLcode res;
    std::string readBuffer;
    bool result = false;

    // address to perform post
    QString serverAddress = "https://" + hostIp + ":" + QString::number(apiPort) + "/api/clients/unpair";
    // use string cause QString intoduces unwanted symbol at start.
    std::string postData = "\0";

    handle = curl_easy_init();
    if(handle) {
//        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
//                  "HostAPI::unpairHostClients():host:%s\n",
//                  serverAddress.toStdString().c_str());
//        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
//                  "HostAPI::unpairHostClients():postData:%s\n",
//                  postData.c_str());

        curl_easy_setopt(handle, CURLOPT_URL, serverAddress.toStdString().c_str());
        /* complete within 3 seconds */
        curl_easy_setopt(handle, CURLOPT_TIMEOUT, 3L);

        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, postData.size());
        curl_easy_setopt(handle, CURLOPT_POST, 1L);

        // configure output
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, &readBuffer);

        // TBD: investigate how to deal with certificates properly
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);

        // fill headers
        struct curl_slist *headers=NULL;
        headers = curl_slist_append(headers, "Content-Type:text/plain;charset=UTF-8");
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);

        res = curl_easy_perform(handle); /* post away! */
        if(res != CURLE_OK) {
                   SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "curl_easy_perform() failed: %s\n",
                                         curl_easy_strerror(res));
        }

        // cleanup
        curl_slist_free_all(headers); /* free the header list */
        curl_easy_cleanup(handle);

        // parse output
//        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
//                  "HostAPI::host_response:%s\n",
//                  readBuffer.c_str());

        // parse and serialize JSON
        try {
            json jHostResponse = json::parse(readBuffer.c_str());
            HostResponse hostResponse = jHostResponse;
            result = hostResponse.status == "true";
        } catch (json::exception& ex){
            result = false;
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Failed to parse host response");
        }
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "unpairHostClients(): %d\n",
                          result);

    return result;
}
