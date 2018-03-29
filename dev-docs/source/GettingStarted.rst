.. _GettingStartedWithMantid:

===========================
Getting Started with Mantid
===========================

.. contents::
  :local:

Environment
###########

Some intial setup is required before being able to build the code. This is platform
specific and described here.

Windows
-------

Install the following:

* `Visual Studio 2015 Community Edition <https://go.microsoft.com/fwlink/?LinkId=532606&clcid=0x409>`_. If you are at RAL then
  ask for the location of the locally-cached offline version.
* `Git <https://git-scm.com/>`_
* `Git LFS <https://git-lfs.github.com/>`_. After installation open Git Bash and run ``git lfs install``

* `CMake <https://cmake.org/download/>`_
* `MiKTeX <https://miktex.org/download>`_. Instructions are
  `available here <https://miktex.org/howto/install-miktex>`_.
* `NSIS <http://nsis.sourceforge.net/Download>`_ (optional). Used for building packages

Linux
-----

Red Hat/Cent OS/Fedora
^^^^^^^^^^^^^^^^^^^^^^
Follow the `Red Hat instructions <http://download.mantidproject.org/redhat.html>`_ to add the
stable release yum repository and then install the ``mantid-developer`` package:

.. code-block:: sh

   yum install mantid-developer

Ubuntu
^^^^^^
Follow the `Ubuntu instructions <http://download.mantidproject.org/ubuntu.html>`_ to add the
stable release repository and mantid ppa. Download the latest
`mantid-developer <https://sourceforge.net/projects/mantid/files/developer>`_
package and install it:

.. code-block:: sh

   apt install gdebi-core
   apt install ~/Downloads/mantid-developer.X.Y.Z.deb

where ``X.Y.Z`` should be replaced with the version that was downloaded.

OSX
---
The build environment on OS X is described here :ref:`BuildingOnOSX`.

Getting the Mantid code
############################
We use `Git`_ as our version control system (VCS). The master copies of our repositories are located at `GitHub <http://github.com/mantidproject>`_. We have a number of repositories, of which the main one (the one containing all the source code for Mantid itself) is called simply `mantid <http://github.com/mantidproject/mantid>`_.

If you are not already set up with Git, you can follow these `instructions <https://git-scm.com/book/en/v2/Getting-Started-First-Time-Git-Setup>`_.

If you are at RAL then please run the following commands before cloning the repository:

.. code-block:: sh

    git config --global url.git@github.com:mantidproject.insteadOf http://mantidweb.nd.rl.ac.uk/mirror/git/

This will speed up the clone and intial cmake run considerably.

There are a number of URLs via which the code can be checked out using various protocols. The easiest way to get the one you want is to select the protocol you want on the right side of the `mantid <http://github.com/mantidproject/mantid>`_ repository page on github and copy the url into your clipboard. The way to clone the repository via ssh on the command line, into a directory called Mantid, is:

.. code-block:: sh

    git clone git@github.com:mantidproject/mantid.git

If at RAL now remove the config section above

.. code-block:: sh

    git config --global --unset url.git@github.com:mantidproject


Building Mantid
###############
See :ref:`BuildingWithCMake` for information about building Mantid.

Building VATES
##############
See :ref:`BuildingVATES` for infromation about building VATES.
