.. _TrainingWorkspaces:

==========
Workspaces
==========

Workspaces contain data for inspection and visualisation.
They contain information about the experiment and the instrument for easy access via the graphical user interface or the Python programming language.
Various types of workspaces are available. Most commonly used will be TableWorkspaces, GroupWorkspaces and importantly MatrixWorkspaces.

A :literal:`MatrixWorkspace` contains the following values holding measured or derived data in form of a histogram or distribution:

- X: bin boundaries or points
- Y: counts or frequencies
- E: corresponding error values
- Dx: X error data 

.. figure:: /images/Training/WorkspacesAlgorithmsInterfaces/Workspaces1.png
   :align: center

Please note that 1D plots will draw the Y data normalised by bin width (default) and at bin centre position.
A right click inside the plot and select 'Normalization' in order to change this behaviour, e.g. switching the normalization off.

Create a new MatrixWorkspace:

Main menu -> File -> New -> Matrix

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

.. figure:: /images/Training/WorkspacesAlgorithmsInterfaces/Workspaces3.png
   :align: center

and informs about each detector position (R, theta, phi) and if it is a monitor or not.

Please note the different meaning of

- Workspace indices: start counting from 0, used within Python.
- Spectrum numbers: unique identifier of a spectrum. Multiple detectors can contribute to a spectrum.
- Detector IDs: unique identifier of a detector. Set by the instrument definition file (IDF), e.g. monitors have detector IDs 100000 and 100001.

:literal:`TableWorkspaces` descibe their content by column names and contain, for instance, fit results of peak positions

.. figure:: /images/Training/WorkspacesAlgorithmsInterfaces/Workspace4.png
   :align: center

Create a new table workspace:

Main menu -> File -> New -> Table

Several workspaces can be combined to a :literal:`GroupWorkspace` which may be the preferred output for workflow algorithms.

.. figure:: /images/Training/WorkspacesAlgorithmsInterfaces/GroupWorkspace.png
   :align: center

Saving Workspaces
-----------------

The recommended way is to save as a Mantid NeXus file, as this will preserve extra information such as the instrument and workspaces history.

To do this right click on workspace and :code:`SaveNexus`.

Via executing algorithms:

- :ref:`SaveNexusProcessed <algm-SaveNexusProcessed>` (SaveNexus)
- :ref:`SaveAscii <algm-SaveAscii>`
- :ref:`SaveILLCosmosAscii <algm-SaveILLCosmosAscii>`
- ...

From the main MantidPlot menu, select
File->Save->Nexus
File->Export Ascii

.. figure:: /images/Training/WorkspacesAlgorithmsInterfaces/Saving1.png
   :align: center

In line 2, the spectrum number is given.
