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

New Algorithms
##############

- :ref:`LoadSampleShape <algm-LoadSampleShape>` loads a shape into the sample in a workspace from an STL file,
  which contains a list of triangles.

- :ref:`CalculateCarpenterSampleCorrection <algm-CalculateCarpenterSampleCorrection>` outputs a group workspace with the separate absorption and multiple scattering corrections for flexibility to the User to apply them to the sample workspace

- :ref:`CarpenterSampleCorrection <algm-CarpenterSampleCorrection>` replaces *MultipleScatteringCylinderAbsorption* and uses :ref:`CalculateCarpenterSampleCorrection <algm-CalculateCarpenterSampleCorrection>` for calculating its corrections. 

Improved
########

- :ref:`Maxent <algm-Maxent>` when outputting the results of the iterations, it no longer pads with zeroes but
  returns as many items as iterations done for each spectrum, making the iterations easy to count.
- :ref:`ConvertToPointData <algm-ConvertToPointData>` and :ref:`ConvertToHistogram <algm-ConvertToHistogram>` now propagate the Dx errors to the output.
- The algorithm :ref:`CreateWorkspace <algm-CreateWorkspace>` can now optionally receive the Dx errors.

- :ref:`ConjoinXRuns <algm-ConjoinXRuns>` joins Dx errors if present
- The algorithm :ref:`SortXAxis <algm-SortXAxis>` has a new input option that allows ascending (default) and descending sorting. Furthermore, Dx values will be considered if present. The documentation needed to be corrected.


Bug fixes
#########

- The documentation of the algorithm :ref:`algm-CreateSampleWorkspace` did not match its implementation. The axis in beam direction will now be correctly described as Z instead of X.
- The :ref:`ExtractMask <algm-ExtractMask>` algorithm now returns a non-empty list of detector ID's when given a MaskWorkspace.
- Fixed a crash when the input workspace for :ref:`GroupDetectors <algm-GroupDetectors>` contained any other units than spectrum numbers.
- :ref:`ConvertToMD <algm-ConvertToMD>` can now be used with workspaces that aren't in the ADS. 
- Fixed :ref:`SumSpectra <algm-SumSpectra>` to avoid a crash when validation of inputs was called with a WorkspaceGroup.

New
###

- Algorithm :ref:`FitPeaks <algm-FitPeaks>` is implemented as a generalized multiple-spectra multiple-peak fitting algorithm.


:ref:`Release 3.13.0 <v3.13.0>`
