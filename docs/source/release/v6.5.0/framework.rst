=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

New Features
------------
- GSL has been updated to v2.7.


Algorithms
----------

New features
############
- Added Sphere to the preset shapes supplied in :ref:`SetSample <algm-SetSample>`.
- The algorithm :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` now simulates scattering in a container (and more generally in any sample environment components) if a user sets up the shapes\materials in the input workspace prior to running the algorithm. As part of this work the previous requirement that the :math:`S(Q,\omega)` profile must have some non-zero values for :math:`\omega <0` has been removed.
- The algorithm :ref:`FitIncidentSpectrum <algm-FitIncidentSpectrum>` is now able to return the second derivative of the fitted function in addition to the standard outputs of the fitted function and its first derivative.

Bugfixes
############
- Fixed bug for :ref:`ApplyPaalmanPingsCorrection <algm-ApplyPaalmanPingsCorrection>` when the input and output workspace names were identical for this algorithm and the child :ref:`Minus <algm-Minus>` algorithm.
- :ref:`SetSample <algm-SetSample>` now accepts numeric inputs of type float, int, or string, rather than requiring float.
- :ref:`ExtractSpectra <algm-ExtractSpectra>` now checks if the bins flagged as masked are still there after processing, and if they are not, the masking flag is unchecked for that spectrum.
- Single valued workspaces can now be saved by :ref:`SaveNexusProcessed <algm-SaveNexusProcessed>` and are properly loaded by :ref:`LoadNexusProcessed <algm-LoadNexusProcessed>`.
- If an event nexus file is loaded using :ref:`LoadEventNexus <algm-LoadEventNexus>` with ``LoadAllLogs=True``, any duplicate log entries no longer give an error. Log entries that are nested lower than two levels down are also properly distinguished.
- Fix a crash that could happen in :ref:`LoadEventNexus <algm-LoadEventNexus>` if there are no pulse times.
- :ref:`GenerateLogbook <algm-GenerateLogbook>` now does not stop the execution when optional headers are not defined in the relevant :ref:`Instrument Parameter File <InstrumentParameterFile>`, but only sends a warning that they are not available.
- Fixed an issue in :ref:`Fit <algm-Fit>` where a lack of operator between parameter and value caused a hard crash. The input string is now validated.
- Fixed bug in documentation of :ref:`GroupToXResolution <algm-GroupToXResolution>` where the figures showing the algorithm output were all blank.


MantidWorkbench
---------------

See :doc:`mantidworkbench`.

:ref:`Release 6.5.0 <v6.5.0>`
