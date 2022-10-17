#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QQuickStyle>
#include <QMutex>
#include <QtDebug>
#include <QNetworkProxyFactory>
#include <QPalette>
#include <QFont>
#include <QCursor>
#include <QElapsedTimer>
#include <QFile>

// Don't let SDL hook our main function, since Qt is already
// doing the same thing. This needs to be before any headers
// that might include SDL.h themselves.
#define SDL_MAIN_HANDLED
#include <SDL.h>

#ifdef HAVE_FFMPEG
#include "streaming/video/ffmpeg.h"
#endif

#if defined(Q_OS_WIN32)
#include "antihookingprotection.h"
#elif defined(Q_OS_LINUX)
#include <openssl/ssl.h>
#endif

#include "cli/listapps.h"

#ifdef _DEBUG
// TBD: Fix duplicate in startstream.cpp
// use stubs to simulate backend pararmeters. Trigger stubs usage if key session is "12345".
#define USE_STUBS_INSTEAD_OF_BACKEND
#endif

#include <curl/curl.h>
#include "Exiter.h"
#include "onePlayAPI/OnePlayBackend.h"
#include "onePlayAPI/OnePlayBackendAPI.h"
#include "UrlEventFilter.h"

#ifdef USE_STUBS_INSTEAD_OF_BACKEND
#include "onePlayAPI/OnePlayBackendAPIStubs.h"
#include "onePlayAPI/OnePlayBackendAPILocalDev.h"
#endif

#include "cli/quitstream.h"
#include "cli/startstream.h"
#include "cli/pair.h"
#include "cli/commandlineparser.h"
#include "path.h"
#include "utils.h"
#include "gui/computermodel.h"
#include "gui/appmodel.h"
#include "backend/autoupdatechecker.h"
#include "backend/computermanager.h"
#include "backend/systemproperties.h"
#include "streaming/session.h"
#include "settings/streamingpreferences.h"
#include "gui/sdlgamepadkeynavigation.h"

#if !defined(QT_DEBUG) && defined(Q_OS_WIN32)
// Log to file for release Windows builds
#define USE_CUSTOM_LOGGER
#define LOG_TO_FILE
#elif defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
// Use stdout logger on all Linux/BSD builds
#define USE_CUSTOM_LOGGER
#elif !defined(QT_DEBUG) && defined(Q_OS_DARWIN)
// Log to file for release Mac builds
#define USE_CUSTOM_LOGGER
#define LOG_TO_FILE
#else
// For debug Windows and Mac builds, use default logger
#endif

#ifdef USE_CUSTOM_LOGGER
static QElapsedTimer s_LoggerTime;
static QTextStream s_LoggerStream(stdout);
static QMutex s_LoggerLock;
static bool s_SuppressVerboseOutput;
#ifdef LOG_TO_FILE
#define MAX_LOG_LINES 10000
static int s_LogLinesWritten = 0;
static bool s_LogLimitReached = false;
static QFile* s_LoggerFile;
#endif

void logToLoggerStream(QString& message)
{
    QMutexLocker lock(&s_LoggerLock);

#ifdef LOG_TO_FILE
    if (s_LogLimitReached) {
        return;
    }
    else if (s_LogLinesWritten == MAX_LOG_LINES) {
        s_LoggerStream << "Log size limit reached!";
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        s_LoggerStream << Qt::endl;
#else
        s_LoggerStream << endl;
#endif
        s_LogLimitReached = true;
        return;
    }
    else {
        s_LogLinesWritten++;
    }
#endif

    s_LoggerStream << message;
    s_LoggerStream.flush();
}

void sdlLogToDiskHandler(void*, int category, SDL_LogPriority priority, const char* message)
{
    QString priorityTxt;

    switch (priority) {
    case SDL_LOG_PRIORITY_VERBOSE:
        if (s_SuppressVerboseOutput) {
            return;
        }
        priorityTxt = "Verbose";
        break;
    case SDL_LOG_PRIORITY_DEBUG:
        if (s_SuppressVerboseOutput) {
            return;
        }
        priorityTxt = "Debug";
        break;
    case SDL_LOG_PRIORITY_INFO:
        if (s_SuppressVerboseOutput) {
            return;
        }
        priorityTxt = "Info";
        break;
    case SDL_LOG_PRIORITY_WARN:
        if (s_SuppressVerboseOutput) {
            return;
        }
        priorityTxt = "Warn";
        break;
    case SDL_LOG_PRIORITY_ERROR:
        priorityTxt = "Error";
        break;
    case SDL_LOG_PRIORITY_CRITICAL:
        priorityTxt = "Critical";
        break;
    default:
        priorityTxt = "Unknown";
        break;
    }

    QTime logTime = QTime::fromMSecsSinceStartOfDay(s_LoggerTime.elapsed());
    QString txt = QString("%1 - SDL %2 (%3): %4\n").arg(logTime.toString()).arg(priorityTxt).arg(category).arg(message);

    logToLoggerStream(txt);
}

