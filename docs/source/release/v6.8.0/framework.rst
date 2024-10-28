=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New features
############
- :ref:`RemovePromptPulse <algm-RemovePromptPulse>` now has options to specify the time range of the data. This can speed up workflows which already know the data range such as :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder>`.
- :ref:`ConvertDiffCal <algm-ConvertDiffCal>` and :ref:`CalculateDIFC <algm-CalculateDIFC>` now have the option to specify Offset Mode, by selecting `Signed` you can calculate the offset for logarithmically binned data.
- :ref:`Rebin <algm-Rebin>` now has an optional parameter to explicitly set binning mode (linear/log/reverse log/power) regardless of binwidth sign or other properties set.  This can enforce a particular binning type, reducing inadvertent errors from errant binning mode specifications.
- New ouput property `OutputScalingWorkspace` in algorithm :ref:`Stitch1D <algm-Stitch1D>` generates a WorkspaceSingleValue containing the scale factor and its error. No workspace creation if left empty.
- New algorithm :ref:`GenerateGoniometerIndependentBackground <algm-GenerateGoniometerIndependentBackground>` extract the background from a dataset where sample is rotated through multiple positions.

Bugfixes
############
- Fixed bug in :ref:`UnGroupWorkspace <algm-UnGroupWorkspace>` where the algorithm history would not be added to the workspaces being ungrouped.
- :ref:`ExtractSpectra <algm-ExtractSpectra>` no longer sorts events when doing x-range trimming. The algorithm that extracts the events does not require it. This will likely move execution time of workflows from ExtractSpectra to elsewhere, but the overall execution may be reduced.
- Fix for the :ref:`LoadEMU <algm-LoadEMU>` and :ref:`LoadPLN <algm-LoadPLN>` loader to capture all the environment parameters in the datafile.
- :ref:`MaskBTP <algm-MaskBTP>` can now mask BIOSANS data containing the midrange detector.
- The progress bar for :ref:`LoadMD <algm-LoadMD>` now includes loading experiment information.


Fit Functions
-------------

Bugfixes
############
- An inconsistency in the E units when compared to Tau has been fixed in the documentation for the :ref:`StretchedExpFT <func-StretchedExpFT>` function.


Data Objects
------------

New features
############
- :ref:`GenerateGroupingPowder <algm-GenerateGroupingPowder>` was edited to allow for optional saving as a nexus file, and for different labeling methods.  To save as nexus file, set the property `FileFormat` to either `nxs` or `nx5`.  To distinguish left/right sides of instrument, set the property `AzimuthalStep` to a number other than 360.  To label groups in order, as opposed to by their angular position, set the property `NumberByAngle` to false.

Bugfixes
############
- Fixed bug in ``mantid.kernel.LogFilter`` usage case from the tutorial site
- Fixed bug in ``TimeSplitter`` where workspace rows with zero-length time were corrupting the time intervals map.
- Fixed bug in ``TimeSplitter.addROI()`` when the ROI starts at the beginning of the ``TimeSplitter`` object.
- It is now possible to use the archive on macOS once it has been mounted. Follow the archive mounting instructions here: https://developer.mantidproject.org/GettingStarted/GettingStarted.html#osx


Python
------

New features
############
- Upgraded to Python 3.10.
- :ref:`SNSPowderReduction <algm-SNSPowderReduction>`, can now perform linear interpolation for temperature of two empty container background runs using new :ref:`InterpolateBackground <algm-InterpolateBackground>` algorithm.
- New algorithm available, :ref:`InterpolateBackground <algm-InterpolateBackground>`, used to perform linear interpolation for temperature of two background runs.
- Drop support for numpy v1.21 because it's no longer maintained.
- Added a new documentation page on how to extend Mantid with a :ref:`pip install <pip-install-ref>`.


MantidWorkbench
---------------

See :doc:`mantidworkbench`.
:ref:`Release 6.8.0 <v6.8.0>`
