# <img src="Anchor_wText.png" height="100">

![](hitman_anchor.gif)
## This mod is an alternative to the default ZHM freecam mod, and allows you to anchor the camera to any entity!
- ### A fork of the default [freecam mod](https://github.com/OrfeasZ/ZHMModSDK/tree/master/Mods/FreeCam) by [Orfeasz](https://github.com/OrfeasZ)
- ### Icon and gif provided by [ThatObserver](https://www.twitch.tv/thatobserver) :D
- ### Project made using the ZHM Mod SDK and the [ZHM Mod Generator](https://zhmmod.nofate.me/)

## Usage
- ### This mod is designed as an alternative to the default freecam mod, please disable the default if using this mod
- ### The camera will anchor the entity in the center of the screen

## Controls
| Keys                             | Action                                        |
|----------------------------------|-----------------------------------------------|
| K                                | Toggle freecam                                |
| F3                               | Toggle camera lock and 47 input               |
| Ctrl + W/S                       | Change FOV                                    |
| Ctrl + A/D                       | Roll camera                                   |
| Alt + W/S                        | Change camera speed                           |
| Space + Q/E                      | Change camera height                          |
| Space +  W/S                     | Move camera forward/backward on axis          |
| Shift                            | Increase speed while holding                  |
| Ctrl + F9                        | Anchor Camera to entity                       |
| Shift + (8,9,0)                  | Add to offset on (x,y,z) axis                 |
| Ctrl + (8,9,0)                   | Subtract to offset on (x,y,z) axis            |
| L                                | Unanchor from object (does not close freecam) |
| ;                                | Reset Offset                                  |
| [ / ]                            | Decrease/Increase offset step                 |

## Installation Instructions

1. Download the latest version of [ZHMModSDK](https://github.com/OrfeasZ/ZHMModSDK) and install it.
2. Download the latest version of `EntityPathTrace` and copy it to the ZHMModSDK `mods` folder (e.g. `C:\Games\HITMAN 3\Retail\mods`).
3. Run the game and once in the main menu, press the `~` key (`^` on QWERTZ layouts) and enable `EntityPathTrace` from the menu at the top of the screen.
4. Enjoy!

## Build It Yourself!

### 1. Clone this repository locally with all submodules.

You can either use `git clone --recurse-submodules` or run `git submodule update --init --recursive` after cloning.

### 2. Install Visual Studio (any edition).

Make sure you install the C++ and game development workloads.

### 3. Open the project in your IDE of choice.

See instructions for [Visual Studio](https://github.com/OrfeasZ/ZHMModSDK/wiki/Setting-up-Visual-Studio-for-development) or [CLion](https://github.com/OrfeasZ/ZHMModSDK/wiki/Setting-up-CLion-for-development).
