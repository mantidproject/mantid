.. _GettingStarted:

===============
Getting Started
===============

.. contents::
  :local:

.. toctree::
   :hidden:

   GettingStartedCondaWindows
   GettingStartedCondaLinux
   GettingStartedCondaOSX
   GettingStartedNoneConda

The Choice
##########

With the addition of Conda to our workflow we at present have two different methods of Getting Started with mantid
development. Therefore you can choose which method to setup with (We recommend Conda as it will be the only supported
method eventually).

* Conda on Windows: :ref:`GettingStartedCondaWindows`
* Conda on Linux: :ref:`GettingStartedCondaLinux`
* Conda on MacOS: :ref:`GettingStartedCondaOSX`
* None Conda: :ref:`GettingStartedNoneConda`

Custom git setup for inside the ORNL firewall:
----------------------------------------------

Due to security configuration at ORNL one needs to do additional configuration to access github from within the lab.
One option is to use the ``https`` protocol listed above
The alternative is to "corkscrew the snowman" which allows for using the ``git`` protocol by modifying the ssh configuration.
Corkscrew can be installed from your package manager, or it is a single ``c`` file found on github.
Add the following lines to ``~/.ssh/config``:

.. code:: bash

    ProxyCommand corkscrew snowman.ornl.gov 3128 %h %p
    Host github.com


If you need further help, ask another developer at the facility how to configure the corkscrew option.


Setting up GitHub
#################
Please install the ZenHub Browser extension from this `page <https://www.zenhub.com/extension>`_.

Building Mantid
###############
See :ref:`BuildingWithCMake` for information about building Mantid.

Archive access - ISIS
#####################

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
