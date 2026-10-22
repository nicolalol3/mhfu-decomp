#!/bin/bash
# Test di avvio: mette build/EBOOT.BIN nel disco di prova, avvia una istanza separata di PPSSPP
# (impostazioni di test via --appendconfig, la configurazione dell'utente non viene toccata),
# aspetta N secondi, salva lo screenshot in build/screenshot.png e chiude l'istanza.
# Uso (Git Bash): tools/boottest.sh [secondi]
cd "$(dirname "$0")/.."
WAIT=${1:-20}
cp build/EBOOT.BIN ../work/disc/PSP_GAME/SYSDIR/EBOOT.BIN
cmd.exe //c start "" "C:\Program Files\PPSSPP\PPSSPPWindows64.exe" --windowed \
  "--appendconfig=C:\Users\main\Desktop\FUPC\decomp\tools\ppsspp_test.ini" \
  "C:\Users\main\Desktop\FUPC\work\disc"
sleep "$WAIT"
PID=$(netstat -ano | grep LISTENING | grep ":45677 " | head -1 | awk '{print $5}')
if [ -z "$PID" ]; then echo "PPSSPP non risponde"; exit 1; fi
node tools/ppsspp_debug.js 45677 '{"event":"game.status"}' '{"event":"cpu.status"}'
powershell -NoProfile -ExecutionPolicy Bypass -File tools/winshot.ps1 -ProcessId "$PID" -Out "C:\Users\main\Desktop\FUPC\decomp\build\screenshot.png"
taskkill //PID "$PID" //F > /dev/null
