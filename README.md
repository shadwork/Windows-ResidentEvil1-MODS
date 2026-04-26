![Resident Evil 1 Mods](images/mod_launcher_logo.jpg)

# Windows Resident Evil 1 Mods

A collection of Windows mods for Resident Evil 1 PC using the Classic REbirth
mod SDK / loader workflow.

## Folder Layout

- `images/` - screenshots and project images used by this README.
- `src/` - source folders for each mod plus the release packaging script.
- `src/make_release.ps1` - builds a release archive from mod folders that contain a Release DLL, `manifest.txt`, and `description.txt`.

## Mods

| Folder | Mod | Description |
|--------|-----|-------------|
| [`src/mod_bighead`](src/mod_bighead/) | Bighead | Scales the head bone of the player and enemies to make heads bigger. |
| [`src/mod_doorskip`](src/mod_doorskip/) | Doorskip | Skips door-opening transition animations and shortens room transitions. |
| [`src/mod_healthbar`](src/mod_healthbar/) | Healthbar | Draws HP bars above the player and enemies. |
| [`src/mod_hud`](src/mod_hud/) | HUD | Adds a configurable real-time overlay with inventory icons, ammo counts, equipped weapon highlight, and optional health / weapon panels. |
| [`src/mod_launcher`](src/mod_launcher/) | Mod Launcher | Proxy mod that loads multiple mod DLLs listed in `mods_list.ini`. |

## Download Latest Release

For normal use, you do not need to build the source code.

1. Open the [latest GitHub release](../../releases/latest).
2. Download the release `.zip` archive from the Assets section.
3. Extract the archive.
4. Copy the desired `mod_*` folder into your Resident Evil 1 game directory,
   next to `Bio.exe`.
5. Start the game through Classic REbirth.

The extracted mod folder should contain `manifest.txt`, `description.txt`, and
the mod DLL. Keep those files together inside the same `mod_*` folder.

## Installation

Classic REbirth normally loads one mod folder at a time. To install a single
mod, copy that `mod_*` folder into the Resident Evil 1 game directory next to
`Bio.exe`.

To run multiple mods together, install `mod_launcher` and configure
`mods_list.ini` with the DLL paths you want to load. The launcher forwards the
Modsdk callbacks to each loaded mod.

## Configuration

- `mod_launcher/mods_list.ini` lists the DLLs loaded by Mod Launcher.
- `mod_hud/mod_hud.ini` controls HUD position and enabled panels. If it is
  missing, the HUD mod creates a default config next to `mod-hud.dll` on first
  run.

## Building

Requirements:

- Visual Studio 2019 with the v142 C++ toolset.
- Windows 10 SDK.
- NASM, required by mods that build `import.asm` trampolines.

Each mod folder has its own `build.bat`. Run it from that mod folder to build
the Release DLL.

## Release Packaging

Run `src/make_release.ps1` from the `src` directory after building the desired
mods. The script stages each mod folder that has:

- `Release/*.dll`
- `manifest.txt`
- `description.txt`

It then creates a timestamped `Release-*.zip` containing only the runtime files
needed by Classic REbirth, plus root-level `.exe` helpers such as
`mod_config.exe` when present.
