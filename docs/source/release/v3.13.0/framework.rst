=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.


Bug fixes
#########

- The documentation of the algorithm :ref:`algm-CreateSampleWorkspace` did not match its implementation. The axis in beam direction will now be correctly described as Z instead of X.
- The :ref:`ExtractMask <algm-ExtractMask>` algorithm now returns a non-empty list of detector ID's when given a MaskWorkspace.

Algorithms
----------

- :ref:`ConvertToPointData <algm-ConvertToPointData>` and :ref:`ConvertToHistogram <algm-ConvertToHistogram>` now propagate the Dx errors to the output.

:ref:`Release 3.13.0 <v3.13.0>`
