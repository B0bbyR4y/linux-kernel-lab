# Repository-Struktur und Arbeitsregeln

## Grundsatz

Die Grundstruktur wird zuerst angelegt und mit jeder Übung um tatsächlich benötigte Dateien erweitert. Versioniert wird, was zum Verstehen, Bauen und Wiederholen nötig ist. Die unten als später bezeichneten Pfade sind geplant; sie existieren noch nicht. Leere Platzhalterverzeichnisse sind nicht erforderlich.

## Ablageorte

Alle relativen Pfade in dieser Übersicht beziehen sich auf die Repository-Wurzel, lokal vorgesehen als `~/linux-kernel-lab` unter Ubuntu/WSL2.

| Pfad | Zweck | Einführung / Git |
| --- | --- | --- |
| `README.md` | Einstieg und Navigation | Grundstruktur; versioniert |
| `docs/Linux_Kernel_und_Embedded_Linux_Nachschlagewerk.md` | Fachbegriffe und Zusammenhänge | Grundstruktur; versioniert |
| `docs/REPOSITORY_STRUCTURE.md` | Ablage- und Befehlsregeln | Grundstruktur; versioniert |
| `docs/assets/` | Eigene Abbildungen und Diagrammquellen | Bei Bedarf; versioniert |
| `labs/` | Übungen mit Anleitung und zusammengehörigem Code | Beginn mit Stufe 0; versioniert |
| `scripts/` | Gemeinsam verwendete Hilfsskripte | Zunächst README; Skripte bei erstem Einsatz; versioniert |
| `configs/` | Wiederverwendbare Kernel-, QEMU- und Image-Konfiguration | Später; versioniert |
| `rootfs/overlay/` | Eigene Dateien zur Integration in das Gast-Dateisystem | Später; versioniert |
| `patches/` | Eigene Änderungen an externen Projekten | Bei Bedarf; versioniert |
| `versions.env` | Gewählte Versionen, URLs, Commit-IDs und Prüfsummen | Mit erster externer Abhängigkeit; versioniert |
| `.gitignore` | Ausschluss lokaler und erzeugter Dateien | Grundstruktur; versioniert |
| `.gitattributes` | LF-Zeilenenden für Textdateien | Grundstruktur; versioniert |
| `.editorconfig` | Editor-Grundeinstellungen | Grundstruktur; versioniert |
| `external/` | Externe Quellen, etwa `external/linux/` | Lokal; ignoriert |
| `downloads/` | Archive und Ausgangs-Images | Lokal; ignoriert |
| `build/` | Build-Ausgaben, Gast-Images und Logs | Lokal; ignoriert |
| `build/reports/` | Wiederholbare lokale Prüfergebnisse | Bei Bedarf; ignoriert |

Externe Kernel-Quellen werden über eine feste Version oder Commit-ID und gegebenenfalls eigene Patches reproduzierbar zugeordnet. Gewollte Konfigurationen werden unter `configs/` gespeichert; eine generierte Kernel-`.config` im Build-Verzeichnis bleibt lokal. Für externe Module wird der passende Gastkernel-Build angegeben, nicht automatisch der laufende WSL-Kernel.

## Zuordnung der Lernstufen

| Stufe / Thema | Übungsverzeichnis | Relevante weitere Ablageorte |
| --- | --- | --- |
| 0: Entwicklungsumgebung | `labs/00-environment/` (angelegt) | `scripts/`; später `configs/`, `downloads/`, `build/` |
| 1: Systemarchitektur | `labs/01-system-architecture/` (später) | Fachkapitel im Nachschlagewerk |
| 2: Kernel-Build | `labs/02-kernel-build/` (später) | `configs/`, `external/linux/`, `build/kernel/` |
| 3: Kernel-Modul | `labs/03-kernel-module/` (später) | Zugehörige Build-Dateien und Gastkernel-Build |
| 4: Character Device | `labs/04-character-device/` (später) | `kernel/`, `userspace/`, bei Bedarf `include/` und `tests/` innerhalb der Übung |
| 5: Device Model | `labs/05-device-model/` (später) | Treiber und jeweilige Tests innerhalb der Übung |
| 6: Device Tree | `labs/06-device-tree/` (später) | DTS-Quellen innerhalb der Übung; wiederverwendbare Plattformkonfiguration unter `configs/` |
| 7: Nebenläufigkeit und Hardwarezugriff | `labs/07-concurrency-hardware/` (später) | Treiber, Testprogramme und gemeinsame Schnittstellen innerhalb der Übung |
| 8: Bootkette | `labs/08-boot-chain/` (später) | `configs/`, `scripts/`, externe Bootloader-Quellen unter `external/` |
| 9: Image-Erzeugung | `labs/09-image-build/` (später) | `configs/`, `rootfs/overlay/`, erzeugte Images unter `build/` |
| 10: IoT-Gateway | `labs/10-iot-gateway/` (später) | Anwendung, Tests und Dienste; gemeinsam genutzte Image-Konfiguration |

