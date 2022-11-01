#include "computermanager.h"
#include "boxartmanager.h"
#include "nvhttp.h"
#include "settings/streamingpreferences.h"

#include <Limelight.h>
#include <QtEndian>

#include <QThread>
#include <QThreadPool>
#include <QCoreApplication>

#include <random>

#define SER_HOSTS "hosts"

