rmdir /Q /S .\base-source
mkdir .\base-source
mkdir .\base-source\app
mkdir .\base-source\AntiHooking
robocopy third-party\moonlight\app .\base-source\app /e /xf *

FOR /R "third-party\moonlight\" %%x IN (AntiHooking\*.h) DO xcopy "%%x" "base-source/AntiHooking" /Y /I

FOR /R "third-party\moonlight\" %%x IN (app\*.h,app\*.cpp) DO xcopy "%%x" "base-source/app" /Y /I


@REM streaming directory
FOR /R "third-party\moonlight\" %%x IN (app\streaming\*.h,app\streaming\*.cpp) DO xcopy "%%x" "base-source/app/streaming" /Y /I
FOR /R "third-party\moonlight\" %%x IN (app\streaming\audio\*.h,app\streaming\audio\*.cpp) DO xcopy "%%x" "base-source/app/streaming\audio" /Y /I
FOR /R "third-party\moonlight\" %%x IN (app\streaming\audio\renderers\*.h,app\streaming\audio\renderers\*.cpp) DO xcopy "%%x" "base-source/app/streaming\audio\renderers" /Y /I

FOR /R "third-party\moonlight\" %%x IN (app\streaming\video\*.h,app\streaming\video\*.cpp) DO xcopy "%%x" "base-source/app/streaming\video" /Y /I
FOR /R "third-party\moonlight\" %%x IN (app\streaming\video\ffmpeg-renderers\*.h,app\streaming\video\ffmpeg-renderers\*.cpp) DO xcopy "%%x" "base-source/app/streaming\video\ffmpeg-renderers" /Y /I
FOR /R "third-party\moonlight\" %%x IN (app\streaming\video\ffmpeg-renderers\pacer\*.h,app\streaming\video\ffmpeg-renderers\pacer\*.cpp) DO xcopy "%%x" "base-source/app/streaming\video\ffmpeg-renderers\pacer" /Y /I

FOR /R "third-party\moonlight\" %%x IN (app\streaming\input\*.h,app\streaming\input\*.cpp) DO xcopy "%%x" "base-source/app/streaming\input" /Y /I
@REM

@REM shaders directory
FOR /R "third-party\moonlight\" %%x IN (shaders\*.vert,shaders\*.frag,shaders\*.hlsl,shaders\*.hlsi,shaders\*.fxc,shaders\*.bat) DO xcopy "%%x" "base-source/app/shaders" /Y /I
@REM


@REM gui directory
FOR /R "third-party\moonlight\" %%x IN (app\gui\*.qml,app\gui\*.h,app\gui\*.cpp) DO xcopy "%%x" "base-source/app/gui" /Y /I


@REM cli settings backend directory
FOR /R "third-party\moonlight\" %%x IN (app\settings\*.h,app\settings\*.cpp) DO xcopy "%%x" "base-source/app/settings" /Y /I
FOR /R "third-party\moonlight\" %%x IN (app\backend\*.h,app\backend\*.cpp) DO xcopy "%%x" "base-source/app/backend" /Y /I
FOR /R "third-party\moonlight\" %%x IN (app\cli\*.h,app\cli\*.cpp) DO xcopy "%%x" "base-source/app/cli" /Y /I