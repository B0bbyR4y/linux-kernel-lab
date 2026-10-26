# Stufe 0 – Entwicklungsumgebung

## Ziel und Einordnung

Windows dient als Arbeitsplatz, Ubuntu unter WSL2 als Build-Umgebung und ein Linux-Gast in QEMU als Testziel. Die Rollen erläutert das [Nachschlagewerk](../../docs/Linux_Kernel_und_Embedded_Linux_Nachschlagewerk.md). Ablage- und Befehlsregeln stehen in der [Repository-Struktur](../../docs/REPOSITORY_STRUCTURE.md).

Diese Anleitung enthält zunächst die vollständige Bestandsaufnahme. Installation, Gast-Image und QEMU-Start werden anhand der ermittelten Architektur und Versionen ergänzt. Sie enthält noch keinen ausführbaren QEMU-Aufbau. Der eigene Kernel-Build folgt in Stufe 2.

Die folgenden Abfragen können bereits vor dem Klonen des Repositorys ausgeführt werden. Sie benötigen keine Administratorrechte und verändern keine Konfiguration. Bei fehlendem Ubuntu oder WSL 1 zunächst diesen Befund klären; Installations- oder Konvertierungsbefehle folgen dann passend zur Umgebung.

## Windows und WSL erfassen

**Umgebung:** Windows · **Shell:** PowerShell · **Arbeitsverzeichnis:** beliebig

**Voraussetzung:** Zugriff auf den Windows-Rechner. **Ergebnis:** Terminalausgabe; keine Dateien. **Versionierung:** keine.

Eine normale **Windows-PowerShell** öffnen. Administratorrechte sind für diese Abfragen nicht erforderlich. Die Befehle einzeln ausführen:

```powershell
Get-CimInstance Win32_OperatingSystem | Select-Object Caption, Version, BuildNumber, OSArchitecture
Get-CimInstance Win32_ComputerSystem | Select-Object NumberOfLogicalProcessors, @{Name='RAM_GiB';Expression={[math]::Round($_.TotalPhysicalMemory / 1GB, 1)}}
wsl --version
wsl --status
wsl --list --verbose
```

`wsl --version` nennt die Versionen der WSL-Komponenten. Die Spalte `VERSION` in `wsl --list --verbose` bezeichnet dagegen WSL 1 oder WSL 2 für die jeweilige Distribution. Für unsere Entwicklungsumgebung soll dort bei Ubuntu **2** stehen. `Stopped` ist kein Fehler: Die Distribution läuft dann gerade nicht. Bei älteren WSL-Installationen kann `--version` unbekannt sein; die Fehlermeldung und die übrigen Ausgaben helfen bei der Einordnung.

Ist Ubuntu nicht aufgelistet oder steht dort WSL 1, zuerst diesen Befund klären. Eine vorhandene Distribution wird nicht anhand eines angenommenen Namens konvertiert oder ersetzt.

## Die richtige Ubuntu-Shell öffnen

**Umgebung:** Windows · **Shell:** PowerShell · **Arbeitsverzeichnis:** beliebig

**Voraussetzung:** Name einer vorhandenen Ubuntu-Distribution aus Schritt 1. **Ergebnis:** geöffnete WSL-Shell; keine Projektdateien. **Versionierung:** keine.

Ubuntu über das Windows-Terminal-Profil öffnen oder in PowerShell den exakten Distributionsnamen aus der vorherigen Ausgabe verwenden. Beispiel nur für eine tatsächlich so benannte Distribution:

```powershell
wsl --distribution Ubuntu-24.04
```

Ab hier werden die Linux-Befehle **in Ubuntu/WSL2** ausgeführt. Ein PowerShell-Prompt und ein Linux-Shell-Prompt sind unterschiedliche Befehlsumgebungen.

## Ubuntu und verfügbare Ressourcen prüfen

**Umgebung:** Ubuntu/WSL2 · **Shell:** Bash · **Arbeitsverzeichnis:** beliebig

**Voraussetzung:** Ubuntu-Shell aus Schritt 2. **Ergebnis:** Terminalausgabe; keine Dateien. **Versionierung:** keine.

```bash
cat /etc/os-release
uname -r
uname -m
id -u
nproc
free -h
df -h "$HOME"
```

