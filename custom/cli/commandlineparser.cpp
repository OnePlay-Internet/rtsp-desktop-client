#include "commandlineparser.h"

#include <QCommandLineParser>
#include <QRegularExpression>

#if defined(Q_OS_WIN)
#include <qt_windows.h>
#endif

static bool inRange(int value, int min, int max)
{
    return value >= min && value <= max;
}

static std::vector<std::string> split (std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find (delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

// This method returns key's value from QMap where the key is a QString.
// Key matching is case insensitive.
template <typename T>
static T mapValue(QMap<QString, T> map, QString key)
{
    for(auto& item : map.toStdMap()) {
        if (QString::compare(item.first, key, Qt::CaseInsensitive) == 0) {
            return item.second;
        }
    }
    return T();
}

class CommandLineParser : public QCommandLineParser
{
public:
    enum MessageType {
        Info,
        Error
    };

    void setupCommonOptions()
    {
        setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
        addHelpOption();
        addVersionOption();
    }

    void handleHelpAndVersionOptions()
    {
        if (isSet("help")) {
            showInfo(helpText());
        }
        if (isSet("version")) {
            showVersion();
        }
    }

    void handleUnknownOptions()
    {
        if (!unknownOptionNames().isEmpty()) {
            showError(QString("Unknown options: %1").arg(unknownOptionNames().join(", ")));
        }
    }

    void showMessage(QString message, MessageType type) const
    {
    #if defined(Q_OS_WIN32)
        UINT flags = MB_OK | MB_TOPMOST | MB_SETFOREGROUND;
        flags |= (type == Info ? MB_ICONINFORMATION : MB_ICONERROR);
        QString title = "Moonlight";
        MessageBoxW(nullptr, reinterpret_cast<const wchar_t *>(message.utf16()),
                    reinterpret_cast<const wchar_t *>(title.utf16()), flags);
    #endif
        message = message.endsWith('\n') ? message : message + '\n';
        fputs(qPrintable(message), type == Info ? stdout : stderr);
    }

    [[ noreturn ]] void showInfo(QString message) const
    {
        showMessage(message, Info);
        exit(0);
    }

    [[ noreturn ]] void showError(QString message) const
    {
        showMessage(message + "\n\n" + helpText(), Error);
        exit(1);
    }

    int getIntOption(QString name) const
    {
        bool ok;
        int intValue = value(name).toInt(&ok);
        if (!ok) {
            showError(QString("Invalid %1 value: %2").arg(name, value(name)));
        }
        return intValue;
    }

    bool getToggleOptionValue(QString name, bool defaultValue) const
    {
        QRegularExpression re(QString("^(%1|no-%1)$").arg(name));
        QStringList options = optionNames().filter(re);
        if (options.isEmpty()) {
            return defaultValue;
        } else {
            return options.last() == name;
        }
    }

    QString getChoiceOptionValue(QString name) const
    {
        if (!m_Choices[name].contains(value(name), Qt::CaseInsensitive)) {
            showError(QString("Invalid %1 choice: %2").arg(name, value(name)));
        }
        return value(name);
    }

    QPair<int,int> getResolutionOptionValue(QString name) const
    {
        QRegularExpression re("^(\\d+)x(\\d+)$", QRegularExpression::CaseInsensitiveOption);
        auto match = re.match(value(name));
        if (!match.hasMatch()) {
            showError(QString("Invalid %1 format: %2").arg(name, value(name)));
        }
        return qMakePair(match.captured(1).toInt(), match.captured(2).toInt());
    }

    void addFlagOption(QString name, QString descriptiveName)
    {
        addOption(QCommandLineOption(name, QString("Use %1.").arg(descriptiveName)));
    }

    void addToggleOption(QString name, QString descriptiveName)
    {
        addOption(QCommandLineOption(name, QString("Use %1.").arg(descriptiveName)));
        addOption(QCommandLineOption("no-" + name, QString("Do not use %1.").arg(descriptiveName)));
    }

    void addValueOption(QString name, QString descriptiveName)
    {
        addOption(QCommandLineOption(name, QString("Specify %1 to use.").arg(descriptiveName), name));
    }

    void addChoiceOption(QString name, QString descriptiveName, QStringList choices)
    {
        addOption(QCommandLineOption(name, QString("Select %1: %2.").arg(descriptiveName, choices.join('/')), name));
        m_Choices[name] = choices;
    }

private:
    QMap<QString, QStringList> m_Choices;
};

GlobalCommandLineParser::GlobalCommandLineParser()
{
}

GlobalCommandLineParser::~GlobalCommandLineParser()
{
}

GlobalCommandLineParser::ParseResult GlobalCommandLineParser::parse(const QStringList &args)
{
    CommandLineParser parser;
    parser.setupCommonOptions();
    parser.setApplicationDescription(
        "\n"
        "Starts OnePlay normally if no arguments are given.\n"
        "\n"
        "Available actions:\n"
        "  key <key>          Starts streaming an app using session key\n"
        "  oneplay:key?<key>  Starts streaming an app using session key\n"
        "\n"
        "See 'oneplay <action> --help' for help of specific action."
    );
    parser.addPositionalArgument("action", "Action to execute", "<action>");
    parser.parse(args);
    auto posArgs = parser.positionalArguments();


    if (posArgs.isEmpty()) {
        // This method will not return and terminates the process if --version
        // or --help is specified
        parser.handleHelpAndVersionOptions();
        parser.handleUnknownOptions();
        return ErrorParsing;
    } else if (posArgs.size() == 1) {
        QString arg = posArgs.at(0).toLower();
        //SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "cli posArg - %s\n", posArgs.at(0).toStdString().c_str());
        std::string delimiter = "?";
        std::vector<std::string> v = split(arg.toStdString(), delimiter);
        if (v[0].compare("oneplay:key") == 0) {
            return StreamRequested;
        } else {
            return ErrorParsing;
        }
    } else {
        return ErrorParsing;
    }
}



StreamCommandLineParser::StreamCommandLineParser()
{
    m_WindowModeMap = {
        {"fullscreen", StreamingPreferences::WM_FULLSCREEN},
        {"windowed",   StreamingPreferences::WM_WINDOWED},
        {"borderless", StreamingPreferences::WM_FULLSCREEN_DESKTOP},
    };
    m_AudioConfigMap = {
        {"stereo",       StreamingPreferences::AC_STEREO},
        {"5.1-surround", StreamingPreferences::AC_51_SURROUND},
        {"7.1-surround", StreamingPreferences::AC_71_SURROUND},
    };
    m_VideoCodecMap = {
        {"auto",  StreamingPreferences::VCC_AUTO},
        {"H.264", StreamingPreferences::VCC_FORCE_H264},
        {"HEVC",  StreamingPreferences::VCC_FORCE_HEVC},
    };
    m_VideoDecoderMap = {
        {"auto",     StreamingPreferences::VDS_AUTO},
        {"software", StreamingPreferences::VDS_FORCE_SOFTWARE},
        {"hardware", StreamingPreferences::VDS_FORCE_HARDWARE},
    };
    m_CaptureSysKeysModeMap = {
        {"never",      StreamingPreferences::CSK_OFF},
        {"fullscreen", StreamingPreferences::CSK_FULLSCREEN},
        {"always",     StreamingPreferences::CSK_ALWAYS},
    };
}

StreamCommandLineParser::~StreamCommandLineParser()
{
}

void StreamCommandLineParser::parse(const QStringList &args, StreamingPreferences *preferences)
{
    CommandLineParser parser;
    parser.setupCommonOptions();
    parser.setApplicationDescription(
        "\n"
        "Starts directly streaming a given app."
    );
    parser.addPositionalArgument("stream", "Start stream");

    // Add other arguments and options
    parser.addPositionalArgument("host", "Host computer name, UUID, or IP address", "<host>");
    parser.addPositionalArgument("app", "App to stream", "\"<app>\"");

    parser.addFlagOption("720",  "1280x720 resolution");
    parser.addFlagOption("1080", "1920x1080 resolution");
    parser.addFlagOption("1440", "2560x1440 resolution");
    parser.addFlagOption("4K", "3840x2160 resolution");
    parser.addValueOption("resolution", "custom <width>x<height> resolution");
    parser.addToggleOption("vsync", "V-Sync");
    parser.addValueOption("fps", "FPS");
    parser.addValueOption("bitrate", "bitrate in Kbps");
    parser.addValueOption("packet-size", "video packet size");
    parser.addChoiceOption("display-mode", "display mode", m_WindowModeMap.keys());
    parser.addChoiceOption("audio-config", "audio config", m_AudioConfigMap.keys());
    parser.addToggleOption("multi-controller", "multiple controller support");
    parser.addToggleOption("quit-after", "quit app after session");
    parser.addToggleOption("absolute-mouse", "remote desktop optimized mouse control");
    parser.addToggleOption("mouse-buttons-swap", "left and right mouse buttons swap");
    parser.addToggleOption("touchscreen-trackpad", "touchscreen in trackpad mode");
    parser.addToggleOption("game-optimization", "game optimizations");
    parser.addToggleOption("audio-on-host", "audio on host PC");
    parser.addToggleOption("frame-pacing", "frame pacing");
    parser.addToggleOption("mute-on-focus-loss", "mute audio when Moonlight window loses focus");
    parser.addToggleOption("background-gamepad", "background gamepad input");
    parser.addToggleOption("reverse-scroll-direction", "inverted scroll direction");
    parser.addToggleOption("swap-gamepad-buttons", "swap A/B and X/Y gamepad buttons (Nintendo-style)");
    parser.addToggleOption("keep-awake", "prevent display sleep while streaming");
    parser.addChoiceOption("capture-system-keys", "capture system key combos", m_CaptureSysKeysModeMap.keys());
    parser.addChoiceOption("video-codec", "video codec", m_VideoCodecMap.keys());
    parser.addChoiceOption("video-decoder", "video decoder", m_VideoDecoderMap.keys());

    if (!parser.parse(args)) {
        parser.showError(parser.errorText());
    }

    parser.handleUnknownOptions();

    // Resolve display's width and height
    QRegularExpression resolutionRexExp("^(720|1080|1440|4K|resolution)$");
    QStringList resoOptions = parser.optionNames().filter(resolutionRexExp);
    bool displaySet = !resoOptions.isEmpty();
    if (displaySet) {
        QString name = resoOptions.last();
        if (name == "720") {
            preferences->width  = 1280;
            preferences->height = 720;
        } else if (name == "1080") {
            preferences->width  = 1920;
            preferences->height = 1080;
        } else if (name == "1440") {
            preferences->width  = 2560;
            preferences->height = 1440;
        } else if (name == "4K") {
            preferences->width  = 3840;
            preferences->height = 2160;
        } else if (name == "resolution") {
            auto resolution = parser.getResolutionOptionValue(name);
            preferences->width  = resolution.first;
            preferences->height = resolution.second;
        }
    }

    // Resolve --fps option
    if (parser.isSet("fps")) {
        preferences->fps = parser.getIntOption("fps");
        if (!inRange(preferences->fps, 30, 240)) {
            parser.showError("FPS must be in range: 30 - 240");
        }
    }

    // Resolve --bitrate option
    if (parser.isSet("bitrate")) {
        preferences->bitrateKbps = parser.getIntOption("bitrate");
        if (!inRange(preferences->bitrateKbps, 500, 150000)) {
            parser.showError("Bitrate must be in range: 500 - 150000");
        }
    } else if (displaySet || parser.isSet("fps")) {
        preferences->bitrateKbps = preferences->getDefaultBitrate(
            preferences->width, preferences->height, preferences->fps);
    }

    // Resolve --packet-size option
    if (parser.isSet("packet-size")) {
        preferences->packetSize = parser.getIntOption("packet-size");
        if (preferences->packetSize < 1024) {
            parser.showError("Packet size must be greater than 1024 bytes");
        }
    }

    // Resolve --display option
    if (parser.isSet("display-mode")) {
        preferences->windowMode = mapValue(m_WindowModeMap, parser.getChoiceOptionValue("display-mode"));
    }

    // Resolve --vsync and --no-vsync options
    preferences->enableVsync = parser.getToggleOptionValue("vsync", preferences->enableVsync);

    // Resolve --audio-config option
    if (parser.isSet("audio-config")) {
        preferences->audioConfig = mapValue(m_AudioConfigMap, parser.getChoiceOptionValue("audio-config"));
    }

    // Resolve --multi-controller and --no-multi-controller options
    preferences->multiController = parser.getToggleOptionValue("multi-controller", preferences->multiController);

    // Resolve --quit-after and --no-quit-after options
    preferences->quitAppAfter = parser.getToggleOptionValue("quit-after", preferences->quitAppAfter);

    // Resolve --absolute-mouse and --no-absolute-mouse options
    preferences->absoluteMouseMode = parser.getToggleOptionValue("absolute-mouse", preferences->absoluteMouseMode);

    // Resolve --mouse-buttons-swap and --no-mouse-buttons-swap options
    preferences->swapMouseButtons = parser.getToggleOptionValue("mouse-buttons-swap", preferences->swapMouseButtons);

    // Resolve --touchscreen-trackpad and --no-touchscreen-trackpad options
    preferences->absoluteTouchMode = !parser.getToggleOptionValue("touchscreen-trackpad", !preferences->absoluteTouchMode);

    // Resolve --game-optimization and --no-game-optimization options
    preferences->gameOptimizations = parser.getToggleOptionValue("game-optimization", preferences->gameOptimizations);

    // Resolve --audio-on-host and --no-audio-on-host options
    preferences->playAudioOnHost = parser.getToggleOptionValue("audio-on-host", preferences->playAudioOnHost);

    // Resolve --frame-pacing and --no-frame-pacing options
    preferences->framePacing = parser.getToggleOptionValue("frame-pacing", preferences->framePacing);

    // Resolve --mute-on-focus-loss and --no-mute-on-focus-loss options
    preferences->muteOnFocusLoss = parser.getToggleOptionValue("mute-on-focus-loss", preferences->muteOnFocusLoss);

    // Resolve --background-gamepad and --no-background-gamepad options
    preferences->backgroundGamepad = parser.getToggleOptionValue("background-gamepad", preferences->backgroundGamepad);

    // Resolve --reverse-scroll-direction and --no-reverse-scroll-direction options
    preferences->reverseScrollDirection = parser.getToggleOptionValue("reverse-scroll-direction", preferences->reverseScrollDirection);

    // Resolve --swap-gamepad-buttons and --no-swap-gamepad-buttons options
    preferences->swapFaceButtons = parser.getToggleOptionValue("swap-gamepad-buttons", preferences->swapFaceButtons);

    // Resolve --keep-awake and --no-keep-awake options
    preferences->keepAwake = parser.getToggleOptionValue("keep-awake", preferences->keepAwake);

    // Resolve --capture-system-keys option
    if (parser.isSet("capture-system-keys")) {
        preferences->captureSysKeysMode = mapValue(m_CaptureSysKeysModeMap, parser.getChoiceOptionValue("capture-system-keys"));
    }

    // Resolve --video-codec option
    if (parser.isSet("video-codec")) {
        preferences->videoCodecConfig = mapValue(m_VideoCodecMap, parser.getChoiceOptionValue("video-codec"));
    }

    // Resolve --video-decoder option
    if (parser.isSet("video-decoder")) {
        preferences->videoDecoderSelection = mapValue(m_VideoDecoderMap, parser.getChoiceOptionValue("video-decoder"));
    }

    // This method will not return and terminates the process if --version or
    // --help is specified
    parser.handleHelpAndVersionOptions();

    // Verify that both host and app has been provided
    auto posArgs = parser.positionalArguments();
    if (posArgs.length() < 2) {
        parser.showError("Host not provided");
    }
    std::string url = parser.positionalArguments().at(1);
    std::vector<std::string> v = split(url.toStdString(), "?");
    m_Token = QString(v[1]);
}


QString StreamCommandLineParser::getToken() const
{
    return m_Token;
}
