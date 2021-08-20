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
- A new algorithm :ref:`Stitch <algm-Stitch>` will perform stitching of multiple 2D workspaces by calculating the scale factors as medians of point-wise ratios in the overlap regions.
- All remote algorithms have been deprecated as they have not been used since v3.8.
- :ref:`SaveAscii <algm-SaveAscii>` will no longer throw an error if `WriteXErrors` is requested, but there are no `Dx` data present in the workspace.

Improvements
############

- :ref:`CreateSampleWorkspace <algm-CreateSampleWorkspace>` has new property InstrumentName.
- :ref:`CrossCorrelate <algm-CrossCorrelate>` has additional parameter to set the maximum d-space shift during cross correlation.
- :ref:`Load <algm-Load>` will now set output properties of all types, not just workspaces.
- :ref:`LoadRaw <algm-LoadRaw>` will now ignore empty ICPalarm log files.

Bugfixes
########

- Fix rare divide-by zero error when running :ref:`GetEi <algm-GetEi>` on noisy data.
- Fix crash when running :ref:`IntegrateEPP <algm-IntegrateEPP>` on a workspace group via the algorithm dialog.
- Fixed bug in :ref:`FitGaussianPeaks <algm-FitGaussianPeaks>` algorithm in which a peak at the end of range would cause an error due to not enough data point being available to fit parameters
- Fixed bug where :ref:`LoadRaw <algm-LoadRaw>` would not load all log files for raw files with an alternate data stream.

Fit Functions
-------------
- new method `IPeakFunction::intensityError` calculates the error in the integrated intensity of the peak due to uncertainties in the values of the fit parameters.
- Exposed method ''functionDeriv'' to the python interface


Data Objects
------------
- **Sample shapes which are CSGObjects can now be plotted. Shapes can also be merged, such as a sphere with a cylindrical hole. For more details see** :ref:`Mesh_Plots`.
- CSGObject Sample Shapes defined with :ref:`SetSample <algm-SetSample>` can be manually rotated using the ``rotate`` and ``rotate-all`` tags.
  Also, Sample shapes (both MeshObjects and CSGObjects) are automatically rotated by any rotations from :ref:`SetGoniometer <algm-SetGoniometer>`.
  This works with :ref:`CopySample <algm-CopySample>`, so the copied shape can be plotted, but the goniometer angle set on the new workspace is applied.

Python
------


.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Bugfixes
########
- Fix a bug (crash) in plotting MD workspaces when "Normalize to bin width" is set to False

Installation
------------


MantidWorkbench
---------------

See :doc:`mantidworkbench`.

Algorithms
----------

Improvements
############
- :ref:`LoadNexusMonitors <algm-LoadNexusMonitors-v2>` now utilizes the log filter provided by `LoadNexusLogs <algm-LoadNexusLogs>`

Bugfixes
########
- :ref:`LoadNexusLogs <algm-LoadNexusLogs>` now logs that are poorly formed create a warning message and the other logs are loaded. Previously it stopped loading logs at that point.

SliceViewer
-----------

Improvements
############

Bugfixes
########
- Fix cursor tracking from getting stuck and displaying incorrect signals when viewing MDHistogram workspaces in :ref:`sliceviewer`.
- Added parser for input Names to :ref:`algm-CreateMDHistoWorkspace` to allow inputs such as `Names='[H,0,0],[0,K,0],[0,0,L]'`.
- Fixed bug in :ref:`algm-ConvertToMDMinMaxLocal` where wrong min max calculated if the workspace includes monitor spectra or spectra without any detectors
- Fix bug in :ref:`CalculateMultipleScattering <algm-CalculateMultipleScattering>` where detector position was incorrectly determined on a workspace where the workspace index didn't match the detector
  index eg if the workspace was loaded with SpectrumMin specified to exclude some monitors

:ref:`Release 6.2.0 <v6.2.0>`