| Abfrage | Interpretation |
| --- | --- |
| `/etc/os-release` | Distribution und Release bestimmen die verfügbaren Pakete. |
| `uname -r` | Aktuell laufender Kernel; hier der WSL2-Kernel. |
| `uname -m` | Architektur; `x86_64` für Intel/AMD-64-Bit, `aarch64` für ARM64. |
| `id -u` | Benutzer-ID; `0` bedeutet root. Für die normale Entwicklungsarbeit verwenden wir einen gewöhnlichen Benutzer. |
| `nproc` | Für den Prozess verfügbare Verarbeitungseinheiten; später Grundlage für Build-Parallelität. |
| `free -h` | Linux-seitig sichtbarer Arbeitsspeicher; kann vom physischen Windows-RAM abweichen. |
| `df -h "$HOME"` | Freier Platz auf dem Linux-Dateisystem. Bei WSL zusätzlich freien Platz auf dem Windows-Laufwerk mit der Distributionsdatei prüfen, da deren virtuelle Platte wachsen kann. |

Quellen und Build-Verzeichnisse sollen im Linux-Dateisystem liegen, beispielsweise unter `~/linux-kernel-lab`. Linux-Werkzeuge profitieren dort gegenüber einem Projekt unter `/mnt/c/...` von besserer Dateisystemleistung. Wir legen die Struktur im Installationsschritt gemeinsam an.

## Vorhandene Werkzeuge und KVM erfassen

**Umgebung:** Ubuntu/WSL2 · **Shell:** Bash · **Arbeitsverzeichnis:** beliebig

**Voraussetzung:** Ubuntu-Shell. **Ergebnis:** Werkzeugpfade und KVM-Zugriffsstatus; keine Dateien. **Versionierung:** keine.

Dieser Block prüft lediglich, welche Programme aufrufbar sind. Fehlende Programme werden im nächsten Schritt gezielt installiert.

```bash
for tool in git gcc make gdb python3 curl qemu-system-x86_64 qemu-system-aarch64 qemu-img; do
    if command -v "$tool" >/dev/null 2>&1; then
        printf '%-24s %s\n' "$tool" "$(command -v "$tool")"
    else
        printf '%-24s %s\n' "$tool" 'nicht installiert / nicht im PATH'
    fi
done

if [ -e /dev/kvm ]; then
    ls -l /dev/kvm
    if [ -r /dev/kvm ] && [ -w /dev/kvm ]; then
        printf '%s\n' 'KVM-Gerät für aktuellen Benutzer les- und schreibbar; Funktionstest folgt.'
    else
        printf '%s\n' 'KVM-Gerät vorhanden, aber Zugriffsrechte fehlen.'
    fi
else
    printf '%s\n' 'Kein /dev/kvm vorhanden; QEMU mit TCG bleibt möglich.'
fi
```

`gcc` übersetzt C-Code, `make` steuert Build-Abhängigkeiten, `gdb` dient zum Debuggen und `git` verwaltet Quelltext. `qemu-system-*` emuliert eine komplette Maschine; `qemu-img` verwaltet virtuelle Datenträger. QEMU-User-Mode zum Ausführen einzelner fremder Programme wäre für das Booten eines eigenen Kernels nicht ausreichend. Es muss später nur die benötigte Systemarchitektur installiert werden.

## Ergebnisse auswerten

Die Ausgaben der Windows-/WSL-Abfragen sowie der Ressourcen-, Werkzeug- und KVM-Prüfung bilden die Grundlage für die Installation. Zusätzlich den freien Platz auf dem Windows-Laufwerk mit der WSL-Distribution feststellen. Eine fehlende QEMU-Installation oder fehlendes KVM ist bei dieser Bestandsaufnahme kein Fehler.

Ausgaben können für die gemeinsame Auswertung als Text bereitgestellt werden. Individuelle Rechnerdaten und komplette Laufzeitprotokolle gehören nicht in das öffentliche Repository. Spätere automatisch erzeugte Berichte liegen lokal unter `build/reports/`.

## Weitere Einrichtung und Abschlusskriterien

Nach der Bestandsaufnahme folgen Werkzeuginstallation, ein geprüftes Gast-Image, ein dokumentierter QEMU-Start, Zugriff auf die Gast-Shell und Dateiaustausch. Stufe 0 ist abgeschlossen, wenn der Gast wiederholt bootet, seine Kernel-Version und Bootmeldungen sichtbar sind, Dateien übertragen werden können und Herunterfahren sowie Wiederherstellung eines definierten Ausgangszustands funktionieren.

Ein erfolgreicher Aufruf von `uname -r` in WSL bestätigt nur den WSL-Kernel. Der spätere gleiche Aufruf im Gast bestätigt den Gastkernel. Die konkrete Gast-Shell und deren Arbeitsverzeichnis werden in jedem entsprechenden Schritt separat angegeben.
