=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New
###

-  :ref:`AlignComponents <algm-AlignComponents>`
   This algorithm will take the calibration from the output of
   :ref:`GetDetOffsetsMultiPeaks <algm-GetDetOffsetsMultiPeaks>`, :ref:`CalibrateRectangularDetectors <algm-CalibrateRectangularDetectors>`, *et al* and
   minimizes the difference between the *DIFC* of the instrument and
   calibration by moving and rotating instrument components.

Improved
########

-  :ref:`EnggCalibrate <algm-EnggCalibrate>`
   has a new output property with the fitted parameters of the
   calibration sample peaks. It also logs more details about the peaks
   fitted.
-  :ref:`Integration <algm-Integration>`
   now correctly works for event data that has not been binned.
-  :ref:`FFT <algm-FFT>`
   now has an extra (optional) parameter, ``AcceptXRoundingErrors``. When
   set, this enables the algorithm to run even when the bin widths are
   slightly different. (An error is still produced for large
   deviations). By default, this is set to false, keeping the original
   behaviour.
   `#15325 <https://github.com/mantidproject/mantid/pull/15325>`_
-  :ref:`ConvertUnits <algm-ConvertUnits>`
   now works correctly for 'distribution' data in a :ref:`MatrixWorkspace <MatrixWorkspace>` in
   in-place mode (``InputWorkspace`` = ``OutputWorkspace``).
   `#15489 <https://github.com/mantidproject/mantid/pull/15489>`_

Deprecated
##########

-  The `UserAlgorithms` package is no longer being shipped with the Windows packages.

MD Algorithms (VATES CLI)
#########################

-  The algorithm :ref:`SaveMDWorkspaceToVTK <algm-SaveMDWorkspaceToVTK>` is now available. It allows the
   user to save 3D MDHisto or 3D MDEvent workspaces as either a ``.vts`` or
   ``.vtu`` files. These file types can be loaded into a standalone version
   of ParaView.

Performance
-----------

- :ref:`ChangeBinOffset <algm-ChangeBinOffset>` should now run faster for a :ref:`MatrixWorkspace <MatrixWorkspace>` (not EventWorkspaces).

CurveFitting
------------

Improved
########

Python
------

Python Algorithms
#################

|

Full list of
`Framework <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Framework%22>`__
and
`Python <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Python%22>`__
changes on GitHub

