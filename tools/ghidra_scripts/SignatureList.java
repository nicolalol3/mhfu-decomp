// Writes, for each listed function, where its parameters are according to the decompiler
// (register or stack, and size in bytes) and whether it returns a float.
// Usage: -postScript SignatureList.java <output_file> <file_with_addresses (one per line)>
// Output line: address<TAB>return<TAB>reg:size,reg:size,...
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.pcode.*;
import java.io.*;
import java.nio.file.*;

public class SignatureList extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        DecompInterface d = new DecompInterface();
        d.openProgram(currentProgram);
        try (PrintWriter w = new PrintWriter(new FileWriter(args[0]))) {
            for (String a : Files.readAllLines(Paths.get(args[1]))) {
                a = a.trim();
                if (a.isEmpty()) continue;
                Function f = getFunctionAt(toAddr(a));
                if (f == null) { w.println(a + "\t?\t?"); continue; }
                DecompileResults res = d.decompileFunction(f, 60, monitor);
                if (!res.decompileCompleted()) { w.println(a + "\t?\t?"); continue; }
                HighFunction hf = res.getHighFunction();
                FunctionPrototype p = hf.getFunctionPrototype();
                StringBuilder sb = new StringBuilder();
                for (int i = 0; i < p.getNumParams(); i++) {
                    HighSymbol s = p.getParam(i);
                    VariableStorage st = s.getStorage();
                    String loc = st.isRegisterStorage() ? st.getRegister().getName()
                            : (st.isStackStorage() ? "S" + st.getStackOffset() : "?");
                    if (i > 0) sb.append(",");
                    sb.append(loc).append(":").append(s.getSize());
                }
                String ret = p.getReturnType().getName();
                VariableStorage rs = p.getReturnStorage();
                String rloc = (rs != null && rs.isRegisterStorage()) ? rs.getRegister().getName() : "-";
                w.println(a + "\t" + rloc + ":" + p.getReturnType().getLength() + "\t" + sb);
            }
        }
        d.dispose();
    }
}
