#pragma once

#include <fstream>
#include <nlohmann/json.hpp>
#include <SDL.h>
#include <QCoreApplication>

#include "IOnePlayBackendAPI.h"
#include "settings/serverpreferences.h"

#define DYNAMIC_PORTS

using json = nlohmann::json;

// below are the structures holding game session details.
// IMPORTANT: This should be inline with backend json structures.
class game_details {
public:
    std::string arguments;
    std::string background;
    std::string file_name;
    std::string file_path;
    std::string id;
    std::string launch_type;
    std::string monitor_process;
    std::string title;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(game_details, arguments, background, file_name, file_path, id, launch_type, monitor_process, title)
};

class advance_details {
public:
    bool absolute_mouse_mode;
    bool absolute_touch_mode;
    bool background_gamepad;
    bool frame_pacing;
    bool game_optimizations;
    bool multi_color;
    bool mute_on_focus_loss;
    std::int32_t packet_size;
    bool play_audio_on_host;
    bool quit_app_after;
    bool reverse_scroll_direction;
    bool swap_face_buttons;
    bool swap_mouse_buttons;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(advance_details, absolute_mouse_mode, absolute_touch_mode, background_gamepad,
                                   frame_pacing, game_optimizations, multi_color, mute_on_focus_loss,
                                   packet_size, play_audio_on_host, quit_app_after, reverse_scroll_direction,
                                   swap_face_buttons, swap_mouse_buttons)
};

class other_details {
public:
    ::advance_details advance_details;
    std::string audio_type;
    std::int64_t bitrate_kbps;
    bool capture_sys_keys;
    bool controller_mouse_emulation;
    bool controller_usb_driver_support;
    bool disable_frame_drop;
    bool enable_hdr;
    bool enable_perf_overlay;
    bool enable_pip;
    bool enable_post_stream_toast;
    std::int32_t game_fps;
    bool is_vsync_enabled;
    std::int64_t max_bitrate_kbps;
    std::int32_t max_fps;
    std::string max_resolution;
    bool mouse_nav_buttons;
    bool onscreen_controls;
    std::string resolution;
    std::int64_t session_expire_sec;
    std::string stream_codec;
    bool unlock_fps;
    bool vibrate_osc;
    std::string video_decoder_selection;
    std::string window_mode;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(other_details, advance_details, audio_type, bitrate_kbps, capture_sys_keys,
                                controller_mouse_emulation, controller_usb_driver_support, disable_frame_drop,
                                enable_hdr, enable_perf_overlay, enable_pip, enable_post_stream_toast,
                                game_fps, is_vsync_enabled, max_bitrate_kbps, max_fps, max_resolution,
                                mouse_nav_buttons, onscreen_controls,
                                resolution, session_expire_sec, stream_codec, unlock_fps, vibrate_osc,
                                video_decoder_selection, window_mode)
};

class port_details {
public:
    std::int64_t audio_port;
    std::int64_t control_port;
    std::int64_t http_port;
    std::int64_t https_port;
    std::int64_t rtsp_port;
    std::int64_t video_port;
    std::int64_t pin_port;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(port_details, audio_port, control_port, http_port, https_port, rtsp_port, video_port, pin_port)
};

class server_details {
public:
    bool any_client_connected;
    bool is_online;
    std::int64_t last_pinged;
    ::port_details port_details;
    std::string server_ip;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(server_details, any_client_connected, is_online, last_pinged, port_details, server_ip)
};

class user_details {
public:
    std::string client_ip;
    std::int64_t timestamp;
    std::string user_id;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(user_details, client_ip, timestamp, user_id)
};

class data {
public:
    ::game_details game_details;
    std::string host_session_key;
    ::other_details other_details;
    ::server_details server_details;
    ::user_details user_details;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(data, game_details, host_session_key, other_details, server_details, user_details)
};

class session_details {
public:
    ::data data;
    std::string msg;
    std::string status;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(session_details, data, msg, status)
};

