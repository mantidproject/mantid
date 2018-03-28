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
The build environment on OS X is `described here <https://github.com/mantidproject/mantid/wiki/Build-environment-setup-on-OS-X-10.12-Sierra>`_.

Checking out the Mantid code
############################
We use `Git`_ as our version control system (VCS). The master copies of our repositories are located at `GitHub <http://github.com/mantidproject>`_. We have a number of repositories, of which the main one (the one containing all the source code for Mantid itself) is called simply `mantid <http://github.com/mantidproject/mantid>`_.

Naturally, you will require git installed on your machine. Windows and Mac OSX downloads are available at https://www.git-scm.com/; Linux users should install via their package manager. This will give you access to git via the command line, with the Windows and Mac installers also including a simple graphical tool (Git Gui). Other graphical tools include `TortoiseGit <http://code.google.com/p/tortoisegit/>`_ (for windows) and a Visual Studio extension. There are others (do add your favourite!). Git is also available within Eclipse and QtCreator.

Set up Git
----------
Follow the instructions on github. This boils down to install git and set your name and email. It's very important for the integrity of the repository that everyone sets this on their machine (to match their github account).

.. code-block:: sh

    git config --global user.name "Firstname Lastname"
    git config --global user.email "your_email@youremail.com"

You can have multiple email addresses associated with a single github account, so pick the one you want people to send to about mantid.

A further setting that everyone should put in place will ensure that our line endings are consistent. Linux/Mac users should set the following:

.. code-block:: sh

    git config --global core.autocrlf input

For Windows users, the 'input' should be changed to 'true' - though the git installer will have set it to this by default.

Have a global ignored files list for all git repositories

.. code-block:: sh

    git config --global core.excludesfile = ~/.gitexcludes

Create tracking branches by default

.. code-block:: sh

    git config --global push.default tracking

If you are using an older git client

.. code-block:: sh

    git config --global push.default current
    git config --global branch.autosetupmerge true

And some bonus ways to look at diffs (the one without a name is the default when you git difftool)

.. code-block:: sh

    [difftool "kompare"]
            external = kompare
            prompt = false
    [difftool]
            external = meld
            prompt = false
    [difftool "sourcetree"]
            cmd = opendiff \"$LOCAL\" \"$REMOTE\"
            path = 
    [mergetool "sourcetree"]
            cmd = /Applications/SourceTree.app/Contents/Resources/opendiff-w.sh \"$LOCAL\" \"$REMOTE\" -ancestor \"$BASE\" -merge \"$MERGED\"
            trustExitCode = true

Extra Windows Step (Do Not Skip This!)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
On Windows, third party libraries are automatically pulled in when cmake is run. This requires Git LFS to be installed first. Download `git lfs <https://git-lfs.github.com/>`_ and install it. At RAL make sure you install this using the admin account in to Program Files.

Once the installation has competed open a new Git bash prompt and type ``git lfs install``.

Cloning the repository
----------------------
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
See `Building with CMake <http://www.mantidproject.org/Building_with_CMake>`_ for information about building Mantid.

Building VATES
##############
See `Building VATES <https://www.mantidproject.org/Building_VATES>`_ for infromation about building VATES.
