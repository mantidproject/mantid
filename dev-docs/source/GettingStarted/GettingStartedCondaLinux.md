# Develop with conda on Linux

## Install Git

-

  [Git](https://git-scm.com/)

  :   - On Debian based systems (Ubuntu) install using apt with the
        following command: `sudo apt install git`
      - On CentOS based systems (RHEL) install using yum with the
        following command: `sudo yum install git`

```{include} CloningMantid.md
```

## Install [Miniforge](https://github.com/conda-forge/miniforge/releases)

- Choose the latest version of `Miniforge3-Linux-x86_64.sh`
- Run your downloaded script from the terminal using
  `bash Miniforge3-Linux-x86_64.sh`. If it asks whether or not you want
  to initialise conda with conda init, choose to do so.
- Restart your terminal.

## (ILL) Setup proxy

- Open ~/.condarc.
- Add the following lines :

``` text
proxy_servers:
  http: http://proxy.ill.fr:8888
  https: http://proxy.ill.fr:8888
```

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

  Inside of your mantid source directory run `cmake --preset=linux`

  :   - Alternatively if you don't want to have your build folder in
        your mantid source then pass the `-B` argument, overriding the
        preset, to cmake:
        `cmake {PATH_TO_SOURCE} --preset=linux -B {BUILD_DIR}`

## How to build

- Navigate to the build directory.
- To build Mantid Workbench use: `ninja`
- To build Unit Tests use: `ninja AllTests`

## Building and debugging with CLion

Please follow the Linux related instructions on [this page](../CLion).

## CMake conda variables

The <span class="title-ref">CONDA_BUILD</span> parameter is used to
customise our installation, which is required when we are using the
conda-build tool to build and package Mantid. This option can be passed
to CMake on the command line using -DCONDA_BUILD=True.

## Debugging with <span class="title-ref">gdb</span>

If you wish to use `gdb` to debug Mantid, then you can use:

`./build/bin/launch_mantidworkbench.sh --debug`

This will start `gdb` with the appropriate command, you can then use the
run command `r` within `gdb` to start Mantid. If you wish to launch
Workbench more directly then you will need to include the
`--single-process` flag for your python process, otherwise you will not
be able to use most breakpoints that you set. For example:

`gdb --args python build/bin/workbench --single-process`

Some useful commands for using `gdb`:

- `r` - Run command
- `c` - Continue (e.g. after stopping at a breakpoint)
- `b my_file.cpp:15` - Insert a breakpoint in `my_file.cpp` at line 15
- `Ctrl+C` - Pause execution (e.g. if you want to insert a breakpoint)
- `l` - Shows source code around the point where you're paused
- `print myVariable` - Show value of a local variable
