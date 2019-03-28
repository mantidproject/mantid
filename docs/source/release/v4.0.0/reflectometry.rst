=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New
###

- Added algorithm :ref:`algm-CreateFloodWorkspace` which makes a workspace for subsequent flood corrections.
- Added algorithm :ref:`algm-ApplyFloodWorkspace` which applies flood corrections to a workspace.
- :ref:`FindReflectometryLines <algm-FindReflectometryLines-v2>` has been rewritten and updated to version 2. The new version finds a single line by a Gaussian fit. Version 1 has been deprecated and will be removed in a future release.
- Added algorithm :ref:`algm-ReflectometrySliceEventWorkspace` which slices an input event workspace into multiple slices, producing a histogram workspace suitable for use with :ref:`algm-ReflectometryReductionOneAuto`.
- :ref:`algm-SaveReflectometryAscii` is a general algorithm which saves the first spectrum of a workspace in Ascii format particularly suited for reflectometry data.
- Some computations from :ref:`algm-ReflectometryMomentumTransfer` were extracted to a new algorithm, :ref:`algm-ReflectometryBeamStatistics`.
- :ref:`algm-GroupToXResolution` can be used to group the reflectivity data (as point data) to the :math:`Q_z` resolution.
- Added algorithm :ref:`algm-ReflectometryBackgroundSubtraction` which calculates the background of a given workspace and removes it.
- Added algorithm :ref:`algm-ReflectometryISISLoadAndProcess` which performs full preparation and processing for a single run or combined set of runs in an ISIS reflectometry reduction.

Improvements
############

- The ILL reduction workflow algorithms were reorganized to allow correct reflectivity calculation in the :literal:`SumInLambda` case.
- Added flood corrections to :ref:`ReflectometryReductionOneAuto <algm-ReflectometryReductionOneAuto-v2>`. The correction data can be provided either via a flood workspace passed as a property or taken from the parameter file.
- The four Ascii save algorithms :ref:`algm-SaveANSTOAscii`, :ref:`algm-SaveILLCosmosAscii`, :ref:`algm-SaveReflCustomAscii`, and :ref:`algm-SaveReflThreeColumnAscii` now correctly save x-error and can treat correctly point data and histograms. They are, however, deprecated in favour of :ref:`algm-SaveReflectometryAscii`. Please see :ref:`algm-SaveReflectometryAscii` for more documentation.
- :ref:`algm-ReflectometryReductionOneAuto` now supports the Wildes method for polarization corrections as well as Fredrikze when configured in the parameters file.
- :ref:`algm-ReflectometryReductionOneAuto` now crops the unbinned output workspace in Q to ``MomentumTransferMin`` and ``MomentumTransferMax``, if specified
- :ref:`algm-ReflectometryReductionOne`, :ref:`algm-ReflectometryReductionOneAuto`, :ref:`algm-CreateTransmissionWorkspace`, and :ref:`algm-CreateTransmissionWorkspaceAuto` now use spectrum numbers for their processing instructions instead of workspace indices.
- :ref:`algm-ReflectometryReductionOne` and :ref:`algm-ReflectometryReductionOneAuto` now take a parameter to pass processing instructions to the transmission workspace algorithms and no longer accept strict spectrum checking.
- Common naming of slit component name and size properties across algorithms.
- :ref:`algm-SpecularReflectionPositionCorrect` is now compatible with the reflectometers at ILL.
- :ref:`algm-CreateTransmissionWorkspace` and :ref:`algm-CreateTransmissionWorkspaceAuto` now use NormalizeByIntegratedMontitors instead of using MonitorIntegrationWavelengthMin and MonitorIntegrationWavelengthMax being defined, to determine how to normalize. 
- ILL reflectometry algorithms name changes: *LinePosition* replaces *BeamCentre*, *DirectLineWorkspace* replaces *DirectBeamWorkspace*, and *TwoTheta* is the preferred input property which replaces *BraggAngle*.
- :ref:`algm-ReflectometryILLPreprocess` does not accept and will not output an `OutputBeamPositionWorkspace`. The peak position and two theta scattering anlge are now present in the SampleLog information as documented.
- ILL's D17 and Figaro instrument definition files are modified to let the detector ID's begin with index 0 instead of 1.
- :ref:`algm-LoadILLReflectometry` lets spectrum numbers start with 0 instead of 1.

Bug fixes
#########

- Fixed the error propagation in :math:`Q` grouping in :ref:`ReflectometryILLConvertToQ <algm-ReflectometryILLConvertToQ>`.
- Handling of group workspaces containing single workspaces when scaling by period and using :literal:`ScaleFactorFromPeriod`, i.e. :literal:`UseManualScaleFactors` is true, :literal:`ManualScaleFactors` remains empty.
- A bug has been fixed on the Settings tab where the IncludePartialBins check box had been hidden by a misplaced text entry box.
- :ref:`algm-ReflectometryReductionOneAuto` No longer sums all of a transmission run's workspaces and instead will use the first run only
- In :ref:`algm-ReflectometryReductionOneAuto` an issue where if you gave only one of either MomentumTransferMax or MomentumTransferMin were specified it would be ignored, this has been fixed.
- Reverted property names for polarization correction coefficients in :ref:`ReflectometryReductionOneAuto <algm-ReflectometryReductionOneAuto-v2>` for backwards compatibility.
- Fixed the ``BeamCenter`` property of :ref:`ReflectometrySumInQ <algm-ReflectometrySumInQ>` to actually be a fractional workspace index.

Liquids Reflectometer
---------------------

- Default x-direction pixel range for the scaling factor calculation is now set to the full width of the detector as opposed to a restricted guess.

Magnetism Reflectometer
-----------------------

- Added option to overwrite :literal:`DIRPIX` and :literal:`DANGLE0`.
- Added option to skip the final rebinning.

ISIS Reflectometry Interface
----------------------------

This is the last release of Mantid which will include the old reflectometry interface. It will be removed in version 4.1.

New
###

- The tabs have been restructured so that sets of related tabs now reside inside a ``Batch``. This replaces the old structuring where there were two ``Groups`` inside every tab and the user had to toggle to the correct group separately on each tab. You can create and delete Batches using the menu and the tabs on the left.
- The Settings tab has been split into two separate tabs, ``Experiment Settings`` and ``Instrument Settings``.
- The main table on the Runs tab now supports many additional keyboard shortcuts for entering text and navigation (see the :ref:`ISIS Reflectometry Interface <interface-isis-refl>` documentation for details).
- Better validation has been added to input components to highlight invalid values or disallow entering them where possible.
- Flood correction options have been added to the ``Experiment Settings`` tab.

Improvements
############

- The interface now supports the Wildes method for polarization corrections as well as Fredrikze when configured in the parameters file.

Bug fixes
#########

- The SaveASCII tab from the interface was unable to save in some places on Windows and that has now been fixed.

:ref:`Release 4.0.0 <v4.0.0>`
