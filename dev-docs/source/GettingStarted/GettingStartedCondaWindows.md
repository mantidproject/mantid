# Develop with conda on Windows

## Install [Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/downloads/)

- When asked about installation workloads choose `Desktop development with C++`

- Under the "Installation details" section verify that the following are checked (they should already be checked by default):

  - The latest `Windows 11 SDK`
  - `MSVC v143 - VS 2022 C++ x64/x86 build tools (latest)`

- If your machine has less than 32GB of memory Mantid may not build.
  If you have problems change the maximum number of parallel project builds to 1 in Visual Studio in Tools -> Options -> Projects and Solutions -> Build And Run.

## Install [Git](https://git-scm.com/)

- Install the latest version of Git, and ensure git bash was installed and the git executable location was added to your PATH, if you didn't do this as part of your installation you can do this [manually](<https://docs.microsoft.com/en-us/previous-versions/office/developer/sharepoint-2010/ee537574(v=office.14)#to-add-a-path-to-the-path-environment-variable>).
- We no longer need Git LFS as conda handles the dependencies that used to be in the third party directory.

## Turn off Python association in Windows

- Because of the way Windows associates Python files with it's Windows Store version of python we need to turn off this association.
  If you don't turn this off you will have issues when running the pre-commit framework.
- Navigate to your Settings -> Manage App Execution Aliases, and turn off all Python Aliases.

```{include} CloningMantid.md
```

## Install [Miniforge](https://github.com/conda-forge/miniforge/releases)

- Choose the latest version of `Miniforge3-Windows-x86_64.exe`
- Run your downloaded `Miniforge3-Windows-x86_64.exe` and work through the installer until it finishes. In order to make it easier later on, check the box that adds conda to your path.

## Setup the mantid conda environment

```{include} MantidDeveloperCondaSetup.md
```

## Setup the mantid pixi environment

```{include} MantidDeveloperPixiSetup.md
```

## Configure CMake and generate build files

- You can configure CMake using an MS Visual Studio or Ninja generator. Choose one of the following:

  - For MS Visual Studio, use the terminal or powershell prompt from the last step.
  - For Ninja, open the `x64 Native Tools Command Prompt for VS 2019` or `x64 Native Tools Command Prompt for VS 2022` from your search bar.

- Navigate to your mantid source directory.

- If not already activated in the previous step, run `conda activate mantid-developer` to activate your conda environment.

- If you want your build directory inside your source directory, run either:

  - `cmake --preset=win-vs` for configuring with Visual Studio 2022, or
  - `cmake --preset=win-vs-2019` for configuring with Visual Studio 2019, or
  - `cmake --preset=win-ninja` for configuring with Ninja.

- Alternatively, if you want to specify a different build directory, run either:

  - `cmake --preset=win-vs -B {BUILD_DIR}`, or
  - `cmake --preset=win-vs-2019  -B {BUILD_DIR}`, or
  - `cmake --preset=win-ninja -B {BUILD_DIR}`

## Compile and Build using MS Visual Studio

- Open visual studio with `visual-studio.bat`, which is found in the build folder, and then click build.
- It's not possible to compile in Debug on Windows with conda libraries, however Release, RelWithDebInfo, and DebugWithRelRuntime for Debugging will compile fine.
- Once in visual studio, the correct target to use as a startup project in visual studio is `workbench`, not `MantidWorkbench`.
  You can then press F5 to start workbench.
- If you want to use your computer while Mantid is compiling, you can enable the following option: `Tools > Options > Projects and Solutions > Build And Run > Run build at low process priority`.

## Compile and Build using Ninja

- From the command line:

  - Navigate to the build directory using the `x64 Native Tools Command Prompt for VS 2019` or `x64 Native Tools Command Prompt for VS 2022` from the previous step.
  - To build Mantid Workbench use: `ninja`
  - To build the unit tests use: `ninja AllTests`

- In Visual Studio:

  - Open Visual Studio with `visual-studio_ninja.bat`, which is found in the build folder
  - Select "win-ninja" from the Configuration dropdown at the top of the screen
  - Select "Switch between solutions and available views" in the Solution Explorer and click on "CMake Targets View"
  - It is recommended that the "When cache is out of date" option is set to "Never run configure step automatically" in Tools->Options->CMake

## Building and debugging with CLion

Please follow the Windows related instructions on [this page](../CLion).

## CMake conda variables

The <span class="title-ref">CONDA_BUILD</span> parameter is used to customise our installation, which is required when we are using the conda-build tool to build and package Mantid.
This option can be passed to CMake on the command line using -DCONDA_BUILD=True.
