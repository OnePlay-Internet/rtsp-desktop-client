#pragma once

#include <QString>
#include <SDL.h>

// Class which is responsible for client <-> server communication
class HostAPI {
public:
    explicit HostAPI()
    {
    }

    ~HostAPI()
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "HostAPI:%s",
                    "HostAPI is destroyed");
    }

    // Share the session key with the host, also it is used as the pin.
    void setHostSessionKey(QString hostIp, int apiPort, QString hostSessionKey);

    // Remove known pairings at the server side, so new session can be started with new session key, which is also used as a pin.
    bool unpairHostClients(QString hostIp, int apiPort);
};
