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

`info kvm` meldet bei funktionierender KVM-Initialisierung sinngemäß `kvm support: enabled`; `info accelerators` zeigt den verwendeten Accelerator. `quit` beendet den QEMU-Prozess. Tritt schon beim Start eine Fehlermeldung auf, diese vollständig dokumentieren und keine unkontrollierten Workarounds an Virtualisierungs- oder Sicherheitskonfigurationen durchführen.

Der positive Test beweist **KVM-Initialisierung**, noch keinen erfolgreichen Linux-Boot und keine längere fehlerfreie vCPU-Ausführung. Erst ein nachfolgender Start eines vorbereiteten Gast-Images kann die gesamte Virtualisierungskette belegen. Wenn KVM nicht funktioniert, kann die geplante QEMU-Lernumgebung stattdessen mit `-accel tcg` betrieben werden, mit erwartbar geringerer Performance.

Referenzen: [QEMU – Invocation](https://www.qemu.org/docs/master/system/invocation.html), [QEMU – Monitor](https://www.qemu.org/docs/master/system/monitor.html).

## Weitere Einrichtung und Abschlusskriterien

Nach der Bestandsaufnahme folgen Werkzeuginstallation, ein geprüftes Gast-Image, ein dokumentierter QEMU-Start, Zugriff auf die Gast-Shell und Dateiaustausch. Stufe 0 ist abgeschlossen, wenn der Gast wiederholt bootet, seine Kernel-Version und Bootmeldungen sichtbar sind, Dateien übertragen werden können und Herunterfahren sowie Wiederherstellung eines definierten Ausgangszustands funktionieren.

Ein erfolgreicher Aufruf von `uname -r` in WSL bestätigt nur den WSL-Kernel. Der spätere gleiche Aufruf im Gast bestätigt den Gastkernel. Die konkrete Gast-Shell und deren Arbeitsverzeichnis werden in jedem entsprechenden Schritt separat angegeben.
