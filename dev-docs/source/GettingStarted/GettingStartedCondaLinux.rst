.. _GettingStartedCondaLinux:

===========================
Develop with Conda on Linux
===========================

Install `Git <https://git-scm.com/>`_
-------------------------------------
* On Debian based systems (Ubuntu) install using apt with the following command: ``sudo apt-get install git``
* On CentOS based systems (RHEL) install using yum with the following command: ``sudo yum install git``

Clone the mantid source code
----------------------------
* There are 2 common ways developers have been doing this.

    * Using git in the terminal and cloning the codebase by calling ``git clone git@github.com:mantidproject/mantid.git`` in the directory you want the code to clone to. This sets you up with accessing the remote repository via SSH so make sure to setup git properly using this `startup guide <https://git-scm.com/book/en/v2/Getting-Started-First-Time-Git-Setup>`_ and ensure your ssh key is setup using this `guide to Github with SSH <https://docs.github.com/en/github/authenticating-to-github/connecting-to-github-with-ssh>`_.
    * Or by using `GitKraken <https://www.gitkraken.com/>`_.

Install `Mambaforge <https://github.com/conda-forge/miniforge/releases>`_
-------------------------------------------------------------------------
* Choose the latest version of ``Mambaforge-Linux-x86_64.sh``
* Run your downloaded script from the terminal using ``bash Mambaforge-Linux-x86_64.sh``. If it asks whether or not you want to initialise conda with conda init, choose to do so.
* Restart your terminal.

(ILL) Setup proxy
-----------------
* Open ~/.condarc.
* Add the following lines :

.. code-block:: text

  proxy_servers:
    http: http://proxy.ill.fr:8888
    https: http://proxy.ill.fr:8888

Setup the mantid conda environment
----------------------------------
* With your restarted terminal.
* Create the mantid conda environment by navigating to your mantid source code directory in your terminal and running ``conda env create -f mantid-developer-linux.yml``

Configure CMake and generate build files
----------------------------------------
* Still using the terminal.
* Run ``conda activate mantid-developer`` to activate your conda environment.
* Navigate back to your mantid source directory using ``cd mantid`` if you used the default name during cloning from git.
* Inside of your mantid source directory run ``cmake --preset=linux``

    * Alternatively if you don't want to have your build folder in your mantid source then pass the ``-B`` argument, overriding the preset, to cmake: ``cmake {PATH_TO_SOURCE} --preset=linux -B {BUILD_DIR}``

How to build
-------------
* Navigate to the build directory.
* To build Mantid Workbench use: ``ninja``
* To build Unit Tests use: ``ninja AllTests``

Building and debugging with CLion
---------------------------------
CLion will fail to find the build targets if the conda environment is not enabled:

* Navigate to ``File > Settings > Build,Execution,Deployment > Toolchains``
* Edit the CMake and Build Tool settings to point to your conda installed ``cmake`` and ``ninja`` respectively, e.g. ``/home/<user>/mambaforge/envs/mantid-developer/bin/cmake``
* Navigate to ``File > Settings > Build,Execution,Deployment > CMake`` and change the generator to ``Ninja`` and the CMake options to ``--preset=linux``. Change the build directory to ``build`` (or your own build directory if not the default; you'll need to use the full path if outside the source directory).
* The configurations drop-down at the top should show all of the build targets. If not, the CMake project is probably not loaded; open the root ``CMakeLists.txt`` file in a tab and there should be a ``Load CMake Project`` option at the top right. Click it and the configurations should be populated.
* To test a build, navigate to ``Build > Build All in 'Debug'``. Check that the build command displayed in the Messages window is running the correct cmake executable from your conda installation.
* Note that you can build a specific target by selecting it in the configurations drop-down menu and clicking the hammer icon next to it.
* To debug workbench, you'll need to edit the ``workbench`` CMake Application configuration. Set the executable to ``python`` in your conda installation, and program arguments to ``workbench --single-process``. Set the working directory to the full path to your ``build/bin`` directory

If this fails you may need to open CLion from a terminal with conda activated, although I have not found this necessary:

* Open a terminal and run ``conda activate mantid-developer``
* Launch CLion through that terminal (``<CLION_INSTALL>/bin/clion.sh``)

It is also useful to have your terminals in CLion also run with this environment:

* In your ``home`` directory create a file named ``.clionrc`` and open in your favourite text editor, adding these lines:

.. code-block:: text

    source ~/.bashrc
    source ~/mambaforge/bin/activate mantid-developer

* Start CLion using the above steps
* Navigate to ``File > Settings > Tools > Terminal``.
* To the end of the ``Shell path`` option, add: ``--rcfile ~/.clionrc``.

CMake Conda variables
-----------------------
There are two Conda specific CMake options `CONDA_ENV` and `CONDA_BUILD`. The former should be used whenever you are building within a Conda environment, as it ensures our finders (e.g FindNexus.cmake) look in the correct locations for libraries.
The CONDA_BUILD parameter is used to customise our installation, which is required when we are using the conda-build tool to build and package Mantid. These options can be passed to CMake on the command line using -DCONDA_ENV=True for example.