// OnePlayBackendAPI is an implementation of IOnePlayBackendAPI.
// Log session key obtained from cli and hardcoded data in SDL log.
class OnePlayBackendAPI: public IOnePlayBackendAPI {
public:
    explicit OnePlayBackendAPI(QString sessionKey):
        mSessionKey(std::move(sessionKey))
    {
        ServerPreferences serverPreferences;
        mBackendAddr = serverPreferences.getBackend();
        mSessionDetailsPath = serverPreferences.getSessionDetailsPath();

        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Backend:%s%s",
                    mBackendAddr.toStdString().c_str(),
                    mSessionDetailsPath.toStdString().c_str());

//        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
//                    "Client session key:%s",
//                    mSessionKey.toStdString().c_str());
    }

    ~OnePlayBackendAPI()
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "OnePlayBackendAPI:%s",
                    "OnePlayBackendAPI is destroyed");
    }

    // get data from backend using session key obtained during initializtion.
    bool getOnePlayStreamingPreferences() override;

    // get the session key for host.
    QString getHostSessionKey() override;

    // get the session key for client.
    QString getSessionKey() override;

    // get app name to start
    QString getAppToStart() override;

    // get audio mode.
    QString getAudioConfig() override;

    // get bitrate.
    void getBitrateKbps(std::int32_t & bitrateKbps) override;

    // get fps.
    void getFps(std::int32_t & fps) override;

    // get screen resolution.
    void getScreenResolution(std::int32_t & width, std::int32_t & height) override;

    // get server IP adress.
    QString getStreamingServerIP() override;

    // get selected video codec.
    QString getVideoCodecConfig() override;

    // get window mode.
    QString getWindowMode() override;

    // get vsync option value.
    bool isVsyncEnabled() override;

    // get video packet size. 0 means that video packet size will be resolved later by the client depending on NvComputer::isReachableOverVpn() output (Either 1024 or 1392).
    std::int32_t getPacketSize() override;

    // multiple controllers support.
    bool isMultipleControllersSupportEnabled() override;

    // is app or game needs to be closed if the streaming client is closed.
    bool isQuitAppAfterEnabled() override;

    // check if remote desktop optimized mouse control is enabled. Will not work in most games.
    bool isAbsoluteMouseModeEnabled() override;

    // check if swap left and right mouse buttons is enabled.
    bool isSwapMouseButtonsEnabled() override;

    // check if touchscreen in trackpad mode is enabled.
    bool isAbsoluteTouchModeEnabled() override;

    // check if optimize game settings for streaming is enabled.
    bool isGameOptimizationsEnabled() override;

    // check if play audio on the host PC is enabled.
    bool isPlayAudioOnHostEnabled() override;

    // check if delay for frames that come too early is enabled.
    bool isFramePacingEnabled() override;

    // check if audio mute is enabled if the streaming client window loses focus.
    bool isMuteOnFocusLossEnabled() override;

    // check if gamepad input processing is enabled if the streaming client window loses focus.
    bool isBackgroundGamepadEnabled() override;

    // is invert scroll direction enabled.
    bool isReverseScrollDirectionEnabled() override;

    // is swap A/B and X/Y gamepad buttons enabled (Nintendo-style).
    bool isSwapFaceButtonsEnabled() override;

    // get capture system-wide keyboard shortcuts mode (like Alt+Tab). Available options: "never", "fullscreen", "always"
    QString getCaptureSysKeysMode() override;

    // Choose decoder mode automatically. Available options: "auto", "software", "hardware".
    QString getVideoDecoderSelection() override;

    // get the user id.
    std::string getUserId() override;

    int getAudioPort() override;
    int getVideoPort() override;
    int getControlPort() override;
    int getRtspPort() override;
    int getAPIPort() override;

protected:
    std::string postRequest(const QString &url, const QString &postData, const QStringList headersList);

protected:
    // resour.
    QString const & getSessionDetailsPath()
    {
        // trailing slashes are preserved
        return mSessionDetailsPath;
    }

    // backend address getter.
    QString const & getBackendAddress()
    {
        return mBackendAddr;
    }

    // client version content getter.
    static QString const & getClientVersion()
    {
        static QString version = "v1.0";
        return version;
    }

    // platform getter. Possible values: pc, mac.
    // TBD: mac should be returned for mac clients. Investigate if android is required also.
    static QString const & getPlatform()
    {
        static QString platform = "pc";
        return platform;
    }

    session_details mSessionDetails;
    QString mSessionKey;
    QString mHost;
    QString mBackendAddr;
    QString mSessionDetailsPath;
};
