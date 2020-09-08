.. _GettingStarted:

===============
Getting Started
===============

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

if you wish to setup eclipse for use developing mantid, then instructions can be found :ref:`here <Eclipse>`.

Ubuntu 20.04
~~~~~~~~~~~~
- Mantid uses `qtpy` to talk to Python bindings of Qt.  It is recommended to have the _
  environment var `QT_API=pyqt5` exported to the shell before building with CMake.
- The header and lib shipped with Anaconda (if installed) could interfere with Mantid building _
  process. It is highly recommended to remove Anaconda Python from your env prior to building _
  using `conda deactivate`.
- Mantid is not yet officially supported on Ubuntu 20.04 as Qt4 has been removed but Workbench can be built by installing:

.. code-block:: sh

   apt-get install -y \
     git \
     g++ \
     clang-format-6.0 \
     cmake \
     dvipng \
     doxygen \
     libtbb-dev \
     libgoogle-perftools-dev \
     libboost-all-dev \
     libpoco-dev \
     libnexus-dev \
     libhdf5-dev \
     libhdf4-dev \
     libjemalloc-dev \
     libgsl-dev \
     liboce-visualization-dev \
     libmuparser-dev \
     libssl-dev \
     libjsoncpp-dev \
     librdkafka-dev \
     qtbase5-dev \
     qttools5-dev \
     qttools5-dev-tools \
     libqt5webkit5-dev \
     libqt5x11extras5-dev \
     libqt5opengl5-dev \
     libqscintilla2-qt5-dev \
     libpython3-dev \
     ninja-build \
     python3-setuptools \
     python3-sip-dev \
     python3-pyqt5 \
     pyqt5-dev \
     pyqt5-dev-tools \
     python3-qtpy \
     python3-numpy \
     python3-scipy \
     python3-sphinx \
     python3-sphinx-bootstrap-theme \
     python3-dateutil \
     python3-matplotlib \
     python3-qtconsole \
     python3-h5py \
     python3-mock \
     python3-psutil \
     python3-requests \
     python3-toml \
     python3-yaml

and passing the `-DENABLE_MANTIDPLOT=OFF` option to the cmake command line or selecting this in the cmake GUI.

OSX
---
The build environment on OS X is described here :ref:`BuildingOnOSX`.

Docker
------

On Docker supported systems you may use the `mantid-development
<https://github.com/mantidproject/dockerfiles/tree/master/development>`_
images to develop Mantid without having to configure your system as a suitable
build environment. This will give you an out of the box working build
environment, including ParaView/VATES, Python 3 (where available) and ccache.

More details and instructions can be found at the GitHub link above.

Getting the Mantid code
#######################
We use `Git`_ as our version control system (VCS). The master copies of our repositories are located at `GitHub <http://github.com/mantidproject>`_. We have a number of repositories, of which the main one (the one containing all the source code for Mantid itself) is called simply `mantid <http://github.com/mantidproject/mantid>`_.

If you are not already set up with Git, you can follow these `instructions <https://git-scm.com/book/en/v2/Getting-Started-First-Time-Git-Setup>`_.

There are a number of URLs via which the code can be checked out using various protocols. The easiest way to get the one you want is to select the protocol you want on the right side of the `mantid <http://github.com/mantidproject/mantid>`_ repository page on github and copy the url into your clipboard. The way to clone the repository via ssh on the command line, into a directory called Mantid, is:

.. code-block:: sh

    git clone git@github.com:mantidproject/mantid.git


Setting up GitHub
#################
Please install the ZenHub Browser extension from this `page <https://www.zenhub.com/extension>`_.

Building Mantid
###############
See :ref:`BuildingWithCMake` for information about building Mantid.

Building VATES
##############
See :ref:`BuildingVATES` for infromation about building VATES.

Archive access
##############

It is very convenient to be able to access the data archive directly.
At ISIS, this is automatically done on the Windows machines, however OSX and Linux
require some extra setup.

OSX
---

* In Finder "command"+k opens a mounting dialogue
* For `Server address` enter `smb://isisdatar80/inst$/` hit Connect
* This should prompt you for federal ID `clrc\....` and password
* After completing this the drive is now mounted
* It can be found at `/Volumes/inst$`

**NB** the address in step 2 sometimes changes - if it does not work, replace `80` with `55` or `3`.

Linux
------
1. Install packages:

``sudo apt-get install -y autofs cifs-utils keyutils``

2. Create an ``/archive.creds`` file in the root directory containing this, filling in the relevant details:

This should only be done if full disk encryption is enabled or if the ``archive.creds`` file is stored in a secure (encrypted) location; to ensure passwords are kept safe.

.. code-block:: text

   username=FEDERAL_ID_HERE
   password=FED_PASSWD_HERE
   domain=CLRC

3. Edit ``/etc/auto.master`` and add the line:

.. code-block:: text

   /archive      /etc/auto.archive

4. Create ``/etc/auto.archive`` and add the single line:

.. code-block:: text

   *     -fstype=cifs,ro,credentials=/archive.creds,file_mode=0444,dir_mode=0555,vers=3.0,noserverino,nounix    ://isis.cclrc.ac.uk/inst\$/&

5. Enter the following commands:

.. code-block:: bash

   sudo chmod 400 /archive.creds
   sudo mkdir /archive
   service autofs restart

Done. You can now access directories in the archive. Test it by doing:

.. code-block:: bash

   ls /archive/ndxalf

If it's working the command should return ``ls: cannot access '/archive/ndxalf/DfsrPrivate': Permission denied``
