========================
Direct Inelastic Changes
========================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Algorithms
----------


New Algorithms
##############

- Added a new algorithm to ILL's reduction workflow: :ref:`DirectILLTubeBackground <algm-DirectILLTubeBackground>` which can be used to calculate the time-independent backgrounds for instruments with PSD detectors such as IN5.
- The new algorithm :ref:`SofTwoThetaTOF <algm-SofTwoThetaTOF>` can be used to convert a workspace from (spectrum number, TOF) units to (:math:`2\theta`, TOF) averaging the intensities over constant scattering angles.

Improvements
############

- :ref:`DirectILLIntegrateVanadium <algm-DirectILLIntegrateVanadium>` now masks zero counting detectors from the integral.
- Changes to :ref:`ComputeCalibrationCoefVan <algm-ComputeCalibrationCoefVan>`:

  - It is now possible to turn off the Debye-Waller correction
  - The temperature sample log entry can be given in an instrument parameter ``temperature_sample_log``.
  - The temperature sample log can now be a time series.

- :ref:`ComputeIncoherentDOS <algm-ComputeIncoherentDOS>` now supports computation from :math:`S(2\theta,E)` workspace.
- The upper limit of the empty container scaling factor in :ref:`DirectILLApplySelfShielding <algm-DirectILLApplySelfShielding>` has been removed.

Bugfixes
########

- Fixed a bug in :ref:`DirectILLCollectData <algm-DirectILLCollectData>` which prevented the *OutputIncidentEnergyWorkspace* being generated if *IncidentEnergyCalibration* was turned off.
- Fixed the detector :math:`2\theta` width calculation in :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>`. The algorithm was computing the angle between the detector center and top point, not the actual :math:`2\theta` width.

Interfaces
----------


New features
############

- Added the ability to save the results of the TOFTOF reduction as Ascii files.


Improvements
############

- New instrument geometry for CNCS
- Improved ``Save``-section of the TOFTOF reduction dialog.
- Behavior of the :ref:`LoadDNSLegacy <algm-LoadDNSLegacy>` for TOF data has been changed: the algorithm does not try to guess elastic channel any more, but asks for the user input.
- :ref:`LoadDNSSCD <algm-LoadDNSSCD>` has been improved to be able to load TOF data.
- :ref:`MDNormDirectSC <algm-MDNormDirectSC>` now can handle merged MD workspaces.

Python
------


Improved
########

- The ``directtools`` plotting and utility module has been updated with improved automatic E ranges, cut labels and other visuals. All functions now should also be applicable to non-ILL data as well.

:ref:`Release 3.14.0 <v3.14.0>`

