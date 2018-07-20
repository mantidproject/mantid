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
	
then save and exit gedit.

type eclipse into command line, it should open.

4.
Install gdb by running

.. code-block:: sh

	sudo apt install gdb
	
	
Building with CMake
######################
1.
from the pre-requisites section, you should already have installed/setup everything to the point where you are ready to run CMake, create two 
folders, one will be for running in eclipse and debugging, whereas the other will be a quck test run build. Ensure that you do not name the debug
folder "debug" as this will prevent eclipse from properly finding files within it.
	
Enter the debugging folder and run 


.. code-block:: sh
	
	cmake -G"Eclipse CDT4 - Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE -DMANTID_DATA_STORE=/path/to/MantidExternalData -DCXXTEST_ADD_PERFORMANCE=TRUE -DENABLE_WORKBENCH=TRUE ../mantid 



Where /path/to/MantidExternalData should be a path in a home folder (not that the MantidExternalData folder does not yet have to exist) and 
../mantid is the path to the git repository, if the git repository root, and the folder you are currently in are in the same directory then leave as is.
	
	
then enter the testing folder and run

.. code-block:: sh
	
	cmake -G"Ninja" -DCMAKE_BUILD_TYPE=Release -DMANTID_DATA_STORE=/users/bush/MantidExternalData -DCXXTEST_ADD_PERFORMANCE=TRUE -DENABLE_WORKBENCH=TRUE ../mantid 
	
2.
open eclipse, leaving your workspace as it is, then go to ``File>Import>General>Existing Projects into Workspace`` and then click next
click browse and find your debug folder and then add it to eclipse

3.
select ``Project>Build All``

4.
navigate to Run>Run Configurations>C/C++ Application then create a new launch configuration. on the Main tab ensure the correct project is selected then under C/C++ Application
click browse and navigate to /bin/MantidPlot





Suggested Plugins
####################
To install these plugins, run eclipse, Select Help>Eclipse Marketplace and install them from there.

Eclox: Eclipse plugin for Doxygen.

cppStyle: ClangFormat tool as a code formatter.

