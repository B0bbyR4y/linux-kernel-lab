# Stufe 1 – Linux-Systemarchitektur und Grundlagen

Dieses Lab untersucht den bereits qualifizierten Alpine-/QEMU-Gast. Der [Stufe-1-Plan](../../docs/Linux_Kernel_und_Embedded_Linux_Nachschlagewerk.md#stufe-1-linux-systemarchitektur-und-grundlagen) sowie allgemeine Konzepte stehen im [Nachschlagewerk](../../docs/Linux_Kernel_und_Embedded_Linux_Nachschlagewerk.md). Für bereits qualifizierte VM- und Rollback-Verfahren gilt [Stufe 0](../00-environment/README.md#standardbetrieb-nach-stufe-0).

## 1.1 – Systemarchitektur und Schutzgrenzen

**Status: in Bearbeitung.** Start-Preflight und erste lesende Shell-Prozess-Inspektion sind erfolgreich durchgeführt. Eine Gegenüberstellung der Prozessidentitäten und eine Prüfung der Schutzgrenzen folgen.

### Experiment 1.1-A: Kernel und Prozessadressraum

**Ziel:** Kernelversion, Prozessattribute und virtuelle Speicherabbildungen mit rein lesenden Befehlen ermitteln.

**Schritt A1 – Host-Preflight.** Umgebung: Ubuntu/WSL2 · Shell: Bash · Arbeitsverzeichnis: `~/linux-kernel-lab`. Voraussetzungen: qualifizierte Images und Hashmanifest aus Stufe 0, keine laufende QEMU-Instanz. Erwartet: Basis-Hash `OK`, zweistufige QCOW2-Backing-Chain und `PASS`. Projektdateien: keine.

```bash
cd "$HOME/linux-kernel-lab" || exit 1

(
    set -euo pipefail
    git status --short --branch
    git log -1 --format='%h %s'
    if pgrep -af '[q]emu-system-x86_64'; then
        echo "ABBRUCH: QEMU laeuft bereits."
        exit 1
    fi
    sha256sum -c build/reports/stage0/alpine-base.sha256
    qemu-img info --backing-chain build/images/alpine-work.qcow2
    echo "PASS: Start-Preflight."
)
```

**Gaststart.** Umgebung: Ubuntu/WSL2 · Bash · Arbeitsverzeichnis: `~/linux-kernel-lab`. Nur nach bestandenem Preflight; QEMU beschreibt ausschließlich das Arbeits-Overlay. Der Gaststart ist bereits Bestandteil des qualifizierten Stufe-0-Standardbetriebs.

```bash
cd "$HOME/linux-kernel-lab" || exit 1

qemu-system-x86_64 \
    -machine q35 \
    -accel kvm \
    -m 512M \
    -smp 1 \
    -boot c \
    -drive file="$HOME/linux-kernel-lab/build/images/alpine-work.qcow2",format=qcow2,if=virtio \
    -nic user,model=virtio-net-pci \
    -nographic
```

**Schritt A2 – Gast-Inspektion.** Umgebung: Alpine/QEMU · Shell: BusyBox ash · Arbeitsverzeichnis: beliebig. Voraussetzung: serieller Login als `root`. Erwartet: Gastkernel `6.18.58-0-virt` auf `x86_64`, gültige Prozessattribute und Mappings für Code, Heap, Stack und vDSO. Keine erzeugten Testdateien; keine Git-Änderungen.

```sh
printf '\n=== 1. Laufender Kernel ===\n'
uname -srm

printf '\n=== 2. Benutzeridentitaet ===\n'
id

printf '\n=== 3. Aktueller Shell-Prozess ===\n'
printf 'Shell-PID: %s\n' "$$"

grep -E \
    '^(Name|State|Pid|PPid|Uid|Gid|VmSize|VmRSS|Threads|CapEff):' \
    "/proc/$$/status"

printf '\n=== 4. Virtuelle Speicherbereiche ===\n'
head -n 12 "/proc/$$/maps"

printf '\n=== 5. Spezielle Speicherbereiche ===\n'
grep -E '\[(stack|heap|vdso|vvar)\]' \
    "/proc/$$/maps" || true
```

**Validierung 10.11.2026: PASS (A1/A2).**

| Prüfung | Tatsächlicher Befund |
| --- | --- |
| Git vor Start | Sauberer lokaler Branch `main`, lokaler HEAD `259cb9d`; auf GitHub lag bereits der neuere Dokumentationscommit |
| Basis / Overlay | Basis-SHA-256 `OK`; aktive Backing-Chain Overlay → Basis (beide `corrupt: false`); keine QEMU-Instanz vor Start |
| Boot | KVM-Gast mit OpenRC 0.63.2 und serieller Anmeldung über `ttyS0` erfolgreich gestartet |
| Kernel | `Linux 6.18.58-0-virt x86_64` |
| Shell | `sh`, PID 2165, PPID 2163, `S (sleeping)`, 1 Thread |
| Credentials | UID und GID jeweils viermal 0; `CapEff=000001ffffffffff` |
| Speicher | `VmSize=1728 kB`; `VmRSS=1308 kB` |
| Mappings | BusyBox mit `r--p`/`r-xp`/`rw-p`; Heap `---p`/`rw-p`; Stack `rw-p`; `[vdso]`, `[vvar]`, `[vvar_vclock]`; musl-Loader |

**Grenzen der Aussage:** Der Shell-Zustand `S` ist plausibel, weil sie auf den Kindprozess `grep` wartet. Die Werte sind Momentaufnahmen. `VmRSS` ist keine exakte Angabe exklusiv belegten RAMs. Die unter `/proc/PID/maps` aufgeführten Speicherregionen und Berechtigungen belegen weder den aktuellen CPU-Ring noch die praktische Wirkung einer verbotenen Speicheroperation. Der `---p`-Heap-Bereich ist nicht zugreifbar, sein konkreter Zweck wurde nicht nachgewiesen. Das Muster des letzten `grep` erfasst `[vvar_vclock]` nicht.

**Offen:** Experimente A, B und C3 fachlich erfolgreich; kontrollierter Unmount/Shutdown, Host-Integritätscheck und Bereinigung sind noch nachzuweisen. Noch kein Abschlussstatus.



### Experiment 1.1-B: Root- und Lab-Prozess vergleichen

**Ziel:** Im bestehenden Alpine-Gast Unterschiede zwischen Root- und unprivilegierten User-Space-Prozessen in Credentials, Capabilities, Prozesshierarchie, Dateiberechtigungen und Speicherabbildungen sichtbar machen. Die Untersuchung ist lesend und erstellt keine Dateien. Sie beweist noch keinen hardwareseitigen MMU-Schutzverstoß.

**Schritt B1 – Root-Shell.** Umgebung: Alpine/QEMU · Shell: BusyBox ash · Benutzer: `root` · Arbeitsverzeichnis: beliebig. Voraussetzungen: laufende und angemeldete Gast-VM; lokaler Benutzer `lab` aus Stufe 0 vorhanden. Erwartet: UID 0, effektive Capabilities, lesbare `/etc/shadow` aufgrund Dateieigentümers und Rechte, `lab` als lokaler Benutzer vorhanden.

```sh
printf '\n=== Root-Identitaet ===\n'
id

printf '\n=== Root-Prozessattribute ===\n'
printf 'Shell-PID: %s\n' "$$"
grep -E \
    '^(Name|State|Pid|PPid|Uid|Gid|CapEff|VmSize|VmRSS|Threads):' \
    "/proc/$$/status"

printf '\n=== Root-Speicherabbildungen ===\n'
head -n 5 "/proc/$$/maps"

printf '\n=== Dateiberechtigungstest ===\n'
ls -l /etc/shadow
if [ -r /etc/shadow ]; then
    echo "Root: Datei lesbar"
else
    echo "Root: Datei nicht lesbar"
fi

printf '\n=== Benutzer lab vorhanden? ===\n'
id lab
```

**Schritt B2 – Lab-Shell.** Umgebung: Alpine/QEMU · Shell: BusyBox ash · Benutzer: `lab` nach `su - lab` · Arbeitsverzeichnis: Benutzer-Home. Voraussetzung: erfolgreicher Schritt B1 und existierender Benutzer `lab`. Erwartet: nichtzero UID/GID, keine effektiven Capabilities, kein Lesezugriff auf `/etc/shadow` und eigene Speichermappings. Die Shell wird danach mit `exit` verlassen.

```sh
su - lab
```

In der nun gestarteten `lab`-Shell:

```sh
printf '\n=== Lab-Identitaet ===\n'
id

printf '\n=== Lab-Prozessattribute ===\n'
printf 'Shell-PID: %s\n' "$$"
grep -E \
    '^(Name|State|Pid|PPid|Uid|Gid|CapEff|VmSize|VmRSS|Threads):' \
    "/proc/$$/status"

printf '\n=== Lab-Speicherabbildungen ===\n'
head -n 5 "/proc/$$/maps"

printf '\n=== Dateiberechtigungstest ===\n'
if [ -r /etc/shadow ]; then
    echo "Lab: Datei lesbar"
else
    echo "Lab: Zugriff nicht erlaubt"
fi
```

Zur Root-Shell zurückkehren:

```sh
exit
```

**Validierung B1/B2 (11.11.2026): PASS.**

| Beobachtung | Root-Shell | Lab-Shell |
| --- | --- | --- |
| Prozess | PID `2132`, PPID `2130` | PID `2152`, PPID `2132` |
| UID/GID | viermal `0` / viermal `0` | viermal `1000` / viermal `1000` |
| Zustand/Threads | `S (sleeping)` / 1 | `S (sleeping)` / 1 |
| `CapEff` | `000001ffffffffff` | `0000000000000000` |
| `VmSize` / `VmRSS` | 1728 / 1380 kB | 1720 / 1276 kB |
| Ausführbares BusyBox-`r-xp`-Mapping | `55c30be50000-55c30bee9000` | `563fd30f3000-563fd318c000` |
| `/etc/shadow` lesbar nach Shell-Test | Ja | Nein |

`/etc/shadow` besaß `-rw-r-----` (`0640`) mit Eigentümer `root` und Gruppe `shadow`. `root` kann die Datei bereits aufgrund der Eigentümer-Leseberechtigung lesen; das Experiment **beweist keine Verwendung von** `CAP_DAC_OVERRIDE`. Der Benutzer `lab` ist nicht Mitglied der Gruppe `shadow` und besitzt kein sonstiges Leserecht. Der beobachtete Shell-Test `[ -r /etc/shadow ]` prüft die Zugriffsmöglichkeit, nicht den Inhalt der Passwortdatei. Es wurde kein Dateiinhalt ausgelesen.

**Interpretation:** Die unterschiedliche PPID/PID-Beziehung bestätigt die Lab-Shell als Kind der Root-Shell. Beide verwenden dieselbe BusyBox-Datei mit denselben Abbildungs-Offsets/Berechtigungen, aber unterschiedlichen virtuellen Startadressen. Dies ist mit getrennten Adressräumen und ASLR vereinbar; die Beobachtung für sich beweist weder vollständige Prozessisolation noch einen MMU-Fault. Der Unterschied bei `VmSize`/`VmRSS` ist nur eine Momentaufnahme, kein Beleg für generellen Mehrverbrauch privilegierter Prozesse. Die beiden UID/Credential-Profile begründen auch keinen CPU-Ringwechsel: Normaler Shell-Code läuft weiterhin als User-Space-Code.

**Nächste Prüfung:** Technisch kontrollierte Leseoperation gegen eine ausdrücklich unzugängliche Speicherabbildung (etwa `mmap` mit `PROT_NONE`) in einem isolierten User-Space-Testprozess und Prüfung, ob `SIGSEGV` auftritt, ohne andere Prozesse zu beeinträchtigen. Werkzeugverfügbarkeit vorher prüfen.

Zusätzliche Referenzen: [Linux capabilities(7)](https://man7.org/linux/man-pages/man7/capabilities.7.html), [mmap(2)](https://man7.org/linux/man-pages/man2/mmap.2.html), [mprotect(2)](https://man7.org/linux/man-pages/man2/mprotect.2.html).



### Experiment 1.1-C: Speicherschutz und Prozessisolation

**Status: C1, C2 und C3 einschließlich Target-Lauf erfolgreich validiert; kontrollierter Shutdown, Nachprüfung und Bereinigung stehen noch aus.**

**Ziel:** In einem kontrollierten User-Space-Experiment mit `mmap()` und `mprotect(PROT_NONE)` den Schutz einer Speicherseite untersuchen. Der absichtlich ungültige Zugriff wird später ausschließlich von einem Kindprozess ausgeführt; der Elternprozess soll das Ergebnis mit `waitpid()` und `WIFSIGNALED`/`WTERMSIG` auswerten. Ein erwartetes `SIGSEGV` soll weder den Elternprozess noch den Kernel beenden.

**Schritt C1 – Ist-Zustand im Gast.** Umgebung: Alpine/QEMU · Shell: BusyBox ash · Benutzer: `root` · Arbeitsverzeichnis: beliebig. Voraussetzung: laufender Gast. Erwartet: Architektur und vorhandene Compiler werden ermittelt, Core-Dump-Limit und Testbenutzer geprüft. Dateien und Git-Änderungen: keine.

```sh
printf '\n=== Architektur ===\n'
uname -m

printf '\n=== C-Compiler ===\n'
for tool in cc gcc clang; do
    if command -v "$tool" >/dev/null 2>&1; then
        printf '%s: %s\n' "$tool" "$(command -v "$tool")"
    else
        printf '%s: nicht vorhanden\n' "$tool"
    fi
done

printf '\n=== Core-Dump-Limit ===\n'
ulimit -c

printf '\n=== Testbenutzer ===\n'
id lab
```

**Validierung C1 (11.11.2026): PASS als Werkzeug-Vorprüfung.**

| Prüfaspekt | Tatsächliche Ausgabe / Bewertung |
| --- | --- |
| CPU-Architektur | `x86_64` – passend zur geplanten Host-Zielarchitektur |
| `cc`, `gcc`, `clang` | Alle drei nicht vorhanden; auf diesem schlanken Gast erwartbar und kein Fehler |
| `ulimit -c` | `0`; keine Core-Dateien unter diesem Limit, aber `SIGSEGV` bleibt möglich |
| Benutzer `lab` | Vorhanden; UID/GID 1000 und bekannte zusätzlichen Gruppen |

**Build-Entscheidung:** Kein Compilerpaket im Alpine-Arbeits-Overlay installieren. Zunächst Ubuntu/WSL2-Host-Toolchain prüfen und den kleinen Test auf dem Host möglichst statisch für `x86_64` linken, damit keine unpassende dynamische glibc-/musl-Loader-Abhängigkeit entsteht. Das fertige ELF wird kontrolliert über einen **lesenden** VirtIO-9P-Export bereitgestellt und im QEMU-Gast ausgeführt. Der derzeit dokumentierte QEMU-Standardstart bindet noch keinen 9P-Export ein; eine dafür nötige Gast-Neustartsequenz muss separat geplant und validiert werden. Host-Quellcode liegt später unter `labs/01-system-architecture/src/`, Buildprodukte unter dem ignorierten `build/`.

**Noch nicht im Alpine-Gast validiert:** Transfer über VirtIO-9P, `PROT_NONE`/`SIGSEGV`-Test, Elternprozess-Auswertung und Bereinigung.



**Schritt C2 – statische Host-Toolchain qualifizieren.** Umgebung: Ubuntu 26.04 / WSL2 · Shell: Bash · Arbeitsverzeichnis: `~/linux-kernel-lab`. Voraussetzungen: GCC, `file` und `readelf`; vorhandenes ignoriertes `build/`. Erwartet: statisch gelinktes x86_64-ELF ohne `PT_INTERP`, erfolgreich ausführbar. Temporäres Verzeichnis wird mittels Shell-`trap` bereinigt; keine Git-Änderungen.

```bash
cd "$HOME/linux-kernel-lab" || exit 1

(
    set -euo pipefail

    echo "=== 1. Host-Architektur ==="
    uname -m

    echo "=== 2. Build-Werkzeuge ==="
    command -v gcc
    command -v file
    command -v readelf
    gcc -dumpmachine
    gcc --version | head -n 1

    echo "=== 3. Statischer Link-Test ==="
    TMP="$(mktemp -d "$PWD/build/.static-preflight.XXXXXXXX")"
    trap 'rm -f -- "$TMP/probe.c" "$TMP/probe"; rmdir -- "$TMP"' EXIT
    cat > "$TMP/probe.c" <<'EOF'
#include <stdio.h>
int main(void)
{
    puts("PASS: Statisches Testprogramm ausgefuehrt.");
    return 0;
}
EOF
    gcc -O2 -Wall -Wextra -Werror -static \
        -o "$TMP/probe" "$TMP/probe.c"

    echo "=== 4. ELF-Analyse ==="
    file "$TMP/probe"
    if readelf -l "$TMP/probe" | grep -q 'INTERP'; then
        echo "FAIL: Dynamischer ELF-Interpreter vorhanden."
        exit 1
    fi
    echo "PASS: Kein dynamischer ELF-Interpreter."

    echo "=== 5. Ausfuehrung ==="
    "$TMP/probe"
    echo "PASS: Host-Toolchain qualifiziert."
)
```

**Validierung C2 (11.11.2026): PASS für die Host-Toolchain.** `uname -m` = `x86_64`; `/usr/bin/gcc`, `/usr/bin/file` und `/usr/bin/readelf` vorhanden; Target `x86_64-linux-gnu`, GCC `15.2.0` (Ubuntu-Paket `15.2.0-16ubuntu1`). `gcc -static` erstellte ein ELF64-x86-64-Executable, `readelf -l` zeigte keinen `INTERP`-Eintrag, der Build war warning-free unter `-Wall -Wextra -Werror`. Die Testausgabe war `PASS: Statisches Testprogramm ausgefuehrt.`, gefolgt von `PASS: Host-Toolchain qualifiziert.`. Der QEMU-Gast wurde zuvor geordnet mit `poweroff` beendet. Dies belegt **noch nicht** die Ausführbarkeit im Alpine-Gast. Der temporäre Preflight-Ordner wurde vom Skript zur Entfernung vorgesehen; ein nachträglicher Dateisystem-Check wurde nicht vorgelegt.

**Schritt C3 – Testprogramm auf Host bauen und kontrolliert im Gast ausführen (noch offen).**

Die C-Quelle liegt unter [`src/memory_protection.c`](src/memory_protection.c). Sie reserviert eine anonyme Seite, beschreibt sie und führt `fork()` aus. Nur der **Kindprozess** ruft `mprotect(PROT_NONE)` auf und führt anschließend einen mittels `volatile` erzwungenen Lesezugriff aus. Der Elternprozess erwartet per `waitpid()`/`WIFSIGNALED`/`WTERMSIG` das Signal `SIGSEGV`, liest seine eigene weiterhin zugängliche Seite und gibt nur dann `PASS` zurück. Das Kind unterbindet Core-Dumps mit `setrlimit(RLIMIT_CORE,0)`. Für einen unerwarteten Verlauf liefern Kind beziehungsweise Elternprozess einen Fehlercode. Nur als **User-Space-Programm** ausgeführt; kein eigener Kernel-Code.

**Host-Synchronisation.** Umgebung: Ubuntu/WSL2 · Bash · Arbeitsverzeichnis: Repository-Wurzel. Voraussetzung: Gast heruntergefahren, lokaler Branch `main`, sauberer Git-Arbeitsbaum. Keine eigenen Änderungen überschreiben; Fast-Forward nur unter diesen Bedingungen.

```bash
cd "$HOME/linux-kernel-lab" || exit 1
(
    set -euo pipefail
    [ "$(git branch --show-current)" = "main" ]
    [ -z "$(git status --porcelain=v1)" ] || {
        echo "ABBRUCH: ungesicherte lokale Änderungen."
        exit 1
    }
    git fetch origin
    git merge --ff-only origin/main
    test -f labs/01-system-architecture/src/memory_protection.c
    git status --short --branch
    git log -1 --format='%h %s'
)
```

**Build im getrennten Host-Export.** Umgebung: Ubuntu/WSL2 · Bash · Arbeitsverzeichnis: Repository-Wurzel. Voraussetzung: erfolgreicher Fast-Forward, `gcc`/`file`/`readelf` aus C2. Erzeugte Datei: ausschließlich `build/share/stage1-memory/memory_protection` (über `.gitignore` ignoriert). Erwartet: statisches x86_64-Executable ohne dynamischen Interpreter.

```bash
cd "$HOME/linux-kernel-lab" || exit 1
(
    set -euo pipefail
    mkdir -p build/share/stage1-memory
    gcc -std=c11 -O2 -Wall -Wextra -Werror -static \
        -o build/share/stage1-memory/memory_protection \
        labs/01-system-architecture/src/memory_protection.c
    file build/share/stage1-memory/memory_protection
    if readelf -l build/share/stage1-memory/memory_protection | grep -q INTERP; then
        echo "FAIL: ELF enthaelt einen dynamischen Interpreter."
        exit 1
    fi
    sha256sum build/share/stage1-memory/memory_protection
    echo "PASS: Statisches Target-ELF erstellt."
)
```

**Gast-Start-Preflight und Boot.** Umgebung: Ubuntu/WSL2 · Bash · Arbeitsverzeichnis: Repository-Wurzel. Voraussetzungen: QEMU vollständig gestoppt; das Binary erstellt; Basis-Manifest verfügbar. Exportiert **nur das Testverzeichnis** und ausschließlich lesend. QEMU verwendet weiterhin nur das Arbeits-Overlay, nicht das Basisimage. Erwartet: Alpine-Start mit Login über `ttyS0`.

```bash
cd "$HOME/linux-kernel-lab" || exit 1
(
    set -euo pipefail
    if pgrep -af '[q]emu-system-x86_64'; then
        echo "ABBRUCH: QEMU laeuft."
        exit 1
    fi
    test -x build/share/stage1-memory/memory_protection
    sha256sum -c build/reports/stage0/alpine-base.sha256
    qemu-img info --backing-chain build/images/alpine-work.qcow2
    echo "PASS: QEMU-Preflight."
)
```

Nur bei `PASS`:

```bash
cd "$HOME/linux-kernel-lab" || exit 1
qemu-system-x86_64 \
    -machine q35 \
    -accel kvm \
    -m 512M \
    -smp 1 \
    -boot c \
    -drive file="$HOME/linux-kernel-lab/build/images/alpine-work.qcow2",format=qcow2,if=virtio \
    -nic user,model=virtio-net-pci \
    -virtfs local,path="$HOME/linux-kernel-lab/build/share/stage1-memory",mount_tag=labshare,security_model=none,readonly=on \
    -nographic
```

**Mount und Target-Lauf.** Umgebung: Alpine/QEMU · Shell: ash · Benutzer: `root` · Arbeitsverzeichnis: beliebig. Erwartet: 9P-Export als `ro`, Kopie in ein temporäres Gastverzeichnis, Kindprozess stirbt durch `SIGSEGV`, Elternprozess läuft unverändert weiter und Gesamttest gibt Exitcode `0` zurück. Der Binärinhalt wird nicht vom Gast in den Host-Export zurückgeschrieben.

```sh
mkdir -p /mnt/labshare
mount -t 9p -o trans=virtio,version=9p2000.L,ro labshare /mnt/labshare
mount | grep labshare
sha256sum /mnt/labshare/memory_protection

(
    set -eu
    WORK="$(mktemp -d /tmp/stage1-memory.XXXXXXXX)"
    trap 'rm -f -- "$WORK/memory_protection"; rmdir -- "$WORK"' EXIT
    cp /mnt/labshare/memory_protection "$WORK/memory_protection"
    chmod 0700 "$WORK/memory_protection"
    "$WORK/memory_protection"
    echo "PASS: Target-Ausfuehrung erfolgreich."
)
```

Nach erfolgreicher Auswertung den 9P-Mount mit `umount /mnt/labshare` lösen und den Gast regulär mit `poweroff` herunterfahren. **Bis zur gemeinsamen Auswertung bleiben Binary und Host-Export erhalten**; anschließend kontrollierte Bereinigung unter `build/share/stage1-memory/`, keine zusätzlichen Artefakte behalten.

**Erwartete Programmmeldungen:** `PARENT: ... initial=0x5a`, `CHILD: ... inherited=0x5a`, `CHILD: PROT_NONE gesetzt...`, `PASS: Kind wurde durch SIGSEGV beendet (Speicherschutz aktiv)`, `PASS: Elternprozess liest weiterhin 0x5a`, `PASS: Gesamttest abgeschlossen`. PIDs und virtuelle Adressen variieren. Ein beobachteter `SIGSEGV` bestätigt einen Schutzverstoß im User Space bei weiterhin funktionsfähigem Elternprozess; er demonstriert nicht die direkte CPU-Ringnummer oder vollständige Isolation gegen sämtliche Angriffe.

**Validierungsstatus:** C1, C2 und C3 wurden erfolgreich ausgeführt (siehe protokollierten Target-Befund unten). Vor endgültiger Abnahme fehlen noch kontrollierter Unmount/Shutdown, Host-Nachprüfung und Bereinigung.


**Tatsächlich bestätigte Target-Validierung C3 (11.11.2026): PASS.**

| Prüfschritt | Beobachtetes Ergebnis | Bewertung |
| --- | --- | --- |
| C3.1 Git | Fast-Forward `259cb9d..211e612`, sauberer Branch `main...origin/main` | PASS |
| C3.2 Compiler | GCC 15.2.0, `-std=c11 -O2 -Wall -Wextra -Werror -static` ohne Warnungen; ELF64 x86_64 statisch, kein `PT_INTERP` | PASS |
| C3.2 Binär-Hash | `91e4252be620101138d9992344c8bd1c397de2865f0742d5050fd52a64cb676c` | Erfasst |
| C3.3 QEMU-Preflight | Basis-SHA-256 `OK`, Overlay → Basis, beide `corrupt: false`; keine QEMU-Instanz zuvor | PASS |
| C3.3 Start | KVM-Boot, Alpine 3.24.x, Kernel `6.18.58-0-virt`, serieller Root-Login | PASS |
| C3.4 Mount | `labshare on /mnt/labshare type 9p (ro,relatime,access=client,trans=virtio)` | PASS |
| C3.4 Transferintegrität | SHA-256 im Gast identisch zum Host | PASS |
| C3.4 Kindprozess | Eltern-PID 2164, Kind-PID 2165, 4096 Byte, Startwert `0x5a`, Kind nach `PROT_NONE` durch `SIGSEGV` terminiert | PASS |
| C3.4 Elternprozess | `waitpid()`-Status erfolgreich geprüft, eigener Lesezugriff weiterhin `0x5a` | PASS |
| C3.4 Gesamttest | `PASS: Gesamttest abgeschlossen` und `PASS: Target-Ausfuehrung erfolgreich.` | PASS |

Der Prozess wurde als `root` gestartet: Dateisystemrechte beziehungsweise Capabilities ersetzen nicht die CPU/MMU-Seitenschutzprüfung. `fork()` erzeugte einen getrennten Prozessadressraum; `mprotect()` änderte ausschließlich die Zugriffsrechte im Kind. Der verbotene Lesezugriff führte zu einer vom Kernel behandelten Ausnahme und zur `SIGSEGV`-Signalzustellung an das Kind. Dies ist ein erfolgreicher Test der Speicherrechte und der Fehlerbegrenzung **in diesem konkreten Fall**, kein genereller Beweis vollständiger Prozessisolation oder eine direkte Messung des CPU-Rings.

**Noch nicht vom Benutzer bestätigt:** `umount /mnt/labshare`, `poweroff`, ausgeschalteter QEMU-Prozess, QCOW2-Prüfung nach der Session, sowie kontrollierte Entfernung des temporären Host-Binärartefakts unter `build/share/stage1-memory/`. Das Referenzbasis-Image und seine Sicherung dürfen nicht verändert oder zurückgesetzt werden.

Referenzen: [mprotect(2)](https://man7.org/linux/man-pages/man2/mprotect.2.html), [mmap(2)](https://man7.org/linux/man-pages/man2/mmap.2.html), [wait(2)](https://man7.org/linux/man-pages/man2/wait.2.html).

Referenzen: [proc_pid_status(5)](https://man7.org/linux/man-pages/man5/proc_pid_status.5.html), [proc_pid_maps(5)](https://man7.org/linux/man-pages/man5/proc_pid_maps.5.html), [Kernel-Dokumentation zu procfs](https://docs.kernel.org/filesystems/proc.html).
