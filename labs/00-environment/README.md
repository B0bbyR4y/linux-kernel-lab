# Stufe 0 – Entwicklungsumgebung

## Ziel und Einordnung

Windows dient als Arbeitsplatz, Ubuntu unter WSL2 als Build-Umgebung und ein Linux-Gast in QEMU als Testziel. Die Rollen erläutert das [Nachschlagewerk](../../docs/Linux_Kernel_und_Embedded_Linux_Nachschlagewerk.md). Ablage- und Befehlsregeln stehen in der [Repository-Struktur](../../docs/REPOSITORY_STRUCTURE.md).

Diese Anleitung enthält die Bestandsaufnahme und den Einstieg in die versionsabhängige Einrichtung der Werkzeuge. Installation, Gast-Image und QEMU-Start werden anhand der ermittelten Architektur und Versionen schrittweise ergänzt und validiert. Sie enthält noch keinen ausführbaren QEMU-Gaststart. Der eigene Kernel-Build folgt in Stufe 2.

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

id -nG
getent group kvm
```

Die beiden letzten Befehle zeigen die Gruppen des aktuellen Benutzers und den Eintrag der Gruppe `kvm`. Bei einem `/dev/kvm` mit Eigentümer `root`, Gruppe `kvm` und Modus `crw-rw----` benötigt QEMU normalerweise die Zugehörigkeit zur Gruppe `kvm`. Ein vorhandener Geräteknoten oder passende Rechte beweisen noch nicht, dass verschachtelte Hardwarevirtualisierung funktioniert. Dies prüft erst ein späterer QEMU/KVM-Start.

`gcc` übersetzt C-Code, `make` steuert Build-Abhängigkeiten, `gdb` dient zum Debuggen und `git` verwaltet Quelltext. `qemu-system-*` emuliert eine komplette Maschine; `qemu-img` verwaltet virtuelle Datenträger. QEMU-User-Mode zum Ausführen einzelner fremder Programme wäre für das Booten eines eigenen Kernels nicht ausreichend. Es muss später nur die benötigte Systemarchitektur installiert werden.

## Ergebnisse auswerten

Die Ausgaben der Windows-/WSL-Abfragen sowie der Ressourcen-, Werkzeug- und KVM-Prüfung bilden die Grundlage für die Installation. Zusätzlich den freien Platz auf dem Windows-Laufwerk mit der WSL-Distribution feststellen. Eine fehlende QEMU-Installation oder fehlendes KVM ist bei dieser Bestandsaufnahme kein Fehler.

**Umgebung:** Windows · **Shell:** PowerShell · **Arbeitsverzeichnis:** beliebig.

**Voraussetzungen:** Vorhandene, für den Benutzer registrierte WSL-Ubuntu-Distribution. **Ergebnis:** Registrierter Basispfad (zur Laufwerkszuordnung) und freie Kapazität der Windows-Dateisystemlaufwerke. **Erzeugte Dateien:** keine. **Versionierung:** keine.

```powershell
$ubuntu = Get-ChildItem 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Lxss' |
    Get-ItemProperty |
    Where-Object { $_.DistributionName -eq 'Ubuntu' }

$ubuntu | Select-Object DistributionName, BasePath

Get-PSDrive -PSProvider FileSystem |
    Select-Object Name,
        @{Name='Used_GiB';Expression={[math]::Round($_.Used / 1GB, 1)}},
        @{Name='Free_GiB';Expression={[math]::Round($_.Free / 1GB, 1)}}
