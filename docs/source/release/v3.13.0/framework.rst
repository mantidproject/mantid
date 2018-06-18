=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Instrument Definition Updates
-----------------------------

- The ALF IDF has been updated following a detector array alteration.

Algorithms
----------

New Features
############

- A list of Related Algorithms has been added to each algorithm, and is displayed in the documentation page of each algorithm as part of it's summary.
- The error reporter now catches hard crashes to desktop.

New Algorithms
##############

- :ref:`LoadSampleShape <algm-LoadSampleShape>` loads a shape into the sample in a workspace from an 
  ASCII `STL <https://en.wikipedia.org/wiki/STL_(file_format)>`_  file,
  which contains a list of triangles or an 
  ASCII `OFF <https://en.wikipedia.org/wiki/OFF_(file_format)>`_ file, 
  which has a list of vertices and triangles. 

- :ref:`CalculateCarpenterSampleCorrection <algm-CalculateCarpenterSampleCorrection>` outputs a group workspace with the separate absorption and multiple scattering corrections for flexibility to the User to apply them to the sample workspace

- :ref:`CarpenterSampleCorrection <algm-CarpenterSampleCorrection>` replaces *MultipleScatteringCylinderAbsorption* and uses :ref:`CalculateCarpenterSampleCorrection <algm-CalculateCarpenterSampleCorrection>` for calculating its corrections. 

- :ref:`SaveBankScatteringAngles <algm-SaveBankScatteringAngles>` was added to save theta and phi values for diffraction banks to a MAUD-readable `grouping.new` file format

- :ref:`ExportSampleLogsToHDF5 <algm-ExportSampleLogsToHDF5>` saves a
  workspace's samples logs to an HDF5 file

- :ref:`SaveGDA <algm-SaveGDA>` saves a focused diffraction workspace to MAUD-readable ``.gda`` format

- :ref:`SaveGEMMAUDParamFile <algm-SaveGEMMAUDParamFile>`, which acts as a partner to :ref:`SaveGDA <algm-SaveGDA>`,
  saves a MAUD calibration file to convert the output of **SaveGDA** back to d-spacing

Improved
########

- :ref:`Maxent <algm-Maxent>` when outputting the results of the iterations, it no longer pads with zeroes but
  returns as many items as iterations done for each spectrum, making the iterations easy to count.
- XError values (Dx) can now be treated by the following algorithms: :ref:`ConjoinXRuns <algm-ConjoinXRuns>`, :ref:`ConvertToHistogram <algm-ConvertToHistogram>`, :ref:`ConvertToPointData <algm-ConvertToPointData>`, :ref:`CreateWorkspace <algm-CreateWorkspace>`, :ref:`SortXAxis <algm-SortXAxis>`, :ref:`algm-Stitch1D` and :ref:`algm-Stitch1DMany` (both with repect to point data).
- :ref:`Stitch1D <algm-Stitch1D>` can treat point data.
- The algorithm :ref:`SortXAxis <algm-SortXAxis>` has a new input option that allows ascending (default) and descending sorting. The documentation needed to be corrected in general.

Bug fixes
#########

- The documentation of the algorithm :ref:`algm-CreateSampleWorkspace` did not match its implementation. The axis in beam direction will now be correctly described as Z instead of X.
- The :ref:`ExtractMask <algm-ExtractMask>` algorithm now returns a non-empty list of detector ID's when given a MaskWorkspace.
- Fixed a crash when the input workspace for :ref:`GroupDetectors <algm-GroupDetectors>` contained any other units than spectrum numbers.
- :ref:`ConvertToMD <algm-ConvertToMD>` can now be used with workspaces that aren't in the ADS. 
- Fixed :ref:`SumSpectra <algm-SumSpectra>` to avoid a crash when validation of inputs was called with a WorkspaceGroup.
- Fixed a bug in TableWorkspaces where vector column data was set to 0 when the table was viewed    
- The output workspace of :ref:`LineProfile <algm-LineProfile>` now has correct sample logs, instrument and history.

New
###

- Algorithm :ref:`FitPeaks <algm-FitPeaks>` is implemented as a generalized multiple-spectra multiple-peak fitting algorithm.


Python
------

Improved
########

- Python fit functions that use from ``IPeakFunction`` as a base no longer require a ``functionDeriveLocal`` method to compute an analytical derivative. If
  the method is absent then a numerical derivative is calculate.

Bugfixes
########

- Checks on the structure of Python fit function classes have been improved to avoid scenarios, such as writing ``function1d`` rather than ``function1D``, which
  would previously have resulted in a hard crash.
- Fit functions defined in a python script can be used with the new fit function API right after sibscription.

:ref:`Release 3.13.0 <v3.13.0>`