void qtLogToDiskHandler(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
    QString typeTxt;

    switch (type) {
    case QtDebugMsg:
        if (s_SuppressVerboseOutput) {
            return;
        }
        typeTxt = "Debug";
        break;
    case QtInfoMsg:
        if (s_SuppressVerboseOutput) {
            return;
        }
        typeTxt = "Info";
        break;
    case QtWarningMsg:
        if (s_SuppressVerboseOutput) {
            return;
        }
        typeTxt = "Warning";
        break;
    case QtCriticalMsg:
        typeTxt = "Critical";
        break;
    case QtFatalMsg:
        typeTxt = "Fatal";
        break;
    }

    QTime logTime = QTime::fromMSecsSinceStartOfDay(s_LoggerTime.elapsed());
    QString txt = QString("%1 - Qt %2: %3\n").arg(logTime.toString()).arg(typeTxt).arg(msg);

    logToLoggerStream(txt);
}

#ifdef HAVE_FFMPEG

void ffmpegLogToDiskHandler(void* ptr, int level, const char* fmt, va_list vl)
{
    char lineBuffer[1024];
    static int printPrefix = 1;

    if ((level & 0xFF) > av_log_get_level()) {
        return;
    }
    else if ((level & 0xFF) > AV_LOG_WARNING && s_SuppressVerboseOutput) {
        return;
    }

    // We need to use the *previous* printPrefix value to determine whether to
    // print the prefix this time. av_log_format_line() will set the printPrefix
    // value to indicate whether the prefix should be printed *next time*.
    bool shouldPrefixThisMessage = printPrefix != 0;

    av_log_format_line(ptr, level, fmt, vl, lineBuffer, sizeof(lineBuffer), &printPrefix);

    if (shouldPrefixThisMessage) {
        QTime logTime = QTime::fromMSecsSinceStartOfDay(s_LoggerTime.elapsed());
        QString txt = QString("%1 - FFmpeg: %2").arg(logTime.toString()).arg(lineBuffer);
        logToLoggerStream(txt);
    }
    else {
        QString txt = QString(lineBuffer);
        logToLoggerStream(txt);
    }
}

#endif

#endif

#ifdef Q_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>

#if defined (Q_OS_WIN32)
#include "cli/systemprocessenumerator.h"
#endif

#if defined (Q_OS_WIN32)
#include "cli/systemprocessenumerator.h"
#endif

static UINT s_HitUnhandledException = 0;

