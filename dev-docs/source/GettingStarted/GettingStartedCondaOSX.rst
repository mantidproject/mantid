.. _GettingStartedCondaOSX:

===========================
Develop with Conda on MacOS
===========================

Install `Git <https://git-scm.com/>`_
-------------------------------------
* On MacOS install using brew with the following command: ``brew install git``

Clone the mantid source code
----------------------------
* There are 2 common ways developers have been doing this.

    * Using git in the terminal and cloning the codebase by calling ``git clone git@github.com:mantidproject/mantid.git`` in the directory you want the code to clone to.
      This sets you up with accessing the remote repository via SSH so make sure to setup git properly using this `startup guide <https://git-scm.com/book/en/v2/Getting-Started-First-Time-Git-Setup>`_ and ensure your ssh key is setup using this `guide to Github with SSH <https://docs.github.com/en/github/authenticating-to-github/connecting-to-github-with-ssh>`_.
    * Or by using `GitKraken <https://www.gitkraken.com/>`_.

Install `Miniforge <https://github.com/conda-forge/miniforge/releases>`_
-------------------------------------------------------------------------
* Choose the latest version of ``Miniforge3-MacOSX-x86_64.sh`` for intel based Macs or for the new arm versions use ``Miniforge3-MacOSX-arm64.sh``
* Run your downloaded script from the terminal using ``bash Miniforge3-MacOSX-x86_64.sh`` or ``bash Miniforge3-MacOSX-arm64.sh`` depending on your downloaded variant.
* If it asks whether or not you want to initialise conda with conda init, choose to do so.
* Restart your terminal.

Setup the mantid conda environment
----------------------------------
.. include:: ./MantidDeveloperSetup.rst

Configure CMake and generate build files
----------------------------------------
* Still using the terminal.
* If not already activated in the previous step, run ``conda activate mantid-developer`` to activate your conda environment.
* Navigate back to your mantid source directory using ``cd mantid`` if you used the default name during cloning from git.
* Inside of your mantid source directory run ``cmake --preset=osx``

    * Alternatively if you don't want to have your build folder in your mantid source then pass the ``-B`` argument, overriding the preset, to cmake: ``cmake {PATH_TO_SOURCE} --preset=osx -B {BUILD_DIR}``

How to build
-------------
* Navigate to the build directory.
* To build Mantid Workbench use: ``ninja``
* To build Unit Tests use: ``ninja AllTests``

CMake Conda variables
-----------------------
The ``CONDA_BUILD`` parameter is used to customise our installation, which is required when we are using the conda-build tool to build and package Mantid.
This option can be passed to CMake on the command line using ``-DCONDA_BUILD=True``.

Running Workbench
-----------------

To run workbench from the commandline, ensure you conda environment is activated, and bin (in the build directory) is added to the python paths.

.. code:: sh

   export PYTHONPATH="${PYTHONPATH}:replace-with-full-file-path-to-bin"
   workbench

.. include:: ../macos-opengl-version-warning.txt
