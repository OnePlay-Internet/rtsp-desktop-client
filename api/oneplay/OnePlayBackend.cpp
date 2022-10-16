#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "OnePlayBackend.h"

using json = nlohmann::json;

// libcurl helper function to read result value
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// This method returns key's value from QMap where the key is a QString.
// Key matching is case insensitive.
template <typename T>
static T mapValue(QMap<QString, T> map, QString key)
{
    for(auto& item : map.toStdMap()) {
        if (QString::compare(item.first, key, Qt::CaseInsensitive) == 0) {
            return item.second;
        }
    }
    return T();
}

QString OnePlayBackend::getHost() const
{
    return mIOnePlayBackendAPISPtr->getStreamingServerIP();
}

QString OnePlayBackend::getAppName() const
{
    QString appName = mIOnePlayBackendAPISPtr->getAppToStart();
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackend::getAppToStart():%s\n",
                appName.toStdString().c_str());

    return appName;
}

void OnePlayBackend::getStreamingPreferences(StreamingPreferences *preferences)
{
    if (mIOnePlayBackendAPISPtr->isInitialized()) {
        // set preferences defined by backend.
        mIOnePlayBackendAPISPtr->getScreenResolution(preferences->width, preferences->height);
        mIOnePlayBackendAPISPtr->getFps(preferences->fps);
        mIOnePlayBackendAPISPtr->getBitrateKbps(preferences->bitrateKbps);
        preferences->enableVsync = mIOnePlayBackendAPISPtr->isVsyncEnabled();
        preferences->windowMode = mapValue(mWindowModeMap, mIOnePlayBackendAPISPtr->getWindowMode());
        preferences->audioConfig = mapValue(mAudioConfigMap, mIOnePlayBackendAPISPtr->getAudioConfig());
        preferences->videoCodecConfig = mapValue(mVideoCodecMap, mIOnePlayBackendAPISPtr->getVideoCodecConfig());
        preferences->packetSize = mIOnePlayBackendAPISPtr->getPacketSize();
        preferences->multiController = mIOnePlayBackendAPISPtr->isMultipleControllersSupportEnabled();
        preferences->quitAppAfter = mIOnePlayBackendAPISPtr->isQuitAppAfterEnabled();
        preferences->absoluteMouseMode = mIOnePlayBackendAPISPtr->isAbsoluteMouseModeEnabled();
        preferences->swapMouseButtons = mIOnePlayBackendAPISPtr->isSwapMouseButtonsEnabled();
        preferences->absoluteTouchMode = mIOnePlayBackendAPISPtr->isAbsoluteTouchModeEnabled();
        preferences->gameOptimizations = mIOnePlayBackendAPISPtr->isGameOptimizationsEnabled();
        preferences->playAudioOnHost = mIOnePlayBackendAPISPtr->isPlayAudioOnHostEnabled();
        preferences->framePacing = mIOnePlayBackendAPISPtr->isFramePacingEnabled();
        preferences->muteOnFocusLoss = mIOnePlayBackendAPISPtr->isMuteOnFocusLossEnabled();
        preferences->backgroundGamepad = mIOnePlayBackendAPISPtr->isBackgroundGamepadEnabled();
        preferences->reverseScrollDirection = mIOnePlayBackendAPISPtr->isReverseScrollDirectionEnabled();
        preferences->swapFaceButtons = mIOnePlayBackendAPISPtr->isSwapFaceButtonsEnabled();
        preferences->captureSysKeysMode = mapValue(mCaptureSysKeysModeMap, mIOnePlayBackendAPISPtr->getCaptureSysKeysMode());
        preferences->videoDecoderSelection = mapValue(mVideoDecoderMap, mIOnePlayBackendAPISPtr->getVideoDecoderSelection());
    }
}

QString OnePlayBackend::getHostSessionKey() const
{
    return mIOnePlayBackendAPISPtr->getHostSessionKey();
}

QString OnePlayBackend::getQuitQString() const
{
    QString url = "https://www.oneplay.in/quit?session_id=" + mIOnePlayBackendAPISPtr->getSessionKey();
    return url;
}

void OnePlayBackend::setHostSessionKey()
{
    mHostAPI.setHostSessionKey(mIOnePlayBackendAPISPtr->getStreamingServerIP(),
                               mIOnePlayBackendAPISPtr->getAPIPort(),
                               mIOnePlayBackendAPISPtr->getHostSessionKey());
}

bool OnePlayBackend::unpairHostClients()
{
    return mHostAPI.unpairHostClients(mIOnePlayBackendAPISPtr->getStreamingServerIP(),
                                      mIOnePlayBackendAPISPtr->getAPIPort());
}

void OnePlayBackend::reportError(QString err)
{
    CURL *handle;
    CURLcode res;
    std::string readBuffer;

    json data;
    data["error"] = err.toStdString().c_str();
    data["user_id"] = mIOnePlayBackendAPISPtr->getUserId();

    std::string postData = data.dump();
    std::string sessionKey = mIOnePlayBackendAPISPtr->getHostSessionKey().toStdString().c_str();

    if (sessionKey == "")
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                  "try to send error:%s\n",
                  postData.c_str());

        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                  "sesson key is not configured, error will not be delivered");
        return;
    }
    std::string serverAddress = "https://client-apis.oneplay.in/backend/events/register";

    handle = curl_easy_init();
    if(handle) {
        curl_easy_setopt(handle, CURLOPT_URL, serverAddress.c_str());
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
        std::string authHeader = "Authorization: ";
        authHeader += sessionKey;
        headers = curl_slist_append(headers, sessionKey.c_str());
        headers = curl_slist_append(headers, "User-Agent: OnePlayPCClient/V1.0");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);

        res = curl_easy_perform(handle); /* post away! */
        if(res != CURLE_OK) {
            // can't report error, just log it.
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                      "curl_easy_perform() failed to delivery error report");
        }

        // cleanup
        curl_slist_free_all(headers); /* free the header list */
        curl_easy_cleanup(handle);

        // parse output
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                  "try to send error result: %s\n",
                  readBuffer.c_str());
    }
}
