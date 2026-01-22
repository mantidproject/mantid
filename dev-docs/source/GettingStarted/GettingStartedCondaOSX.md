# Develop with conda on MacOS

## Install [Git](https://git-scm.com/)

- On MacOS install using brew with the following command:
  `brew install git`

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

See `Packaging` for more information on this topic.

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
