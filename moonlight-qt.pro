TEMPLATE = subdirs
SUBDIRS = \
    app \
    third-party/moonlight/moonlight-common-c \
    third-party/moonlight/qmdnsengine \
    third-party/moonlight/h264bitstream

# Build the dependencies in parallel before the final app
app.depends = third-party/moonlight/qmdnsengine \
              third-party/moonlight/moonlight-common-c \
              third-party/moonlight/h264bitstream
              
win32:!winrt {
    SUBDIRS += third-party/moonlight/AntiHooking
    app.depends += third-party/moonlight/AntiHooking
}
!winrt:win32|macx {
    SUBDIRS += third-party/moonlight/soundio
    app.depends += third-party/moonlight/soundio
}

# Support debug and release builds from command line for CI
CONFIG += debug_and_release

# Run our compile tests
load(configure)
qtCompileTest(SL)
qtCompileTest(EGL)
