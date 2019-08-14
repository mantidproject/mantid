=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:


Concepts
--------

- :class:`mantid.kernel.IntArrayProperty` now supports specifying step sizes
- Support has been added for negative indexing of :ref:`WorkspaceGroups <WorkspaceGroup>`.
  Try :code:`ws_group[-1]` to get the last workspace in the WorkspaceGroup :code:`ws_group`.
- Updated the clone method of IFunction to copy parameter errors across as well as parameter values.

Algorithms
----------

New
###

- :ref:`RemoveSpectra <algm-RemoveSpectra>` is a new general purpose algorithm that enables you to optionally remove spectra from a workspace given one of 3 criteria, if it's masked, if it doesn't have a detector, and if presented in a user defined list.

Improvements
############

- All the ILL nexus file loaders were speeded up due to parallelization of the loading.
- :ref:`LoadEventNexus <algm-LoadEventNexus>` has an additional option `LoadNexusInstrumentXML` = `{Default, True}`,  which controls whether or not the embedded instrument definition is read from the NeXus file.
- The numerical integration absorption algorithms (:ref:`AbsorptionCorrection <algm-AbsorptionCorrection>`, :ref:`CuboidGaugeVolumeAbsorption <algm-CuboidGaugeVolumeAbsorption>`, :ref:`CylinderAbsorption <algm-CylinderAbsorption>`, :ref:`FlatPlateAbsorption <algm-FlatPlateAbsorption>`) have been modified to use a more numerically stable method for performing the integration, `pairwise summation <https://en.wikipedia.org/wiki/Pairwise_summation>`_.
- Improved support for thin-walled hollow cylinder shapes in :ref:`algm-MonteCarloAbsorption`
- :ref:`LoadSampleEnvironment <algm-LoadSampleEnvironment>` and :ref:`LoadSampleShape <algm-LoadSampleShape>` now support setting the scale of the stl to millimetres, centimetres, or metres. It also takes rotation as angles applied in order X then Y then Z, as opposed to a matrix.
- :ref:`LoadSampleShape <algm-LoadSampleShape>` now rotates the sample based off of the setting of the goniometer.
- :ref:`GenerateEventsFilter <algm-GenerateEventsFilter>` is able to accept any `MatrixWorkspace`, as long as it has run objects loaded from :ref:`LoadNexusLogs <algm-LoadNexusLogs>`, other than `EventWorkspace`.
- :ref:`AbsorptionCorrection <algm-AbsorptionCorrection>` has a new property `ScatterFrom` which allows for calculating the correction for the other components (i.e. container and environment)
- :ref:`SetSample <algm-SetSample>` can calculate the density from the sample mass
- We have prevented an error due to locale settings which may appear when reading, for instance, the incident energy Ei value from the logs in :ref:`ConvertUnits <algm-ConvertUnits>` and many other algorithms.
- :ref:`Pseudo-Voigt <func-PseudoVoigt>` has been modified to be more in line with FULLPROF and GSAS.  One of its basic parameter, Height, is changed to Intensity.
- :ref:`CalculatePolynomialBackground <algm-CalculatePolynomialBackground>` has an option to select the the minimizer used during fitting.
- :ref:`CalculatePolynomialBackground <algm-CalculatePolynomialBackground>` has been modified to pass parameters with double precision to make it more correct.
- 10x performance improvement in calls to ``Mantid::PhysicalConstants::getAtom``.
- :ref:`SetSample <algm-SetSample>` will now look for facility wide sample environments. instrument specific ones will be loaded first.
- :ref:`SolidAngle <algm-SolidAngle>` is extended to accommodate new options for fast analytical calculation for SANS-type detectors.
- :ref:`FilterEvents <algm-FilterEvents>` has a property `InformativeOutputNames` which changes the name of output workspace to include the start and end time of the slice.
- :ref:`algm-SumOverlappingTubes` was speeded up due to parallelization of the actual histogramming step.
- :ref:`CylinderAbsorption <algm-CylinderAbsorption>` now has a `CylinderAxis` property to set the direction of the cylinder axis.
- :ref:`SaveNexusProcessed <algm-SaveNexusProcessed>` now throws a clearer error on saving of nested groups of groups.

Instrument Definition Files
###########################

- A new attribute, name-count-increment, has been introduced to the <locations> tag which allows the auto-generated location names to be incremented by a user-defined amount.
- ARCS, CNCS, HYSPEC, NOMAD, POWGEN, SEQUOIA, SNAP, and VULCAN have had the axis that signed two-theta is calculated against changed from ``+y`` to ``+x``

Bug fixes
#########

- :ref:`SetSample <algm-SetSample>` now correctly handles the Sample number density being passed as a string, before the algorithm would execute, but silently ignored the provided number density, the number density is now properly used.
- Mantid no longer crashed when invalid period logs encountered in `LoadEventNexus <algm-LoadEventNexus>`. A clear error message is displayed which explains the problem.
- ISIS sample logs are now correctly filtered by status and period on loading.
- A bug has been fixed in status log filtering where if a log contained times before the first running log entry then they would be included rather than excluded.

Removed
#######

- The deprecated version 1 of the `FindEPP` algorithm has been removed. Use :ref:`FindEPP-v2 <algm-FindEPP>` instead.

Data Objects
------------
- Added method `isCommonLogBins` to check if the `MatrixWorkspace` contains common X bins with logarithmic spacing.

Python
------

New
###

- The ``mantid.plots`` module now registers a ``power`` and ``square`` scale type to be used with ``set_xscale`` and ``set_xscale`` functions.
- In :class:`mantid.kernel.DateAndTime`, the method :py:meth:`~mantid.kernel.DateAndTime.total_nanoseconds` has been deprecated, :py:meth:`~mantid.kernel.DateAndTime.totalNanoseconds` should be used instead.
- In :class:`mantid.kernel.time_duration`, The method :py:meth:`~mantid.kernel.time_duration.total_nanoseconds` has been deprecated, :py:meth:`~mantid.kernel.time_duration.totalNanoseconds` should be used instead.
- :py:obj:`mantid.geometry.DetectorInfo.indexOf` has been exposed to python
- :code:`indices` and :code:`slicepoint` options have been added to :ref:`mantid.plots <mantid.plots>` to allow selection of which plane to plot from an MDHistoWorkspace. :code:`transpose` has also been added to transpose the axes of any 2D plot.

Bugfixes
########

- The TypeError raised when calibrating tubes has been fixed.

:ref:`Release 4.1.0 <v4.1.0>`
