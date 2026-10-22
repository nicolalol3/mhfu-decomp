# Decompilazione di Monster Hunter Freedom Unite (PSP, EU, ULES01213)

Obiettivo: riscrivere in C il codice del gioco, funzione per funzione, in modo che sia
facile da modificare. Non serve che il codice compilato sia identico byte per byte
all'originale: deve solo **comportarsi** allo stesso modo.

## Come funziona

1. Il gioco originale (EBOOT decifrato) resta la base.
2. Ogni funzione riscritta in C (`src/`) viene compilata e caricata in memoria subito dopo
   il gioco; all'inizio della funzione originale viene messo un salto verso la versione in C.
3. Così il gioco gira con un misto di codice originale e codice C, e la parte in C cresce
   un po' alla volta.

## Cartelle

| Percorso | Contenuto |
|---|---|
| `src/` | il codice C decompilato (`libc/` = libreria C, `libm/` = matematica) |
| `include/` | header comuni |
| `symbols.txt` | nomi leggibili di funzioni e variabili del gioco (indirizzo → nome) |
| `tools/` | strumenti: compilazione, iniezione, test |
| `build/` | file generati (EBOOT modificato, screenshot dei test, ...) |
| `../work/` | analisi Ghidra, EBOOT originale decifrato, disco estratto per i test |

## Verifica di ogni funzione

- **Banco di prova** (`tools/difftest.py`): esegue la funzione originale e quella in C su
  una CPU MIPS emulata con centinaia di input casuali (compresi valori estremi) e confronta
  risultato, memoria scritta e chiamate al sistema operativo.
- **Test di avvio** (`tools/boottest.sh`): avvia il gioco in PPSSPP con l'EBOOT modificato
  e salva uno screenshot in `build/screenshot.png`.

## Comandi (li esegue Claude)

```
wsl.exe -d Ubuntu -- bash tools/build.sh                 # compila e crea build/EBOOT.BIN
tools/.venv/Scripts/python.exe -I tools/difftest.py      # banco di prova
tools/boottest.sh 25                                     # test di avvio in PPSSPP
python tools/progress.py                                 # percentuale completata
```

## Stato

Vedi `python tools/progress.py`. La percentuale si riferisce al solo EBOOT (1.089.080 byte
di codice); il gioco carica altro codice (overlay) da `DATA.BIN`, non ancora contato.
