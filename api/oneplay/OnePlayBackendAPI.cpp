#include <curl/curl.h>
#include <sstream>

#include "OnePlayBackendAPI.h"
#include "Limelight.h"

// libcurl helper function to read result value
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string OnePlayBackendAPI::postRequest(const QString &url, const QString &postData, const QStringList headersList)
{
    CURL *handle;
    CURLcode res;
    std::string readBuffer;
    bool result = false;

    handle = curl_easy_init();

    if(handle) {
        curl_easy_setopt(handle, CURLOPT_URL, url.toStdString().c_str());
        /* complete within 3 seconds */
        curl_easy_setopt(handle, CURLOPT_TIMEOUT, 3L);
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postData.toStdString().c_str());
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, postData.size());
        curl_easy_setopt(handle, CURLOPT_POST, 1L);

        // configure output
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, &readBuffer);

        // TODO: investigate how to deal with certificates properly
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);

        // fill headers
        struct curl_slist *headers = NULL;
        for(QString header : headersList)
        {
            headers = curl_slist_append(headers, header.toStdString().c_str());
        }
        headers = curl_slist_append(headers, "Accept: application/json");
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);

        res = curl_easy_perform(handle); /* post away! */
        if(res != CURLE_OK) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));
        }

        // cleanup
        curl_slist_free_all(headers); /* free the header list */
        curl_easy_cleanup(handle);

    }

    return readBuffer;
}


QString OnePlayBackendAPI::getHostSessionKey()
{
    QString hostSessionKey = QString::fromStdString(mSessionDetails.data.host_session_key);
//    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
//                "OnePlayBackendAPI::hostSessionKey():%s\n",
//                hostSessionKey.toStdString().c_str());

    return hostSessionKey;
}

QString OnePlayBackendAPI::getSessionKey()
{
    return mSessionKey;
}

bool OnePlayBackendAPI::getOnePlayStreamingPreferences()
{
    bool result = false;

    // address to perform post
    QString sessionDetailsAddress = getBackendAddress() + getSessionDetailsPath() + "/" + getSessionKey();
    QString clientVersion = getPlatform() + "_build_" + getClientVersion();
    // client header example: "client:pc_build_v1.0".
    QString clientVersionHeader = "client:" + clientVersion;
    // need to post something or libcurl will hangs
    QString postData = clientVersionHeader;

    std::string readBuffer = OnePlayBackendAPI::postRequest(
                sessionDetailsAddress, postData, QStringList() << clientVersionHeader);

    try
    {
        json sessionDetailsJson = json::parse(readBuffer.c_str());
        mSessionDetails = sessionDetailsJson;

#ifdef DYNAMIC_PORTS
        LiSetPorts(
            mSessionDetails.data.server_details.port_details.https_port,
            mSessionDetails.data.server_details.port_details.http_port,
            mSessionDetails.data.server_details.port_details.rtsp_port,
            mSessionDetails.data.server_details.port_details.audio_port,
            mSessionDetails.data.server_details.port_details.video_port,
            mSessionDetails.data.server_details.port_details.control_port);
#else
        // LiSetPorts can not be called in moonlight-common-c.
        // You need to use custom moonlight-common-c located at:
        // https://github.com/OnePlay-Internet/custom-moonlight-protocol
        Q_ASSERT(false);
#endif
        result = true;
    }
    catch (json::exception& ex)
    {
        result = false;
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                  "OnePlayBackendAPI::json_exception:%s\n%s",
                  ex.what(), readBuffer.c_str());
    }

    return result;
}

QString OnePlayBackendAPI::getAppToStart()
{
    QString appName = QString::fromStdString(mSessionDetails.data.game_details.title);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::getAppToStart():%s\n",
                appName.toStdString().c_str());

    return appName;
}

QString OnePlayBackendAPI::getAudioConfig()
{
    QString audioConfig = QString::fromStdString(mSessionDetails.data.other_details.audio_type);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::getAudioConfig():%s\n",
                audioConfig.toStdString().c_str());

    return audioConfig;
}

void OnePlayBackendAPI::getBitrateKbps(std::int32_t & bitrateKbps)
{
    bitrateKbps = mSessionDetails.data.other_details.bitrate_kbps;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::getBitrateKbps():%d\n",
                bitrateKbps);
}

void OnePlayBackendAPI::getFps(std::int32_t & fps)
{
    fps = mSessionDetails.data.other_details.game_fps;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::getFps():%d\n",
                fps);
}

void OnePlayBackendAPI::getScreenResolution(std::int32_t & width, std::int32_t & height)
{
    // parse string to actual width and height
    std::vector<std::string> result;
    std::istringstream resolution(mSessionDetails.data.other_details.resolution);

    std::string tmp;
    while (getline(resolution, tmp, 'x')) {
        result.push_back(tmp);
    }
    if (result.size() == 2) {
        width = std::stoi(result[0]);
        height = std::stoi(result[1]);
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::getScreenResolution():w = %d, h = %d\n",
                width, height);
}

QString OnePlayBackendAPI::getStreamingServerIP()
{
    QString streamingServerIP = QString::fromStdString(mSessionDetails.data.server_details.server_ip);
//    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
//                "OnePlayAPI::getStreamingServerIP():%s\n",
//                streamingServerIP.toStdString().c_str());

    return streamingServerIP;
}

QString OnePlayBackendAPI::getVideoCodecConfig()
{
    QString videoCodec = QString::fromStdString(mSessionDetails.data.other_details.stream_codec);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::getVideoCodecConfig():%s\n",
                videoCodec.toStdString().c_str());

    return videoCodec;
}

QString OnePlayBackendAPI::getWindowMode()
{
    QString windowMode = QString::fromStdString(mSessionDetails.data.other_details.window_mode);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::getWindowMode():%s\n",
                windowMode.toStdString().c_str());

    return windowMode;
}

