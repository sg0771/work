where python >nul 2>nul
if %errorlevel% neq 0 (
    echo Python Not Install
    dir
) else (
    echo Python installed
    python %1 %2
)
dir

