# M2609 – Horloge ininterrompue (DAB + NTP + GPS)

[English version](README.md)

## Vue d’ensemble

Ce projet implémente une horloge robuste sur Arduino avec **plusieurs sources de temps** et une stratégie de priorité claire :

Priorité des sources : **DAB+ > NTP > GPS**.

Le système évalue en continu les sources, ignore les données incohérentes (±10 secondes), maintient l’heure via une horloge logicielle, et offre le pilotage via :

- une interface **LCD 8x2 + boutons + encodeur rotatif**,
- une **interface web / API REST** embarquée,
- une documentation **Doxygen** générée.

## Périmètre fonctionnel (cahier des charges)

Objectifs implémentés :

- Récupération date/heure via **NTP** (Wi-Fi)
- Récupération date/heure via **DAB+**
- Récupération date/heure via **GPS**
- Priorité des sources : **DAB > NTP > GPS**
- Rejet d’une source incohérente au-delà de **±10 s**
- Affichage date/heure sur **LCD 8x2**
- Affichage des erreurs de synchronisation sur **LCD 8x2**
- Acquittement des erreurs via **SET**
- Acquittement automatique après **60 s** si au moins une source est valide
- **Mode sélection de source** avec CFG/SET/encodeur
- **Mode configuration manuelle** avec appuis longs/courts et logique sauvegarde/annulation

## Fonctionnalités principales

- **Coordination de synchronisation** (`TimeCoordinator`)
  - interroge les sources périodiquement,
  - vérifie la cohérence inter-sources,
  - choisit la meilleure source valide selon la priorité,
  - maintient une horloge logicielle entre deux synchronisations.
- **Abstraction des sources** (`TimeSource`)
  - `DABTimeSource`
  - `NTPTimeSource`
  - `GPSTimeSource`
- **Modèle d’erreurs et gestion** (`SyncErrors`)
  - erreurs globales et par source,
  - acquittement manuel et automatique (timeout).
- **Contrôle local + distant**
  - UI matérielle locale (`UiController`),
  - contrôle réseau via `RestApiServer`.

## Matériel et plateforme

- Carte cible principale : **Arduino UNO R4 WiFi**
- Également configuré dans les tâches : compilation/upload **UNO R4 Minima**
- Périphériques principaux utilisés :
  - shield DAB (SPI)
  - module GPS (SoftwareSerial)
  - LCD 8x2
  - expander I/O MCP23017 (encodeur + CFG + lignes LCD)
  - bouton SET (GPIO)

Le mapping des broches est centralisé dans `src/platform/PinDefinitions.hpp`.

## Architecture logicielle

Structure principale :

- `DabGps.ino` : point d’entrée (`setup()` / `loop()`)
- `src/time/` : implémentations de sources + coordination
- `src/ui/` : machine à états LCD et commandes
- `src/network/` : Wi-Fi + serveur web/API REST embarqué
- `src/core/` : erreurs et journalisation/notifications
- `src/platform/` : définitions de broches
- `src/config/` : en-têtes de configuration locale/secrets

## API REST et interface web

Le serveur embarqué (port par défaut `80`) expose :

- `GET /` → interface web embarquée
- `GET /status` → statut JSON (état sources + état horloge manuelle)
- `POST /toggle-source` → active/désactive une source
- `POST /set-time` → définit la date/heure manuellement

L’interface web permet d’activer/désactiver les sources et de régler l’heure depuis un navigateur sur le même réseau.

## Compilation et upload

### Tâches VS Code (déjà configurées)

- `Compile Arduino Code (WiFi)`
- `Upload Arduino Code (WiFi)`
- `Compile Arduino Code (minima)`
- `Upload Arduino Code (minima)`

### Arduino CLI (équivalent)

Compilation pour UNO R4 WiFi :

```bash
arduino-cli compile -b arduino:renesas_uno:unor4wifi --build-path ./.build
```

Upload pour UNO R4 WiFi (port exemple) :

```bash
arduino-cli upload -p COM11 -b arduino:renesas_uno:unor4wifi --input-dir ./.build
```

## Configuration

Créer `src/config/SECRETS.hpp` avec vos identifiants locaux :

```cpp
constexpr char WIFI_SSID[] = "votre-ssid";
constexpr char WIFI_PASSWORD[] = "votre-mot-de-passe";
```

`src/config/SECRETS.hpp` est ignoré par Git (`.gitignore`).

## Documentation Doxygen

Générer la doc avec :

```bash
doxygen Doxyfile
```

PDF optionnel (batch Windows du projet) :

```bash
./docs/latex/make.bat
```

La page d’accueil Doxygen est configurée pour utiliser `README.md`.

## Améliorations possibles

- Portail captif pour configuration dynamique :
  - fuseau horaire,
  - identifiants Wi-Fi,
  - serveur NTP,
  - activation/désactivation des sources.
- Extension réveil utilisant une station DAB pour diffuser de la musique.
- Synchronisation inter-Arduino via Bluetooth.

## Auteur

David GOLETTA  
CPNV – M2609
