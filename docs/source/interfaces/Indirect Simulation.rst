Indirect Simulation
===================

.. contents:: Table of Contents
  :local:

Overview
--------

This interface contains loaders for data created by various simulation software.

.. interface:: Simulation
  :align: right
  :width: 350

Action Buttons
~~~~~~~~~~~~~~

Settings
  Opens the :ref:`Settings <interface-indirect-settings>` GUI which allows you to
  customize the settings for the Indirect interfaces.

?
  Opens this help page.

Manage Directories
  Opens the Manage Directories dialog allowing you to change your search directories
  and default save directory and enable/disable data archive search.

MolDyn
------

The MolDyn interface is used to import simulation data created using nMOLDYN (by
using the :ref:`MolDyn <algm-MolDyn>` algorithm), tab operates on either *.dat*
or *.cdl* files for nMOLDYN 3 or a directory containing the files extracted from
the *.tar* archive created by nMOLDYN 4.

.. interface:: Simulation
  :widget: molDyn

Options
~~~~~~~

Version
  The version of nMOLDYN the imported data was exported from.

Data
  The data file (*.cdl* or *.dat*) to load when using nMOLDYN 3 or the directory
  for the export taken from nMOLDYN 4.

Function Names
  A comma separated list of functions to load from a .cdl file.

Crop Max Energy
  Allows the maximum energy for loaded functions in energy to be cropped, this
  can be useful to remove the additional simulation data that is out of the
  energy range of an instrument.

Symmetrise Energy
  Symmetrises the functions in energy about x = 0.

Instrument Resolution
  Allows convolution with an instrument resolution file or workspace.

Run
  Runs the processing configured on the current tab.

Plot
  Plots either a spectra or contour plot (or both) of the output data.

Save Result
  Saves the result in the default save directory.

Sassena
-------

The Sassena interface is used to load simulations from the Sassena software.
This is done through the :ref:`LoadSassena <algm-LoadSassena>` algorithm.

.. interface:: Simulation
  :widget: sassena

Options
~~~~~~~

Sample File
  The data file (*.h5* or *.hd5*) to load.

Time per Data Point
  Specifies the time interval between each data point in the loaded data file.

Sort by Q Vectors
  If checked will sort the structure factors by momentum transfer in ascending order.

Run
  Runs the processing configured on the current tab.

Plot Result
  If clicked will create a spectra plot of the output data.

Save Result
  Saves the result in the default save directory.

DensityOfStates
---------------

The DensityOfStates interface is used to load CASTEP simulations using the
:ref:`SimulatedDensityOfStates <algm-SimulatedDensityOfStates>` algorithm. It supports loading
full and partial densities of states, raman and IR spectroscopy.

.. interface:: Simulation
  :widget: dos

Options
~~~~~~~

The following options are common to each spectrum type:

Spectrum Type
  Selects the type of spectrum to extract from the file.

Peak Shape
  Selects the shape of peaks to fit over the intensities extracted from the file.

Peak Width
  Sets the FWHM to which the fitted peaks should be broadened.

Bin Width
  Sets the histogram resolution for binning.

Zero Threshold
  Frequencies below this threshold will be ignored.

Scale by Factor
  Optionally apply scaling by a given factor to the output spectra.

DensityOfStates
~~~~~~~~~~~~~~~

When loading a partial density of states (from a *.phonon* file) the following
additional options are available (note that they will be disabled when using a
*.castep* file):

.. interface:: Simulation
  :widget: pgDOS

Ion List
  Lists all the ions in a given file, individual ions can then be selected to be
  included in a partial density of states.

(De)Select All
  Provides a quick method of selecting or deselecting all ions in the current
  file.

Sum Ion Contributions
  If selected, the contributions of each selected ion will be summed into a
  single :ref:`MatrixWorkspace`, otherwise a :ref:`WorkspaceGroup` with a
  :ref:`MatrixWorkspace` for each ion will be produced.

Scale by cross sections
  If selected the contribution for each ion will be multiplied by the given
  scattering cross section.

Raman
~~~~~

When loading a raman spectroscopy spectra the following additional options are available.

.. interface:: Simulation
  :widget: pgRaman

Temperature
  Temperature to use in Kelvin.

Other Options
~~~~~~~~~~~~~

Run
  Runs the processing configured on the current tab.

Plot Result
  If clicked will create a spectra plot of the outputted data.

Save Result
  Saves the result in the default save directory.

.. categories:: Interfaces Indirect
