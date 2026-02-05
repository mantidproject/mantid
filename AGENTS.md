Mantid is a c++20 and python 3.11 repository that has a qt gui frontend.
It is configured using cmake and uses conda for dependencies.
The Mantid project provides tools to support the processing of materials-science data.
This data can be gathered from Neutron scattering or Muon spectroscopy experiments or as the result of simulation.

# Dev environment setup

- Install pixi and activate the environment specified in `pixi.toml`:
  - `pixi shell` (or `eval $(pixi shell-hook)` for non-interactive use)
- Dependencies are tracked from a conda metapackage in `conda/recipes/mantid-developer`

# Dev workflow

- Configure:
  - **Linux:** `cmake --preset=linux . && cd build/`
  - **Windows:** `cmake --preset=win64 . && cd build/`
  - **macOS (Apple Silicon):** `cmake --preset=osx-arm64 . && cd build/`
  > _Choose the preset that matches your platform. If you are unsure, check the available presets in `CMakePresets.json` or ask for guidance._
- Build:
  - framework `ninja Framework`
  - gui `ninja workbench`
  - tests `ninja AllTests`
- Run tests
  - unit tests `ctest`
  - system tests `./systemtest`

# PR checklist

- Run `clang-tidy` using the configuration in `.clang-tidy`
- Format using `pre-commit run --all-files`
- Follow PR template in `.github/PULL_REQUEST_TEMPLATE.md`

# Project structure

- `.github/workflows` build configuration for github-actions
- `buildconfig/Jenkins` build configuration for jenkins CI
- `buildconfig/CMake` custom cmake modules
- `conda/recipes` recipes for conda packages
- `dev-docs` developer documentation
- `docs` user documentation
- `Framework` non-gui code
- `instrument` xml files describing facilities, instruments, and sample environments
- `qt` gui code
- `scripts` python code
- `Testing` test data and system test framework
- `tools` small tools for maintaining code and report generation

# When stuck

- Ask a clarifying question, propose a short plan, or open a draft PR with notes
- Look for PRs with similar changes
