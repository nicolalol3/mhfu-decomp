#!/bin/bash
# Boot test: puts build/EBOOT.BIN on the test disc, starts a separate PPSSPP instance
# (test settings via --appendconfig, the user's configuration is not touched),
# waits N seconds, saves the screenshot to build/screenshot.png and closes the instance.
# Usage (Git Bash): tools/boottest.sh [seconds]
cd "$(dirname "$0")/.."
WAIT=${1:-20}
cp build/EBOOT.BIN ../work/disc/PSP_GAME/SYSDIR/EBOOT.BIN
cmd.exe //c start "" "C:\Program Files\PPSSPP\PPSSPPWindows64.exe" --windowed \
  "--appendconfig=C:\Users\main\Desktop\FUPC\decomp\tools\ppsspp_test.ini" \
  "C:\Users\main\Desktop\FUPC\work\disc"
sleep "$WAIT"
PID=$(netstat -ano | grep LISTENING | grep ":45677 " | head -1 | awk '{print $5}')
if [ -z "$PID" ]; then echo "PPSSPP is not responding"; exit 1; fi
node tools/ppsspp_debug.js 45677 '{"event":"game.status"}' '{"event":"cpu.status"}'
powershell -NoProfile -ExecutionPolicy Bypass -File tools/winshot.ps1 -ProcessId "$PID" -Out "C:\Users\main\Desktop\FUPC\decomp\build\screenshot.png"
taskkill //PID "$PID" //F > /dev/null
