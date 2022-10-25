#include "listapps.h"

#include "backend/boxartmanager.h"
#include "backend/computermanager.h"
#include "backend/computerseeker.h"

#include <QCoreApplication>
#include <QTimer>

#define COMPUTER_SEEK_TIMEOUT 10000
#define APP_SEEK_TIMEOUT 10000

