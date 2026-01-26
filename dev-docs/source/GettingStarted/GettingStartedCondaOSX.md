# Develop with conda on MacOS

## Install [Git](https://git-scm.com/)

- On MacOS install using brew with the following command:
  `brew install git`

```{include} CloningMantid.md
```

## Install [Miniforge](https://github.com/conda-forge/miniforge/releases)

- Choose the latest version of `Miniforge3-MacOSX-x86_64.sh` for intel
  based Macs or for the new arm versions use
  `Miniforge3-MacOSX-arm64.sh`
- Run your downloaded script from the terminal using
  `bash Miniforge3-MacOSX-x86_64.sh` or
  `bash Miniforge3-MacOSX-arm64.sh` depending on your downloaded
  variant.
- If it asks whether or not you want to initialise conda with conda
  init, choose to do so.
- Restart your terminal.

## Setup the mantid conda environment

```{include} MantidDeveloperCondaSetup.md
```

## Setup the mantid pixi environment

```{include} MantidDeveloperPixiSetup.md
```

## Configure CMake and generate build files

- Still using the terminal.

- If not already activated in the previous step, run
  `conda activate mantid-developer` to activate your conda environment.

- Navigate back to your mantid source directory using `cd mantid` if you
  used the default name during cloning from git.

-

  Inside of your mantid source directory run `cmake --preset=osx`

  :   - Alternatively if you don't want to have your build folder in
        your mantid source then pass the `-B` argument, overriding the
        preset, to cmake:
        `cmake {PATH_TO_SOURCE} --preset=osx -B {BUILD_DIR}`

## How to build

- Navigate to the build directory.
- To build Mantid Workbench use: `ninja`
- To build Unit Tests use: `ninja AllTests`

## CMake conda variables

The `CONDA_BUILD` parameter is used to customise our installation, which
is required when we are using the conda-build tool to build and package
Mantid. This option can be passed to CMake on the command line using
`-DCONDA_BUILD=True`.

## Running Workbench

To run workbench from the commandline, ensure you conda environment is
activated, and bin (in the build directory) is added to the python
paths.

``` sh
export PYTHONPATH="${PYTHONPATH}:replace-with-full-file-path-to-bin"
workbench
```

```{note}
macOS developers will see the warning from Qt in the terminal:

`An OpenGL surfcace format was requested that is either not version 3.2 or higher or a not Core Profile.`
`Chromium on macOS will fall back to software rendering in this case.`
`Hardware acceleration and features such as WebGL will not be available.`

It can be safely ignored but is present as a reminder of some deprecated
OpenGL functionality being used. It is only visible to developers and
the spelling mistake is real.
```
