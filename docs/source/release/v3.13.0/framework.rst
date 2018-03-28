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

- :ref:`ConvertToPointData <algm-ConvertToPointData>` and :ref:`ConvertToHistogram <algm-ConvertToHistogram>` now propagate the Dx errors to the output.

Bug fixes
#########

- The documentation of the algorithm :ref:`algm-CreateSampleWorkspace` did not match its implementation. The axis in beam direction will now be correctly described as Z instead of X.

:ref:`Release 3.13.0 <v3.13.0>`
