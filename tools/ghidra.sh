#!/bin/bash
# Launches Ghidra in automatic mode (no windows) on the MHFU project.
# Usage: tools/ghidra.sh <arguments for analyzeHeadless after the project name>
export JAVA_HOME="C:/Program Files/Eclipse Adoptium/jdk-25.0.4.101-hotspot"
export PATH="/c/Program Files/Eclipse Adoptium/jdk-25.0.4.101-hotspot/bin:$PATH"
# The Claude app virtualizes %APPDATA%: Ghidra must use folders outside AppData.
export JAVA_TOOL_OPTIONS="-Dapplication.settingsdir=C:\Tools\ghidra_settings -Dapplication.cachedir=C:\Tools\ghidra_cache"
ROOT="C:/Users/main/Desktop/FUPC"
MSYS_NO_PATHCONV=1 cmd.exe /c "C:\Tools\ghidra_12.2_PUBLIC\support\analyzeHeadless.bat" "$ROOT/work/ghidra" MHFU \
  -scriptPath "$ROOT/decomp/tools/ghidra_scripts" "$@" < /dev/null
