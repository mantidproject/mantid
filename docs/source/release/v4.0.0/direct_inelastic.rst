========================
Direct Inelastic Changes
========================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New Algorithms
##############

- Added a new algorithm to ILL's reduction workflow: :ref:`DirectILLTubeBackground <algm-DirectILLTubeBackground>` which can be used to calculate the time-independent backgrounds for instruments with PSD detectors such as IN5.
- The new algorithm :ref:`SofTwoThetaTOF <algm-SofTwoThetaTOF>` can be used to convert a workspace from (spectrum number, TOF) units to (:math:`2\theta`, TOF) averaging the intensities over constant scattering angles.
- The new algorithm :ref:`MDNorm <algm-MDNorm>` can be used to calculate cross section for single crystal direct inelastic measurements.
- :ref:`LoadPLN <algm-LoadPLN>` loader for an ANSTO PELICAN event file.
- Version 3 of :ref:`algm-GetEiMonDet` is now available. The upgraded version now fits a Gaussian to summed detector data making the energy calculation more robust. Version 2 has been deprecated and will be removed in a future release.

Improvements
############

- :ref:`DirectILLIntegrateVanadium <algm-DirectILLIntegrateVanadium>` now masks zero counting detectors from the integral.
- Changes to :ref:`ComputeCalibrationCoefVan <algm-ComputeCalibrationCoefVan>`:

  - It is now possible to turn off the Debye-Waller correction.
  - The temperature sample log entry can be given in an instrument parameter ``temperature_sample_log``.
  - The temperature sample log can now be a time series.

- :ref:`ComputeIncoherentDOS <algm-ComputeIncoherentDOS>` now supports computation from :math:`S(2\theta,E)` workspace.
- The upper limit of the empty container scaling factor in :ref:`DirectILLApplySelfShielding <algm-DirectILLApplySelfShielding>` has been removed.
- :ref:`ConvertToMD <algm-ConvertToMD>` now has `ConverterType = {Default, Indexed}` setting: `Default` keeps the old
  version of algorithm, `Indexed` provide the new one with better performance and some restrictions
  (see :ref:`ConvertToMD <algm-ConvertToMD>` Notes).
- :ref:`DirectILLCollectData <algm-DirectILLCollectData>` now automatically disables incident energy calibration and normalises to time instead of monitor counts if the monitor counts are deemed too low.
- There are two new properties in ref:`DirectILLReduction <algm-DirectILLReduction>`:

  - ``EnergyRebinning`` allows mixing automatic bin widths with user specified ones when rebinning the energy transfer axis.
  - ``GroupingAngleStep`` allows one to specify a :math:`2\theta` step for detector grouping. By default, a step of 0.01 degrees or the value of ``natural-angle-step`` instrument parameter is used.

- The ``SofQW`` algorithms have a new property ``DetectorTwoThetaRanges`` which can be used to supply detector scattering angle coverage information for :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>`.
- The built-in version of MSlice has been updated to include the full CLI, generating scripts from plots and waterfall plotting.

Bugfixes
########

- Fixed a bug in :ref:`DirectILLCollectData <algm-DirectILLCollectData>` which prevented the *OutputIncidentEnergyWorkspace* being generated if *IncidentEnergyCalibration* was turned off.
- Fixed the detector :math:`2\theta` coverage calculation in :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>`. The algorithm was computing the angle between the detector center and top point, not the actual minimum and maximum :math:`2\theta`. The width is now calculated accurately for cylinder and cuboid shapes. For other shapes, an approximate method is used.
- Fixed a bug in :ref:`Rebin2D <algm-Rebin2D>` which requires that an input workspace had to have fractional area weights for the `UseFractionalArea` option to work. The behaviour is now that if the input workspace does not have fractional areas, and `UseFractionalArea` is true, then fractional area tracking will be used with input fractions set to unity.
- :ref:`LoadILLTOF <algm-LoadILLTOF>` now properly closes the loaded file.

Interfaces
----------

New features
############

- Added the ability to save the results of the TOFTOF reduction as Ascii files.


Improvements
############

- New instrument geometry for CNCS.
- Improved ``Save``-section of the TOFTOF reduction dialog.
- Behavior of the :ref:`LoadDNSLegacy <algm-LoadDNSLegacy>` for TOF data has been changed: the algorithm does not try to guess elastic channel any more, but asks for the user input. Neutron wavelength can optionally be specified as user input.
- :ref:`LoadDNSSCD <algm-LoadDNSSCD>` has been improved to be able to load TOF data.
- :ref:`MDNormDirectSC <algm-MDNormDirectSC>` now can handle merged MD workspaces.

Bugfixes
########

- Several bugs in :ref:`PyChop <PyChop>` have been fixed, including the printing out of multiple Ei reps in the "Show Ascii" dialog, the disappearing axes labels in the Q-E tab, and incorrect energies in the multi-rep calculations. The calculation of the time width for LET has also been corrected for the relative sizes of the disk slots and the guide opening, which is important for "High Flux" mode calculations, where the energy widths were calculated to be narrower than is really the case. Finally the time-distance diagrams have been updated with the option to only show the first frame, and the MARI instrument file has been updated with the measured MARI flux.


Python
------

Improvements
############

- The ``directtools`` plotting and utility module has been updated:

  - Added a new function :func:`directtools.plotDOS` to support plotting the density-of-states.
  - Improved the automatic E ranges, cut labels and other visuals.
  - All functions should be applicable to non-ILL data.
  - ``defaultrcParams`` was renamed to ``defaultrcparams`` to be consistent with the rest of the functions.

Instrument definitions
----------------------

Improvements
############

- IN5: pixel radius and height are now more realistic and the detector ID of the monitor is now 100000 instead of 0.

:ref:`Release 4.0.0 <v4.0.0>`
