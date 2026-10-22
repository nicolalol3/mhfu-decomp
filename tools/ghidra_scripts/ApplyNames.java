// Rinomina funzioni in Ghidra da un file TSV: indirizzo<TAB>nome (altre colonne ignorate;
// se ci sono 4 colonne il nome è nella quarta, formato build/imports.tsv).
// Uso: -postScript ApplyNames.java <file.tsv>
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;
import java.nio.file.*;

public class ApplyNames extends GhidraScript {
    @Override
    public void run() throws Exception {
        int n = 0;
        for (String line : Files.readAllLines(Paths.get(getScriptArgs()[0]))) {
            String[] p = line.split("\t");
            if (p.length < 2) continue;
            String name = p.length >= 4 ? p[3] : p[1];
            Function f = getFunctionAt(toAddr(p[0]));
            if (f == null) f = createFunction(toAddr(p[0]), name);
            if (f != null) { f.setName(name, SourceType.USER_DEFINED); n++; }
        }
        println("Nomi applicati: " + n);
    }
}
