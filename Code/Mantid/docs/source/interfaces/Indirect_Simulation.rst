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
using the :ref:`MolDyn <algm-MolDyn>` algorithm), tab operates on either *.dat*
or *.cdl* files.

Options
~~~~~~~

Sample Run
  The data file (*.cdl* or *.dat*) to load.

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

The Sassena interface is used to load simulations from the Sassena software.
This is done through the :ref:`LoadSassena <algm-LoadSassena>` algorithm.

Options
~~~~~~~

Sample File
  The data file (*.h5* or *.hd5*) to load.

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

DensityOfStates
---------------

.. interface:: Simulation
  :widget: dos

The DensityOfStates interface is used to load CASTEP simulations using the
:ref:`DensityOfStates <algm-DensityOfStates>` algorithm. It supports loading
full and partial densities of states, raman and IR spectroscopy.

Options
~~~~~~~

The following options are common to each spectrun type:

Spectrum Type
  Selects the type of spectrum to extract from the file.

Peak Shape
  Selects the shape of peaks to fit over the intensities extracted from the
  file.

Peak Width
  Sets the FWHM to which the fitted peaks should be broadened.

Bin Width
  Sets the histogram resolution for binning.

Zero Threshold
  Frequencies below this threshold will be ignored.

Scale
  Optionally apply scaling by a given fatcor to the output spectra.

DensityOfStates
~~~~~~~~~~~~~~~

.. interface:: Simulation
  :widget: pgDOS

When loading a partial density of states (from a *.phonon* file) the following
additional options are available (note that they will be disabled when using a
*.castep* file):

Ion List
  Lists all the ions in a given file, individual ions can then be selected to be
  included in a partial density of states.

(De)Select All
  Provides a quick method of selecting or deselecting all ions in the current
  file.

Sum Ion Contributions
  If selected the contirbutions of each selected ion will be summed into a
  single :ref:`MatrixWorkspace`, otherwise a :ref:`WorkspaceGroup` with a
  :ref:`MatrixWorkspace` for each ion will be produced.

Scale by cross sections
  If selected the contribution for each ion will be multiplied by the given
  scattering cross section.

Raman
~~~~~

.. interface:: Simulation
  :widget: pgRaman

When loading a raman spectroscopy spectra the following additional options can
be used:

Temperature
  Temperature to use in Kelvin.

.. categories:: Interfaces Indirect
