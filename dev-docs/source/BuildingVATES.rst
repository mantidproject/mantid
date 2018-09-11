.. _BuildingVATES:

==============
Building VATES
==============

.. contents::
  :local:

What Is the VSI?
----------------

The VSI stands for VATES Simple Interface. This is best thought of as the way to visualise 3D+ dimensional data in Mantid. Mantid stores n dimensional data in MDWorkspaces, which may be rendered using the ParaView visualisation tools.

How Do I Get the VSI?
---------------------

The VSI components are part of the Mantid distribution.

ParaView
--------

The visualisation components for the VSI require the ParaView visualisation platform. However, the CMake option ``MAKE_VATES``, can be used to turn off its use. The patch files used in this section as well as build scripts for Windows & OSX/Linux which automate the steps described below can be found `here <https://github.com/mantidproject/paraview-build>`__.

- Execute the following lines from the command prompt

.. code-block:: sh

  git clone https://gitlab.kitware.com/paraview/paraview.git <Your ParaView source root>
  cd <Your ParaView source root>
  git checkout v5.4.0
  git submodule init
  git submodule update

The VSI uses a custom data array layout to minimize memory copies. The name and header must be available to ParaView at compile time.

.. code-block:: sh

  mkdir vtkMDHWSignalArray
  cd  vtkMDHWSignalArray
  wget https://raw.githubusercontent.com/mantidproject/paraview-build/2b28ebc5fd40ad727ca66772522bf220b834c1f7/vtkMDHWSignalArray/vtkMDHWSignalArray.h
  SIGNAL_NAME=vtkArrayDispatch_extra_arrays=vtkMDHWSignalArray\<double\>
  SIGNAL_HEADER=vtkArrayDispatch_extra_headers=<path to vtkMDHWSignalArray.h>

Everyone is encouraged to apply the `additional patchfiles <https://github.com/mantidproject/paraview-build/tree/875fe2a3c800996b75591c8dbe26909b51bdf963/patches>`__ in our buildscript.

Building ParaView
------------------

This is the visualisation plugins to build and use. This works on Windows/Linux/Mac. Download the source code and build using CMake out of source. You'll need the Qt development libraries, in order to build the GUI and also python. For Windows user, the Third_Party directory contains qmake as well as all the development libraries you should need to build Paraview.

The scripts and cmake cache files used by the build servers are available `here <https://github.com/mantidproject/paraview-build/tree/875fe2a3c800996b75591c8dbe26909b51bdf963>`__.

.. code-block:: sh

  cd <Your paraview source root>
  mkdir Build
  cd Build

  cmake -D$SIGNAL_NAME -D$SIGNAL_HEADER -C<path to common.cmake> -C<path to platform specific cache file>  ..
  cmake --build .

Building the VSI
----------------

- Get Paraview (see above)
- Edit the CMake configuration for Mantid

  - Via CMake enable ``MAKE_VATES`` (If editing from the GUI, press ``Configure`` after checking this)
  - Via CMake set ``ParaView_DIR`` to the directory where paraview has been built.
- Make Mantid

This should produce the VSI related binaries for Mantid as well as plugins usable by ParaView.

Sample Mantid cmake command
^^^^^^^^^^^^^^^^^^^^^^^^^^^

This build command enables the VSI:

.. code-block:: sh

  cmake -G Ninja  -DCMAKE_BUILD_TYPE=Release -DMAKE_VATES=TRUE -DParaView_DIR=~/Code/paraview/build ../Code/Mantid
  cmake --build .

Additional Libraries for Paraview/Mantid
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Ubuntu: The package list from :ref:`GettingStarted` contains all the required libraries.

Using Paraview Plugins
----------------------

This section will be fleshed-out or appear as a separate page in the near future.

- Launch the Paraview GUI
- Go to Tools -> Manage Plugins and load the MantidParaview... libraries listed above (except for MantidParaViewQtWidgets)
- There are several reasons why you may get a warning symbol when you try to load the plugins (see troubleshooting)
- Of those loaded plugins, you may wish to expand them using the (+) tree and assigning them as autoload, so that they are immediately available the next time Paraview is launched.
- You can now open a sqw file in Paraview.
- Use the Rebinning Cutter filter to rebin,integrate,rearrange,slice the workspace.

Help
----

We suggest contacting the core Mantid development team if any problems are experienced.

Troubleshooting
---------------

- Can't load plugins

  - Have you built both Mantid and Paraview to be either Debug or Release (both the same)?
  - Do you have the Mantid binaries present and in the right order in the system path (windows)?

- Can't start-up Paraview

  - Try deleting or temporarily renaming the ParaView directory in ``%APPDATA/Roaming%`` Paraview may be crashing as it's trying to autoload plugins that are causing problems.

- Cannot complete the loading of a file

  - Check you have ``MANTIDPATH`` set correctly.