LONG WINAPI UnhandledExceptionHandler(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
    // Only write a dump for the first unhandled exception
    if (InterlockedCompareExchange(&s_HitUnhandledException, 1, 0) != 0) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    WCHAR dmpFileName[MAX_PATH];
    swprintf_s(dmpFileName, L"%ls\\OnePlay-%I64u.dmp",
               (PWCHAR)QDir::toNativeSeparators(Path::getLogDir()).utf16(), QDateTime::currentSecsSinceEpoch());
    QString qDmpFileName = QString::fromUtf16((const char16_t*)dmpFileName);
    HANDLE dumpHandle = CreateFileW(dmpFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (dumpHandle != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION info;

        info.ThreadId = GetCurrentThreadId();
        info.ExceptionPointers = ExceptionInfo;
        info.ClientPointers = FALSE;

        DWORD typeFlags = MiniDumpWithIndirectlyReferencedMemory |
                MiniDumpIgnoreInaccessibleMemory |
                MiniDumpWithUnloadedModules |
                MiniDumpWithThreadInfo;

        if (MiniDumpWriteDump(GetCurrentProcess(),
                               GetCurrentProcessId(),
                               dumpHandle,
                               (MINIDUMP_TYPE)typeFlags,
                               &info,
                               nullptr,
                               nullptr)) {
            qCritical() << "Unhandled exception! Minidump written to:" << qDmpFileName;
        }
        else {
            qCritical() << "Unhandled exception! Failed to write dump:" << GetLastError();
        }

        CloseHandle(dumpHandle);
    }
    else {
        qCritical() << "Unhandled exception! Failed to open dump file:" << qDmpFileName << "with error" << GetLastError();
    }

    // Let the program crash and WER collect a dump
    return EXCEPTION_CONTINUE_SEARCH;
}

#endif

int main(int argc, char *argv[])
{
    SDL_SetMainReady();
    // TBD: consider options for MAC
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Set the app version for the QCommandLineParser's showVersion() command
    QCoreApplication::setApplicationVersion(VERSION_STR);

    // Set these here to allow us to use the default QSettings constructor.
    // These also ensure that our cache directory is named correctly. As such,
    // it is critical that these be called before Path::initialize().
    QCoreApplication::setOrganizationName("OnePlay");
    QCoreApplication::setOrganizationDomain("https://client-apis.oneplay.in/");
    QCoreApplication::setApplicationName("OnePlay");

    if (QFile(QDir::currentPath() + "/portable.dat").exists()) {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QDir::currentPath());
        QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, QDir::currentPath());

        // Initialize paths for portable mode
        Path::initialize(true);
    }
    else {
        // Initialize paths for standard installation
        Path::initialize(false);
    }

#ifdef USE_CUSTOM_LOGGER
#ifdef LOG_TO_FILE
    QDir tempDir(Path::getLogDir());
    s_LoggerFile = new QFile(tempDir.filePath(QString("OnePlay-%1.log").arg(QDateTime::currentSecsSinceEpoch())));
    if (s_LoggerFile->open(QIODevice::WriteOnly)) {
        QTextStream(stderr) << "Redirecting log output to " << s_LoggerFile->fileName() << Qt::endl;
        s_LoggerStream.setDevice(s_LoggerFile);
    }
#endif

    s_LoggerTime.start();
    qInstallMessageHandler(qtLogToDiskHandler);
    SDL_LogSetOutputFunction(sdlLogToDiskHandler, nullptr);

#ifdef HAVE_FFMPEG
    av_log_set_callback(ffmpegLogToDiskHandler);
#endif

#endif

#ifdef Q_OS_WIN32
    // Create a crash dump when we crash on Windows
    SetUnhandledExceptionFilter(UnhandledExceptionHandler);
#endif

#if defined(Q_OS_WIN32)
    // Force AntiHooking.dll to be statically imported and loaded
    // by ntdll on Win32 platforms by calling a dummy function.
    AntiHookingDummyImport();
#elif defined(Q_OS_LINUX)
    // Force libssl.so to be directly linked to our binary, so
    // linuxdeployqt can find it and include it in our AppImage.
    // QtNetwork will pull it in via dlopen().
    SSL_free(nullptr);
#endif

    // Avoid using High DPI on EGLFS. It breaks font rendering.
    // https://bugreports.qt.io/browse/QTBUG-64377
    //
    // NB: We can't use QGuiApplication::platformName() here because it is only
    // set once the QGuiApplication is created, which is too late to enable High DPI :(
    if (WMUtils::isRunningWindowManager()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        // Enable High DPI support on Qt 5.x. It is always enabled on Qt 6.0
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        // Enable fractional High DPI scaling on Qt 5.14 and later
        QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
    }
    else {
#ifndef STEAM_LINK
        if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM")) {
            qInfo() << "Unable to detect Wayland or X11, so EGLFS will be used by default. Set QT_QPA_PLATFORM to override this.";
            qputenv("QT_QPA_PLATFORM", "eglfs");

            if (!qEnvironmentVariableIsSet("QT_QPA_EGLFS_ALWAYS_SET_MODE")) {
                qInfo() << "Setting display mode by default. Set QT_QPA_EGLFS_ALWAYS_SET_MODE=0 to override this.";

                // The UI doesn't appear on RetroPie without this option.
                qputenv("QT_QPA_EGLFS_ALWAYS_SET_MODE", "1");
            }

            if (!QFile("/dev/dri").exists()) {
                qWarning() << "Unable to find a KMSDRM display device!";
                qWarning() << "On the Raspberry Pi, you must enable the 'fake KMS' driver in raspi-config to use OnePlay outside of the GUI environment.";
            }
        }
