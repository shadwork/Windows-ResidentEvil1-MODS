![SHDP Mods](images/mod_launcher_logo.jpg)

# Resident Evil 1 SHDP Mods

A collection of mods for Resident Evil 1 PC (Classic REbirth / RE1 HD).

## Mods

| Mod | Description |
|-----|-------------|
| [Bighead](src/mod_bighead/) | Scales the head bone of player and enemies to make their heads bigger. |
| [Doorskip](src/mod_doorskip/) | Skips the door-opening transition animations that mask loading times. |
| [Healthbar](src/mod_healthbar/) | Displays HP bars above player and enemies using Add_line rendering. |
| [Mod Launcher](src/mod_launcher/) | Proxy DLL that loads multiple mod DLLs from mods_list.ini. |
## Setup

1. Copy each mod folder into the RE1 game directory (next to `Bio.exe`)
2. Each `mod_*` folder is auto-detected by Classic REbirth
3. Use **Mod Launcher** (`mod_launcher`) to run multiple mods simultaneously
4. Configure mods with **mod_config.exe** in the `mod_launcher` folder

## Architecture

Classic REbirth (`ddraw.dll`) loads one mod folder per session.
**Mod Launcher** acts as a proxy - it reads `mods_list.ini` from the
game root and loads multiple mod DLLs, forwarding Modsdk callbacks to all.

Mods that hook the same address (`0x440A10` render queue clear) use
**hook chaining** - each mod detects if another already hooked the address
and calls the previous hook before adding its own logic.

## Build Requirements

- Visual Studio 2019 (v142 toolset)
- NASM assembler (for import.asm trampolines)
- Windows 10 SDK

Each mod has a `build.bat` that compiles via MSBuild or cl.exe.

## Download

Get the latest release from the [Releases](../../releases/latest) page.

The release archive contains all mods compiled as optimized Release DLLs,
packaged with their manifest and description files. Extract the desired
mod folders into the game directory next to `Bio.exe` and they will be
auto-detected by Classic REbirth on next launch.

