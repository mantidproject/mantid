# CLion

::: {.contents local=""}
:::

## Installing CLion

Please note that these instructions only work when using a Ninja
generator from a Windows or Linux operating system.

You will also need to have Visual Studio installed on windows.

If you haven't installed CLion yet do that now, CLion can be installed
from [here](https://jetbrains.com/clion/download/).

## Opening CLion

The first time you build from CLion, you will most likely need to launch
it from a terminal or command line to make sure you have access to all
the relevant tools.

- On Linux,
  1.  Open any terminal
  2.  Run `conda activate mantid-developer`
  3.  Then launch CLion from this terminal with
      `<CLION_INSTALL>/bin/clion.sh`
- On Windows,
  1.  Using your search bar, open the
      `x64 Native Tools Command Prompt for VS 2019` command prompt
  2.  Run `conda activate mantid-developer`
  3.  Then launch CLion with `<CLION_INSTALL>/bin/clion.bat`

If you get errors about being unable to compile a 'simple test program',
then doing the above should fix your issue.

## Setup for a CLion Build

Follow these instructions when the CLion IDE has opened:

To set up your toolchain:

1.  Navigate to
    `File > Settings > Build, Execution, Deployment > Toolchains`

2.  Create a new `System` toolchain using the `+` icon and call it
    `Default`

3.  Edit the CMake field to point to your conda installed `cmake`

    ::: {.hlist columns="1"}
    - On Linux: `/path/to/miniforge/envs/mantid-developer/bin/cmake`
    - On Windows:
      `/path/to/miniforge/envs/mantid-developer/Library/bin/cmake.exe`
    :::

4.  Edit the Build Tool field to point to your conda installed `ninja`

    ::: {.hlist columns="1"}
    - On Linux: `/path/to/miniforge/envs/mantid-developer/bin/ninja`
    - On Windows:
      `/path/to/miniforge/envs/mantid-developer/Library/bin/ninja.exe`
    :::

5.  For the C Compiler and C++ Compiler fields,

    ::: {.hlist columns="1"}
    - On Linux: choose `Let CMake detect`
    - On Windows: direct them both at the same `cl.exe` in your Visual
      Studio installation, e.g.
      `C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/bin/Hostx64/x64/cl.exe`
    :::

To set up CMake:

1.  Navigate to `File > Settings > Build, Execution, Deployment > CMake`

2.  Edit the Build type field by either selecting an option, or typing
    in a string

    ::: {.hlist columns="1"}
    - On Linux: `Debug`
    - On Windows: `DebugWithRelRuntime`
    :::

3.  Set your Toolchain to be the `Default` toolchain that you just
    created

4.  Set your generator to be `Ninja`

5.  Edit your Cmake options to be

    ::: {.hlist columns="1"}
    - On Linux: `--preset=linux`
    - On Windows: `--preset=win-ninja`
    :::

6.  Set the build directory to the `build` directory if it is not the
    default (you'll need to use the full path if its outside the source
    directory)

7.  The configurations drop-down at the top should show all of the build
    targets. If not, the CMake project is probably not loaded. Go to
    `File > Reload CMake Project`. The configurations should be
    populated

### Additional Build Configuration

This (optional) additional configuration allows one to start Clion from
the JetBrains Toolbox or from a terminal without having to activate the
conda environment in the terminal. This is useful when you're working on
both Mantid and other projects in CLion simultaneously.

1.  Navigate to `File > Settings > Build, Execution, Deployment > CMake`
2.  Under `environment`, add new environment variable `CONDA_PREFIX`
    with value `/path/to/miniforge/envs/mantid-developer`.
3.  Navigate to
    `File > Settings > Build, Execution, Deployment > Python Interpreter > Add Interpreter > Add Local Interpreter > Conda Environment > Use existing environment`,
    then select `mantid-developer`.

## Building with CLion

- To build all targets, navigate to `Build > Build All in 'Debug'`.
  Check that the build command displayed in the Messages window is
  running the correct cmake executable from your conda installation.
- To build a specific target, select it in the configurations drop-down
  menu and click the hammer icon next to it.

If this fails, you may need to open CLion from a terminal with your
conda environment activated.

It is also useful to have your terminals in CLion to run with this
environment:

1.  In your `home` directory create a file named `.clionrc` and open in
    your favourite text editor, adding these lines:

    ``` sh
    source ~/.bashrc
    source ~/miniforge/bin/activate mantid-developer
    ```

2.  Start CLion using the above steps

3.  Navigate to `File > Settings > Tools > Terminal`

4.  To the end of the `Shell path` option, add `--rcfile ~/.clionrc`

## Debugging with CLion

To debug workbench, you'll need to edit the `workbench` CMake
Application configuration.

1.  Set the executable to be the `python` executable in your conda
    installation:

    ::: {.hlist columns="1"}
    - On Linux & macOS:
      `/path/to/miniforge/envs/mantid-developer/bin/python`
    - On Windows: `/path/to/miniforge/envs/mantid-developer/python.exe`
    :::

2.  Set the program arguments:

    ::: {.hlist columns="1"}
    - On Linux, macOS and Windows: `-m workbench --single-process`
    :::

3.  Set the working directory:

    ::: {.hlist columns="1"}
    - All OS: `path/to/miniforge/envs/md/bin/`
    :::

4.  Set any relevant environment variables:

    ::: {.hlist columns="1"}
    - On macOS: `PYTHONPATH=${PYTHONPATH}:/full/path/to/build/bin/`
    :::

The `--single-process` flag is necessary for debugging. See the
`Running Workbench <RunningWorkbench>` documentation for more
information.

You should now be able to set breakpoints and start debugging by
clicking the bug icon.

::: {.note}
::: {.title}
Note
:::

macOS developers will see the warning from Qt in the terminal:

`An OpenGL surfcace format was requested that is either not version 3.2 or higher or a not Core Profile.`
`Chromium on macOS will fall back to software rendering in this case.`
`Hardware acceleration and features such as WebGL will not be available.`

It can be safely ignored but is present as a reminder of some deprecated
OpenGL functionality being used. It is only visible to developers and
the spelling mistake is real.
:::