#endif
    }

    // This avoids using the default keychain for SSL, which may cause
    // password prompts on macOS.
    qputenv("QT_SSL_USE_TEMPORARY_KEYCHAIN", "1");

#if defined(Q_OS_WIN32) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (!qEnvironmentVariableIsSet("QT_OPENGL")) {
        // On Windows, use ANGLE so we don't have to load OpenGL
        // user-mode drivers into our app. OGL drivers (especially Intel)
        // seem to crash OnePlay far more often than DirectX.
        qputenv("QT_OPENGL", "angle");
    }
#endif

#if !defined(Q_OS_WIN32) || QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // OnePlay requires the non-threaded renderer because we depend
    // on being able to control the render thread by blocking in the
    // main thread (and pumping events from the main thread when needed).
    // That doesn't work with the threaded renderer which causes all
    // sorts of odd behavior depending on the platform.
    //
    // NB: Windows defaults to the "windows" non-threaded render loop on
    // Qt 5 and the threaded render loop on Qt 6.
    qputenv("QSG_RENDER_LOOP", "basic");
#endif

    // We don't want system proxies to apply to us
    QNetworkProxyFactory::setUseSystemConfiguration(false);

    // Clear any default application proxy
    QNetworkProxy noProxy(QNetworkProxy::NoProxy);
    QNetworkProxy::setApplicationProxy(noProxy);

    // Register custom metatypes for use in signals
    qRegisterMetaType<NvApp>("NvApp");

    // Allow the display to sleep by default. We will manually use SDL_DisableScreenSaver()
    // and SDL_EnableScreenSaver() when appropriate. This hint must be set before
    // initializing the SDL video subsystem to have any effect.
    SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");

    // For SDL backends that support it, use double buffering instead of triple buffering
    // to save a frame of latency. This doesn't matter for MMAL or DRM renderers since they
    // are drawing directly to the screen without involving SDL, but it may matter for other
    // future KMSDRM platforms that use SDL for rendering.
    SDL_SetHint(SDL_HINT_VIDEO_DOUBLE_BUFFER, "1");

    // We use MMAL to render on Raspberry Pi, so we do not require DRM master.
    SDL_SetHint("SDL_KMSDRM_REQUIRE_DRM_MASTER", "0");

    // Use Direct3D 9Ex to avoid a deadlock caused by the D3D device being reset when
    // the user triggers a UAC prompt. This option controls the software/SDL renderer.
    // The DXVA2 renderer uses Direct3D 9Ex itself directly.
    SDL_SetHint("SDL_WINDOWS_USE_D3D9EX", "1");

    if (SDL_InitSubSystem(SDL_INIT_TIMER) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "SDL_InitSubSystem(SDL_INIT_TIMER) failed: %s",
                     SDL_GetError());
        return -1;
    }

#ifdef STEAM_LINK
    // Steam Link requires that we initialize video before creating our
    // QGuiApplication in order to configure the framebuffer correctly.
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "SDL_InitSubSystem(SDL_INIT_VIDEO) failed: %s",
                     SDL_GetError());
        return -1;
    }
#endif

    // Use atexit() to ensure SDL_Quit() is called. This avoids
    // racing with object destruction where SDL may be used.
    atexit(SDL_Quit);

    // Avoid the default behavior of changing the timer resolution to 1 ms.
    // We don't want this all the time that OnePlay is open. We will set
    // it manually when we start streaming.
    SDL_SetHint(SDL_HINT_TIMER_RESOLUTION, "0");

    // Disable minimize on focus loss by default. Users seem to want this off by default.
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

    // SDL 2.0.12 changes the default behavior to use the button label rather than the button
    // position as most other software does. Set this back to 0 to stay consistent with prior
    // releases of OnePlay.
    SDL_SetHint("SDL_GAMECONTROLLER_USE_BUTTON_LABELS", "0");

    // Disable relative mouse scaling to renderer size or logical DPI. We want to send
    // the mouse motion exactly how it was given to us.
    SDL_SetHint("SDL_MOUSE_RELATIVE_SCALING", "0");

    // Set our app name for SDL to use with PulseAudio and PipeWire. This matches what we
    // provide as our app name to libsoundio too. On SDL 2.0.18+, SDL_APP_NAME is also used
    // for screensaver inhibitor reporting.
    SDL_SetHint("SDL_AUDIO_DEVICE_APP_NAME", "OnePlay");
    SDL_SetHint("SDL_APP_NAME", "OnePlay");

    // We handle capturing the mouse ourselves when it leaves the window, so we don't need
    // SDL doing it for us behind our backs.
    SDL_SetHint("SDL_MOUSE_AUTO_CAPTURE", "0");

