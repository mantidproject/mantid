.. _GettingStartedNoneConda:

=============================
Getting Started without Conda
=============================

.. contents::
  :local:

Environment
###########

Some initial setup is required before being able to build the code. This is platform
specific and described here.

Windows
-------

Install the following:

* `Visual Studio 2019 Community Edition <https://visualstudio.microsoft.com/downloads/>`_.

  * When asked about installation workloads choose ``Desktop development with C++``
  * Under the "Installation details" section verify that the following are checked:

    * ``Windows Universal CRT SDK``
    * The latest Windows 10 SDK
  * If your machine has less than 32GB of memory Mantid may not build. If you have problems change the maximum number of parallel project builds to 1 in Visual Studio in Tools -> Options -> Projects and Solutions -> Build And Run.


* `Git <https://git-scm.com/>`_.

  * install the latest version and ensure that Git LFS is checked to be included
  * when the install has completed create a directory for storage of the LFS objects, e.g. ``C:\GitLFSStorage``
  * open up Git Bash and run ``git config --global lfs.storage C:/GitLFSStorage``
  * run ``git lfs install`` to initialize Git LFS. (Note that if you miss this step you may get errors due to the third party libraries not checking out properly. This can be fixed later by running ``git lfs fetch`` and ``git lfs checkout`` in the ``external\src\ThirdParty`` directory.)

* `CMake <https://cmake.org/download/>`_ >= 3.15
* `MiKTeX <https://miktex.org/download>`_. Installation instructions are  `available here <https://miktex.org/howto/install-miktex>`_. Once installed:

  * open the MikTeX console from the start menu
  * switch to administrator mode
  * settings -> select "Always install missing packages on-the-fly"

* `NSIS <http://nsis.sourceforge.net/Download>`_ (optional). Used for building packages

`Graphviz <http://graphviz.org/download/>`__ is required to generate the workflow diagrams in the documentation.
Unfortunately CMake can't find it out of the box and the following steps are required to make this link

* open regedit
* add a new key ``[HKEY_LOCAL_MACHINE\\SOFTWARE\\ATT\\Graphviz]``
* create a new string value named ``InstallPath`` within this key and set the value
  to point to the install directory of Graphviz.

Linux
-----

The goal of this section is to (in broad terms) get to the state where one can build and run a subset of tests

.. code-block:: sh

   git clone git@github.com:mantidproject/mantid.git
   cd mantid
   mkdir build  # this makes an in-source build
   cd build
   cmake3 -GNinja ../  # wrap in scl enable on RHEL7
   ninja all AlgorithmsTest  # ninja-build on RHEL7
   ctest -R ^AlgorithmsTest --output-on-failure


Red Hat/Cent OS/Fedora
~~~~~~~~~~~~~~~~~~~~~~
* Follow the `instructions here <https://fedoraproject.org/wiki/EPEL>`_ to enable the EPEL repository
  for RHEL7
* Run the following to install the mantid-developer package

.. code-block:: sh

  # Install copr plugin
  yum install yum-plugin-copr

  # Enable the mantid repo from copr
  yum copr enable mantid/mantid

  # Install dependencies
  yum install mantid-developer

  # Install pre-commit
  pip3 install pre-commit --user


On fedora, the ``yum`` commands should be replaced with ``dnf``.
For systems with default python3 the ``pip3`` command can be replaced with ``pip``, but it should work either way.


Ubuntu 18.04
~~~~~~~~~~~~
- Setup the Kitware APT repository to get a recent version of CMake by
  following `these instructions <https://apt.kitware.com/>`_
- Follow the `Ubuntu instructions <http://download.mantidproject.org/ubuntu.html>`_
  to add the stable release repository and mantid ppa and
- Download the latest
  `mantid-developer <https://sourceforge.net/projects/mantid/files/developer>`_
  package and install it:

.. code-block:: sh

   apt install gdebi-core
   gdebi ~/Downloads/mantid-developer.X.Y.Z.deb

where ``X.Y.Z`` should be replaced with the version that was downloaded.

Install pre-commit for use in our current developer workflow

.. code-block:: sh

   pip install pre-commit --user

if you wish to setup eclipse for use developing mantid, then instructions can be found :ref:`here <Eclipse>`.

Ubuntu 20.04
~~~~~~~~~~~~
- Mantid uses `qtpy` to talk to Python bindings of Qt.  It is recommended to have the _
  environment var `QT_API=pyqt5` exported to the shell before building with CMake.
- The header and lib shipped with Anaconda (if installed) could interfere with Mantid building _
  process. It is highly recommended to remove Anaconda Python from your env prior to building _
  using `conda deactivate`.
- Mantid is not yet officially supported on Ubuntu 20.04, but mantid-developer package (see Ubuntu 18.04 instructions) has been modified to support it.
- Install pre-commit for use in our current developer workflow

.. code-block:: sh

   pip install pre-commit --user

OSX
---
The build environment on OS X is described here :ref:`BuildingOnOSX`.

Install pre-commit for use in our current developer workflow

.. code-block:: sh

   brew install pre-commit

Docker
------

On Docker supported systems you may use the `mantid-development
<https://github.com/mantidproject/dockerfiles/tree/master/development>`_
images to develop Mantid without having to configure your system as a suitable
build environment. This will give you an out of the box working build
environment, Python 3 (where available) and ccache.

More details and instructions can be found at the GitHub link above.
