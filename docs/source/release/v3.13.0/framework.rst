=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.


Algorithms
----------

New Features
############

- A list of Related Algorithms has been added to each algorithm, and is displayed in the documentation page of each algorithm as part of it's summary.

New Algorithms
##############


Improved
########

- :ref:`Maxent <algm-Maxent>` when outputting the results of the iterations, it no longer pads with zeroes but
  returns as many items as iterations done for each spectrum, making the iterations easy to count.
- XError values (Dx) can now be treated by the following algorithms: :ref:`ConjoinXRuns <algm-ConjoinXRuns>`, :ref:`ConvertToHistogram <algm-ConvertToHistogram>`, :ref:`ConvertToPointData <algm-ConvertToPointData>`, :ref:`CreateWorkspace <algm-CreateWorkspace>`, :ref:`SortXAxis <algm-SortXAxis>`, :ref:`algm-Stitch1D`.
- :ref:`Stitch1D <algm-Stitch1D>` can treat point data.
- The algorithm :ref:`SortXAxis <algm-SortXAxis>` has a new input option that allows ascending (default) and descending sorting. The documentation needed to be corrected in general.

Bug fixes
#########

- The documentation of the algorithm :ref:`algm-CreateSampleWorkspace` did not match its implementation. The axis in beam direction will now be correctly described as Z instead of X.
- The :ref:`ExtractMask <algm-ExtractMask>` algorithm now returns a non-empty list of detector ID's when given a MaskWorkspace.

:ref:`Release 3.13.0 <v3.13.0>`