```

Der Basispfad kann je nach Installation oder Import abweichen; bei Bedarf den tatsächlichen VHDX-Speicherort gesondert bestimmen. Die von `df -h` unter Ubuntu angezeigte Kapazität des virtuellen Dateisystems ist nicht gleich dem physisch freien Speicherplatz auf dem Windows-Laufwerk. Beide Grenzen sind für große Builds und Gast-Images relevant.

Ausgaben können für die gemeinsame Auswertung als Text bereitgestellt werden. Individuelle Rechnerdaten und komplette Laufzeitprotokolle gehören nicht in das öffentliche Repository. Spätere automatisch erzeugte Berichte liegen lokal unter `build/reports/`.

## Paketquellen für die Werkzeug-Einrichtung prüfen

**Umgebung:** Ubuntu/WSL2 (Entwicklungs-Host) · **Shell:** Bash · **Arbeitsverzeichnis:** Benutzerverzeichnis (`~`).

**Voraussetzungen:** Abgeschlossene Bestandsaufnahme, Ubuntu mit `apt`, Netzwerkzugang und `sudo`-Berechtigung. Die Architektur bestimmt das notwendige QEMU-Systempaket; auf einem `x86_64`-Host verwenden wir zunächst den x86-Systememulator.

**Erwartetes Ergebnis:** Erfolgreiche Aktualisierung der APT-Paketindizes und installierbare Kandidaten für die benötigten Pakete. `apt update` schreibt Paketmetadaten in den lokalen APT-Cache, installiert aber noch keine Pakete; `apt-cache policy` zeigt nur Informationen an. **Projektdateien / Git-Versionierung:** keine.

```bash
cd ~
sudo apt update
apt-cache policy build-essential gdb qemu-system-x86 qemu-utils
```

`build-essential` zieht unter Ubuntu unter anderem `gcc` und `make` nach sich. `qemu-system-x86` stellt den Systememulator `qemu-system-x86_64` bereit, `qemu-utils` unter anderem `qemu-img`. Die Pakete werden erst nach Kontrolle der Kandidaten und Paketquellen gezielt installiert. Für andere Architekturen muss die Paketauswahl angepasst werden. Fehlende Kandidaten sind Anlass zur Prüfung der konfigurierten APT-Quellen und aktivierten Ubuntu-Komponenten, nicht zum ungeprüften Hinzufügen fremder Repositories. Auf Ubuntu 26.04 können die für diesen Einstieg benötigten Pakete aus `main` verfügbar sein.

## Grundlegende Entwicklungswerkzeuge installieren

Die Paketprüfung muss für `build-essential`, `gdb`, `qemu-system-x86` und `qemu-utils` jeweils einen installierbaren Kandidaten zeigen. Auf Ubuntu `x86_64` werden anschließend nur diese benötigten Pakete ausdrücklich angefordert. Eine allgemeine Systemaktualisierung mit `apt upgrade` gehört nicht zu diesem Schritt.

**Umgebung:** Ubuntu/WSL2 (Entwicklungs-Host) · **Shell:** Bash · **Arbeitsverzeichnis:** Benutzerverzeichnis (`~`).

**Voraussetzungen:** Erfolgreiches `sudo apt update`, installierbare Kandidaten in `apt-cache policy`, Internetzugang und `sudo`-Berechtigung.

**Erwartetes Ergebnis:** APT zeigt die geplanten Abhängigkeiten und installiert nach Bestätigung Compiler, Build-Werkzeuge, GDB, QEMU-Systememulation und QEMU-Datenträgerwerkzeuge. Vor der Bestätigung die vorgeschlagenen Änderungen auf unerwartete Paketentfernungen oder umfangreiche, sachfremde Aktualisierungen prüfen. Falls die Pakettransaktion unerwartet aussieht, **abbrechen und die Ausgabe prüfen**, statt die Installation blind zu bestätigen. **Ablage / Versionierung:** systemweite Pakete und APT-Metadaten unter Ubuntu; keine erzeugten Repository-Dateien, keine Git-Versionierung.

```bash
cd ~
sudo apt install build-essential gdb qemu-system-x86 qemu-utils
```

APT entscheidet über die tatsächlichen Versionen und erforderlichen Abhängigkeiten. Ubuntu kann Aktualisierungen phasenweise verteilen (`phased updates`). Eine explizite Paketinstallation ist nicht dasselbe wie ein allgemeines Upgrade und kann bei der Versionsauswahl anders mit phasenweise bereitgestellten Aktualisierungen umgehen. Deshalb nicht aus einem früheren `apt-cache policy`-Ergebnis auf die tatsächlich installierte Version schließen. Versionen nach der Installation ermitteln, keine Phasing-Sperre deaktivieren oder umgehen, um eine vermeintlich passende Version zu erzwingen.

**Umgebung:** Ubuntu/WSL2 (Entwicklungs-Host) · **Shell:** Bash · **Arbeitsverzeichnis:** Benutzerverzeichnis (`~`).

**Voraussetzung:** Erfolgreicher Abschluss der obigen Paketinstallation. **Erwartetes Ergebnis:** Für jedes Paket eine installierte Version sowie Versionsangaben aller direkt benötigten Programme. **Dateien / Versionierung:** keine.

```bash
cd ~
dpkg-query -W -f='${Package}\t${Version}\n' build-essential gcc make gdb qemu-system-x86 qemu-utils

