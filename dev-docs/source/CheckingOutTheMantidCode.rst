.. _CheckingOutTheMantidCode:

============================
Checking out the Mantid Code
============================

.. contents::
   :local:

We use `git <http://www.git-scm.com/>`_ as our version control system (VCS). The master copies of our repositories are located at `GitHub <http://github.com/mantidproject>`_. We have a number of repositories, of which the main one (the one containing all the source code for Mantid itself) is called simply `mantid <http://github.com/mantidproject/mantid>`_.

Naturally, you will require git installed on your machine. Windows and Mac OSX downloads are available at http://www.git-scm.com/. Linux users should install via their package manager. This will give you access to git via the command line, with the Windows and Mac installers also including a simple graphical tool (Git Gui). Other graphical tools include `TortoiseGit <http://code.google.com/p/tortoisegit/>`_ (for windows) and a Visual Studio extension. There are others (do add your favourite!). Git is also available within Eclipse and QtCreator.

Set up Git
##########

Follow the instructions on `GitHub <http://help.github.com/set-up-git-redirect>`_. This boils down to install git and set your name and email. It's very important for the integrity of the repository that everyone sets this on their machine (to match their github account).

.. code-block:: sh

   git config --global user.name "Firstname Lastname"
   git config --global user.email "your_email@youremail.com"

You can have multiple email addresses associated with a single github account, so pick the one you want people to send to about mantid.

A further setting that everyone should put in place will ensure that our line endings are consistent.
Linux/Mac users should set the following:

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

And some bonus ways to look at diffs (the one without a name is the default when you ``git difftool``)

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
######################################

On Windows, third party libraries are automatically pulled in when cmake is run. This requires Git LFS to be installed first. Download `git lfs <https://git-lfs.github.com/>`_ and install it. **At ISIS make sure you install this using the admin account in to Program Files**.

Once the installation has competed open a new Git bash prompt and type ``git lfs install``.

Cloning the repository
######################

If you are at ISIS then please run the following commands '''before''' cloning the repository:

.. code-block:: sh

   git config --global url.git@github.com:mantidproject.insteadOf http://mantidweb.nd.rl.ac.uk/mirror/git/

This will speed up the clone and intial cmake run considerably.

There are a number of URLs via which the code can be checked out using various protocols. The easiest way to get the one you want is to select the protocol you want on the right side of the [https://github.com/mantidproject/mantid repository page on github] and copy the url into your clipboard. The way to clone the repository via ssh on the command line, into a directory called Mantid, is:

.. code-block:: sh

   git clone git@github.com:mantidproject/mantid.git

If at ISIS now remove the config section above

.. code-block:: sh

   git config --global --unset url.git@github.com:mantidproject

Getting the dependencies (Linux and OS X)
#########################################

Additional information on this can be found at [[Mantid Prerequisites]].

Mac OSX
#######

The procedure on OS X is described [https://github.com/mantidproject/mantid/wiki/Building-Mantid-on-OS-X-10.9-&-10.10-using-clang-and-Xcode here].

Linux
#####

Debian and RPM developer packages exist to pull in the dependencies. For example, those on RedHat just need to run <code>sudo yum install mantid-developer</code>.


:ref:`BuildingWithCMake`
