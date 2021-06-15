.. _GettingStartedCondaWindows:

================
Conda on Windows
================

* Install `Visual Studio 2019 Community Edition <https://visualstudio.microsoft.com/downloads/>`_.
  * When asked about installation workloads choose ``Desktop development with C++``
  * Under the "Installation details" section verify that the following are checked:
    * ``Windows Universal CRT SDK``
    * The latest Windows 10 SDK
  * If your machine has less than 32GB of memory Mantid may not build. If you have problems change the maximum number of parallel project builds to 1 in Visual Studio in Tools -> Options -> Projects and Solutions -> Build And Run.

* Install `Git <https://git-scm.com/>`_.
  * install the latest version and ensure that Git LFS is checked to be included
  * when the install has completed create a directory for storage of the LFS objects, e.g. ``C:\GitLFSStorage``
  * open up Git Bash and run ``git config --global lfs.storage C:/GitLFSStorage``
  * run ``git lfs install`` to initialize Git LFS. (Note that if you miss this step you may get errors due to the third party libraries not checking out properly. This can be fixed later by running ``git lfs fetch`` and ``git lfs checkout`` in the ``external\src\ThirdParty`` directory.)

* Clone the source code
  * There are 2 common ways developers have been doing this:
    * Using git bash and cloning the codebase by calling `git clone https://www.github.com/mantidproject/mantid` in the directory you want the code to clone to
    * Or by using `GitKraken <https://www.gitkraken.com/>`_.

* Install `Mambaforge <https://github.com/conda-forge/miniforge/releases>`_.
  * Choose the latest version of `Mambaforge-Windows-x86_64.exe`
  * Run your downloaded `Mambaforge-Windows-x86_64.exe` and work through the installer until it finishes.

* Setup the mantid conda environment
  * Open a terminal or powershell with conda enabled there are two ways to do this:
    * Open an Anaconda prompt (Mambaforge)
    * Add the path to your Mambaforge installation to your path. Then you can use conda from the Command Prompt and Powershell
  * Create the mantid conda environment by navigating to your source code directory and running `conda env create -f {SOURCE}/mantid-developer-windows.yml`

* Configure CMake and generate build files
  * Still using the terminal or powershell prompt from the last step and in your source directory.
  * Run `conda activate mantid` to activate your conda environment
  * Create your build directory with `mkdir build` and navigate to it with `cd build`
  * Inside of your build directory run `cmake --preset=win`, if you made your build directory inside your source the command is `cmake {SOURCE} --preset=win`
    * Alternatively if you don't want to have your build folder in your source then pass then call this command (THESE ARE PENDING AND LIKELY TO CHANGE):
    `cmake {SOURCE} -G"Visual Studio 16 2019" -DCMAKE_PREFIX_PATH=${CONDA_PREFIX} -DCONDA_BUILD=true -DHDF5_DIR${CONDA_PREFIX}/Library/cmake/hdf5`

* Compile and Build using Visual Studio UNDER CONSTRUCTION
    - Temp: Open visual studio with `visualstudio.bat` then click build.