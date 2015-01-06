Indirect Simulation
===================

.. contents:: Table of Contents
  :local:

Overview
--------

.. interface:: Simulation
  :align: right
  :width: 350

This interface contains loaders for data created by various simulation software.

Action Buttons
~~~~~~~~~~~~~~

?
  Opens this help page.

Run
  Runs the processing configured on the current tab.

Manage Directories
  Opens the Manage Directories dialog allowing you to change your search directories
  and default save directory and enable/disable data archive search.

MolDyn
------

.. interface:: Simulation
  :widget: molDyn

The MolDyn interface is used to import simulation data created using nMOLDYN (by
using the Mantid algorithm MolDyn), tab operates on either .dat or .cdl files.

Options
~~~~~~~

Sample Run
  The data file (.cdl or .dat) to load.

Function Names
  A comm separated list of functions to load from a .cdl file.

Crop Max Energy
  Allows the maximum energy for loaded functions in energy to be cropped, this
  can be useful to remove the additional simulation data that is out of the
  energy range of an instrument.

Use Instrument Resolution
  Allows convolution with an instrument resolution file or workspace.

Verbose
  Outputs more information to the Results Log.

Plot Result
  Allows creation of either a spectra or contour plot (or both) when the tab
  is run.

Save Result
  If checked will save the loaded data as a NeXus file in the default save
  directory.

Sassena
-------

.. interface:: Simulation
  :widget: sassena

The Indirect Sassena interface is used to load simulations from the Sassena
software.

Options
~~~~~~~

Sample File
  The data file (.h5 or .hd5) to load.

Time per Data Point
  Specifies the time interval between each data point in the loaded data file.

Sort by Q Vectors
  If checked will sort the structure factors by momentum transfer in ascending
  order.

Plot Result
  If checked will create a spectra plot of the loaded data when run.

Save Result
  If checked will save the loaded data as a NeXus file in the default save
  directory.

.. categories:: Interfaces Indirect
