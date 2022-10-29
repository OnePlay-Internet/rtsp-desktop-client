#pragma once

#include <QObject>
#include <QVariant>
#include <Limelight.h>

class ComputerManager;
class NvComputer;
class Session;
class StreamingPreferences;

namespace CliStartStream
{

class Event;
class LauncherPrivate;

class Launcher : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE_D(m_DPtr, Launcher)

public:
    explicit Launcher(QString token,
                      StreamingPreferences* preferences,
                      QObject *parent = nullptr);
    ~Launcher();
    Q_INVOKABLE void execute();
    Q_INVOKABLE bool isExecuted() const;

signals:
    void sessionCreated(QString appName, Session *session);
    void failed(QString text);

private slots:
    void onTimeout  ();



private:
    QScopedPointer<LauncherPrivate> m_DPtr;
};

}
