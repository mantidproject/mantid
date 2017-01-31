=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

Algorithms
----------

* New versions of algorithms :ref:`algm-ReflectometryReductionOne` and :ref:`algm-CreateTransmissionWorkspace`
  have been added to remove duplicate steps in the reduction. An improvement in performance of factor x3 has
  been observed when the reduction is performed with no monitor normalization.

* New versions of :ref:`algm-ReflectometryReductionOneAuto`, :ref:`algm-CreateTransmissionWorkspaceAuto` and
  :ref:`algm-SpecularReflectionPositionCorrect` have been added. The new versions fix the following known issues:

  * When :literal:`CorrectionAlgorithm` was set to :literal:`AutoDetect` the algorithm was not able to find polynomial
    corrections, as it was searching for :literal:`polynomial` instead of :literal:`polystring`.
  * When an correction algorithm was applied, monitors were integrated if :literal:`NormalizeByIntegratedMonitors`
    was set to true, which is the default. In the new version of the algorithms, monitors will never be integrated if a correction algorithm
    is specified, even if :literal:`NormalizeByIntegratedMonitors` is set to true.
  * Fix some problems when moving the detector components in the instrument. The new version uses :literal:`ProcessingInstructions`
    to determine which detector components need to be moved.
  * Monitor integration range was not being applied properly to CRISP data. The problem was that in the parameter
    file the wavelength range used to crop the workspace in wavelength is [0.6, 6.5] and the monitor integration range is outside of these limits ([4, 10]). This was causing the algorithm to integrate over [0.6, 4].
  * :ref:`algm-ReflectometryReductionOneAuto` rebins and scales the output workspace and outputs a third output workspace, :literal:`OutputWorkspaceBinned`.

* :ref:`algm-Stitch1D` documentation has been improved, it now includes a workflow diagram illustrating the different steps in the calculation and a note about how errors are propagated.

* :ref:`Stitch1DMany <algm-Stitch1DMany>` has a new property 'ScaleFactorFromPeriod' which enables it to apply scale factors from a particular period when stitching group workspaces. Its documentation has also been
  updated and improved, including more detail on algorithm properties and adds a workflow description and diagram.

* :ref:`algm-ConvertToReflectometryQ` corrects the detector position before performing any type of calculation. Detectors are corrected to an angle theta read from the log value *stheta*.

Reflectometry Reduction Interface
---------------------------------

ISIS Reflectometry (Polref)
###########################

- Settings tab now displays individual global options for experiment and instrument settings.
- New 'Save ASCII' tab added, similar in function and purpose to the 'Save Workspaces' window accessible from Interfaces->ISIS Reflectometry->File->Save Workspaces.
- When runs are transferred to the processing table groups are now labeled according to run title.
- Column :literal:`dQ/Q` is used as the rebin parameter to stitch workspaces.
- Fixed a bug where if the user answered 'no' to a popup asking if they wanted to process all runs, the progress bar would show activity as though a data reduction was occurring.
- The interface is now arranged in two different groups. Groups apply to tabs 'Run' and 'Settings'.
- Documentation regarding the interface has been updated accordingly.
- Error messages are displayed if the user either attempts to transfer zero runs or transfer runs with a different strategy to the one they used to search for runs with. 
- A new tab, 'Event handling' has been added. This tab allows users to select custom time slices to analyze the data.

ISIS Reflectometry
##################

- Processing runs now produces the un-binned IvsQ workspace as well.
- Error in transfer button not working fixed.
- Error in 'Save Workspaces' dialog not populating the save path correctly fixed.

|

`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Reflectometry%22>`__
