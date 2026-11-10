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

**Offen:** Vergleich privilegierter/unprivilegierter Prozessattribute und Untersuchung der Schutzgrenzen; abschließende Auswertung und Abnahme von 1.1. Kein vorzeitiger Abschlussstatus.

Referenzen: [proc_pid_status(5)](https://man7.org/linux/man-pages/man5/proc_pid_status.5.html), [proc_pid_maps(5)](https://man7.org/linux/man-pages/man5/proc_pid_maps.5.html), [Kernel-Dokumentation zu procfs](https://docs.kernel.org/filesystems/proc.html).
