#pragma once

#include <SDL.h>

#include "IOnePlayBackendAPI.h"
#include "OnePlayBackendAPI.h"

// OnePlayBackendAPIStubs is a stub implementation of IOnePlayBackendAPI.
// Log session key obtained from cli and hardcoded data in SDL log.
class OnePlayBackendAPIStubs: public IOnePlayBackendAPI {
public:
    explicit OnePlayBackendAPIStubs(QString sessionKey):
        mSessionKey(std::move(sessionKey))
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Client session key:%s",
                    mSessionKey.toStdString().c_str());
    }

    ~OnePlayBackendAPIStubs()
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "OnePlayBackendAPIStubs:%s",
                    "OnePlayBackendAPIStubs is destroyed");
    }

    // init client data stub
    bool getOnePlayStreamingPreferences() override
    {
        return true;
    }

    // get the session key for host.
    QString getHostSessionKey() override;

    // get the session key for client.
    QString getSessionKey() override;

    // stub to get app name. Returns "Desktop".
    QString getAppToStart() override;

    // stub to get audio mode. Returns "stereo".
    QString getAudioConfig() override;

    // stub to get bitrate. Set bitrateKbps = 10000.
    void getBitrateKbps(std::int32_t & bitrateKbps) override;

    // stub to get fps. Set fps = 60.
    void getFps(std::int32_t & fps) override;

    // stub to get screen resolution. Set width = 1280, height = 720.
    void getScreenResolution(std::int32_t & width, std::int32_t & height) override;

    // stub to get server IP adress. Returns "server" from "OnePlay.config" or "localhost".
    QString getStreamingServerIP() override;

    // stub to get selected video codec. Returns "auto".
    QString getVideoCodecConfig() override;

    // stub to get window mode. Returns "windowed".
    QString getWindowMode() override;

    // stub to get vsync option value. Returns "false".
    bool isVsyncEnabled() override;

    // stub to get video packet size. Returns "0".
    std::int32_t getPacketSize() override;

    // stub to get multiple controllers support status. Returns "true".
    bool isMultipleControllersSupportEnabled() override;

    // stub to get "app or game needs to be closed if the streaming client is closed" status. Returns "false".
    bool isQuitAppAfterEnabled() override;

    // stub to get remote desktop optimized mouse control status. Returns "false".
    bool isAbsoluteMouseModeEnabled() override;

    // stub to get "swap left and right mouse buttons" status. Returns "false".
    bool isSwapMouseButtonsEnabled() override;

    // stub to get "touchscreen in trackpad mode" status. Returns "true".
    bool isAbsoluteTouchModeEnabled() override;

    // stub to get "optimize game settings for streaming" status. Returns "true".
    bool isGameOptimizationsEnabled() override;

    // stub to get "play audio on the host PC" status. Returns "false".
    bool isPlayAudioOnHostEnabled() override;

    // stub to get "delay for frames that come too early" status. Returns "false".
    bool isFramePacingEnabled() override;

    // stub to get "audio mute is enabled if the streaming client window loses focus" status. Returns "false".
    bool isMuteOnFocusLossEnabled() override;

    // stub to get "gamepad input processing is enabled if the streaming client window loses focus" status. Returns "false".
    bool isBackgroundGamepadEnabled() override;

    // stub to get "invert scroll direction" status. Returns "false".
    bool isReverseScrollDirectionEnabled() override;

    // stub to get "swap A/B and X/Y gamepad buttons" status. Returns "false".
    bool isSwapFaceButtonsEnabled() override;

    // stub to get "capture system-wide keyboard shortcuts mode". Available options: "never", "fullscreen", "always". Returns "never".
    QString getCaptureSysKeysMode() override;

    // stub to get videoDicoderSelection mode. Available options: "auto", "software", "hardware". Returns "auto".
    QString getVideoDecoderSelection() override;

    // get the user id.
    std::string getUserId() override;

    // get audio port
    int getAudioPort() override;

    // get video port
    int getVideoPort() override;

    // get control port
    int getControlPort() override;

    // get rstp port
    int getRtspPort() override;

    // get API port (http + 1)
    int getAPIPort() override;

private:
    QString mSessionKey;
};