gcc --version | head -n 1
make --version | head -n 1
gdb --version | head -n 1
qemu-system-x86_64 --version | head -n 1
qemu-img --version | head -n 1
```

Die Paketprüfung kontrolliert die installierten Pakete; die Versionsaufrufe bestätigen zusätzlich, dass die entsprechenden ausführbaren Programme erreichbar sind. Ein erfolgreicher Versionsaufruf des QEMU-Systememulators beweist noch nicht, dass ein Gast bootet oder KVM funktioniert. Diese Tests folgen in getrennten Schritten.

## Reguläre Ubuntu-Updates vor der Virtualisierung prüfen

Nach der gezielten Werkzeug-Installation ist die Ubuntu-Basis auf einem gepflegten Stand zu halten. Dies betrifft Sicherheits- und Fehlerkorrekturen der **WSL2-Distribution**, etwa `systemd`, `sudo`, kryptografische Bibliotheken und `ubuntu-wsl`. Es aktualisiert **nicht** den von Microsoft bereitgestellten WSL-Kernel; dessen Versionsverwaltung erfolgt getrennt unter Windows. Offizielle Hinweise: [Microsoft WSL – Linux-Pakete aktualisieren](https://learn.microsoft.com/de-de/windows/wsl/setup/environment#update-and-upgrade-packages), [Ubuntu – Paketverwaltung](https://ubuntu.com/server/docs/how-to/software/package-management/).

Zunächst nur den geplanten Upgrade-Vorgang simulieren und mögliche zurückgehaltene Pakete erfassen. **Umgebung:** Ubuntu/WSL2 (Entwicklungs-Host) · **Shell:** Bash · **Arbeitsverzeichnis:** Benutzerverzeichnis (`~`). **Voraussetzungen:** funktionierende APT-Paketquellen, installierte Werkzeuge, `sudo`-Zugang. **Erwartetes Ergebnis:** Pakettransaktion mit vorgesehenen Upgrades und eventuellen Zusatzpaketen, ohne Installation. **Dateien / Versionierung:** Paketindex wird von `apt update` lokal aktualisiert; die Simulation verändert keine installierten Pakete und erzeugt keine Projektdateien.

```bash
cd ~
sudo apt update
sudo apt -s upgrade
apt-mark showhold
```

Das Ergebnis der Simulation vor einem echten Upgrade prüfen, insbesondere auf unerwartete Paketentfernungen, Paketkonflikte, ungewöhnliche Quellen oder geänderte Abhängigkeiten. `apt upgrade` ist für normale Wartungsupdates vorgesehen; `apt full-upgrade`, ein Release-Wechsel (`do-release-upgrade`) oder das Erzwingen phasenweise zurückgehaltener Updates gehören **nicht** zu diesem Schritt.

**Nach erfolgreicher Prüfung** die vorgeschlagenen Updates installieren. **Umgebung:** Ubuntu/WSL2 · **Shell:** Bash · **Arbeitsverzeichnis:** `~`. **Voraussetzungen:** Simulation geprüft und keine unerwarteten Änderungen; bei Diskrepanzen erst Ursache klären. **Ergebnis:** Aktualisierte Ubuntu-Pakete; ggf. neue lokale Paketabhängigkeiten. **Versionierung:** Keine Projektdateien betroffen.

```bash
cd ~
sudo apt upgrade
```

`apt` kann Updates wegen *phased updates* vorübergehend zurückhalten; Sicherheitsupdates werden nicht phasenweise verteilt. Bei Änderungen an Systemdiensten oder der WSL-Integration prüfen wir nach der Installation, ob ein WSL-Neustart erforderlich ist. Ein `wsl --shutdown` wird nur gezielt in einem getrennten Windows-/PowerShell-Schritt durchgeführt, da es **alle** laufenden WSL-Distributionen beendet, einschließlich etwaiger Docker-Desktop-Instanzen.

## Nach einem größeren Ubuntu-Update prüfen

Nach einem umfangreichen Upgrade sollten die Paketdatenbank und der privilegierte Zugriff überprüft werden, bevor weitere Systemkonfigurationen erfolgen. Das gilt insbesondere, wenn Pakete wie `sudo`, `systemd`, `udev` oder die Ubuntu-WSL-Integration aktualisiert wurden. Warnungen bei `update-alternatives` oder Zertifikat-Rehash sind nicht automatisch Fehler; entscheidend sind der Abschluss der Paketkonfiguration und die anschließenden Funktionstests.

**Umgebung:** Ubuntu/WSL2 (Entwicklungs-Host) · **Shell:** Bash · **Arbeitsverzeichnis:** `~`.

**Voraussetzungen:** Paketupdate abgeschlossen, aktive Ubuntu-Shell. **Erwartetes Ergebnis:** `sudo` funktioniert und `dpkg --audit` meldet keine unvollständigen Paketinstallationen. **Erzeugte Dateien:** keine Projektdateien; `sudo` kann seinen Authentifizierungs-Zeitstempel im System verwalten. **Versionierung:** keine.

```bash
cd ~
command -v sudo
readlink -f "$(command -v sudo)"
sudo -v && printf '%s\n' 'sudo: OK'
dpkg --audit
```

`dpkg --audit` sollte bei vollständig konfigurierten Paketen keine Ausgabe erzeugen. Der erfolgreiche Aufruf von `sudo -v` prüft die Authentifizierung; der aufgelöste Dateipfad hilft, Alternativen-Verknüpfungen zu diagnostizieren. Ein wegen *phased updates* zurückgestelltes Paket zählt nicht als fehlerhaft installiertes Paket und muss nicht erzwungen werden.

Nach größeren Änderungen an Systemdiensten und der WSL-Integration ist es sinnvoll, die Ubuntu-Distribution für den folgenden QEMU/KVM-Test neu zu starten. Ein gezieltes `wsl --terminate Ubuntu` aus **Windows/PowerShell** beendet nur die genannte Distribution; vorher laufende Prozesse und offene Dateien darin ordnungsgemäß schließen. Anders als `wsl --shutdown` stoppt es nicht absichtlich sämtliche WSL-Distributionen.

## KVM-Berechtigungen herstellen und überprüfen

Wenn `/dev/kvm` existiert, der Geräteknoten `root:kvm` mit `crw-rw----` gehört und der normale Benutzer noch nicht Mitglied von `kvm` ist, ergänzen wir seine Gruppenmitgliedschaft. Die Arbeit erfolgt nicht als root; nur die Änderung an der Gruppendatenbank benötigt `sudo`.

**Umgebung:** Ubuntu/WSL2 (Entwicklungs-Host) · **Shell:** Bash · **Arbeitsverzeichnis:** beliebig.

**Voraussetzungen:** Erfolgreiche Paket- und `sudo`-Validierung, vorhandene Gruppe `kvm`, passend berechtigtes `/dev/kvm`. **Erwartetes Ergebnis:** Der Benutzer wird zusätzlich Mitglied der Gruppe `kvm` und erscheint im Gruppeneintrag. **Betroffene Dateien:** `/etc/group`, gegebenenfalls `/etc/gshadow`. **Projektdateien / Git-Versionierung:** keine.

```bash
sudo usermod -aG kvm "$USER"
getent group kvm
```

`-aG` ergänzt eine zusätzliche Gruppe. Ohne `-a` könnte `usermod -G` bestehende zusätzliche Gruppenmitgliedschaften ersetzen. Die Änderung wird nicht rückwirkend auf laufende Prozesse und Shells angewandt.

Nun in Ubuntu alle wichtigen Arbeiten beenden und die Shell verlassen:

```bash
exit
```

**Umgebung:** Windows · **Shell:** PowerShell · **Arbeitsverzeichnis:** beliebig.

**Voraussetzungen:** Ubuntu-Sitzung verlassen, keine unerledigten Arbeiten oder wichtigen Hintergrundprozesse innerhalb dieser Distribution. **Erwartetes Ergebnis:** Die Distribution `Ubuntu` wird beendet und mit einer neuen Login-Sitzung gestartet. **Auswirkungen:** Prozesse in `Ubuntu` werden beendet; andere WSL-Distributionen werden nicht gezielt gestoppt. **Git-Versionierung:** keine.

```powershell
wsl --terminate Ubuntu
wsl --distribution Ubuntu
```

**Umgebung:** Ubuntu/WSL2 (neue Sitzung des Entwicklungs-Hosts) · **Shell:** Bash · **Arbeitsverzeichnis:** beliebig.

**Voraussetzung:** Erfolgreicher Neustart der Distribution. **Erwartetes Ergebnis:** `kvm` erscheint in `id -nG`, die Geräterechtigungen werden angezeigt und der Zugriffscheck meldet `KVM-Zugriff: OK`. **Dateien / Git-Versionierung:** keine.

```bash
id -nG
ls -l /dev/kvm

