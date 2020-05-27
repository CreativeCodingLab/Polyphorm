# Polyphorm
Interactive visualization software tool to analyze dark matter filaments using the Monte Carlo Physarum Machine algorithm.

![title.png](docs/title.png)

## Requirements
- Windows 10 (might run on older)
- Decent GPU, especially in terms of available VRAM (tested on NVIDIA TitanX 12 GB)
- Visual Studio 2017 or 2019 (might be adapted to older versions)
- DirectX 11 installed on system
- DirectXTex library (repo and cofiguration: https://github.com/Microsoft/DirectXTex)

## Build Instructions
- Clone the repo
- Setup build tool (**./builder/**)
  - Make sure that path in **build.bat** points to existing **vcvarsall.bat** (depends on your VS installation version)
  - Run **setup.bat** to add builder into PATH (or set it manually to point to **./builder/bin/**)
- Check that **polyphorm.build** points to the correct directories (especially wrt. the DirectXTex library installation)
- Run `build run polyphorm.build --release` from the root (the build process will produce a short log - if not then something went wrong, make sure that all the paths are configured correctly and the required dependencies exist)

## Quick Manual
The software is launched simply by running **./bin/polyphorm.exe**. The **./bin/config.polyp** file holds most of the settings. The supplied sample dataset is a corpus of 37.6k galaxies from the SDSS catalog described in *Burchett et al. 2020: Mapping the Dark Threads of the Cosmic Web*. The dataset can be changed in the preamble of **main.cpp** (will be freely configurable later).

### Controls
Most of Polyphorm's controls are a part of the UI, including changing the visualization modality and its parameters. The rest is mapped as follows:
- Left/right/middle mouse: rotate/pan/zoom camera
- F1: toggle UI
- F2: reset the simulation
- F3: pause/resume the simulation
- F4: autorotating camera
- F5: export agent data (./bin/export/)
- F6: export field data (./bin/export/)
- F7: toggle continuous screen capture (stored in ./bin/capture)
- F8: reset the field data (maintain agents' state)
- F9: save current visualization state (camera + visual settings)
- F10: load the saved visualization state
- '1': take a single screenshot (stored in ./bin/capture)
- Esc: terminate Polyphorm
