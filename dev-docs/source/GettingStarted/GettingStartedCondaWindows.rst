.. _GettingStartedCondaWindows:

=============================
Develop with Conda on Windows
=============================

Install `Visual Studio 2019 Community Edition <https://visualstudio.microsoft.com/downloads/>`_
-----------------------------------------------------------------------------------------------

* When asked about installation workloads choose ``Desktop development with C++``
* Under the "Installation details" section verify that the following are checked:

    * ``Windows Universal CRT SDK``
    * The latest ``Windows 10 SDK``

* If your machine has less than 32GB of memory Mantid may not build. If you have problems change the maximum number of parallel project builds to 1 in Visual Studio in Tools -> Options -> Projects and Solutions -> Build And Run.

Install `Git <https://git-scm.com/>`_
-------------------------------------

* Install the latest version of Git, and ensure git bash was installed and the git executable location was added to your PATH, if you didn't do this as part of your installation you can do this `manually <https://docs.microsoft.com/en-us/previous-versions/office/developer/sharepoint-2010/ee537574(v=office.14)#to-add-a-path-to-the-path-environment-variable>`_.
* We no longer need Git LFS as conda handles the dependencies that used to be in the third party directory.

Clone the source code
---------------------

* There are 2 common ways developers have been doing this.

    * Using git bash and cloning the codebase by calling ``git clone git@github.com:mantidproject/mantid.git`` in the directory you want the code to clone to. This sets you up with accessing the remote repository via SSH so make sure to setup git properly using this `startup guide <https://git-scm.com/book/en/v2/Getting-Started-First-Time-Git-Setup>`_ and ensure your ssh key is setup using this `guide to Github with SSH <https://docs.github.com/en/github/authenticating-to-github/connecting-to-github-with-ssh>`_.
    * Or by using `GitKraken <https://www.gitkraken.com/>`_.

Install `Mambaforge <https://github.com/conda-forge/miniforge/releases>`_
-------------------------------------------------------------------------

* Choose the latest version of ``Mambaforge-Windows-x86_64.exe``
* Run your downloaded ``Mambaforge-Windows-x86_64.exe`` and work through the installer until it finishes.

Setup the mantid conda environment
----------------------------------

* Open a terminal or powershell with conda enabled there are two ways to do this:

    * Open an Anaconda prompt (Mambaforge).
    * Add the path to your Mambaforge installation to your path. Then you can use conda from the Command Prompt and Powershell.

* Create the mantid conda environment by navigating to your source code directory and running ``conda env create -f {SOURCE}/mantid-developer-windows.yml``

Configure CMake and generate build files
----------------------------------------

* Still using the terminal or powershell prompt from the last step and in your source directory.
* Run ``conda activate mantid`` to activate your conda environment.
* Navigate back to your source directory using ``cd mantid`` if you used the default name during cloning from git.
* Inside of your source directory run ``cmake --preset=win``

    * Alternatively if you don't want to have your build folder in your source then pass the ``-B`` argument, overriding the preset, to cmake: ``cmake {SOURCE} --preset=win -B {BUILD_DIR}``


Compile and Build using Visual Studio
----------------------------------------------------------

* Open visual studio with ``visualstudio.bat`` then click build.