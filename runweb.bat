@echo off
rem Always rebuild before serving so embedded shell.html and WASM assets are current.
call buildweb.bat
if errorlevel 1 (
  echo Web build failed; server was not started.
  exit /b 1
)
python assets/web/serve.py 8000 build-web