if [ -r /dev/kvm ] && [ -w /dev/kvm ]; then
    printf '%s\n' 'KVM-Zugriff: OK'
else
    printf '%s\n' 'KVM-Zugriff: NICHT VERFÜGBAR'
fi
```

Dies validiert ausschließlich die **Dateisystemberechtigungen** für `/dev/kvm`, noch nicht den Betrieb des KVM-Accelerators. Der tatsächliche QEMU/KVM-Test folgt im nächsten Abschnitt. Falls `/dev/kvm` fehlt, weder blind Gruppen ändern noch Zugriff durch `chmod 666` erzwingen; QEMU mit TCG ist der alternative Weg.

## KVM-Accelerator mit QEMU prüfen (ohne Gast-Image)

Dieser Funktionstest prüft, ob sich QEMU **ohne TCG-Fallback** mit KVM initialisieren lässt. Wir verwenden absichtlich kein heruntergeladenes Gast-Image und halten die virtuelle CPU an. Der Test setzt die erfolgreich geprüften Zugriffsrechte auf `/dev/kvm` voraus.

**Umgebung:** Ubuntu/WSL2 (Entwicklungs-Host) · **Shell:** Bash · **Arbeitsverzeichnis:** beliebig.

**Voraussetzungen:** `qemu-system-x86_64` installiert, aktueller Benutzer hat Lese- und Schreibzugriff auf `/dev/kvm`. **Erwartetes Ergebnis:** QEMU öffnet seinen Monitor mit `(qemu)` und weist den aktiven Accelerator als KVM aus. **Erzeugte Dateien:** keine. **Git-Versionierung:** keine.

```bash
qemu-system-x86_64 \
    -machine q35 \
    -accel kvm \
    -m 256M \
    -smp 1 \
    -display none \
    -nodefaults \
    -S \
    -monitor stdio
```

`-machine q35` wählt die emulierte PC-Plattform; `-accel kvm` verlangt explizit KVM, ohne automatischen TCG-Rückfall; `-m 256M` und `-smp 1` konfigurieren RAM und vCPU. `-display none` verhindert ein Grafikfenster, `-nodefaults` entfernt nicht angeforderte Standardgeräte. `-S` hält die vCPU an, und `-monitor stdio` macht den QEMU-Monitor im Terminal verfügbar.

**Umgebung:** Interaktiver QEMU-Monitor im **WSL2-Hostterminal** (nicht die Shell eines Linux-Gasts). **Arbeitsverzeichnis:** nicht relevant. **Voraussetzung:** Prompt `(qemu)` nach erfolgreicher Initialisierung. **Erwartetes Ergebnis:** KVM-Informationen sowie Anzeige des benutzten Accelerators; QEMU wird geordnet beendet. **Dateien / Versionierung:** keine.

```text
info kvm
info accelerators
quit
```

`info kvm` meldet bei funktionierender KVM-Initialisierung sinngemäß `kvm support: enabled`; `info accelerators` kennzeichnet den aktiven Accelerator in eckigen Klammern, beispielsweise `[kvm] mshv tcg`. `quit` beendet den QEMU-Prozess. Eine Warnung wie `host doesn't support requested feature: CPUID[eax=80000001h].ECX.svm [bit 2]` betrifft das nicht verfügbare Gast-CPU-Merkmal AMD SVM. Ein trotzdem aktives `[kvm]` belegt die erfolgreiche KVM-Initialisierung, nicht die Unterstützung verschachtelter Virtualisierung **innerhalb** des Gasts. Die konkrete CPU-Feature-Konfiguration und die tatsächliche Gast-Ausführung werden beim späteren Boot geprüft; keine nicht unterstützten Features erzwingen. Tritt schon beim Start eine Fehlermeldung auf, diese vollständig dokumentieren und keine unkontrollierten Workarounds an Virtualisierungs- oder Sicherheitskonfigurationen durchführen.

Der positive Test beweist **KVM-Initialisierung**, noch keinen erfolgreichen Linux-Boot und keine längere fehlerfreie vCPU-Ausführung. Erst ein nachfolgender Start eines vorbereiteten Gast-Images kann die gesamte Virtualisierungskette belegen. Wenn KVM nicht funktioniert, kann die geplante QEMU-Lernumgebung stattdessen mit `-accel tcg` betrieben werden, mit erwartbar geringerer Performance.