#ifdef QT_DEBUG
    // Allow thread naming using exceptions on debug builds. SDL doesn't use SEH
    // when throwing the exceptions, so we don't enable it for release builds out
    // of caution.
    SDL_SetHint(SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING, "0");
#endif

    QGuiApplication app(argc, argv);


    SDL_version compileVersion;
    SDL_VERSION(&compileVersion);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "Compiled with SDL %d.%d.%d",
                compileVersion.major, compileVersion.minor, compileVersion.patch);

    SDL_version runtimeVersion;
    SDL_GetVersion(&runtimeVersion);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "Running with SDL %d.%d.%d",
                runtimeVersion.major, runtimeVersion.minor, runtimeVersion.patch);

#ifdef Q_OS_MACX
    UrlEventFilter* appEventFilter = new UrlEventFilter();
    app.installEventFilter(appEventFilter);
#endif

#if defined (Q_OS_WIN32)
    {
        if (utils::SystemProcessEnumerator
                 ::isProcessRunning(app.applicationName())) {
            exit(-1); // TODO: Add a separate status
        }
    }
#endif

    // Apply the initial translation based on user preference
    StreamingPreferences prefs;
    prefs.retranslate();

    // After the QGuiApplication is created, the platform stuff will be initialized
    // and we can set the SDL video driver to match Qt.
    if (WMUtils::isRunningWayland() && QGuiApplication::platformName() == "xcb") {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Detected XWayland. This will probably break hardware decoding! Try running with QT_QPA_PLATFORM=wayland or switch to X11.");
        qputenv("SDL_VIDEODRIVER", "x11");
    }
    else if (QGuiApplication::platformName().startsWith("wayland")) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Detected Wayland");
        qputenv("SDL_VIDEODRIVER", "wayland");
    }

#ifdef STEAM_LINK
    // Qt 5.9 from the Steam Link SDK is not able to load any fonts
    // since the Steam Link doesn't include any of the ones it looks
    // for. We know it has NotoSans so we will explicitly ask for that.
    if (app.font().family().isEmpty()) {
        qWarning() << "SL HACK: No default font - using NotoSans";

        QFont fon("NotoSans");
        app.setFont(fon);
    }

    // Move the mouse to the bottom right so it's invisible when using
    // gamepad-only navigation.
    QCursor().setPos(0xFFFF, 0xFFFF);
#elif !SDL_VERSION_ATLEAST(2, 0, 11) && defined(Q_OS_LINUX) && (defined(__arm__) || defined(__aarch64__))
    if (qgetenv("SDL_VIDEO_GL_DRIVER").isEmpty() && QGuiApplication::platformName() == "eglfs") {
        // Look for Raspberry Pi GLES libraries. SDL 2.0.10 and earlier needs some help finding
        // the correct libraries for the KMSDRM backend if not compiled with the RPI backend enabled.
        if (SDL_LoadObject("libbrcmGLESv2.so") != nullptr) {
            qputenv("SDL_VIDEO_GL_DRIVER", "libbrcmGLESv2.so");
        }
        else if (SDL_LoadObject("/opt/vc/lib/libbrcmGLESv2.so") != nullptr) {
            qputenv("SDL_VIDEO_GL_DRIVER", "/opt/vc/lib/libbrcmGLESv2.so");
        }
    }
#endif

#ifndef Q_OS_DARWIN
    // Set the window icon except on macOS where we want to keep the
    // modified macOS 11 style rounded corner icon.
    app.setWindowIcon(QIcon(":/res/oneplay.svg"));
