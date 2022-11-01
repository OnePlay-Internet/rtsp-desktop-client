#pragma once

#include "nvhttp.h"
#include "nvaddress.h"

#include <QThread>
#include <QReadWriteLock>
#include <QSettings>
#include <QRunnable>
#include <signaling_rtsp.h>
#include <signaling_rtsp.h>

class NvDisplayMode
{
public:
    NvDisplayMode(DisplayMode* mode)
    {

        this->width = mode->width;
        this->height = mode->height;
        this->refreshRate = mode->refreshRate;
        active = true;
    }

    bool operator==(const NvDisplayMode& other) const
    {
        return width == other.width &&
                height == other.height &&
                refreshRate == other.refreshRate;
    }

    int width;
    int height;
    int refreshRate;
    bool active;
};

class NvComputer
{
public:
    explicit NvComputer(ServerInfor* serverInfor);

    bool isReachableOverVpn();

    QString gfeVersion;
    QString appVersion;
    QString gpuModel;
    bool isSupportedServerVersion;

    NvDisplayMode* displayModes[5];

    int maxLumaPixelsHEVC;
    int serverCodecModeSupport;

    // Ephemeral traits
    char activeAddress[50];
};
