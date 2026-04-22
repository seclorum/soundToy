@ECHO OFF
SET OSDK=C:\OSDK
CALL osdk_config.bat
IF "%OSDK%"=="" GOTO ErCfg
CALL %OSDK%\SETOSDK.BAT
CALL %OSDK%\bin\make.bat %OSDKFILE%
GOTO End
:ErCfg
ECHO == ERROR ==
ECHO The Oric SDK was not configured properly
ECHO You should have a OSDK environment variable setted to the location of the SDK
ECHO OSDK="%OSDK%"
IF "%OSDKBRIEF%"=="" PAUSE
GOTO End

:End
