#include "OnePlayBackendAPIStubs.h"
#include "settings/serverpreferences.h"

QString OnePlayBackendAPIStubs::getHostSessionKey()
{
    QString sessionKey = "e5fef61d2ac24948afe456ce672a970f:a3d57f403a5308a28cf6b7f46c59f8947f288120ac5bdaf51d49b1b42b3c26b5416ccafff97d6b89bfe7b866919f6fcc20f770762363472be587a94a11c23692:1635118032";
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPIStubs::getHostSessionKey():%s\n",
                sessionKey.toStdString().c_str());

    return sessionKey;
}

QString OnePlayBackendAPIStubs::getSessionKey()
{
    return mSessionKey;
}

QString OnePlayBackendAPIStubs::getAppToStart()
{
    QString appName = "Desktop";
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPIStubs::getAppToStart():%s\n",
                appName.toStdString().c_str());

    return appName;
}

QString OnePlayBackendAPIStubs::getAudioConfig()
{
    QString audioConfig = "stereo";
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPIStubs::getAudioConfig():%s\n",
                audioConfig.toStdString().c_str());

    return audioConfig;
}

void OnePlayBackendAPIStubs::getBitrateKbps(std::int32_t & bitrateKbps)
{
    bitrateKbps = 10000;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPIStubs::getBitrateKbps():%d\n",
                bitrateKbps);
}

void OnePlayBackendAPIStubs::getFps(std::int32_t & fps)
{
    fps = 60;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPIStubs::getFps():%d\n",
                fps);
}

void OnePlayBackendAPIStubs::getScreenResolution(std::int32_t & width, std::int32_t & height)
{
    width = 1280;
    height = 720;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPIStubs::getScreenResolution():w = %d, h = %d\n",
                width, height);
}

QString OnePlayBackendAPIStubs::getStreamingServerIP()
{
    ServerPreferences serverPreferences;
    const QString server = serverPreferences.getServer();

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPIStubs::getStreamingServerIP():%s\n",
                server.toStdString().c_str());

    return server;
}

QString OnePlayBackendAPIStubs::getVideoCodecConfig()
{
    QString videoCodec = "auto";
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPIStubs::getVideoCodecConfig():%s\n",
                videoCodec.toStdString().c_str());

    return videoCodec;
}

QString OnePlayBackendAPIStubs::getWindowMode()
{
    QString windowMode = "windowed";
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPIStubs::getWindowMode():%s\n",
                windowMode.toStdString().c_str());

    return windowMode;
}

bool OnePlayBackendAPIStubs::isVsyncEnabled()
{
    bool isVsyncEnabled = false;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPIStubs::isVsyncEnabled():%d\n",
                isVsyncEnabled);

    return isVsyncEnabled;
}

std::int32_t OnePlayBackendAPIStubs::getPacketSize()
{
    std::int32_t packetSize = 0;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::getPacketSize():%d\n",
                packetSize);

    return packetSize;
}

bool OnePlayBackendAPIStubs::isMultipleControllersSupportEnabled()
{
    bool multicontrollerSupport = true;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isMultipleControllersSupportEnabled():%d\n",
                multicontrollerSupport);

    return multicontrollerSupport;
}

bool OnePlayBackendAPIStubs::isQuitAppAfterEnabled()
{
    bool quitAppAfter = true;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isQuitAppAfterEnabled():%d\n",
                quitAppAfter);

    return quitAppAfter;
}

bool OnePlayBackendAPIStubs::isAbsoluteMouseModeEnabled()
{
    bool absoluteMouseMode = false;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::absoluteMouseMode():%d\n",
                absoluteMouseMode);

    return absoluteMouseMode;
}

bool OnePlayBackendAPIStubs::isSwapMouseButtonsEnabled()
{
    bool swapMouseButtons = false;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isSwapMouseButtonsEnabled():%d\n",
                swapMouseButtons);

    return swapMouseButtons;
}

bool OnePlayBackendAPIStubs::isAbsoluteTouchModeEnabled()
{
    bool absoluteTouchMode = true;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isAbsoluteTouchModeEnabled():%d\n",
                absoluteTouchMode);

    return absoluteTouchMode;
}

bool OnePlayBackendAPIStubs::isGameOptimizationsEnabled()
{
    bool gameOptimizations = true;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isGameOptimizationsEnabled():%d\n",
                gameOptimizations);

    return gameOptimizations;
}

bool OnePlayBackendAPIStubs::isPlayAudioOnHostEnabled()
{
    bool playAudioOnHost = false;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isPlayAudioOnHostEnabled():%d\n",
                playAudioOnHost);

    return playAudioOnHost;
}

bool OnePlayBackendAPIStubs::isFramePacingEnabled()
{
    bool framePacing = false;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isFramePacingEnabled():%d\n",
                framePacing);

    return framePacing;
}

bool OnePlayBackendAPIStubs::isMuteOnFocusLossEnabled()
{
    bool muteOnFocusLoss = false;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isMuteOnFocusLossEnabled():%d\n",
                muteOnFocusLoss);

    return muteOnFocusLoss;
}

bool OnePlayBackendAPIStubs::isBackgroundGamepadEnabled()
{
    bool backgroundGamepad = false;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isBackgroundGamepadEnabled():%d\n",
                backgroundGamepad);

    return backgroundGamepad;
}

bool OnePlayBackendAPIStubs::isReverseScrollDirectionEnabled()
{
    bool reverseScroll = false;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isReverseScrollDirectionEnabled():%d\n",
                reverseScroll);

    return reverseScroll;
}

bool OnePlayBackendAPIStubs::isSwapFaceButtonsEnabled()
{
    bool swapFaceButtons = false;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::isSwapFaceButtonsEnabled():%d\n",
                swapFaceButtons);

    return swapFaceButtons;
}

QString OnePlayBackendAPIStubs::getCaptureSysKeysMode()
{
    // TBD:QString captureSysKeysMode = QString::fromStdString(mSessionDetails.data.other_details.capture_sys_keys);
    QString captureSysKeysMode = "never";
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::getCaptureSysKeysMode():%s\n",
                captureSysKeysMode.toStdString().c_str());

    return captureSysKeysMode;
}

QString OnePlayBackendAPIStubs::getVideoDecoderSelection()
{
    QString videoDecoder = "auto";
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::getVideoDecoderSelection():%s\n",
                videoDecoder.toStdString().c_str());

    return videoDecoder;
}

std::string OnePlayBackendAPIStubs::getUserId()
{
    std::string user_id = "1234";
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "OnePlayBackendAPI::getUserId():%d\n",
                user_id.c_str());

    return user_id;
}

int OnePlayBackendAPIStubs::getAudioPort()
{
    return 48000;
}

int OnePlayBackendAPIStubs::getVideoPort()
{
    return 47998;
}

int OnePlayBackendAPIStubs::getControlPort()
{
    return 47999;
}

int OnePlayBackendAPIStubs::getRtspPort()
{
    return 48010;
}

int OnePlayBackendAPIStubs::getAPIPort()
{
    return 47990;
}
