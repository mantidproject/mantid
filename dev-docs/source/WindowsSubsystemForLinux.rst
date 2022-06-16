.. _WindowsSubsystemForLinux:

==================================
Windows Subsystem for Linux (WSL2)
==================================

.. contents::
  :local:

The Windows Subsystem for Linux lets developers run a Linux environment directly on Windows, unmodified, without the overhead of a traditional virtual machine or dualboot setup.

Initial Setup
#############

Some initial setup is required on your Windows system before the Linux environment of your choice can be installed. Before you start this process make sure you have the latest version of Windows 10.

1. Install `Docker Desktop for Windows <https://hub.docker.com/editions/community/docker-ce-desktop-windows>`_. This will install WSL2 for you, and also enables us to get a centos7 image later on.
2. In the Windows Start menu open `Turn Windows features on or off`.
3. Make sure `Windows System for Linux` and `Virtual Machine Platform` are ticked. Click OK.

If you now open a File Explorer and go to the `\\\\wsl$` directory. This will be the location of your linux subsystem.

Install a Ubuntu-18.04 Subsystem
################################

1. Go to the Microsoft Store and install Ubuntu 18.04. Notice that a new `Ubuntu-18.04` directory has appeared in the `\\\\wsl$` location. This is your Ubuntu 18.04 subsystem.
2. In the Windows Start menu type Ubuntu 18.04 to open its terminal. If it says you are missing a package then follow the link provided to install this package.
3. Enter a username and password in the terminal when prompted.

You are now in an Ubuntu 18.04 terminal. Any files in the `\\\\wsl$\\Ubuntu-18.04\\home\\<user name>` directory will appear in this Ubuntu environment.

Install a Centos7 Subsystem
###########################

1. Open the Windows command prompt and run the follow to get the latest centos7 image

.. code-block:: sh

  docker image pull centos:centos7

2. Then make a `*.tar` file from this image, where `998e` is the first four characters of the text outputted by the `docker create` command.

.. code-block:: sh

  docker create -i centos:centos7 bash

  docker export 998e > centos7.tar

3. Finally import the file to the wsl directory:

.. code-block:: sh

  wsl --import Centos7 .\CentosImage\ centos7.tar

Notice that a new `Centos7` directory has appeared in the `\\\\wsl$` location. Any files in the `\\\\wsl$\\Centos7\\root` directory will appear in this Centos7 environment.

Running a Linux Subsystem
#########################

You can run either of these linux subsystems from the Windows command prompt using

.. code-block:: sh

  wsl -d [OS]

where [OS] is replaced by `Ubuntu-18.04` or `Centos7`.

.. _wsl-cloning-mantid-ref:

Cloning Mantid
##############

Before you clone Mantid code, follow the :ref:`getting started <GettingStarted>` instructions to install all of the required dependencies for your linux environment.

The Mantid code can then be retrieved in the usual way using `git clone` from the Ubuntu or Centos7 terminal. The first time you do this you might need to set up an `ssh key <https://docs.github.com/en/github/authenticating-to-github/connecting-to-github-with-ssh>`_ for authentication.

You are now ready to CMake and build the Mantid code. Follow the Ubuntu 18.04 or Centos 7 build instructions `here <https://developer.mantidproject.org/GettingStarted.html#linux>`_.

Tips
####

* Make sure you install `devtoolset-7 <https://developer.mantidproject.org/BuildingWithCMake.html#from-the-command-line>`_ for Centos 7 as described in the provided link before CMake and build.
* It might also be necessary to install some addition packages for Ubuntu 18.04, including `libnexus0-dev`.