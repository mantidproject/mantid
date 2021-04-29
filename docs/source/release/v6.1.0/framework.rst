=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Concepts
--------

Algorithms
----------

- :ref:`LoadNexusLogs <algm-LoadNexusLogs>` has additional parameters to allow or block specific logs from being loaded.
- :ref:`LoadEventNexus <algm-LoadEventNexus>` now utilizes the log filter provided by `LoadNexusLogs <algm-LoadNexusLogs>`.
- New algorithm :ref:`GenerateLogbook <algm-GenerateLogbook>`, that allows creating TableWorkspace
  logbooks based on provided directory path with rawdata.
- :ref:`CompareWorkspaces <algm-CompareWorkspaces>` compares the positions of both source and sample (if extant) when property `checkInstrument` is set.
- :ref:`SetGoniometer <algm-SetGoniometer>` can now set multiple goniometers from log values instead of just the time-avereged value.
- Added the ability to specify the spectrum number in :ref:`FindPeaksAutomatic <algm-FindPeaksAutomatic>`.
- :ref:`LoadLog <algm-LoadLog>` will now detect old unsupported log files and set an appropriate explanatory string in the exception.
- :ref:`Stitch1DMany <algm-Stitch1DMany>` has additional property `IndexOfReference` to allow user to decide which of the provided workspaces should give reference for scaling
- New algorithm :ref:`CalculateMultipleScattering <algm-CalculateMultipleScattering>` to calculate multiple scattering corrections using a Monte Carlo integration approach that doesn't rely on an isotropic scattering assumption. The implementation is based on Fortran code developed by Mike Johnson and Spencer Howells under the names Muscat, MODES and DISCUS. The algorithm only supports elastic instruments so far but support for inelastic instruments will be added at a later date.
- New algorithm :ref:`GeneratePythonFitScript <algm-GeneratePythonFitScript>` allows the creation of a python script for sequential fitting.
- :ref:`SaveAscii <algm-SaveAscii>` can now create a header for the output file containing sample logs specified through the new property `LogList`.

Improvements
------------
- Loading a CORELLI tube calibration returns a ``MaskWorkspace``.

Data Objects
------------

- exposed ``geographicalAngles`` method on :py:obj:`mantid.api.SpectrumInfo`
- ``BinEdgeAxis`` now overrides the label in order to return the bin center and not the edge
- :ref:`Run <mantid.api.Run>` has been modified to allow multiple goniometers to be stored.
- :ref:`FileFinder <mantid.api.FileFinderImpl>` has been modified to improve search times when loading multiple runs on the same instrument.

Python
------


.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Installation
------------


MantidWorkbench
---------------

See :doc:`mantidworkbench`.

SliceViewer
-----------

Improvements
############

Bugfixes
########

- Axes limits correctly reset when home clicked on sliceviewer plot of ragged matrix workspace.
- Fix problem with dictionary parameters on :ref:`SetSample <algm-SetSample>` algorithm when running from the algorithm dialog
- Fix segmentation fault when running :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` algorithm on Ubuntu without a material defined on one of the sample\environment shapes
- Fix calculation of region where scattering points are sampled in :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` when a shape is defined for the environment but not the sample
- Fix crash on macOS when creating a UnitLabel with non-ascii characters using the single argument constructor
- Fix bug in the ass calculation in :ref:`PaalmanPingsMonteCarloAbsorption <algm-PaalmanPingsMonteCarloAbsorption>` when run on shapes already present on input workspace

:ref:`Release 6.1.0 <v6.1.0>`
