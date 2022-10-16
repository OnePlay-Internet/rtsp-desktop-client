#pragma once

#include <QString>

// interface for interaction with OnePlay backend.
class IOnePlayBackendAPI {
public:
    virtual ~IOnePlayBackendAPI() = default;

    void initialize() {
        if (getOnePlayStreamingPreferences())
        {
            m_isInitialized = true;
        }
    }

    bool isInitialized() {
        return m_isInitialized;
    }

    // read streaming preferences from backend. "True" on success, "False" on error.
    virtual bool getOnePlayStreamingPreferences() = 0;

    // get the session key for host.
    virtual QString getHostSessionKey() = 0;

    // get the session key for client.
    virtual QString getSessionKey() = 0;

    // app name getter. App which is registered at streaming server side and which we are about to start.
    virtual QString getAppToStart() = 0;

    // audio mode getter. One of "stereo", "5.1-surround", "7.1-surround".
    virtual QString getAudioConfig() = 0;

    // bitrate getter. Bitrate must be in range: 500 - 150000 kbps.
    virtual void getBitrateKbps(std::int32_t & bitrateKbps) = 0;

    // fps getter. FPS must be in range 30 - 240.
    virtual void getFps(int & fps) = 0;

    // screen resolution getter.
    virtual void getScreenResolution(std::int32_t & width, std::int32_t & height) = 0;

    // streaming server IP adress getter.
    virtual QString getStreamingServerIP() = 0;

    // video codec getter. One of "auto", "H.264", "HEVC".
    virtual QString getVideoCodecConfig() = 0;

    // window mode getter. One of "fullscreen", "windowed", "borderless".
    virtual QString getWindowMode() = 0;

    // vsync option getter.
    virtual bool isVsyncEnabled() = 0;

    // get video packet size. 0 means that video packet size will be resolved later by the client depending on NvComputer::isReachableOverVpn() output (Either 1024 or 1392).
    virtual std::int32_t getPacketSize() = 0;

    // multiple controllers support.
    virtual bool isMultipleControllersSupportEnabled() = 0;

    // is app or game needs to be closed if the streaming client is closed.
    virtual bool isQuitAppAfterEnabled() = 0;

    // check if remote desktop optimized mouse control is enabled. Will not work in most games.
    virtual bool isAbsoluteMouseModeEnabled() = 0;

    // check if swap left and right mouse buttons is enabled.
    virtual bool isSwapMouseButtonsEnabled() = 0;

    // check if touchscreen in trackpad mode is enabled.
    virtual bool isAbsoluteTouchModeEnabled() = 0;

    // check if optimize game settings for streaming is enabled.
    virtual bool isGameOptimizationsEnabled() = 0;

    // check if play audio on the host PC is enabled.
    virtual bool isPlayAudioOnHostEnabled() = 0;

    // check if delay for frames that come too early is enabled.
    virtual bool isFramePacingEnabled() = 0;

    // check if audio mute is enabled if the streaming client window loses focus.
    virtual bool isMuteOnFocusLossEnabled() = 0;

    // check if gamepad input processing is enabled if the streaming client window loses focus.
    virtual bool isBackgroundGamepadEnabled() = 0;

    // is invert scroll direction enabled.
    virtual bool isReverseScrollDirectionEnabled() = 0;

    // is swap A/B and X/Y gamepad buttons enabled (Nintendo-style).
    virtual bool isSwapFaceButtonsEnabled() = 0;

    // get capture system-wide keyboard shortcuts mode (like Alt+Tab). Available options: "never", "fullscreen", "always"
    virtual QString getCaptureSysKeysMode() = 0;

    // choose decoder mode automatically. Available options: "auto", "software", "hardware".
    virtual QString getVideoDecoderSelection() = 0;

    // get the user id.
    virtual std::string getUserId() = 0;

    // get audio port
    virtual int getAudioPort() = 0;

    // get video port
    virtual int getVideoPort() = 0;

    // get control port
    virtual int getControlPort() = 0;

    // get rtsp port
    virtual int getRtspPort() = 0;

    // get API port
    virtual int getAPIPort() = 0;

private:
    bool m_isInitialized = false;
};
