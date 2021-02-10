set OutDir=%1

if not exist %OutDir%\..\..\Contents mkdir %OutDir%\..\..\Contents
if errorlevel 1 goto err
if not exist %OutDir%\..\..\Contents\Resources mkdir %OutDir%\..\..\Contents\Resources
if errorlevel 1 goto err

echo Copy "aaxwrapperPages.xml"
copy /Y ..\resource\aaxwrapperPages.xml %OutDir%\..\..\Contents\Resources\ > NUL
if errorlevel 1 goto err

attrib -r %OutDir%\..\..

if exist %OutDir%\..\..\PlugIn.ico goto PlugIn_ico_exists
copy /Y ..\resource\PlugIn.ico %OutDir%\..\..\ > NUL
if errorlevel 1 goto err
attrib +h +r +s %OutDir%\..\..\PlugIn.ico
if errorlevel 1 goto err
:PlugIn_ico_exists

if exist %OutDir%\..\..\desktop.ini goto desktop_ini_exists
copy /Y ..\resource\desktop.ini %OutDir%\..\..\ > NUL
if errorlevel 1 goto err
attrib +h +r +s %OutDir%\..\..\desktop.ini
if errorlevel 1 goto err
:desktop_ini_exists

attrib +r %OutDir%\..\..

:err
