#include "quitstream.h"

#include "backend/computermanager.h"
#include "backend/computerseeker.h"
#include "streaming/session.h"

#include <QCoreApplication>
#include <QTimer>

#define COMPUTER_SEEK_TIMEOUT 10000