Referenzen: [QEMU – Invocation](https://www.qemu.org/docs/master/system/invocation.html), [QEMU – Monitor](https://www.qemu.org/docs/master/system/monitor.html).

## Projekt-Checkout und vorbereitetes Gast-Image

Für den ersten realen KVM-Gaststart verwenden wir ein schlankes, bereits gebautes Linux: **Alpine Linux 3.24.2, x86_64, Variante `virt`**. Die Variante ist für virtuelle Maschinen ausgelegt und enthält laut Alpine bereits die Konfiguration der seriellen Konsole. Das ISO ist nur für Stufe 0 vorgesehen; der eigene Kernel-Build wird in Stufe 2 untersucht. Wir fixieren die Versionsnummer und verlinken keine unversionierte `latest`-Datei, damit die Anleitung nachvollziehbar bleibt.

Offizielle Quellen: [Alpine-Releaseverzeichnis](https://dl-cdn.alpinelinux.org/alpine/v3.24/releases/x86_64/), [Alpine: serielle Konsole unter QEMU](https://wiki.alpinelinux.org/wiki/Enable_Serial_Console_on_Boot).

### Projektverzeichnis prüfen oder klonen

**Umgebung:** Ubuntu/WSL2 (Entwicklungs-Host) · **Shell:** Bash · **Arbeitsverzeichnis:** Benutzerverzeichnis (`~`). **Voraussetzungen:** Netzwerk, Git und Zugriff auf das Repository. **Erwartetes Ergebnis:** Ein lokaler Repository-Checkout im Linux-Dateisystem. **Erzeugte Dateien:** nur bei noch nicht vorhandenem Checkout: Git-Dateien und versionierte Projektstruktur unter `~/linux-kernel-lab`. **Git-Versionierung:** Das Klonen legt keine neuen Commits an.

```bash
cd ~

if [ ! -e "$HOME/linux-kernel-lab" ]; then
    git clone https://github.com/B0bbyR4y/linux-kernel-lab.git "$HOME/linux-kernel-lab"
fi

git -C "$HOME/linux-kernel-lab" rev-parse --show-toplevel
git -C "$HOME/linux-kernel-lab" status --short --branch
git -C "$HOME/linux-kernel-lab" log -1 --format='%h %s'
```

Existiert das Verzeichnis schon, wird nichts überschrieben und kein automatischer `git pull` durchgeführt. Der Status und der lokale HEAD helfen, bestehende Änderungen vor einer eventuellen Synchronisierung zu erkennen. Bei einem Fehler zuerst diesen Zustand klären, bevor Dateien abgelegt werden.

### ISO und veröffentlichte SHA-256-Prüfsumme herunterladen

**Umgebung:** Ubuntu/WSL2 (Entwicklungs-Host) · **Shell:** Bash · **Arbeitsverzeichnis:** `~/linux-kernel-lab/downloads`. **Voraussetzungen:** Verifizierter Repository-Checkout, Netzwerkzugang, `curl` und `sha256sum`. **Erwartetes Ergebnis:** ISO-Datei sowie dazugehörige Prüfsumme; `sha256sum -c` muss `OK` ausgeben. **Erzeugte Dateien:** zwei nicht versionierte Dateien im ignorierten Verzeichnis `downloads/`. **Git-Versionierung:** keine.

```bash
mkdir -p "$HOME/linux-kernel-lab/downloads"
cd "$HOME/linux-kernel-lab/downloads"

BASE_URL="https://dl-cdn.alpinelinux.org/alpine/v3.24/releases/x86_64"
IMAGE="alpine-virt-3.24.2-x86_64.iso"

curl --fail --location --retry 3 --output "$IMAGE" "$BASE_URL/$IMAGE" &&
curl --fail --location --retry 3 --output "$IMAGE.sha256" "$BASE_URL/$IMAGE.sha256" &&
sha256sum --check "$IMAGE.sha256"
```

Die SHA-256-Prüfung gleicht das heruntergeladene ISO mit der vom Projekt publizierten Prüfsumme ab und erkennt Übertragungsfehler oder abweichende Dateien. Sie ist **keine unabhängige Authentizitätsprüfung** der Herausgeberidentität, da ISO und Hash über denselben Server bezogen werden. Für höhere Anforderungen wäre zusätzlich eine vertrauenswürdig geprüfte Signatur nötig. Bei Prüfsummenfehler nicht booten.

Das ISO wird nicht in Git eingecheckt. Nach erfolgreicher Prüfsumme ist es für den ersten Gaststart bereit.

## Erster Linux-Gast-Boot mit KVM und serieller Konsole

Alpines `virt`-ISO bringt eine vorkonfigurierte serielle Konsole mit. Für den **ersten Funktionstest** booten wir ein temporäres Live-System unmittelbar vom ISO und legen noch keine virtuelle Festplatte an. Diese Sitzung ist nicht persistent; Änderungen innerhalb des Gasts gehen nach dem Herunterfahren verloren. Für spätere Übungen mit dauerhaften Änderungen richten wir einen eigenen Datenträger ein.

### Lokalen Checkout abgleichen

**Umgebung:** Ubuntu/WSL2 (Entwicklungs-Host) · **Shell:** Bash · **Arbeitsverzeichnis:** beliebig.

**Voraussetzungen:** Sauberer lokaler Git-Checkout unter `~/linux-kernel-lab` ohne eigene ungesicherte Änderungen; vorhandenes Netzwerk. **Erwartetes Ergebnis:** Lokaler Branch `main` wird bei Bedarf per Fast-Forward auf den dokumentierten Stand aktualisiert. **Erzeugte/geänderte Dateien:** Versionsverwaltete Projektdateien und Git-Metadaten; die ignorierten Downloads bleiben erhalten. **Versionierung:** Es entsteht kein neuer Commit.

```bash
git -C "$HOME/linux-kernel-lab" status --short --branch
git -C "$HOME/linux-kernel-lab" pull --ff-only
git -C "$HOME/linux-kernel-lab" log -1 --format='%h %s'
```

Vor dem `pull` den Status prüfen. Bei lokalen Änderungen, unerwartetem Branch oder Merge-Konflikten nicht automatisch überschreiben.

### ISO mit aktiviertem KVM booten

**Umgebung:** Ubuntu/WSL2 (Entwicklungs-Host) · **Shell:** Bash · **Arbeitsverzeichnis:** `~/linux-kernel-lab/downloads`.

**Voraussetzungen:** Alpine-`virt`-ISO erfolgreich SHA-256-geprüft, QEMU installiert, KVM-Initialisierung nachgewiesen, die Gast-ISO liegt im Verzeichnis. **Erwartetes Ergebnis:** Das Gast-Linux startet mit Bootmeldungen auf der seriellen Konsole; anschließend erscheint ein Alpine-Login-Prompt. **Erzeugte Dateien:** Keine zusätzlichen Dateien. Es wird **keine** virtuelle Festplatte angelegt; das ISO wird nur gelesen. **Git-Versionierung:** keine.

```bash
cd "$HOME/linux-kernel-lab/downloads"

test -f alpine-virt-3.24.2-x86_64.iso &&
qemu-system-x86_64 \
    -machine q35 \
    -accel kvm \
    -m 512M \
    -smp 1 \
    -boot order=d \
    -cdrom alpine-virt-3.24.2-x86_64.iso \
    -nic none \
    -nographic
```

Optionen:
- `-machine q35`: PC-Maschinenmodell wie beim bisherigen Initialisierungstest;
- `-accel kvm`: KVM ausdrücklich verwenden (kein stiller Rückfall auf TCG);
- `-m 512M`, `-smp 1`: kleiner Ressourcenbedarf für den ersten Boot;
- `-boot order=d`, `-cdrom ...`: vom Alpine-ISO als optischem Laufwerk booten;
- `-nic none`: zunächst keine Netzwerkschnittstelle; kein Netz für diesen Test erforderlich;
- `-nographic`: Terminal als serielle Gastkonsole und QEMU-Monitor nutzen; keine grafische Anzeige.

Hier fehlt absichtlich `-S`: Die virtuelle CPU soll **tatsächlich laufen**. Die Option `-nodefaults` aus dem bloßen Initialisierungstest wird ebenfalls nicht übernommen, weil wir die normale PC-Geräteausstattung einschließlich serieller Schnittstelle zum Booten verwenden.

Die Alpine-`virt`-ISO ist für eine serielle Konsole vorkonfiguriert ([Alpine-Wiki](https://wiki.alpinelinux.org/wiki/Enable_Serial_Console_on_Boot)). Nach dem Boot in der seriellen Gastkonsole am Prompt `localhost login:` mit `root` anmelden; bei der Live-ISO ist standardmäßig noch kein Root-Passwort gesetzt. Bei ausbleibender Anmeldung erst Enter drücken, keine Konfigurationsänderungen auf Verdacht vornehmen. Eine Anmeldung über `ttyS0` beweist die Funktion der seriellen **Login-Sitzung**, aber nicht automatisch die Ausgabe sämtlicher früher Kernelmeldungen über diese Schnittstelle. Ein fehlendes `console=ttyS0` in `/proc/cmdline` bedeutet, dass dieses Ziel nicht ausdrücklich als Kernel-Bootparameter angegeben wurde.

### Gastkernel und Konsole inspizieren

**Umgebung:** Linux-**Gast in QEMU** · **Shell:** Alpine-`ash` · **Arbeitsverzeichnis:** beliebig.

**Voraussetzungen:** Alpine-Gast vollständig gebootet, Anmeldung als `root`. **Erwartetes Ergebnis:** Ausgabe der Alpine-Version, der Gastkernel-Version, der Architektur, der Kernel-Startparameter und der Bootmeldungen. **Dateien / Versionierung:** keine Projektdateien; temporäre Gastumgebung.

```sh
cat /etc/os-release
uname -r
uname -m
cat /proc/cmdline
dmesg | tail -n 35
```

`uname -r` in dieser Shell liefert die Version des **Gastkernels** (nicht den WSL2-Kernel `6.6.114.1-microsoft-standard-WSL2`). `/proc/cmdline` zeigt die dem Gastkernel übergebenen Startparameter, darunter bei serieller Konfiguration insbesondere `console=ttyS0` oder eine entsprechende Einstellung. `dmesg` liest den Ringpuffer des Gastkernels. Der Befehl wird nicht im WSL2-Host ausgeführt.

### Gast geordnet herunterfahren

**Umgebung:** Linux-Gast in QEMU · **Shell:** Alpine-`ash` · **Arbeitsverzeichnis:** beliebig. **Voraussetzung:** Befehle zur Bootprüfung beendet. **Erwartetes Ergebnis:** Gast fährt herunter; QEMU beendet sich normalerweise und gibt den WSL2-Prompt wieder frei. **Dateien / Versionierung:** keine persistenten Gastdateien.

```sh
poweroff
```

Falls die Konsole hängt, kann QEMU über den Tastatur-Multiplexer bedient werden: `Strg+A` und danach `C` zum Monitor, dort `quit` für ein sofortiges Beenden. `Strg+A` danach `X` beendet QEMU direkt. Beides ist ein **Notabbruch**, nicht mit einem sauberen `poweroff` zu verwechseln; [QEMU-Dokumentation zu Multiplexer-Tasten](https://www.qemu.org/docs/master/system/mux-chardev.html). Die Tastenkombinationen sind nacheinander auszuführen, nicht gleichzeitig.

### Diagnosekriterien

1. Der KVM-Accelerator initialisiert ohne Abbruch (die bereits bekannte Warnung über nicht verfügbares Gast-CPU-`svm`-Feature kann erneut auftreten).
2. Das Linux-System zeigt Kernel-Bootmeldungen über die serielle Konsole.
3. Eine Anmeldung in der Alpine-Gast-Shell gelingt.
4. `uname -r` im Gast unterscheidet sich vom WSL2-Hostkernel und `dmesg` zeigt die Gast-Bootmeldungen.
5. `poweroff` beendet den Gast ohne erzwungenen Prozessabbruch.

Bootmeldungen und Prüfausgaben zur gemeinsamen Auswertung bereitstellen. Falls der Start bei der SVM-Warnung stehen bleibt oder eine andere Fehlermeldung erscheint, den Fehler vollständig dokumentieren, statt CPU-Features oder WSL-Konfiguration unkontrolliert anzupassen.

## Wiederholter Gaststart und Dateiübertragung Host → Gast (VirtIO-9P)

Nach dem ersten erfolgreichen Boot wird die Live-ISO ein zweites Mal gestartet. Für den ersten Dateiaustausch soll der Gast ein **eigens angelegtes Hostverzeichnis nur lesend** einbinden. Das gesamte Home-Verzeichnis oder Repository wird nicht exportiert.

### Hostdatei bereitstellen

**Umgebung:** Ubuntu/WSL2 · **Shell:** Bash · **Arbeitsverzeichnis:** beliebig.
**Voraussetzungen:** lokaler Checkout, Schreibrechte unter `~/linux-kernel-lab/build`.
**Ergebnis/Dateien:** `build/share/host-to-guest.txt`; durch `.gitignore` nicht versioniert.

```bash
mkdir -p "$HOME/linux-kernel-lab/build/share"
printf '%s\n' 'Hallo aus Ubuntu/WSL2' > "$HOME/linux-kernel-lab/build/share/host-to-guest.txt"
cat "$HOME/linux-kernel-lab/build/share/host-to-guest.txt"
```

### QEMU mit schreibgeschützter Freigabe starten

**Umgebung:** Ubuntu/WSL2 · **Shell:** Bash · **Arbeitsverzeichnis:** `~/linux-kernel-lab/downloads`.
**Voraussetzungen:** SHA-256-geprüfte Alpine-ISO, funktionierender KVM-Gaststart und vorhandenes `build/share`.
**Ergebnis/Dateien:** erneuter Live-Boot; keine weiteren persistenten Dateien oder Git-Änderungen.

```bash
cd "$HOME/linux-kernel-lab/downloads"
qemu-system-x86_64 \
    -machine q35 \
    -accel kvm \
    -m 512M \
    -smp 1 \
    -boot order=d \
    -cdrom alpine-virt-3.24.2-x86_64.iso \
    -nic none \
    -virtfs local,path="$HOME/linux-kernel-lab/build/share",mount_tag=labshare,security_model=none,readonly=on \
    -nographic
```

`-virtfs` stellt ein VirtIO-9P-Gerät mit dem Tag `labshare` bereit; `readonly=on` schützt die freigegebenen Hostdateien vor Schreibzugriffen des Gasts. Der Gastkernel benötigt Unterstützung für das 9P-Dateisystem und den VirtIO-9P-Transport. **Dies wurde mit Alpine 3.24.2 und dem Gastkernel `6.18.52-0-virt` erfolgreich geprüft.**

### Freigabe im Gast mounten und lesen

**Umgebung:** Alpine-Linux-Gast in QEMU · **Shell:** `ash` · **Benutzer:** `root` · **Arbeitsverzeichnis:** beliebig.
**Voraussetzungen:** erneuter Boot erfolgreich; Anmeldung über `ttyS0`.
**Erwartetes Ergebnis:** Textdatei aus dem Hostverzeichnis wird lesbar; keine persistenten Gastdateien oder Repository-Änderungen.

```sh
mkdir -p /mnt/labshare
mount -t 9p -o trans=virtio,version=9p2000.L,ro labshare /mnt/labshare
cat /mnt/labshare/host-to-guest.txt
mount | grep labshare
```

Wenn der Mount fehlschlägt, die Fehlermeldung sichern und **nicht** durch zusätzliche, ungeprüfte Berechtigungen oder Pakete umgehen. Wir prüfen dann zunächst `9p` und `9pnet_virtio` im Gastkernel. Bei Erfolg den Gast mit `poweroff` geordnet beenden.

**Verifizierter Referenzbefund:** Ein erneuter Alpine-Gaststart war erfolgreich. Die Textdatei wurde vollständig als `Hallo aus Ubuntu/WSL2` gelesen, und `mount` bestätigte `labshare on /mnt/labshare type 9p (ro,relatime,access=client,trans=virtio)`. Das anschließende `poweroff` führte zurück zum Ubuntu/WSL2-Prompt. Dies beweist Host → Gast und einen wiederholbaren Live-ISO-Start; **Gast → Host** ist damit noch nicht geprüft.

Quelle: [QEMU – VirtIO-9P](https://www.qemu.org/docs/master/system/qemu-manpage.html).

## Kontrollierte Dateiübertragung Gast → Host (VirtIO-9P)

Nach dem erfolgreichen Lesezugriff prüfen wir den Rückweg separat. Anders als bei `labshare` wird dafür **ein eigenes, zunächst leeres Hostverzeichnis** schreibbar exportiert. Der Zugriff bleibt auf dieses Verzeichnis beschränkt. Das gesamte Home-Verzeichnis und der Projekt-Checkout werden nicht freigegeben.

### Separates Ausgabe-Verzeichnis vorbereiten

**Umgebung:** Ubuntu/WSL2 – Entwicklungs-Host · **Shell:** Bash · **Arbeitsverzeichnis:** beliebig.

**Voraussetzungen:** Sauberer Repository-Checkout und funktionsfähiger vorheriger Gaststart. **Erwartetes Ergebnis:** Neues privates Verzeichnis `build/guest-out/` für die Gastausgabe. **Dateiänderungen:** Host erstellt das Verzeichnis und setzt dessen Zugriffsrechte auf `0700`; keine getrackten Dateien. **Git-Versionierung:** `build/` ist ignoriert.

```bash
mkdir -p "$HOME/linux-kernel-lab/build/guest-out"
chmod 700 "$HOME/linux-kernel-lab/build/guest-out"
ls -ld "$HOME/linux-kernel-lab/build/guest-out"
```

### ISO mit begrenztem Schreibexport starten

**Umgebung:** Ubuntu/WSL2 – Entwicklungs-Host · **Shell:** Bash · **Arbeitsverzeichnis:** `~/linux-kernel-lab/downloads`.

**Voraussetzungen:** Prüfsummenvalidierte ISO, vorhandenes `build/guest-out/`, funktionierender KVM-Test. **Erwartetes Ergebnis:** Alpine-Live-Gast startet mit einem **schreibbaren** VirtIO-9P-Export unter dem Mount-Tag `guestout`. **Dateiänderungen:** keine weiteren Hostdateien beim Start; spätere Ausgabedateien werden von Gastoperationen im dafür vorgesehenen Hostverzeichnis erstellt. **Git-Versionierung:** keine.

```bash
cd "$HOME/linux-kernel-lab/downloads"

qemu-system-x86_64 \
    -machine q35 \
    -accel kvm \
    -m 512M \
    -smp 1 \
    -boot order=d \
    -cdrom alpine-virt-3.24.2-x86_64.iso \
    -nic none \
    -virtfs local,path="$HOME/linux-kernel-lab/build/guest-out",mount_tag=guestout,security_model=none \
    -nographic
```

**Wichtiger Unterschied:** Im Gegensatz zum vorherigen Lesezugriff enthält der neue Export **kein** `readonly=on`. Laut QEMU-Dokumentation ist `-virtfs` standardmäßig schreibbar. Mit `security_model=none` führt QEMU die Host-Dateioperationen im Kontext des aufrufenden Host-Benutzers aus und ignoriert Fehler beim Setzen bestimmter Besitz-/Metadaten; es ist keine Root-Ausführung von QEMU erforderlich. Darum nur den isolierten Ausgabeordner freigeben und ausschließlich vertrauenswürdige Test-Gäste verwenden. Ein schreibbarer Export ist kein Sicherheits-Sandbox-Ersatz.

### Diagnosebericht im Gast schreiben

**Umgebung:** Alpine-Gast in QEMU · **Shell:** `ash` · **Benutzer:** `root` · **Arbeitsverzeichnis:** beliebig.

**Voraussetzungen:** Erfolgreicher Gaststart und Anmeldung über `ttyS0`. **Erwartetes Ergebnis:** `guestout` wird mit `rw` gemountet und der Gast schreibt einen kurzen Diagnosebericht direkt in das Hostverzeichnis. **Dateiänderungen:** im Host `build/guest-out/guest-report.txt`; das Verzeichnis `/mnt/guestout` im flüchtigen Live-Gast. **Git-Versionierung:** keine.

```sh
mkdir -p /mnt/guestout
mount -t 9p -o trans=virtio,version=9p2000.L guestout /mnt/guestout
mount | grep guestout

{
    printf 'Gast: Alpine Linux\n'
    printf 'Kernel: '
    uname -r
    printf 'Architektur: '
    uname -m
} > /mnt/guestout/guest-report.txt

sync
cat /mnt/guestout/guest-report.txt
sha256sum /mnt/guestout/guest-report.txt
poweroff
```

Die Prüfung auf `rw` im Mount-Ausdruck unterscheidet den Rücktransfer vom bisherigen `ro`-Export. `sync` fordert das Schreiben gepufferter Daten an, `poweroff` beendet den Live-Gast geordnet. Tritt beim Mount oder beim Schreiben ein Fehler auf, nicht eigenmächtig Berechtigungen ausweiten, sondern vollständigen Fehlertext dokumentieren und vor `poweroff` den Fehler untersuchen.

### Ergebnis auf dem Host überprüfen

**Umgebung:** Ubuntu/WSL2 – Entwicklungs-Host · **Shell:** Bash · **Arbeitsverzeichnis:** beliebig.

**Voraussetzung:** QEMU wurde nach dem Gast-`poweroff` beendet. **Erwartetes Ergebnis:** Eine im Gast erzeugte Datei ist jetzt auf dem Host verfügbar und hat dieselbe SHA-256-Prüfsumme. **Dateiänderungen:** keine weiteren, Befehle lesen nur; `git status` zeigt weiterhin keine getrackten Änderungen.

```bash
cat "$HOME/linux-kernel-lab/build/guest-out/guest-report.txt"
sha256sum "$HOME/linux-kernel-lab/build/guest-out/guest-report.txt"
git -C "$HOME/linux-kernel-lab" status --short --branch
```

Wenn Dateiinhalt und Prüfsumme übereinstimmen, sind Host → Gast und Gast → Host mit begrenzten 9P-Verzeichnissen nachgewiesen. Für echte Kernel-Lab-Arbeit mit persistenten Veränderungen wird anschließend ein separater virtueller Datenträger samt definiertem Rücksetzmechanismus vorbereitet.

Quelle: [QEMU-Dokumentation zu VirtIO-9P und `readonly=on`](https://www.qemu.org/docs/master/system/qemu-manpage.html).

## Weitere Einrichtung und Abschlusskriterien

Nach der Bestandsaufnahme folgen Werkzeuginstallation, ein geprüftes Gast-Image, ein dokumentierter QEMU-Start, Zugriff auf die Gast-Shell und Dateiaustausch. Stufe 0 ist abgeschlossen, wenn der Gast wiederholt bootet, seine Kernel-Version und Bootmeldungen sichtbar sind, Dateien übertragen werden können und Herunterfahren sowie Wiederherstellung eines definierten Ausgangszustands funktionieren.

Ein erfolgreicher Aufruf von `uname -r` in WSL bestätigt nur den WSL-Kernel. Der spätere gleiche Aufruf im Gast bestätigt den Gastkernel. Die konkrete Gast-Shell und deren Arbeitsverzeichnis werden in jedem entsprechenden Schritt separat angegeben.
