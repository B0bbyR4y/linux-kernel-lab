# Gemeinsame Skripte

Hier liegen künftig wiederverwendbare Werkzeuge für Einrichtung, Build, QEMU-Start und Prüfungen. Der erste Aufbau enthält noch keine ausführbaren Hilfsskripte; die Bestandsaufnahme steht in [Stufe 0](../labs/00-environment/README.md).

Jedes neue Skript wird zusammen mit seiner Verwendung dokumentiert: Umgebung, Aufruf, Arbeitsverzeichnis, Voraussetzungen, Eingaben, Ausgaben und notwendige Rechte. Ein Skript soll Pfade relativ zu seinem eigenen Standort oder zu einem ausdrücklich angegebenen Projektpfad auflösen. Erzeugte Dateien gehören unter `build/`, heruntergeladene Dateien unter `downloads/`.

Nur für eine einzelne Übung benötigte Skripte bleiben in deren Lab-Verzeichnis. Gemeinsame Shellskripte verwenden LF und eine passende Shebang; ihre ausführbaren Dateirechte werden in Git gesetzt. Die [Repository-Struktur](../docs/REPOSITORY_STRUCTURE.md) enthält die allgemeinen Regeln.
