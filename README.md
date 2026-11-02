# linux-kernel-lab

Embedded Linux Kernel development lab for IoT applications.

Dieses Lernprojekt verbindet vorhandene User-Space- und Embedded-Kenntnisse mit Linux-Kernel-Programmierung: Architektur, Kernel-Build, Module, Treiber, Device Tree, Bootkette und schließlich ein IoT-Gateway. Die Inhalte bauen schrittweise aufeinander auf.

## Einstieg

1. [Repository-Struktur und Befehlsregeln](docs/REPOSITORY_STRUCTURE.md) lesen.
2. [Stufe 0: qualifizierte Entwicklungsumgebung und Lab-Anleitung](labs/00-environment/README.md) nachvollziehen; für den laufenden Betrieb die [Start- und Reset-Befehle](labs/00-environment/README.md#standardbetrieb-nach-stufe-0) verwenden.
3. Das [fachliche Nachschlagewerk](docs/Linux_Kernel_und_Embedded_Linux_Nachschlagewerk.md) zum Verstehen und Wiederholen verwenden.

Wir entwickeln unter Ubuntu in WSL2 und testen eigene Kernel in einem separaten QEMU-Gast. **Stufe 0 wurde praktisch abgeschlossen:** KVM-Boot, serielle Konsole, Host-/Gast-Dateiaustausch, persistentes Alpine-System und QCOW2-Rollback wurden qualifiziert. Die dafür erzeugten Images und Prüfreports liegen lokal unter `build/` und werden nicht eingecheckt. Der eigene Kernel-Build gehört zu Stufe 2.

## Repository lokal beziehen

**Umgebung:** Ubuntu/WSL2 · **Shell:** Bash · **Arbeitsverzeichnis:** Benutzerverzeichnis (`~`).

**Voraussetzungen:** Git installiert, Internetzugang, Zielverzeichnis `~/linux-kernel-lab` noch nicht vorhanden.

```bash
cd ~
git clone https://github.com/B0bbyR4y/linux-kernel-lab.git
cd ~/linux-kernel-lab
git status --short --branch
```

**Ergebnis:** Lokale Arbeitskopie unter `~/linux-kernel-lab`; Git zeigt Branch und Arbeitsbaumstatus. Dateien werden durch das Klonen heruntergeladen, es entsteht kein neuer Commit. Bei bereits vorhandener Arbeitskopie dort zunächst den Status prüfen und die vorhandene Arbeit erhalten.

## Dokumentation und Änderungen

Das Nachschlagewerk erklärt Fachwissen. Lab-READMEs beschreiben die Durchführung. Code, Konfiguration und zugehörige Anleitung werden gemeinsam geändert. Im Repository wird die Dokumentation maßgeblich gepflegt; exportierte Kopien können ältere Stände enthalten.

Die Grundstruktur wächst mit den Übungen. Externe Quellen, Downloads und Build-Ergebnisse werden nicht eingecheckt. Textdateien verwenden UTF-8 und LF. Commit-Scopes bezeichnen betroffene Bereiche, etwa `docs`, `environment` oder `driver`, nicht Lernphasen.
