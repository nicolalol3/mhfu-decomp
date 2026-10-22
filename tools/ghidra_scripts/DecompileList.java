// Decompila le funzioni indicate e scrive il C grezzo di Ghidra in una cartella.
// Uso: -postScript DecompileList.java <cartella_output> <addr1+addr2+...>
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.io.*;

public class DecompileList extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File dir = new File(args[0]);
        dir.mkdirs();
        DecompInterface d = new DecompInterface();
        d.openProgram(currentProgram);
        for (String a : args[1].split("[,+]")) {
            Function f = getFunctionAt(toAddr(a));
            if (f == null) { println("Nessuna funzione a " + a); continue; }
            DecompileResults res = d.decompileFunction(f, 120, monitor);
            String c = res.decompileCompleted() ? res.getDecompiledFunction().getC() : "// ERRORE: " + res.getErrorMessage();
            try (PrintWriter w = new PrintWriter(new FileWriter(new File(dir, f.getEntryPoint().toString() + ".c")))) {
                w.print(c);
            }
        }
        d.dispose();
    }
}
