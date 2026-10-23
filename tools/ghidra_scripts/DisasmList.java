// Writes the Ghidra disassembly of the listed functions into a folder (one .s file each).
// Alternative to tools/disasm.sh that does not need WSL.
// Usage: -postScript DisasmList.java <output_folder> <addr1+addr2+...>
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import java.io.*;

public class DisasmList extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File dir = new File(args[0]);
        dir.mkdirs();
        Listing listing = currentProgram.getListing();
        for (String a : args[1].split("[,+]")) {
            Function f = getFunctionAt(toAddr(a));
            if (f == null) { println("No function at " + a); continue; }
            try (PrintWriter w = new PrintWriter(new FileWriter(new File(dir, f.getEntryPoint().toString() + ".s")))) {
                InstructionIterator it = listing.getInstructions(f.getBody(), true);
                while (it.hasNext()) {
                    Instruction ins = it.next();
                    w.println(ins.getAddress().toString() + "\t" + ins.toString());
                }
            }
        }
    }
}
