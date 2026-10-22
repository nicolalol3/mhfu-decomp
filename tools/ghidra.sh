#!/bin/bash
# Lancia Ghidra in modalità automatica (senza finestre) sul progetto MHFU.
# Uso: tools/ghidra.sh <argomenti per analyzeHeadless dopo il nome progetto>
export JAVA_HOME="C:/Program Files/Eclipse Adoptium/jdk-25.0.4.101-hotspot"
export PATH="/c/Program Files/Eclipse Adoptium/jdk-25.0.4.101-hotspot/bin:$PATH"
# L'app Claude virtualizza %APPDATA%: Ghidra deve usare cartelle fuori da AppData.
export JAVA_TOOL_OPTIONS="-Dapplication.settingsdir=C:\Tools\ghidra_settings -Dapplication.cachedir=C:\Tools\ghidra_cache"
ROOT="C:/Users/main/Desktop/FUPC"
MSYS_NO_PATHCONV=1 cmd.exe /c "C:\Tools\ghidra_12.2_PUBLIC\support\analyzeHeadless.bat" "$ROOT/work/ghidra" MHFU \
  -scriptPath "$ROOT/decomp/tools/ghidra_scripts" "$@" < /dev/null
