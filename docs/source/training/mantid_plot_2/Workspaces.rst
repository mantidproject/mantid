.. _Workspaces:

==========
Workspaces
==========

Workspaces contain data for inspection and visualisation.
They contain information about the experiment and the instrument for easy access via the graphical user interface or the Python programming language.
Various types of workspaces are available. Most commonly used will be TableWorkspaces, GroupWorkspaces and importantly MatrixWorkspaces.

A :literal:`MatrixWorkspace` contains the following values holding measured or derived data in form of a histogram or distribution:

- X: bin boundaries or points
- Y: counts or frequencies
- E: corresponding error values (variances, standard deviations)

.. figure:: /images/Workspaces1.png
   :align: center

A sub-group of the MatrixWorkspace is a Workspace2D consisting of two-dimensional data.

It may contain:

- Axes with Units
- Sample and sample environment data
- Run logs
- A full instrument geometric definition, along with an instrument parameter map
- A spectra-detector mapping
- A distribution flag
- A list of masked bins

For exanmple, a spectra-detector mapping table can be opened by right click on workspace 592724 and select 'Show Detectors':

.. figure:: /images/Workspaces3.png
   :align: center

and informs about each detector position (R, theta, phi) and if it is a monitor or not.

Please note the different meaning of

- Workspace indices: start counting from 0.
- Spectrum numbers: unique identifier of a spectrum. Multiple detectors can contribute to a spectrum.
- Detector IDs: unique identifier of a detector. Set by the instrument definition file (IDF), e.g. monitors have detector IDs 100000 and 100001.

Several workspaces can be combined to a :literal:`GroupWorkspace` which may be the preferred output for workflow algorithms.

.. figure:: /images/GroupWorkspace.png

:literal:`TableWorkspaces` descibe their content by column names and contain reduction specific data, e.g. fit results of peak positions

.. figure:: /images/Workspace4.png

==========
Algorithms
==========

From the main MantidPlot menu, select

View->Algorithms

Algorithms can take care of error propagation. For more details it is recommended to check the corresponding documentation, click on the ? sign in the left lower corner of the specific algorithm graphical user interface. Arithmetic operations propagate errors (Plus, Minus, ...). As well in Python.

Input validation
----------------

Some algorithms have requirements that will be checked before execution, e.g. units, common binning, etc.
This way, the user will be quickly and early guided to correct inputs rather than motivated reading error messages.

Results Log
-----------

Supports the following log levels:

Error:

.. figure:: /images/Algorithms7.png
   :align: center

Warning:

.. figure:: /images/Algorithms6.png
   :align: center

Notice:

.. figure:: /images/Algorithms5.png
   :align: center

Information:

.. figure:: /images/Algorithms3.png
   :align: center

Debug:

.. figure:: /images/Algorithms2.png
   :align: center

History
-------

Mantid keeps the entire history of all algorithms applied to workspaces. Not only does this allow you to audit the data reduction and analysis, it also provides the means to extract a re-executable python script from the GUI.

- Right click on workspace 592724 and select 'Show History'. This will open up the Algorihm History window. In the left-hand side Algorithms panel click on the arrow in front of LoadILLReflectometry v.1:

.. figure:: /images/History1.png
   :align: center

To replay the history do:

- From the main MantidPlot menu select View->Script Window, this opens the 'MantidPlot: Python Window'
- Go back to Algorithm History window and press the 'Script to Clipboard' button and close the Algorithm History window
- Flip to the script window ('MantidPlot: Python Window') and paste what was copied to the clipboard into the script window
- Delete the 592724 workspace from the Algorithms panel
- To recreate the work you have just done script window execute the script via Execute->Execute All on the script window.

Content Clipboard (Python):

.. code-block:: python

   LoadILLReflectometry(Filename='/net4/serdon/illdata/171/figaro/internalUse/rawdata/592724.nxs', OutputWorkspace='592724', XUnit='TimeOfFlight')
   GravityCorrection(InputWorkspace='592724', OutputWorkspace='592724_gc', FirstSlitName='slit3')
   Logarithm(InputWorkspace='592724_gc', OutputWorkspace='592724_gc')
   ConvertUnits(InputWorkspace='592724_gc', OutputWorkspace='592724_gc', Target='Wavelength', ConvertFromPointData=False)

=================
Saving workspaces
=================

Right click on workspace and `SaveNexus`

Via executing algorithms:
- SaveNexusProcessed (SaveNexus)
- SaveAscii
- SaveILLCosmosAscii

From the main MantidPlot menu, select
File->Save->Nexus
File->Export Ascii

.. figure:: /images/Saving1.png
   :align: center

In line 2, the spectrum number is given.

==========
Interfaces
==========

Main MantidPlot menu -> Interfaces -> Indirect -> Indirect Data Analysis

The Indirect Data Analysis interface is a collection of tools within MantidPlot
for analysing reduced data from indirect geometry spectrometers.

The majority of the functions used within this interface can be used with both
reduced files (*_red.nxs*) and workspaces (*_red*) created using the Indirect Data
Reduction interface or using :math:`S(Q, \omega)` files (*_sqw.nxs*) and
workspaces (*_sqw*) created using either the Indirect Data Reduction interface or
taken from a bespoke algorithm or auto reduction.

These interfaces do not support GroupWorkspaces as input.

.. figure:: /images/interface.png
