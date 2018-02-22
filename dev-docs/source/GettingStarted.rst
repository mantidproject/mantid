===========================
Getting Started with Mantid
===========================

.. contents::
  :local:

Prerequisites
#############

Some intial setup is required before being able to build the code. This is platform
specific and described here.

Windows
-------

Install the following:

* `Visual Studio 2015 Community Edition <https://go.microsoft.com/fwlink/?LinkId=532606&clcid=0x409>`_. If you are at RAL then
  ask for the location of the locally-cached offline version.
* `Git <https://git-scm.com/download/win>`_
* `Git LFS <https://git-lfs.github.com/>`_

  * After installation open Git Bash and run ``git lfs install``

* `CMake <https://cmake.org/download/>`_
* `MiKTeX <https://miktex.org/download>`_. Instructions are available
  `here <https://miktex.org/howto/install-miktex>`_.
* `NSIS <http://nsis.sourceforge.net/Download>`_ (optional). Used for building packages

Linux
-----

Red Hat/Cent OS/Fedora
^^^^^^^^^^^^^^^^^^^^^^

Follow the instructions at http://download.mantidproject.org/redhat.html to add the
stable release yum repository and then install the ``mantid-developer`` package:

.. code-block:: sh

   yum install mantid-developer

Ubuntu
^^^^^^

Follow the instructions at http://download.mantidproject.org/ubuntu.html to add the
stable release repository and mantid ppa. Download the latest
`mantid-developer <https://sourceforge.net/projects/mantid/files/developer>`_
package and install it:

.. code-block:: sh

   apt install gdebi-core
   apt install ~/Downloads/mantid-developer.X.Y.Z.deb

where ``X.Y.Z`` should be replaced with the version that was downloaded.

