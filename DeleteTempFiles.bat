@echo off
setlocal

echo Deleting bin, obj, Debug, Release, publish, x64, x86, and ARM64 folders...
echo Skipping vcpkg_installed completely...
echo.

call :ProcessFolder "%cd%"

echo.
echo Done!
pause
exit /b

:ProcessFolder
set "current=%~1"

for /d %%D in ("%current%\*") do (

    rem If folder is vcpkg_installed, skip entirely
    if /i "%%~nxD"=="vcpkg_installed" (
        echo Skipping folder: %%D
    ) else (

        rem If folder matches target names, delete it
        if /i "%%~nxD"=="bin" (
            echo Deleting folder: %%D
            rmdir /s /q "%%D"
        ) else if /i "%%~nxD"=="obj" (
            echo Deleting folder: %%D
            rmdir /s /q "%%D"
        ) else if /i "%%~nxD"=="Debug" (
            echo Deleting folder: %%D
            rmdir /s /q "%%D"
        ) else if /i "%%~nxD"=="Release" (
            echo Deleting folder: %%D
            rmdir /s /q "%%D"
        ) else if /i "%%~nxD"=="publish" (
            echo Deleting folder: %%D
            rmdir /s /q "%%D"
        ) else if /i "%%~nxD"=="x64" (
            echo Deleting folder: %%D
            rmdir /s /q "%%D"
        ) else if /i "%%~nxD"=="x86" (
            echo Deleting folder: %%D
            rmdir /s /q "%%D"
        ) else if /i "%%~nxD"=="ARM64" (
            echo Deleting folder: %%D
            rmdir /s /q "%%D"
        ) else (
            rem Recurse into subfolder
            call :ProcessFolder "%%D"
        )
    )
)

exit /b