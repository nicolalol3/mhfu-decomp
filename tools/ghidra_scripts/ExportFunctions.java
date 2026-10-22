// Esporta l'elenco di tutte le funzioni (indirizzo, dimensione, nome, chiamate) in TSV.
// Uso: -postScript ExportFunctions.java <file_output.tsv>
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.util.*;

public class ExportFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        String out = getScriptArgs()[0];
        try (PrintWriter w = new PrintWriter(new FileWriter(out))) {
            w.println("addr\tsize\tname\tcallees\tcallers\tthunk");
            for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
                AddressSetView body = f.getBody();
                Address entry = f.getEntryPoint();
                // dimensione = range contiguo che parte dall'entry point
                AddressRange r = body.getRangeContaining(entry);
                long size = r == null ? 0 : r.getLength();
                StringBuilder cs = new StringBuilder();
                for (Function c : f.getCalledFunctions(monitor)) {
                    if (cs.length() > 0) cs.append(',');
                    cs.append(c.getEntryPoint().toString());
                }
                int callers = f.getCallingFunctions(monitor).size();
                w.printf("%s\t%d\t%s\t%s\t%d\t%b%n", entry, size, f.getName(), cs, callers, f.isThunk());
            }
        }
        println("Esportate " + currentProgram.getFunctionManager().getFunctionCount() + " funzioni");
    }
}
