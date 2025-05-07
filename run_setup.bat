@echo off
REM 切换到脚本所在目录
cd /d %~dp0
REM 用默认 python 运行 setup.py
python setup.py
pause