#endif

    // This is necessary to show our icon correctly on Wayland
    app.setDesktopFileName("com.oneplay_stream.OnePlay.desktop");
    qputenv("SDL_VIDEO_WAYLAND_WMCLASS", "com.oneplay_stream.OnePlay");
    qputenv("SDL_VIDEO_X11_WMCLASS", "com.oneplay_stream.OnePlay");

    // Register our C++ types for QML
    qmlRegisterType<ComputerModel>("ComputerModel", 1, 0, "ComputerModel");
    qmlRegisterType<AppModel>("AppModel", 1, 0, "AppModel");
    qmlRegisterUncreatableType<Session>("Session", 1, 0, "Session", "Session cannot be created from QML");
    qmlRegisterSingletonType<ComputerManager>("ComputerManager", 1, 0,
                                              "ComputerManager",
                                              [](QQmlEngine*, QJSEngine*) -> QObject* {
                                                  return new ComputerManager();
                                              });
    qmlRegisterSingletonType<AutoUpdateChecker>("AutoUpdateChecker", 1, 0,
                                                "AutoUpdateChecker",
                                                [](QQmlEngine*, QJSEngine*) -> QObject* {
                                                    return new AutoUpdateChecker();
                                                });
    qmlRegisterSingletonType<SystemProperties>("SystemProperties", 1, 0,
                                               "SystemProperties",
                                               [](QQmlEngine*, QJSEngine*) -> QObject* {
                                                   return new SystemProperties();
                                               });
    qmlRegisterSingletonType<SdlGamepadKeyNavigation>("SdlGamepadKeyNavigation", 1, 0,
                                                      "SdlGamepadKeyNavigation",
                                                      [](QQmlEngine*, QJSEngine*) -> QObject* {
                                                          return new SdlGamepadKeyNavigation();
                                                      });
    qmlRegisterSingletonType<StreamingPreferences>("StreamingPreferences", 1, 0,
                                                   "StreamingPreferences",
                                                   [](QQmlEngine* qmlEngine, QJSEngine*) -> QObject* {
                                                       return new StreamingPreferences(qmlEngine);
                                                   });

    // Create the identity manager on the main thread
    IdentityManager::get();

#ifndef Q_OS_WINRT
    // Use the dense material dark theme by default
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_STYLE")) {
        qputenv("QT_QUICK_CONTROLS_STYLE", "Material");
    }
#else
    // Use universal dark on WinRT
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_STYLE")) {
        qputenv("QT_QUICK_CONTROLS_STYLE", "Universal");
    }
