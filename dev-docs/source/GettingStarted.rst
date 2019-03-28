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

* `Visual Studio 2017 Community Edition <https://visualstudio.microsoft.com/downloads/>`_.

  * When asked about installation workloads choose ``Desktop development with C++``
  * Under the "Installation details" section verify that the following are checked:

    * ``Windows 8.1 SDK and UCRT SDK``
    * The latest Windows 10 SDK


* `Git <https://git-scm.com/>`_.

  * install the latest version and ensure that Git LFS is checked to be included
  * when the install has completed create a directory for storage of the LFS objects, e.g. ``C:\GitLFSStorage``
  * open up Git Bash and run ``git config --global lfs.storage C:/GitLFSStorage``
  * run ``git lfs install`` to initialize Git LFS. (Note that if you miss this step you may get errors due to the third party libraries not checking out properly. This can be fixed later by running ``git lfs fetch`` and ``git lfs checkout`` in the ``external\src\ThirdParty`` directory.)

* `CMake <https://cmake.org/download/>`_ >= 3.14
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

Ubuntu
~~~~~~
Follow the `Ubuntu instructions <http://download.mantidproject.org/ubuntu.html>`_ to add the
stable release repository and mantid ppa. Download the latest
`mantid-developer <https://sourceforge.net/projects/mantid/files/developer>`_
package and install it:

.. code-block:: sh

   apt install gdebi-core
   apt install ~/Downloads/mantid-developer.X.Y.Z.deb

where ``X.Y.Z`` should be replaced with the version that was downloaded.

if you wish to setup eclipse for use developing mantid, then instructions can be found :ref:`here <Eclipse>`.

OSX
---
The build environment on OS X is described here :ref:`BuildingOnOSX`.

Getting the Mantid code
#######################
We use `Git`_ as our version control system (VCS). The master copies of our repositories are located at `GitHub <http://github.com/mantidproject>`_. We have a number of repositories, of which the main one (the one containing all the source code for Mantid itself) is called simply `mantid <http://github.com/mantidproject/mantid>`_.

If you are not already set up with Git, you can follow these `instructions <https://git-scm.com/book/en/v2/Getting-Started-First-Time-Git-Setup>`_.

There are a number of URLs via which the code can be checked out using various protocols. The easiest way to get the one you want is to select the protocol you want on the right side of the `mantid <http://github.com/mantidproject/mantid>`_ repository page on github and copy the url into your clipboard. The way to clone the repository via ssh on the command line, into a directory called Mantid, is:

.. code-block:: sh

    git clone git@github.com:mantidproject/mantid.git


Building Mantid
###############
See :ref:`BuildingWithCMake` for information about building Mantid.

Building VATES
##############
See :ref:`BuildingVATES` for infromation about building VATES.

Archive access
##############

It is very convenient to be able to access the data archive directly.
At ISIS, this is automatically done on the Windows machines, however OSX
requires some extra setup.

OSX
---

* In Finder "command"+k opens a mounting dialogue
* For `Server address` enter `smb://isisdatar80/inst$/` hit Connect
* This should prompt you for federal ID `clrc\....` and password
* After completing this the drive is now mounted
* It can be found at `/Volumes/inst$`

**NB** the address in step 2 sometimes changes - if it does not work, replace `80` with `55` or `3`.
