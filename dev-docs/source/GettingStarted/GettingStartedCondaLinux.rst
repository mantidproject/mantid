.. _GettingStartedCondaLinux:

===========================
Develop with Conda on Linux
===========================

Install `Git <https://git-scm.com/>`_
-------------------------------------
* On Debian based systems (Ubuntu) install using apt with the following command: ``sudo apt-get install git``
* On CentOS based systems (RHEL) install using yum with the following command: ``sudo yum install git``

Clone the source code
---------------------
* There are 2 common ways developers have been doing this.

    * Using git in the terminal and cloning the codebase by calling ``git clone git@github.com:mantidproject/mantid.git`` in the directory you want the code to clone to. This sets you up with accessing the remote repository via SSH so make sure to setup git properly using this `startup guide <https://git-scm.com/book/en/v2/Getting-Started-First-Time-Git-Setup>`_ and ensure your ssh key is setup using this `guide to Github with SSH <https://docs.github.com/en/github/authenticating-to-github/connecting-to-github-with-ssh>`_.
    * Or by using `GitKraken <https://www.gitkraken.com/>`_.

Install `Mambaforge <https://github.com/conda-forge/miniforge/releases>`_
-------------------------------------------------------------------------
* Choose the latest version of ``Mambaforge-Linux-x86_64.sh``
* Run your downloaded script from the terminal using ``bash Mambaforge-Linux-x86_64.sh``
* Restart your terminal.

Setup the mantid conda environment
----------------------------------
* With your restarted terminal.
* Create the mantid conda environment by navigating to your source code directory and running ``conda env create -f {SOURCE}/mantid-developer-windows.yml``

Configure CMake and generate build files
----------------------------------------
* Still using the terminal.
* Run ``conda activate mantid`` to activate your conda environment.
* Create your build directory with ``mkdir mantid-build`` and navigate to it with ``cd mantid-build``
* Inside of your build directory run ``cmake {SOURCE} -GNinja`` add ``-DCMAKE_BUILD_TYPE=Debug`` if you want it to be built in debug.

How to build
-------------
* Navigate to the build directory.
* To build Mantid Workbench use: ``ninja``
* To build Unit Tests use: ``ninja AllTests``
