# CLion

## Installing CLion

Please note that these instructions only work when using a Ninja
generator from a Windows or Linux operating system.

You will also need to have Visual Studio installed on windows.

If you haven't installed CLion yet do that now, CLion can be installed
from [here](https://jetbrains.com/clion/download/).

## Development environment

CLion needs to use the tools from the development environment you set up by following the [Getting Started](GettingStarted/GettingStarted) guide.
Follow the section below for your environment.
In the rest of this page, `{ENV}` refers to the root directory of that environment.

### Pixi

- `{ENV}` is `/path/to/source/mantid/.pixi/envs/default`.
- To activate the environment in a terminal, navigate to your mantid source directory and run `pixi shell`.

### Conda

- `{ENV}` is the location of your conda environment, e.g. `/path/to/miniforge/envs/mantid-developer`.
- To activate the environment in a terminal, run `conda activate mantid-developer`.

## Opening CLion

The first time you build from CLion, you will most likely need to launch
it from a terminal or command line to make sure you have access to all
the relevant tools.

- On Linux,
  1. Open any terminal
  1. Activate your development environment
  1. Then launch CLion from this terminal with `<CLION_INSTALL>/bin/clion.sh`
- On Windows,
  1. Using your search bar, open the `x64 Native Tools Command Prompt for VS 2019` command prompt
  1. Activate your development environment
  1. Then launch CLion with `<CLION_INSTALL>/bin/clion.bat`

If you get errors about being unable to compile a 'simple test program', then doing the above should fix your issue.

## Setup for a CLion Build

Follow these instructions when the CLion IDE has opened:

To set up your toolchain:

1. Navigate to `File > Settings > Build, Execution, Deployment > Toolchains`

1. Create a new `System` toolchain using the `+` icon and call it `Default`

1. Edit the CMake field to point to the `cmake` installed in your development environment

   ::: {.hlist columns="1"}

   - On Linux: `{ENV}/bin/cmake`
   - On Windows: `{ENV}/Library/bin/cmake.exe`
     :::

1. Edit the Build Tool field to point to the `ninja` installed in your development environment

   ::: {.hlist columns="1"}

   - On Linux: `{ENV}/bin/ninja`
   - On Windows: `{ENV}/Library/bin/ninja.exe`
     :::

1. For the C Compiler and C++ Compiler fields,

   ::: {.hlist columns="1"}

   - On Linux: choose `Let CMake detect`
   - On Windows: direct them both at the same `cl.exe` in your Visual Studio installation, e.g. `C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/bin/Hostx64/x64/cl.exe`
     :::

To set up CMake:

1. Navigate to `File > Settings > Build, Execution, Deployment > CMake`

1. Edit the Build type field by either selecting an option, or typing in a string

   ::: {.hlist columns="1"}

   - On Linux: `Debug`
   - On macOS: `Debug`
   - On Windows: `DebugWithRelRuntime`
     :::

1. Set your Toolchain to be the `Default` toolchain that you just created

1. Set your generator to be `Ninja`

1. Edit your Cmake options to be

   ::: {.hlist columns="1"}

   - On Linux: `--preset=linux`
   - On macOS: `--preset=osx`
   - On Windows: `--preset=win-ninja`
     :::

1. Set the build directory to the `build` directory if it is not the default (you'll need to use the full path if its outside the source directory)

1. The configurations drop-down at the top should show all of the build targets. If not, the CMake project is probably not loaded. Go to `File > Reload CMake Project`. The configurations should be populated

### Additional Build Configuration

This (optional) additional configuration allows one to start Clion from the JetBrains Toolbox or
from a terminal without having to activate the development environment in the terminal.
This is useful when you're working on both Mantid and other projects in CLion simultaneously.

1. Navigate to `File > Settings > Build, Execution, Deployment > CMake`
1. Under `environment`, add new environment variable `CONDA_PREFIX` with value `{ENV}`.
1. Set up the Python interpreter by following the section below for your environment.

#### Pixi Python interpreter

Navigate to `File > Settings > Build, Execution, Deployment > Python Interpreter > Add Interpreter > Add Local Interpreter > Select existing`,
then select the `python` executable in your pixi environment: `{ENV}/bin/python` on Linux, or `{ENV}/python.exe` on Windows.

#### Conda Python interpreter

Navigate to `File > Settings > Build, Execution, Deployment > Python Interpreter > Add Interpreter > Add Local Interpreter > Conda Environment > Use existing environment`,
then select `mantid-developer`.

## Building with CLion

- To build all targets, navigate to `Build > Build All in 'Debug'`. Check that the build command displayed in the Messages window is running the correct cmake executable from your development environment.
- To build a specific target, select it in the configurations drop-down menu and click the hammer icon next to it.

If this fails, you may need to open CLion from a terminal with your development environment activated.

### Activating the environment in CLion terminals

It is also useful to have your terminals in CLion to run with your development environment.
In your `home` directory create a file named `.clionrc` and open it in your favourite text editor, adding the lines for your environment from the sections below.

#### Pixi `.clionrc`

```sh
source ~/.bashrc
eval "$(pixi shell-hook --manifest-path /path/to/source/mantid --frozen)"
```

#### Conda `.clionrc`

```sh
source ~/.bashrc
source ~/miniforge/bin/activate mantid-developer
```

#### Using `.clionrc` in CLion terminals

1. Start CLion using the above steps

1. Navigate to `File > Settings > Tools > Terminal`

1. To the end of the `Shell path` option, add `--rcfile ~/.clionrc`

## Debugging with CLion

To debug workbench, you'll need to edit the `workbench` CMake Application configuration.

1. Set the executable to be the `python` executable in your development environment:

   ::: {.hlist columns="1"}

   - On Linux & macOS: `{ENV}/bin/python`
   - On Windows: `{ENV}/python.exe`
     :::

1. Set the program arguments:

   ::: {.hlist columns="1"}

   - On Linux, macOS and Windows: `-m workbench --single-process`
     :::

1. Set the working directory:

   ::: {.hlist columns="1"}

   - All OS: `{ENV}/bin/`
     :::

1. Set any relevant environment variables:

   ::: {.hlist columns="1"}

   - On macOS: `PYTHONPATH=${PYTHONPATH}:/full/path/to/build/bin/`
     :::

The `--single-process` flag is necessary for debugging. See the [Running Workbench](RunningWorkbench) documentation for more information.

You should now be able to set breakpoints and start debugging by clicking the bug icon.

```{include} macos-opengl-version-warning.md
```