#endif
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_MATERIAL_THEME")) {
        qputenv("QT_QUICK_CONTROLS_MATERIAL_THEME", "Dark");
    }
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_MATERIAL_ACCENT")) {
        qputenv("QT_QUICK_CONTROLS_MATERIAL_ACCENT", "Purple");
    }
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_MATERIAL_VARIANT")) {
        qputenv("QT_QUICK_CONTROLS_MATERIAL_VARIANT", "Dense");
    }
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_UNIVERSAL_THEME")) {
        qputenv("QT_QUICK_CONTROLS_UNIVERSAL_THEME", "Dark");
    }

    QQmlApplicationEngine engine;
    QString initialView;
    GlobalCommandLineParser parser;
    switch (parser.parse(app.arguments())) {
    case GlobalCommandLineParser::NormalStartRequested:
        initialView = "qrc:/gui/PcView.qml";
        break;
    case GlobalCommandLineParser::StreamRequested:
        {
            initialView = "qrc:/gui/CliStartStreamSegue.qml";
            StreamingPreferences* preferences = new StreamingPreferences(&app);
            StreamCommandLineParser streamParser;
            streamParser.parse(app.arguments(), preferences);
            QString host    = streamParser.getHost();
            QString appName = streamParser.getAppName();
            auto launcher   = new CliStartStream::Launcher(host, appName, preferences, &app);
            engine.rootContext()->setContextProperty("launcher", launcher);
            break;
        }
    case GlobalCommandLineParser::QuitRequested:
        {
            initialView = "qrc:/gui/CliQuitStreamSegue.qml";
            QuitCommandLineParser quitParser;
            quitParser.parse(app.arguments());
            auto launcher = new CliQuitStream::Launcher(quitParser.getHost(), &app);
            engine.rootContext()->setContextProperty("launcher", launcher);
            break;
        }
     case GlobalCommandLineParser::BackendStreamRequested:
        {
            initialView = "qrc:/gui/CliStartStreamSegue.qml";
            StreamingPreferences* preferences = new StreamingPreferences(&app);

            // obtain backend object
            BackendCommandLineParser backendCliParser;
            backendCliParser.parse(app.arguments());
            QString sessionKey = backendCliParser.getSessionKey();

#ifdef USE_STUBS_INSTEAD_OF_BACKEND
            std::shared_ptr<OnePlayBackend> onePlayBackend;
            if (sessionKey == "12345") {
                onePlayBackend = std::make_shared<OnePlayBackend>(std::make_shared<OnePlayBackendAPIStubs>(sessionKey));
            } else {
                onePlayBackend = std::make_shared<OnePlayBackend>(std::make_shared<OnePlayBackendAPI>(sessionKey));
            }
#else
            std::shared_ptr<OnePlayBackend> onePlayBackend = std::make_shared<OnePlayBackend>(std::make_shared<OnePlayBackendAPI>(sessionKey));
#endif

            // get stream preferences
            onePlayBackend->getStreamingPreferences(preferences);
            QString host    = onePlayBackend->getHost();
            // always use "Desktop", as it is pre-defined on server side and actual app will be dictated via backend.
            QString appName = "Desktop";

            auto launcher   = new CliStartStream::Launcher(host, appName, preferences, &app, onePlayBackend);
            engine.rootContext()->setContextProperty("launcher", launcher);
            break;
        }
    case GlobalCommandLineParser::CustomUri:
       {
           initialView = "qrc:/gui/CliStartStreamSegue.qml";
           StreamingPreferences* preferences = new StreamingPreferences(&app);

           // obtain backend object
           CustomUriParser backendCliParser;
           backendCliParser.parse(app.arguments());
           QString sessionKey = backendCliParser.getSessionKey();

#ifdef USE_STUBS_INSTEAD_OF_BACKEND
           std::shared_ptr<OnePlayBackend> onePlayBackend;
           if (sessionKey == "12345") {
               onePlayBackend = std::make_shared<OnePlayBackend>(std::make_shared<OnePlayBackendAPIStubs>(sessionKey));
           } else if (sessionKey == "12345-local") {
               onePlayBackend = std::make_shared<OnePlayBackend>(std::make_shared<OnePlayBackendAPILocalDev>(sessionKey));
           } else {
               onePlayBackend = std::make_shared<OnePlayBackend>(std::make_shared<OnePlayBackendAPI>(sessionKey));
           }
#else
           std::shared_ptr<OnePlayBackend> onePlayBackend = std::make_shared<OnePlayBackend>(std::make_shared<OnePlayBackendAPI>(sessionKey));
#endif
           // usage of Exiter here is safe as we always have some session key here
           static Exiter exiter(onePlayBackend);
           QObject::connect(&app, SIGNAL(aboutToQuit()), &exiter, SLOT(request_exit()));

           // get stream preferences
           onePlayBackend->getStreamingPreferences(preferences);
           QString host    = onePlayBackend->getHost();
           // always use "Desktop", as it is pre-defined on server side and actual app will be dictated via backend.
           QString appName = "Desktop";

           auto launcher   = new CliStartStream::Launcher(host, appName, preferences, &app, onePlayBackend);
           engine.rootContext()->setContextProperty("launcher", launcher);
           break;
       }
       case GlobalCommandLineParser::NoKeyQuitImmidiately:
       {
#ifdef Q_OS_MACX
           initialView = "qrc:/gui/CliStartStreamSegue.qml";
           StreamingPreferences* preferences = new StreamingPreferences(&app);
           QString host    = "TBD";
           // always use "Desktop", as it is pre-defined on server side and actual app will be dictated via backend.
           QString appName = "Desktop";

           auto launcher   = new CliStartStream::Launcher(host, appName, preferences, &app, nullptr, appEventFilter);
           engine.rootContext()->setContextProperty("launcher", launcher);
           break;
#else
           exit(0);
#endif
       }
    }


    engine.rootContext()->setContextProperty("initialView", initialView);

    // Load the main.qml file
    engine.load(QUrl(QStringLiteral("qrc:/gui/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    int err = app.exec();

    // Give worker tasks time to properly exit. Fixes PendingQuitTask
    // sometimes freezing and blocking process exit.
    QThreadPool::globalInstance()->waitForDone(30000);

    return err;
}