bool OnePlayBackendAPI::isVsyncEnabled()
{
    bool isVsyncEnabled = mSessionDetails.data.other_details.is_vsync_enabled;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isVsyncEnabled():%d\n",
                isVsyncEnabled);

    return isVsyncEnabled;
}

std::int32_t OnePlayBackendAPI::getPacketSize()
{
    std::int32_t packetSize = mSessionDetails.data.other_details.advance_details.packet_size;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::getPacketSize():%d\n",
                packetSize);

    return packetSize;
}

bool OnePlayBackendAPI::isMultipleControllersSupportEnabled()
{
    bool multicontrollerSupport = mSessionDetails.data.other_details.advance_details.multi_color;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isMultipleControllersSupportEnabled():%d\n",
                multicontrollerSupport);

    return multicontrollerSupport;
}

bool OnePlayBackendAPI::isQuitAppAfterEnabled()
{
    bool quitAppAfter = mSessionDetails.data.other_details.advance_details.quit_app_after;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isQuitAppAfterEnabled():%d\n",
                quitAppAfter);

    return quitAppAfter;
}

bool OnePlayBackendAPI::isAbsoluteMouseModeEnabled()
{
    bool absoluteMouseMode = mSessionDetails.data.other_details.advance_details.absolute_mouse_mode;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::absoluteMouseMode():%d\n",
                absoluteMouseMode);

    return absoluteMouseMode;
}

bool OnePlayBackendAPI::isSwapMouseButtonsEnabled()
{
    bool swapMouseButtons = mSessionDetails.data.other_details.advance_details.swap_mouse_buttons;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isSwapMouseButtonsEnabled():%d\n",
                swapMouseButtons);

    return swapMouseButtons;
}

bool OnePlayBackendAPI::isAbsoluteTouchModeEnabled()
{
    bool absoluteTouchMode = mSessionDetails.data.other_details.advance_details.absolute_touch_mode;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isAbsoluteTouchModeEnabled():%d\n",
                absoluteTouchMode);

    return absoluteTouchMode;
}

bool OnePlayBackendAPI::isGameOptimizationsEnabled()
{
    bool gameOptimizations = mSessionDetails.data.other_details.advance_details.game_optimizations;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isGameOptimizationsEnabled():%d\n",
                gameOptimizations);

    return gameOptimizations;
}

bool OnePlayBackendAPI::isPlayAudioOnHostEnabled()
{
    bool playAudioOnHost = mSessionDetails.data.other_details.advance_details.play_audio_on_host;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isPlayAudioOnHostEnabled():%d\n",
                playAudioOnHost);

    return playAudioOnHost;
}

bool OnePlayBackendAPI::isFramePacingEnabled()
{
    bool framePacing = mSessionDetails.data.other_details.advance_details.frame_pacing;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isFramePacingEnabled():%d\n",
                framePacing);

    return framePacing;
}

bool OnePlayBackendAPI::isMuteOnFocusLossEnabled()
{
    bool muteOnFocusLoss = mSessionDetails.data.other_details.advance_details.mute_on_focus_loss;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isMuteOnFocusLossEnabled():%d\n",
                muteOnFocusLoss);

    return muteOnFocusLoss;
}

bool OnePlayBackendAPI::isBackgroundGamepadEnabled()
{
    bool backgroundGamepad = mSessionDetails.data.other_details.advance_details.background_gamepad;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isBackgroundGamepadEnabled():%d\n",
                backgroundGamepad);

    return backgroundGamepad;
}

bool OnePlayBackendAPI::isReverseScrollDirectionEnabled()
{
    bool reverseScroll = mSessionDetails.data.other_details.advance_details.reverse_scroll_direction;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isReverseScrollDirectionEnabled():%d\n",
                reverseScroll);

    return reverseScroll;
}

bool OnePlayBackendAPI::isSwapFaceButtonsEnabled()
{
    bool swapFaceButtons = mSessionDetails.data.other_details.advance_details.swap_face_buttons;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isSwapFaceButtonsEnabled():%d\n",
                swapFaceButtons);

    return swapFaceButtons;
}

QString OnePlayBackendAPI::getCaptureSysKeysMode()
{
    // TBD:QString captureSysKeysMode = QString::fromStdString(mSessionDetails.data.other_details.capture_sys_keys);
    QString captureSysKeysMode = "never";
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::getCaptureSysKeysMode():%s\n",
                captureSysKeysMode.toStdString().c_str());

    return captureSysKeysMode;
}

QString OnePlayBackendAPI::getVideoDecoderSelection()
{
    QString videoDecoder = QString::fromStdString(mSessionDetails.data.other_details.video_decoder_selection);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::getVideoDecoderSelection():%s\n",
                videoDecoder.toStdString().c_str());

    return videoDecoder;
}

std::string OnePlayBackendAPI::getUserId()
{
    std::string user_id = mSessionDetails.data.user_details.user_id;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::getUserId():%s\n",
                user_id.c_str());

    return user_id;
}

int OnePlayBackendAPI::getAudioPort()
{
    return mSessionDetails.data.server_details.port_details.audio_port;
}

int OnePlayBackendAPI::getVideoPort()
{
    return mSessionDetails.data.server_details.port_details.video_port;
}

int OnePlayBackendAPI::getControlPort()
{
    return mSessionDetails.data.server_details.port_details.control_port;
}

int OnePlayBackendAPI::getRtspPort()
{
    return mSessionDetails.data.server_details.port_details.rtsp_port;
}

int OnePlayBackendAPI::getAPIPort()
{
    return mSessionDetails.data.server_details.port_details.pin_port;
}
