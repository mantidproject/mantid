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

New Algorithms
--------------

- New algorithm :ref:`PaalmanPingsMonteCarloAbsorption <algm-PaalmanPingsMonteCarloAbsorption>` will calculate all 4 terms in self attenuation corrections following the Paalman and Pings formalism. Simple shapes are supported: FlatPlate, Cylinder, Annulus. Both elastic and inelastic as well as direct and indirect geometries are supported.


Algorithms
----------

- :ref:`SetSample <algm-SetSample>` is extended to support for composite shapes, such as FlatPlateHolder and HollowCylinderHolder. Also the input validation is made more stringent.
- Add specialization to :ref:`SetUncertainties <algm-SetUncertainties>` for the
   case where InputWorkspace == OutputWorkspace. Where possible, avoid the
   cost of cloning the inputWorkspace.
- Adjusted :ref:`AddPeak <algm-AddPeak>` to only allow peaks from the same instrument as the peaks worksapce to be added to that workspace.
- :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` Bug fixed where setting ResimulateTracksForDifferentWavelengths parameter to True was being ignored
- :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` Corrections are not calculated anymore for masked spectra
- :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` Corrections can be calculated for a workspace without a sample eg container only
- :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` now calculates the error on the absorption correction factors
- :ref:`MaskDetectorsIf <algm-MaskDetectorsIf>` has received a number of updates:

  - The algorithm now checks all of the data bins for each spectrum of a workspace, previously it only checked the first bin.
  - A new Operator option has been added `NotFinite` that allows you to mask detectors that contain infinite or `NaN <https://en.wikipedia.org/wiki/NaN>`_ values.

- Added an algorithm, :ref:`ISISJournalGetExperimentRuns <algm-ISISJournalGetExperimentRuns>`, which returns run information for a particular experiment from ISIS journal files.
- Enhanced :ref:`LoadNGEM <algm-LoadNGEM>` to handle partially written events in the data file.
   When such incomplete data is encountered, it is skipped until the next valid data is encountered and a
   warning is printed at algorithm completion of the total number of data bytes discarded.
- A bug introduced in v5.0 causing error values to tend to zero on multiple instances of :ref:`Rebin2D <algm-Rebin2D>` on the same workspace has been fixed.
- A form of reversible masking that could lead to misleading and incorrect results has been removed from Mantid,
  this means that ClearMaskedSpectra is no longer necessary after calling :ref:`MaskInstrument <algm-MaskInstrument>`
  and :ref:`MaskDetectorsIf <algm-MaskDetectorsIf>`.
  ClearMaskedSpectra has been removed as it no longer has a use,
  and :ref:`MaskInstrument <algm-MaskInstrument>` is now deprecated and you should use :ref:`MaskDetectors <algm-MaskDetectors>` instead.
- Add parameters to :ref:`LoadSampleShape <algm-LoadSampleShape>` to allow the mesh in the input file to be rotated and\or translated
- Algorithms now lazily load their documentation and function signatures, improving import times from the `simpleapi`.
- Added alias for GeneratePythonScript as ExportHistory
- Deprecated the RecordPythonScript algorithm

Data Handling
-------------

- Added a case to :ref:`Load <algm-Load>` to handle ``WorkspaceGroup`` as the output type

- Added an algorithm, :ref:`LoadILLPolarizedDiffraction <algm-LoadILLPolarizedDiffraction>` that reads raw NeXuS ILL D7 instrument data

- The material definition has been extended to include an optional filename containing a profile of attenuation factor versus wavelength. This new filename has been added as a parameter to these algorithms:

  - :ref:`SetSampleMaterial <algm-SetSampleMaterial>`
  - :ref:`LoadSampleEnvironment <algm-LoadSampleEnvironment>`

- The attenuation profile filename can also be specified in the materials section of the sample environment xml file
- Fixed a long standing bug where log filtering was not being applied after loading a Mantid processed NeXus file.  This now works correctly so
  run status and period filtering will now work as expected, as it did when you first load the file from a raw or NeXus file.
- The sample environment xml file now supports the geometry being supplied in the form of a .3mf format file (so far on the Windows platform only). Previously it only supported .stl files. The .3mf format is a 3D printing format that allows multiple mesh objects to be stored in a single file that can be generated from many popular CAD applications. As part of this change the algorithms :ref:`LoadSampleEnvironment <algm-LoadSampleEnvironment>` and :ref:`SaveSampleEnvironmentAndShape <algm-SaveSampleEnvironmentAndShape>` have been updated to also support the .3mf format
- Nexus log data alarms are now supported by Mantid. Log data that is marked as invalid will trigger a warning in the log and be filtered by default.  If the entire log is marked as invalid, then the values will be used as unfiltered as no better values exist, but the warning will still appear in the log.


The :ref:`LoadISISNexus <algm-LoadISISNexus>` algorithm has been modified to remove the need for the VMS compatibility block.
This has lead to the removal of the following variables from the sample logs as they were deemed unnecessary: dmp,
dmp_freq, dmp_units dur, dur_freq, dur_secs, dur_wanted, durunits, mon_sum1, mon_sum2, mon_sum3, run_header (this is available in the workspace title).

Data Objects
------------

- Added MatrixWorkspace::findY to find the histogram and bin with a given value
- Matrix Workspaces now ignore non-finite values when integrating values for the instrument view.  Please note this is different from the :ref:`Integration <algm-Integration>` algorithm.

Python
------
- A list of spectrum numbers can be got by calling getSpectrumNumbers on a
  workspace. For example: spec_nums = ws.getSpectrumNumbers()
- Documentation for manipulating :ref:`workspaces <02_scripting_workspaces>` and :ref:`plots <02_scripting_plots>` within a script have been produced.
- Property.units now attempts to encode with windows-1252 if utf-8 fails.
- Property.unitsAsBytes has been added to retrieve the raw bytes from the units string.
- Various file finding methods have been moved to ``mantid.api.InstrumentFileFinder``. For compatibility
  these still exist in ``ExperimentInfo`` but the helpers should be used instead in the future.
- A new method for finding IPF files has been added to the ``InstrumentFileFinder``
  ``getParameterPath``, which will accept an instrument name and return the full path to the associated
  IPF file.

Improvements
------------
- Updated the convolution function in the fitting framework to allow the convolution of two composite functions.

Bugfixes
--------
- Fix an uncaught exception when loading empty fields from NeXus files. Now returns an empty vector.

Deprecations
------------
- **CalculateMonteCarloAbsorption** and **SimpleShapeMonteCarloAbsorption** are deprecated in favour of the new :ref:`PaalmanPingsMonteCarloAbsorption <algm-PaalmanPingsMonteCarloAbsorption>`. While the new one provides more complete and more correct calculation, it can also fall back to the simpler calculation if the container is not specified.


:ref:`Release 5.1.0 <v5.1.0>`
