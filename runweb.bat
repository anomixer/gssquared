@echo off
rem Serve the last completed WASM build. Run buildweb.bat explicitly when
rem source or assets have changed.
python assets/web/update_shell.py build-web
if errorlevel 1 (
  echo Could not update the generated web shell; run buildweb.bat first.
  exit /b 1
)
python assets/web/serve.py 8000 build-web
