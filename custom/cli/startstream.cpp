#include "startstream.h"
#include "backend/computermanager.h"
#include "backend/computerseeker.h"
#include "streaming/session.h"
#include "signaling_rtsp.h"

#include <QtEndian>
#include <QCoreApplication>
#include <QTimer>

#define COMPUTER_SEEK_TIMEOUT 10000
#define APP_SEEK_TIMEOUT 10000

namespace CliStartStream
{

enum State {
    StateInit,
    StateStartSession,
    StateFailure,
};

class Event
{
public:
    enum Type {
        Executed,
        Timedout,
    };

    Event(Type type) : type(type)  {}

    Type type;
};

class LauncherPrivate
{
    Q_DECLARE_PUBLIC(Launcher)

public:
    LauncherPrivate(Launcher *q) : q_ptr(q) {}

    void handleEvent(Event event)
    {
        Q_Q(Launcher);

        switch (event.type) {
        // Occurs when CliStartStreamSegue becomes visible and the UI calls launcher's execute()
        case Event::Executed:
            m_State = StateStartSession;
            Session* session = new Session(m_Token, m_Preferences);
            emit q->sessionCreated("appname", session);
            break;
        }
    }

    Launcher *q_ptr;
    QString m_Token;

    SignalingClient* m_signaling;
    StreamingPreferences *m_Preferences;
    NvComputer *m_Computer;

    State m_State;
    QTimer *m_TimeoutTimer;
};


Launcher::Launcher(QString token, 
                   StreamingPreferences* preferences, 
                   QObject *parent)
    : QObject(parent),
      m_DPtr(new LauncherPrivate(this))
{
    Q_D(Launcher);
    d->m_Token = token;
    d->m_Preferences = preferences;
    
    d->m_State = StateInit;
    d->m_TimeoutTimer = new QTimer(this);
    d->m_TimeoutTimer->setSingleShot(true);
    connect(d->m_TimeoutTimer, &QTimer::timeout,
            this, &Launcher::onTimeout);
}

Launcher::~Launcher()
{
}

void Launcher::execute()
{
    Q_D(Launcher);
    Event event(Event::Executed);
    d->handleEvent(event);
}

bool Launcher::isExecuted() const
{
    Q_D(const Launcher);
    return d->m_State != StateInit;
}

void Launcher::onTimeout()
{
    Q_D(Launcher);
    Event event(Event::Timedout);
    d->handleEvent(event);
}
}
