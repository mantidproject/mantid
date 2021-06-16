.. _GettingStartedCondaOSX:

===========================
Develop with Conda on MacOS
===========================

Install `Git <https://git-scm.com/>`_
-------------------------------------
* On MacOS install using brew with the following command: ``brew install git``

Clone the source code
---------------------
* There are 2 common ways developers have been doing this.

    * Using git in the terminal and cloning the codebase by calling ``git clone git@github.com:mantidproject/mantid.git`` in the directory you want the code to clone to. This sets you up with accessing the remote repository via SSH so make sure to setup git properly using this `startup guide <https://git-scm.com/book/en/v2/Getting-Started-First-Time-Git-Setup>`_ and ensure your ssh key is setup using this `guide to Github with SSH <https://docs.github.com/en/github/authenticating-to-github/connecting-to-github-with-ssh>`_.
    * Or by using `GitKraken <https://www.gitkraken.com/>`_.

Install `Mambaforge <https://github.com/conda-forge/miniforge/releases>`_
-------------------------------------------------------------------------
* Choose the latest version of ``Mambaforge-MacOSX-x86_64.sh`` for intel based Macs or for the new arm versions use ``Mambaforge-MacOSX-arm64.sh``
* Run your downloaded script from the terminal using ```bash Mambaforge-MacOSX-x86_64.sh`` or ``bash Mambaforge-MacOSX-arm64.sh`` depending on your downloaded variant.
* Restart your terminal.

Setup the mantid conda environment
----------------------------------
* With your restarted terminal.
* Create the mantid conda environment by navigating to your source code directory and running ``conda env create -f {SOURCE}/mantid-developer-windows.yml``

Configure CMake and generate build files
----------------------------------------
* Still using the terminal.
* Run ``conda activate mantid`` to activate your conda environment.
* Create your build directory with ``mkdir build`` and navigate to it with ``cd build``
* Inside of your build directory run ``cmake --preset=osx``

    * Alternatively if you don't want to have your build folder in your source then pass these arguments to cmake: ``cmake {SOURCE} -GNinja -DCMAKE_FIND_FRAMEWORK=LAST -DCMAKE_PREFIX_PATH=${CONDA_PREFIX} -DUSE_PYTHON_DYNAMIC_LIB=OFF -DQt5_DIR=${CONDA_PREFIX}/lib/cmake/qt5 -DHDF5_ROOT=${CONDA_PREFIX} -DOPENSSL_ROOT=${CONDA_PREFIX}``

How to build
-------------
* Navigate to the build directory.
* To build Mantid Workbench use: ``ninja``
* To build Unit Tests use: ``ninja AllTests``
