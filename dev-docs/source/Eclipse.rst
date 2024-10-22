.. _Eclipse:

=================
Eclipse on Ubuntu
=================

.. contents::
	:local:

Prerequisites
################
1.
Follow the getting started guide and building with cmake, but stop before actually using cmake to build the project.

2.
Install the JRE by running sudo apt-get install default-jre


Installing Eclipse
###################
1.
Install Ubuntu-Make

run the following in terminal

.. code-block:: sh

  sudo add-apt-repository ppa:ubuntu-desktop/ubuntu-make
  sudo apt update
  sudo apt install ubuntu-make


This will add the umake repository to the list of repositories then install umake

2.
Install eclipse using Umake

run the following in terminal

.. code-block:: sh

	umake ide eclipse-cpp

leave the installation path to the default

3.
add eclipse to the path so it can be run from command line

run the following in terminal

.. code-block:: sh

	gedit .bashrc

then scroll to the bottom of the file and add

.. code-block:: sh

	export PATH=$PATH:/path/to/eclipse-cpp-folder/

then save and exit gedit, and restart terminal.

type eclipse into command line, it should open.

4.
Install gdb by running

.. code-block:: sh

	sudo apt install gdb


Building with CMake
######################
1.
from the pre-requisites section, you should already have installed/setup everything to the point where you are ready to run CMake, create two
folders, one for debugging in eclipse "eclipseDebug" and one for quick test runs EclipseTest

Enter eclipseDebug and run


.. code-block:: sh

	cmake -G"Eclipse CDT4 - Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE -DCXXTEST_ADD_PERFORMANCE=TRUE -DENABLE_WORKBENCH=TRUE /path/to/mantidrepository



then enter eclipseTest and run

.. code-block:: sh

	cmake -G"Ninja" -DCMAKE_BUILD_TYPE=Release  -DCXXTEST_ADD_PERFORMANCE=TRUE -DENABLE_WORKBENCH=TRUE /path/to/mantidrepository

2.
open eclipse, leaving your workspace as it is, then go to ``File>Import>General>Existing Projects into Workspace`` and then click next
click browse and find eclipseDebug folder and then add it to eclipse

3.
select ``Project>Build All``

4.
navigate to Run>Run Configurations>C/C++ Application then create a new launch configuration. on the Main tab ensure the correct project is selected then under C/C++ Application
click browse and navigate to /bin/MantidWorkbench





Suggested Plugins
####################
To install these plugins, run eclipse, Select Help>Eclipse Marketplace and install them from there.

Eclox: Eclipse plugin for Doxygen.

cppStyle: ClangFormat tool as a code formatter.
