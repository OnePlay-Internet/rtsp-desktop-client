#pragma once

#include <memory>
#include <SDL.h>

#include "IOnePlayBackendAPI.h"
#include "HostAPI.h"
#include "settings/streamingpreferences.h"

// OnePlayBackend is a class which uses implementation of IOnePlayBackendAPI to obtain data from
// the backend and make it suitable for client usage.
class OnePlayBackend {
public:
    explicit OnePlayBackend(std::shared_ptr<IOnePlayBackendAPI> onePlayBackendAPI):
        mIOnePlayBackendAPISPtr(std::move(onePlayBackendAPI))
    {
        // init streaming preferences or throw error
        mIOnePlayBackendAPISPtr->initialize();

        mAudioConfigMap = {
            {"stereo",       StreamingPreferences::AC_STEREO},
            {"5.1-surround", StreamingPreferences::AC_51_SURROUND},
            {"7.1-surround", StreamingPreferences::AC_71_SURROUND},
        };
        mVideoCodecMap = {
            {"auto",  StreamingPreferences::VCC_AUTO},
            {"H.264", StreamingPreferences::VCC_FORCE_H264},
            {"HEVC",  StreamingPreferences::VCC_FORCE_HEVC},
        };
        mWindowModeMap = {
            {"fullscreen", StreamingPreferences::WM_FULLSCREEN},
            {"windowed",   StreamingPreferences::WM_WINDOWED},
            {"borderless", StreamingPreferences::WM_FULLSCREEN_DESKTOP},
        };
        mVideoDecoderMap = {
            {"auto",     StreamingPreferences::VDS_AUTO},
            {"software", StreamingPreferences::VDS_FORCE_SOFTWARE},
            {"hardware", StreamingPreferences::VDS_FORCE_HARDWARE},
        };
        mCaptureSysKeysModeMap = {
            {"never",      StreamingPreferences::CSK_OFF},
            {"fullscreen", StreamingPreferences::CSK_FULLSCREEN},
            {"always",     StreamingPreferences::CSK_ALWAYS},
        };
    }

    ~OnePlayBackend()
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "OnePlayBackend:%s",
                    "OnePlayBackend is destroyed");
    }

    bool isOnePlayBackendInitialized() {
        return mIOnePlayBackendAPISPtr->isInitialized();
    }

    // Get streaming server ip.
    QString getHost() const;

    // Get app name which is about to stream.
    QString getAppName() const;

    // Get session key for host.
    QString getHostSessionKey() const;

    // Get session key for client.
    QString getQuitQString() const;

    // Setup preferences using backend as source of configuration.
    void getStreamingPreferences(StreamingPreferences *preferences);

    // Report errors
    void reportError(QString err);

    // TBD: better naming, and probably functions below should be moved to "server tools", as it is not related to backend directly.
    // Share the session key with the host, also it is used as the pin.
    // Note: call this after pairing is started.
    void setHostSessionKey();

    // Remove known pairings at the server side, so new session can be started with new session key, which is also used as a pin.
    bool unpairHostClients();

private:

    // Pointer to backend interface implementation.
    std::shared_ptr<IOnePlayBackendAPI> mIOnePlayBackendAPISPtr;
    HostAPI mHostAPI;

    // Maps which allows convert string backend responses into client values.
    QMap<QString, StreamingPreferences::AudioConfig> mAudioConfigMap;
    QMap<QString, StreamingPreferences::VideoCodecConfig> mVideoCodecMap;
    QMap<QString, StreamingPreferences::WindowMode> mWindowModeMap;
    QMap<QString, StreamingPreferences::VideoDecoderSelection> mVideoDecoderMap;
    QMap<QString, StreamingPreferences::CaptureSysKeysMode> mCaptureSysKeysModeMap;
};
