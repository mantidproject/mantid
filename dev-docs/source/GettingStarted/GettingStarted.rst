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
The alternative is to access github through ssh.github.com.
Add the following lines to ``~/.ssh/config``:

.. code:: bash

    Host github.com
        Hostname ssh.github.com
        Port 443
        PreferredAuthentications publickey

If you need further help, ask another developer at the facility how to configure ssh.

Building Mantid
###############
See :ref:`BuildingWithCMake` for information about building Mantid.

Archive access - ISIS
#####################

It is very convenient to be able to access the data archive directly.
At ISIS, this is automatically done on the Windows machines, however OSX and Linux
require some extra setup.

macOS
-----

#. While in Finder, hit ``Command+K`` to open the "Connect to Server" dialog box.
#. For ``Server address`` enter the below and hit ``Connect``:

   * On Site: ``smb://isisdatar80/inst$/``.
   * Remote: ``smb://isisdatar80.isis.cclrc.ac.uk/inst$/`` (You must be connected to the VPN).

#. This should prompt you for federal ID (you'll need to prefix it with ``clrc\...``) and password.
#. After completing this the archive drive is now mounted.
#. It can be found at ``/Volumes/inst$/``.

*Note:* ``isisdatar80``, ``isisdatar55``, and ``isisdatar3`` can all be used interchangably. We've just used the R80 option in these examples. If one goes down you may be able to connect to one of the others.

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
