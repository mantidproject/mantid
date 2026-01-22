# Develop with conda on Linux

## Install Git

- 

  [Git](https://git-scm.com/)

  :   - On Debian based systems (Ubuntu) install using apt with the
        following command: `sudo apt install git`
      - On CentOS based systems (RHEL) install using yum with the
        following command: `sudo yum install git`

## Clone the mantid source code

- **Important**: If you have any existing non-conda mantid development
  environments, do not re-use the source and build directories for your
  conda environment. We recommend that you clone a new instance of the
  source and keep separate build directories to avoid any cmake
  configuration problems.

- 

  Obtain the mantid source code by either:

  :   - Using git in a terminal and cloning the codebase by calling
        `git clone git@github.com:mantidproject/mantid.git` in the
        directory you want the code to clone to. You will need to follow
        this [Github
        guide](https://docs.github.com/en/github/authenticating-to-github/connecting-to-github-with-ssh)
        to configure SSH access. You may want to follow this [Git setup
        guide](https://git-scm.com/book/en/v2/Getting-Started-First-Time-Git-Setup)
        to configure your git environment.
      - Or using [GitKraken](https://www.gitkraken.com/).

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

Create `mantid-developer` conda environment by following the steps
below:

- First create a new conda environment and install the
  `mantid-developer` conda metapackage. You will normally want the
  nightly version, specified below by adding the label
  `mantid/label/nightly`. Here we have named the conda environment
  `mantid-developer` for consistency with the rest of the documentation
  but you are free to choose any name. The `neutrons` channel is also
  required for PyStoG.

  ``` sh
  mamba create -n mantid-developer mantid/label/nightly::mantid-developer -c neutrons
  ```

- Then activate the conda environment with

  ``` sh
  mamba activate mantid-developer
  ```

- It is important that you regularly update your `mantid-developer`
  environment so that the dependencies are consistent with those used in
  production. With your `mantid-developer` environment activated, run
  the following command:

  ``` sh
  mamba update -c mantid/label/nightly -c neutrons --all
  ```

See `packaging <Packaging>` for more information on this topic.

## Setup the mantid pixi environment

Pixi allows for reducible environments using conda packages. It manages
environments for all supported platforms together to help keep them in
line with each other.

1.  Follow the [pixi installation
    instructions](https://pixi.sh/latest/installation/) for your
    platform

2.  Install the dependencies that are listed in the lockfile with

    ``` sh
    pixi install --frozen
    ```

3.  For in source builds, prefix all commands with
    <span class="title-ref">pixi run</span>. For example

    ``` sh
    pixi run pre-commit install
    ```

4.  For out-of source build, prefix all commands with

    ``` sh
    pixi run --manifest-path path/to/source cmake --build .
    ```

5.  Whenever `pixi.toml` changes, `pixi.lock` will update next time pixi
    is run.

When there are issues, one can often fix them by removing the
`pixi.lock` and recreating it with any pixi command.

To not have to prefix all of the commands with `pixi run`, either use
`pixi shell` (don't forget to `exit` when leaving the directory) or use
[direnv](https://direnv.net/). Once
<span class="title-ref">direnv</span> is properly installed and in your
path, create a file `.envrc` in your source tree with the contents

``` sh
watch_file pixi.lock
eval "$(pixi shell-hook --frozen --change-ps1 false)"
```

For out of source builds, the build directory should have a `.envrc`
with the contents

``` sh
watch_file /path/to/source/mantid/pixi.lock
eval "$(pixi shell-hook --manifest-path=/path/to/source/mantid/ --frozen --change-ps1 false)"
export PYTHONPATH=/path/to/build/bin/:$PYTHONPATH
```

The last line is necessary to get the build results into the python
path.

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

Please follow the Linux related instructions on `this page <clion-ref>`.

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
