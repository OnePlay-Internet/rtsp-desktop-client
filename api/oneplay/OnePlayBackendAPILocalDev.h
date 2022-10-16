#pragma once

#include <fstream>
#include <nlohmann/json.hpp>
#include <SDL.h>
#include <QCoreApplication>

#include "IOnePlayBackendAPI.h"
#include "OnePlayBackendAPI.h"
#include "settings/serverpreferences.h"

// OnePlayBackendAPILocalDev is an implementation of IOnePlayBackendAPI.
// that can be used to run moonlight without browser. This is useful for setup local development environment
class OnePlayBackendAPILocalDev: public OnePlayBackendAPI {
public:
    explicit OnePlayBackendAPILocalDev(QString sessionKey):
        OnePlayBackendAPI(std::move(sessionKey))
    {
        ServerPreferences serverPreferences;
        mStartGame = serverPreferences.getStartGame();
        mServer = serverPreferences.getServer();
    }

    ~OnePlayBackendAPILocalDev()
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "OnePlayBackendAPILocalDev:%s",
                    "OnePlayBackendAPILocalDev is destroyed");
    }

    std::string getSessionSignature();

    // get data from backend using session key obtained during initializtion.
    bool getOnePlayStreamingPreferences() override;

protected:
    QString mStartGame;
    QString mServer;
};