Jede Übung bekommt eine README mit Lernziel, Voraussetzungen, Durchführung, erwarteten Ergebnissen und Links zum Fachkapitel. Quellcode, der nur zu einer Übung gehört, bleibt dort. Gemeinsam verwendete Skripte stehen unter `scripts/`; wiederverwendbare Konfigurationen unter `configs/`.

## Ausführungsort und Pfade

Jeder ausführbare Schritt nennt ausdrücklich:

1. Umgebung: Windows, Ubuntu/WSL2 oder QEMU-Gast.
2. Shell: beispielsweise PowerShell oder Bash.
3. Arbeitsverzeichnis: konkreter Pfad oder ausdrücklich „beliebig“.
4. Voraussetzungen und Eingabedateien.
5. Vollständig kopierbare Befehle einschließlich erforderlicher Verzeichniswechsel.
6. Erwartete Ausgabe und Ablage erzeugter Dateien.
7. Welche Dateien versioniert werden beziehungsweise lokal bleiben.

Relative Pfade in einem Befehl beziehen sich auf dessen angegebenes Arbeitsverzeichnis. Ein Host-Pfad ist im Gast nicht automatisch vorhanden. Übertragung und Gast-Zielpfad werden im jeweiligen Schritt erklärt. Host- und Gast-Befehle stehen in getrennten Blöcken. Platzhalter und reine Illustrationen werden ausdrücklich gekennzeichnet.

Beispiel für eine vorhandene Datei:

**Umgebung:** Ubuntu/WSL2 · **Shell:** Bash · **Arbeitsverzeichnis:** Repository-Wurzel.

**Voraussetzung:** Repository unter `~/linux-kernel-lab` geklont.

```bash
cd ~/linux-kernel-lab
cat scripts/README.md
```

**Ergebnis:** Terminalausgabe der Skript-Konventionen; keine erzeugten Dateien. **Versionierung:** keine Änderung.

Die Bestandsaufnahme in Stufe 0 ist verzeichnisunabhängig und erzeugt keine Dateien. Installations- und Startbefehle werden erst mit festgelegten Werkzeug- und Gastversionen ergänzt.

## Pflege und Versionierung

Das [Nachschlagewerk](Linux_Kernel_und_Embedded_Linux_Nachschlagewerk.md) erklärt die Theorie; die [Lab-Anleitung](../labs/00-environment/README.md) beschreibt die konkrete Praxis. Befehlsfolgen werden an einer Stelle gepflegt und von anderen Dokumenten verlinkt. Pfadänderungen aktualisieren gleichzeitig Code, Skripte und Dokumentation.

Die Dokumentation ist ein fachliches Nachschlagewerk ohne persönliche Fortschrittschronik. Allgemeingültige Erkenntnisse aus Fehlern werden übernommen; vollständige Laufzeitlogs bleiben unter `build/`. Kleine, didaktisch relevante Beispielausgaben dürfen bei der Übung versioniert werden. Zugangsdaten und lokale Schlüssel werden nicht eingecheckt.

Neue Ignore-Regeln werden gezielt eingeführt. Insbesondere bleiben `.config`, `*.config`, DTS-Quellen und gegebenenfalls `dkms.conf` grundsätzlich trackbar. Kernel-Modul-Buildprodukte werden separat ignoriert. LF-Zeilenenden werden über `.gitattributes` festgelegt; Make-Rezeptzeilen behalten die erforderlichen Tabs. Es werden keine globalen Git-Einstellungen verändert.
