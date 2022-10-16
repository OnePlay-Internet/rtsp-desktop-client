#include <curl/curl.h>
#include <sstream>

#include "OnePlayBackendAPILocalDev.h"
#include "Limelight.h"

std::string OnePlayBackendAPILocalDev::getSessionSignature()
{
    QString url = getBackendAddress() + mStartGame + "?vm_ip=" + mServer;
    std::string result;

    QString data =
        "----------------------------244362435980821467765653\r\n"
        "Content-Disposition: form-data; name=\"game_id\"\r\n"
        "\r\n"
        "1\r\n"
        "----------------------------244362435980821467765653--\r\n"
        "\r\n";


    std::string readBuffer = OnePlayBackendAPI::postRequest(
                url, data, QStringList() << "Content-Type: multipart/form-data; boundary=--------------------------244362435980821467765653");

    try
    {
        json startGameJson = json::parse(readBuffer.c_str());
        result = startGameJson["data"]["session_signature"];
    }
    catch (json::exception& ex)
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                  "OnePlayBackendAPILocalDev::json_exception:%s\n",
                  ex.what());
    }

    return result;
}

bool OnePlayBackendAPILocalDev::getOnePlayStreamingPreferences()
{
    mSessionKey = QString(getSessionSignature().c_str());

    if (!OnePlayBackendAPI::getOnePlayStreamingPreferences())
    {
        return false;
    }

    // This is the default ports

    int basePort = 47989;

    mSessionDetails.data.server_details.port_details.https_port = basePort - 5;
    mSessionDetails.data.server_details.port_details.http_port = basePort;
    mSessionDetails.data.server_details.port_details.pin_port = basePort + 1;
    mSessionDetails.data.server_details.port_details.rtsp_port = basePort + 21;
    mSessionDetails.data.server_details.port_details.audio_port = basePort + 11;
    mSessionDetails.data.server_details.port_details.video_port = basePort + 9;
    mSessionDetails.data.server_details.port_details.control_port = basePort + 10;

#ifdef DYNAMIC_PORTS
    LiSetPorts(
        mSessionDetails.data.server_details.port_details.https_port,
        mSessionDetails.data.server_details.port_details.http_port,
        mSessionDetails.data.server_details.port_details.rtsp_port,
        mSessionDetails.data.server_details.port_details.audio_port,
        mSessionDetails.data.server_details.port_details.video_port,
        mSessionDetails.data.server_details.port_details.control_port);
#endif

    return true;
}
